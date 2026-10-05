// 146 (REWRITE_PLAN M2): integer bigram feature ids from model-derived tables. Included by address_parser.c under EXP_RW_BIGRAM.
// Exact by construction (see experiment 146 README): the pair table holds EVERY (unit, unit) split of every model bigram key, so for runtime
// parts A, B that are both units (A non-empty, not ending in '|'), the key "fam|A|B" is in the model iff (fam, uid(A), uid(B)) is in the table.
// Everything else keeps the original string path. Emission keeps its position via a "\x01" placeholder plus a per-token side channel.
#include <mach/mach_time.h>
#include "address_dictionary.h"
typedef bool (*fd_key_cb)(void *ctx, const char *key, size_t len, uint32_t data);

// Enumerates every key the trie can return (each leaf's path + tail, exactly as trie_get_from_index reads them).
// Returns false on any structural doubt. Counts leaves seen and leaves that no lookup can reach.
static bool fd_for_each_key(trie_t *trie, fd_key_cb cb, void *ctx, uint64_t *leaves_visited, uint64_t *skipped) {
    uint32_t nn = (uint32_t)trie->nodes->n;
    trie_node_t *nodes = trie->nodes->a;
    trie_data_node_t *data = trie->data->a;
    uint8_t *tail = trie->tail->a; size_t tail_n = trie->tail->n;
    enum { MAXD = 512 };
    uint32_t stk_node[MAXD]; uint16_t stk_ci[MAXD]; uint8_t path[MAXD + 1];
    char *keybuf = malloc(1 << 16); if (!keybuf) return false;
    int sp = 0; stk_node[0] = ROOT_NODE_ID; stk_ci[0] = 1;
    if (nodes[ROOT_NODE_ID].base < 0) { free(keybuf); return false; }
    bool ok = true;
    *leaves_visited = 0; *skipped = 0;
    while (sp >= 0 && ok) {
        uint32_t pid = stk_node[sp]; int32_t pbase = nodes[pid].base;
        unsigned ci = stk_ci[sp]; bool descended = false;
        for (; ci <= 256; ci++) {
            uint64_t idx = (uint64_t)(uint32_t)pbase + ci;
            if (idx >= nn) break;
            if (nodes[idx].check != (int32_t)pid) continue;
            uint8_t c = trie->alphabet[ci - 1];
            path[sp] = c;
            if (nodes[idx].base < 0) {                               // leaf
                (*leaves_visited)++;
                int32_t di = -nodes[idx].base;
                if ((size_t)di >= trie->data->n) { ok = false; break; }
                uint32_t tpos = data[di].tail;
                if (tpos == 0 || tpos >= tail_n) { (*skipped)++; continue; }
                size_t tl = strnlen((char *)tail + tpos, tail_n - tpos);
                if (tpos + tl >= tail_n) { (*skipped)++; continue; }   // no NUL terminator inside the array
                size_t plen = (size_t)sp + 1;
                size_t klen;
                if (c == 0) { if (tl != 0) { (*skipped)++; continue; } klen = plen - 1; }   // ended on the NUL transition: lookup needs an empty tail
                else klen = plen + tl;
                if (klen + 1 > (1 << 16) || plen > MAXD) { ok = false; break; }
                memcpy(keybuf, path, c == 0 ? plen - 1 : plen);
                if (c != 0) memcpy(keybuf + plen, tail + tpos, tl);
                keybuf[klen] = 0;
                if (!cb(ctx, keybuf, klen, data[di].data)) { ok = false; break; }
            } else {
                if (sp + 1 >= MAXD) { ok = false; break; }
                stk_ci[sp] = (uint16_t)(ci + 1);
                sp++; stk_node[sp] = (uint32_t)idx; stk_ci[sp] = 1;
                descended = true; break;
            }
        }
        if (!ok) break;
        if (!descended) sp--;
    }
    free(keybuf);
    return ok;
}

#define RW_MISS UINT32_MAX
#define RW_SBITS 24                     // string-map and pair-map capacity 2^24 slots; unit ids must stay < 2^24
static inline uint64_t rw_hash(const char *s, size_t n) { uint64_t h = 1469598103934665603ULL; for (size_t i = 0; i < n; i++) { h ^= (uint8_t)s[i]; h *= 1099511628211ULL; } return h ^ (h >> 29); }
static inline uint64_t rw_mix(uint64_t k) { k ^= k >> 33; k *= 0xFF51AFD7ED558CCDULL; k ^= k >> 33; k *= 0xC4CEB9FE1A85EC53ULL; return k ^ (k >> 33); }
static struct { char *arena; size_t an, am; uint32_t *off, *len, *uid; uint32_t n; } rw_s;          // unit strings: off = arena offset + 1 (0 = empty)
static struct { uint64_t *key; uint32_t *val; uint64_t n, splits_multi; } rw_p;                   // (fam << 48 | ua << 24 | ub) -> feature id
static uint32_t *rw_side; static size_t rw_side_n, rw_side_m;
static uint64_t rw_cnt_int, rw_cnt_str, rw_cnt_strip;
static uint32_t rw_unit_get(const char *s, size_t n) {
    uint64_t m = (1ULL << RW_SBITS) - 1, i = rw_hash(s, n) & m;
    for (;; i = (i + 1) & m) {
        uint32_t o = rw_s.off[i];
        if (o == 0) return RW_MISS;
        if (rw_s.len[i] == n && memcmp(rw_s.arena + o - 1, s, n) == 0) return rw_s.uid[i];
    }
}
static bool rw_unit_add(const char *s, size_t n) {
    uint64_t m = (1ULL << RW_SBITS) - 1, i = rw_hash(s, n) & m;
    for (;; i = (i + 1) & m) {
        uint32_t o = rw_s.off[i];
        if (o == 0) break;
        if (rw_s.len[i] == n && memcmp(rw_s.arena + o - 1, s, n) == 0) return true;
    }
    if (rw_s.n + 1 >= (1u << RW_SBITS) / 2) return false;                                            // keep load <= 0.5 and ids < 2^23
    if (rw_s.an + n + 1 > rw_s.am) { size_t nm = rw_s.am ? rw_s.am * 2 : (1u << 26); while (nm < rw_s.an + n + 1) nm *= 2; char *a = realloc(rw_s.arena, nm); if (!a) return false; rw_s.arena = a; rw_s.am = nm; }
    if (rw_s.an + 1 > UINT32_MAX - n - 1) return false;
    memcpy(rw_s.arena + rw_s.an, s, n); rw_s.arena[rw_s.an + n] = 0;
    rw_s.off[i] = (uint32_t)rw_s.an + 1; rw_s.len[i] = (uint32_t)n; rw_s.uid[i] = rw_s.n++; rw_s.an += n + 1;
    return true;
}
static inline uint64_t rw_pkey(uint32_t fam, uint32_t a, uint32_t b) { return ((uint64_t)fam << 48) | ((uint64_t)a << 24) | b; }
static uint32_t rw_pair_get(uint64_t k) {
    uint64_t m = (1ULL << RW_SBITS) - 1, i = rw_mix(k) & m;
    for (;; i = (i + 1) & m) { if (rw_p.key[i] == UINT64_MAX) return RW_MISS; if (rw_p.key[i] == k) return rw_p.val[i]; }
}
static bool rw_pair_put(uint64_t k, uint32_t v) {
    uint64_t m = (1ULL << RW_SBITS) - 1, i = rw_mix(k) & m;
    for (;; i = (i + 1) & m) {
        if (rw_p.key[i] == UINT64_MAX) { if (rw_p.n + 1 > (1ULL << RW_SBITS) * 7 / 10) return false; rw_p.key[i] = k; rw_p.val[i] = v; rw_p.n++; return true; }
        if (rw_p.key[i] == k) return rw_p.val[i] == v;                                                // same (fam, A, B) always means the same key string
    }
}
static bool rw_vocab_cb(void *c, const char *k, size_t n, uint32_t d) { (void)c; return d == 0 ? true : rw_unit_add(k, n); }
static bool rw_unit_cb(void *c, const char *k, size_t n, uint32_t d) { (void)c; (void)d; return rw_unit_add(k, n); }
static bool rw_dict_cb(void *c, const char *k, size_t n, uint32_t d) { (void)c; (void)d; const char *p = memchr(k, '|', n); return p ? rw_unit_add(p + 1, n - (size_t)(p + 1 - k)) : rw_unit_add(k, n); }
static const char *rw_fam_name[2] = {"prev word+word|", "word+next word|"};
static bool rw_state_cb(void *c, const char *k, size_t n, uint32_t d) {
    (void)c;
    for (uint32_t fam = 0; fam < 2; fam++) {
        size_t fl = strlen(rw_fam_name[fam]);
        if (n <= fl || memcmp(k, rw_fam_name[fam], fl) != 0) continue;
        const char *r = k + fl; size_t rn = n - fl; int splits = 0;
        for (size_t i = 0; i < rn; i++) {
            if (r[i] != '|') continue;
            uint32_t ua = rw_unit_get(r, i), ub = rw_unit_get(r + i + 1, rn - i - 1);
            if (ua == RW_MISS || ub == RW_MISS) continue;
            if (!rw_pair_put(rw_pkey(fam, ua, ub), d)) return false;
            splits++;
        }
        if (splits > 1) rw_p.splits_multi++;
        return true;
    }
    return true;
}
static void rw_report(void) {
    if (getenv("RW_VERBOSE") != NULL) fprintf(stderr, "rw runtime: bigrams via int %llu, via string (non-unit part) %llu, via string (left part empty or ends in '|') %llu\n",
        (unsigned long long)rw_cnt_int, (unsigned long long)rw_cnt_str, (unsigned long long)rw_cnt_strip);
}
static bool rw_init(address_parser_t *p) {
    uint64_t t0 = mach_absolute_time(), lv, sk;
    size_t S = (size_t)1 << RW_SBITS;
    rw_s.off = calloc(S, 4); rw_s.len = malloc(S * 4); rw_s.uid = malloc(S * 4);
    rw_p.key = malloc(S * 8); rw_p.val = malloc(S * 4);
    if (!rw_s.off || !rw_s.len || !rw_s.uid || !rw_p.key || !rw_p.val) return false;
    memset(rw_p.key, 0xff, S * 8);
    if (!fd_for_each_key(p->vocab, rw_vocab_cb, NULL, &lv, &sk) || sk) return false;
    if (!fd_for_each_key(p->phrases, rw_unit_cb, NULL, &lv, &sk) || sk) return false;
    address_dictionary_t *ad = get_address_dictionary();
    if (ad != NULL && ad->trie != NULL && (!fd_for_each_key(ad->trie, rw_dict_cb, NULL, &lv, &sk) || sk)) return false;
    if (!rw_unit_add("UNKNOWN", 7) || !rw_unit_add("UNKNOWN_NUMERIC", 15)) return false;
    if (!fd_for_each_key(p->model.crf->state_features, rw_state_cb, NULL, &lv, &sk) || sk) return false;
    mach_timebase_info_data_t tb; mach_timebase_info(&tb);
    if (getenv("RW_VERBOSE") != NULL) fprintf(stderr, "rw init: units %u, arena %.1f MB, pair entries %llu (keys with >1 unit split: %llu), %.2f s\n", rw_s.n, rw_s.an / 1e6,
        (unsigned long long)rw_p.n, (unsigned long long)rw_p.splits_multi, (mach_absolute_time() - t0) * (double)tb.numer / tb.denom / 1e9);
    atexit(rw_report);
    return true;
}
uint32_t rw_side_get(size_t k) { if (k >= rw_side_n) abort(); return rw_side[k]; }
static inline void rw_side_reset(void) { rw_side_n = 0; }
static void rw_side_push(uint32_t v) {
    if (rw_side_n == rw_side_m) { size_t nm = rw_side_m ? rw_side_m * 2 : 64; uint32_t *a = realloc(rw_side, nm * 4); if (!a) abort(); rw_side = a; rw_side_m = nm; }
    rw_side[rw_side_n++] = v;
}
// Replaces feature_array_add(features, 3, rw_fam_label[fam], a, b) for the two bigram families.
static const char *rw_fam_label[2] = {"prev word+word", "word+next word"};
static void rw_bigram(cstring_array *features, uint32_t fam, char *a, char *b) {
    size_t la = strlen(a);
    if (la == 0 || a[la - 1] == '|') { rw_cnt_strip++; feature_array_add(features, 3, (char *)rw_fam_label[fam], a, b); return; }
    uint32_t ua = rw_unit_get(a, la), ub = ua == RW_MISS ? RW_MISS : rw_unit_get(b, strlen(b));
    if (ua == RW_MISS || ub == RW_MISS) { rw_cnt_str++; feature_array_add(features, 3, (char *)rw_fam_label[fam], a, b); return; }
    rw_cnt_int++;
    cstring_array_add_string(features, "\x01");
    rw_side_push(rw_pair_get(rw_pkey(fam, ua, ub)));
}
