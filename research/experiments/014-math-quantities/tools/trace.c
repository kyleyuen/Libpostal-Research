// Experiment 014 (measurement only, nothing timed): feature-key trace for Q3/Q5/Q6 and model-key/vocab enumeration for Q4/Q6.
// Links the PRESERVED baseline libpostal.a (library unchanged). Build h's exact dictionaries are rebuilt from the loaded tries with h's own
// feature_dict.h (sha256 9c334a86..., identical to 011 candidate) using the same calls as h: fd_build(state), fd_build(trans), fd_build_ex(vocab,false,true).
// Every dictionary answer is checked against the baseline trie. Single thread.
//
// usage: trace trace  corpus.txt outdir   -> outdir/dev_features_014.tsv.gz, dev_tokens.tsv.gz, addr_lines.tsv.gz; per-name stats on stdout
//        trace vocab  outdir              -> outdir/vocab_counts.tsv.gz; separator checks on stdout
//        trace model                      -> model-key classification (Lemma 5 set D) on stdout
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libpostal.h"
#include "address_parser.h"
#include "crf.h"
#include "trie.h"
#include "tokens.h"
#include "token_types.h"
#include "feature_dict.h"

static trie_t *V;   // baseline vocabulary trie (ground truth for "is a vocabulary word")
static bool in_vocab(const char *s, size_t n) {
    char buf[4096]; if (n >= sizeof buf) return false;
    memcpy(buf, s, n); buf[n] = 0; uint32_t c; return trie_get_data(V, buf, &c);
}
// Lemma 5 predicate on a key "name|p1|p2..": 0 = no value part (e.g. bias), 1 = all parts in V (set D), 2 = some part not in V
static int classify(const char *key, size_t len) {
    const char *p = memchr(key, '|', len); if (!p) return 0;
    const char *end = key + len, *s = p + 1;
    for (;;) { const char *q = memchr(s, '|', end - s); size_t n = (q ? q : end) - s; if (!in_vocab(s, n)) return 2; if (!q) return 1; s = q + 1; }
}

// ---- per-name counters ----
typedef struct { char name[64]; char kind; uint64_t n, hit, cls[3], cls_hit[3]; } nstat_t;
static nstat_t NS[256]; static int nns;
static nstat_t *nstat(char kind, const char *key, size_t len) {
    const char *p = memchr(key, '|', len); size_t nl = p ? (size_t)(p - key) : len; if (nl > 63) nl = 63;
    for (int i = 0; i < nns; i++) if (NS[i].kind == kind && strlen(NS[i].name) == nl && !memcmp(NS[i].name, key, nl)) return &NS[i];
    if (nns == 256) { fprintf(stderr, "too many names\n"); exit(3); }
    nstat_t *s = &NS[nns++]; memset(s, 0, sizeof *s); memcpy(s->name, key, nl); s->kind = kind; return s;
}

// ---- line touches (128-byte lines: hw.cachelinesize = 128) ----
enum { R_SLOT, R_ARENA, R_INDPTR, R_INDICES, R_DATA, R_N };
static const char *RN[R_N] = {"slot", "arena", "indptr", "indices", "data"};
static uint64_t *TL; static uint8_t *TR; static size_t tn, tcap;   // touches of the current address, in access order
static void touch(const void *lo, const void *hi, int r) {
    for (uint64_t l = (uintptr_t)lo >> 7; l <= (uintptr_t)hi >> 7; l++) {
        if (tn == tcap) { tcap = tcap ? tcap * 2 : 4096; TL = realloc(TL, tcap * 8); TR = realloc(TR, tcap); }
        TL[tn] = l; TR[tn++] = (uint8_t)r;
    }
}
// Same probe as fd_lookup_hashed; records every slot line and every arena byte range it reads (memcmp modelled as reading up to the first difference).
static bool probe(const feature_dict_t *d, const char *key, size_t len, uint32_t *id) {
    uint8_t nb; const char *rest; size_t rl;
    if (!fd_split(d, key, len, &nb, &rest, &rl)) return false;       // unknown name: only the hot name table is read
    uint64_t h = fd_hash_key(nb, rest, rl); uint8_t tag = (uint8_t)h;
    for (uint64_t i = fd_index(d, h);; i = fd_next(d, i)) {
        touch(&d->slots[i], (const char *)&d->slots[i] + 7, R_SLOT);
        uint64_t s = d->slots[i]; if (!s) return false;
        if ((uint8_t)(s >> 56) != tag) continue;
        const uint8_t *e = d->arena + (uint32_t)s;
        if (e[0] != nb) { touch(e, e, R_ARENA); continue; }
        size_t k = 0; while (k < rl + 1 && e[1 + k] == (k < rl ? (uint8_t)rest[k] : 0)) k++;
        touch(e, e + 1 + (k < rl + 1 ? k : rl), R_ARENA);
        if (k == rl + 1) {
            if (d->wide_value) { touch(e + 2 + rl, e + 5 + rl, R_ARENA); memcpy(id, e + 2 + rl, 4); }
            else *id = (uint32_t)((s >> 32) & 0xFFFFFF) - 1;
            return true;
        }
    }
}
static void touch_row(const sparse_matrix_t *w, uint32_t id) {
    uint32_t *ip = w->indptr->a; touch(&ip[id], &ip[id + 1], R_INDPTR);
    uint32_t a = ip[id], b = ip[id + 1];
    if (b > a) { touch(&w->indices->a[a], &w->indices->a[b - 1], R_INDICES); touch(&w->data->a[a], (const char *)&w->data->a[b - 1] + 7, R_DATA); }
}

// ---- exact LRU stack (reuse) distances over the whole touch stream: Fenwick tree over time + line -> last time map ----
static uint32_t *BIT; static size_t BN;
static void bit_add(size_t i, int v) { for (i++; i <= BN; i += i & -i) BIT[i - 1] += v; }
static uint64_t bit_sum(size_t i) { uint64_t s = 0; for (; i; i -= i & -i) s += BIT[i - 1]; return s; }   // sum of [0, i)
static uint64_t *MK; static uint32_t *MV; static size_t MC = 1 << 24, mused;
static uint32_t *map_slot(uint64_t line, bool *fresh) {
    uint64_t k = line + 1, i = (k * 0x9E3779B97F4A7C15ULL) >> (64 - 24);
    for (;; i = (i + 1) & (MC - 1)) {
        if (MK[i] == k) { *fresh = false; return &MV[i]; }
        if (!MK[i]) { if (++mused > MC * 7 / 10) { fprintf(stderr, "line map full\n"); exit(3); } MK[i] = k; *fresh = true; return &MV[i]; }
    }
}
static size_t T;   // global touch clock
enum { NCAP = 4 };
static const uint64_t CAPL[NCAP] = {1024, 131072, 262144, 524288};   // 128 KiB (L1d), 16 MiB (perflevel0 L2), 32 MiB, 64 MiB, in 128-B lines
// returns 1 if compulsory (first touch ever); increments miss[c] when the LRU stack distance (distinct lines since last use) >= capacity c
static int access_line(uint64_t line, uint64_t *miss) {
    bool fresh; uint32_t *slot = map_slot(line, &fresh);
    if (T >= BN) { fprintf(stderr, "BIT full\n"); exit(3); }
    int comp = 0;
    if (fresh) { comp = 1; for (int c = 0; c < NCAP; c++) miss[c]++; }
    else { uint64_t dist = bit_sum(T) - bit_sum(*slot + 1); for (int c = 0; c < NCAP; c++) if (dist >= CAPL[c]) miss[c]++; bit_add(*slot, -1); }
    bit_add(T, 1); *slot = (uint32_t)T; T++;
    return comp;
}

static FILE *gz(const char *dir, const char *name) {
    char cmd[4096]; snprintf(cmd, sizeof cmd, "gzip -n -c > '%s/%s'", dir, name); FILE *f = popen(cmd, "w"); if (!f) exit(4); return f;
}

static int mode_trace(const char *corpus, const char *out) {
    address_parser_t *parser = get_address_parser(); address_parser_context_t *context = parser->context; crf_t *crf = parser->model.crf;
    V = parser->vocab;
    feature_dict_t *sd = fd_build(crf->state_features), *td = fd_build(crf->state_trans_features), *vd = fd_build_ex(parser->vocab, false, true);
    if (!sd || !td || !vd) { fprintf(stderr, "fd_build failed\n"); return 3; }
    printf("dicts: state keys=%llu slots=%llu | trans keys=%llu slots=%llu | vocab keys=%llu slots=%llu\n", (unsigned long long)sd->num_keys, (unsigned long long)sd->size,
           (unsigned long long)td->num_keys, (unsigned long long)td->size, (unsigned long long)vd->num_keys, (unsigned long long)vd->size);
    printf("alignment (addr %% 128): state slots=%lu arena=%lu | trans slots=%lu arena=%lu | W indptr=%lu indices=%lu data=%lu | TW indptr=%lu indices=%lu data=%lu\n",
           (uintptr_t)sd->slots % 128, (uintptr_t)sd->arena % 128, (uintptr_t)td->slots % 128, (uintptr_t)td->arena % 128,
           (uintptr_t)crf->weights->indptr->a % 128, (uintptr_t)crf->weights->indices->a % 128, (uintptr_t)crf->weights->data->a % 128,
           (uintptr_t)crf->state_trans_weights->indptr->a % 128, (uintptr_t)crf->state_trans_weights->indices->a % 128, (uintptr_t)crf->state_trans_weights->data->a % 128);
    BN = 1 << 25; BIT = calloc(BN, 4); MK = calloc(MC, 8); MV = calloc(MC, 4);
    FILE *ff = gz(out, "dev_features_014.tsv.gz"), *ft = gz(out, "dev_tokens.tsv.gz"), *fl = gz(out, "addr_lines.tsv.gz");
    fprintf(ft, "addr\ttok\ttype\tis_word\tword\tfreq\tbias\tn_before_bias\tfirst_S\tprefix\tsuffix\tnS\tnP\n");
    fprintf(fl, "addr\tbypass\ttokens\tnS\tnP\thitS\thitP\ttouches\tdistinct");
    for (int r = 0; r < R_N; r++) fprintf(fl, "\td_%s", RN[r]);
    fprintf(fl, "\tcompulsory\tmiss_128K\tmiss_16M\tmiss_32M\tmiss_64M\n");
    FILE *f = fopen(corpus, "r"); if (!f) return 2;
    char *line = NULL; size_t lc = 0; ssize_t len; size_t ai = 0, nbypass = 0, id_mism = 0, nlook = 0;
    uint64_t *dset = calloc(1 << 16, 8);
    uint64_t tot_miss[NCAP] = {0}, tot_comp = 0, tot_touch = 0, tot_dist = 0;
    while ((len = getline(&line, &lc, f)) >= 0) {
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        if (!len) continue;
        size_t a = ai++;
        // --- pre-CRF flow, as address_parser_parse / stage_bench parse_staged (001, verified 12,863/12,863) ---
        char *normalized = address_parser_normalize_string(line); bool is_norm = normalized != NULL; if (!is_norm) normalized = line;
        token_array *tokens = tokenize(normalized);
        tokenized_string_t *ts = tokenized_string_new_from_str_size(normalized, strlen(normalized), tokens->n);
        uint32_array_clear(context->separators);
        for (size_t i = 0; i < tokens->n; i++) {
            token_t token = tokens->a[i];
            if (ADDRESS_PARSER_IS_SEPARATOR(token.type)) { uint32_array_pop(context->separators); uint32_array_push(context->separators, ADDRESS_SEPARATOR_FIELD_INTERNAL); continue; }
            else if (ADDRESS_PARSER_IS_IGNORABLE(token.type)) continue;
            tokenized_string_add_token(ts, (const char *)normalized, token.len, token.type, token.offset);
            uint32_array_push(context->separators, ADDRESS_SEPARATOR_NONE);
        }
        address_parser_context_fill(context, parser, ts, NULL, NULL);
        size_t nt = ts->tokens->n; bool bypass = false;
        phrase_t only = NULL_PHRASE; bool is_postal = false;
        if (context->component_phrases->n == 1) only = context->component_phrases->a[0];
        else if (context->postal_code_phrases->n == 1) { only = context->postal_code_phrases->a[0]; is_postal = true; }
        if (only.start == 0 && only.len == nt && only.len > 0) {
            uint32_t mc = is_postal ? 0 : parser->phrase_types->a[only.data].most_common;
            if (is_postal || mc == ADDRESS_PARSER_BOUNDARY_CITY || mc == ADDRESS_PARSER_BOUNDARY_STATE || mc == ADDRESS_PARSER_BOUNDARY_COUNTRY ||
                mc == ADDRESS_PARSER_BOUNDARY_STATE_DISTRICT || mc == ADDRESS_PARSER_BOUNDARY_COUNTRY_REGION || mc == ADDRESS_PARSER_BOUNDARY_SUBURB ||
                mc == ADDRESS_PARSER_BOUNDARY_CITY_DISTRICT || mc == ADDRESS_PARSER_BOUNDARY_WORLD_REGION) bypass = true;   // library returns without the CRF
        }
        tn = 0; size_t nS = 0, nP = 0, hS = 0, hP = 0;
        for (size_t t = 0; !bypass && t < nt; t++) {
            cstring_array_clear(context->features); cstring_array_clear(context->prev_tag_features);
            if (!address_parser_features(parser, context, ts, (uint32_t)t)) { fprintf(stderr, "features failed addr %zu\n", a); return 3; }
            uint32_t fi; char *feat; int bias_pos = -1, k = 0; bool pre = false, suf = false; const char *firstS = "";
            for (fi = 0; fi < cstring_array_num_strings(context->features); fi++) { feat = cstring_array_get_string(context->features, fi);
                size_t fl2 = strlen(feat); uint32_t tid = 0, did = 0;
                bool th = trie_get_data(crf->state_features, feat, &tid), dh = probe(sd, feat, fl2, &did);
                if (th != dh || (th && tid != did)) id_mism++;
                if (dh) { touch_row(crf->weights, did); hS++; }
                nstat_t *s = nstat('S', feat, fl2); int c = classify(feat, fl2); s->n++; s->cls[c]++; if (th) { s->hit++; s->cls_hit[c]++; }
                if (bias_pos < 0 && !strcmp(feat, "bias")) bias_pos = k;
                if (k == 0) firstS = feat;
                if (!strncmp(feat, "prefix|", 7)) pre = true;
                if (!strncmp(feat, "suffix|", 7)) suf = true;
                fprintf(ff, "S\t%zu\t%zu\t%s\n", a, t, feat); k++; nS++; nlook++;
            }
            size_t kP = 0;
            for (fi = 0; fi < cstring_array_num_strings(context->prev_tag_features); fi++) { feat = cstring_array_get_string(context->prev_tag_features, fi);
                size_t fl2 = strlen(feat); uint32_t tid = 0, did = 0;
                bool th = trie_get_data(crf->state_trans_features, feat, &tid), dh = probe(td, feat, fl2, &did);
                if (th != dh || (th && tid != did)) id_mism++;
                if (dh) { touch_row(crf->state_trans_weights, did); hP++; }
                nstat_t *s = nstat('P', feat, fl2); int c = classify(feat, fl2); s->n++; s->cls[c]++; if (th) { s->hit++; s->cls_hit[c]++; }
                fprintf(ff, "P\t%zu\t%zu\t%s\n", a, t, feat); kP++; nP++; nlook++;
            }
            char *w = cstring_array_get_string(context->normalized, (uint32_t)t); uint32_t freq = 0; trie_get_data(V, w, &freq);
            uint32_t vid = 0; if (fd_lookup(vd, w, strlen(w), &vid) != (freq > 0) || (freq && vid != freq)) id_mism++;
            char fs[48]; snprintf(fs, sizeof fs, "%.*s", (int)strcspn(firstS, "|"), firstS);
            uint16_t ty = ts->tokens->a[t].type;
            fprintf(ft, "%zu\t%zu\t%u\t%d\t%s\t%u\t%d\t%d\t%s\t%d\t%d\t%d\t%zu\n", a, t, ty, is_word_token(ty) ? 1 : 0, w, freq, bias_pos >= 0, bias_pos, fs, pre, suf, k, kP);
        }
        // per-address distinct lines and the LRU model over the stream
        memset(dset, 0, (1 << 16) * 8); size_t distinct = 0, dr[R_N] = {0}; uint64_t miss[NCAP] = {0}, comp = 0;
        if (tn > (1 << 15)) { fprintf(stderr, "too many touches\n"); return 3; }
        for (size_t i = 0; i < tn; i++) {
            uint64_t k = TL[i] + 1, h = (k * 0x9E3779B97F4A7C15ULL) >> 48;
            for (;; h = (h + 1) & 0xFFFF) { if (dset[h] == k) break; if (!dset[h]) { dset[h] = k; distinct++; dr[TR[i]]++; break; } }
            comp += access_line(TL[i], miss);
        }
        fprintf(fl, "%zu\t%d\t%zu\t%zu\t%zu\t%zu\t%zu\t%zu\t%zu", a, bypass, nt, nS, nP, hS, hP, tn, distinct);
        for (int r = 0; r < R_N; r++) fprintf(fl, "\t%zu", dr[r]);
        fprintf(fl, "\t%llu", (unsigned long long)comp); for (int c = 0; c < NCAP; c++) fprintf(fl, "\t%llu", (unsigned long long)miss[c]);
        fprintf(fl, "\n");
        for (int c = 0; c < NCAP; c++) tot_miss[c] += miss[c];
        tot_comp += comp; tot_touch += tn; tot_dist += distinct; nbypass += bypass;
        tokenized_string_destroy(ts); token_array_destroy(tokens); if (is_norm) free(normalized);
    }
    pclose(ff); pclose(ft); pclose(fl);
    printf("addresses=%zu bypass=%zu lookups=%zu dict_vs_trie_id_mismatches=%zu\n", ai, nbypass, nlook, id_mism);
    printf("touches=%llu sum_distinct_per_addr=%llu compulsory=%llu miss_128K=%llu miss_16M=%llu miss_32M=%llu miss_64M=%llu distinct_lines_total=%zu\n",
           (unsigned long long)tot_touch, (unsigned long long)tot_dist, (unsigned long long)tot_comp, (unsigned long long)tot_miss[0], (unsigned long long)tot_miss[1],
           (unsigned long long)tot_miss[2], (unsigned long long)tot_miss[3], mused);
    printf("kind\tname\tlookups\thits\tnovalue\tinD\tnotD\tnovalue_hits\tinD_hits\tnotD_hits\n");
    for (int i = 0; i < nns; i++) printf("%c\t%s\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\n", NS[i].kind, NS[i].name, (unsigned long long)NS[i].n, (unsigned long long)NS[i].hit,
        (unsigned long long)NS[i].cls[0], (unsigned long long)NS[i].cls[1], (unsigned long long)NS[i].cls[2], (unsigned long long)NS[i].cls_hit[0], (unsigned long long)NS[i].cls_hit[1], (unsigned long long)NS[i].cls_hit[2]);
    return id_mism ? 5 : 0;
}

typedef struct { FILE *f; uint64_t n, bar, bar_end, space, empty, zero; } vctx_t;
static bool vocab_cb(void *c, const char *key, size_t len, uint32_t data) {
    vctx_t *v = c; v->n++;
    if (memchr(key, '|', len)) v->bar++;
    if (len && key[len - 1] == '|') v->bar_end++;
    if (memchr(key, ' ', len)) v->space++;
    if (!len) v->empty++;
    if (!data) v->zero++;
    fprintf(v->f, "%s\t%u\n", key, data); return true;
}
static int mode_vocab(const char *out) {
    V = get_address_parser()->vocab; vctx_t v = {gz(out, "vocab_counts.tsv.gz")}; uint64_t lv, sk;
    bool ok = fd_for_each_key(V, vocab_cb, &v, &lv, &sk); pclose(v.f);
    uint32_t c1 = 0, c2 = 0; bool u1 = trie_get_data(V, "UNKNOWN", &c1), u2 = trie_get_data(V, "UNKNOWN_NUMERIC", &c2)   /* address_parser.c:20-21 */;
    printf("vocab enumerate ok=%d keys=%llu leaves_visited=%llu skipped=%llu header_num_keys=%u\n", ok, (unsigned long long)v.n, (unsigned long long)lv, (unsigned long long)sk, V->num_keys);
    printf("words containing '|'=%llu ending with '|'=%llu containing ' '=%llu empty=%llu count==0: %llu\n", (unsigned long long)v.bar, (unsigned long long)v.bar_end,
           (unsigned long long)v.space, (unsigned long long)v.empty, (unsigned long long)v.zero);
    printf("UNKNOWN in V=%d (count %u)  UNKNOWN_NUMERIC in V=%d (count %u)\n", u1, c1, u2, c2);
    return ok ? 0 : 3;
}

typedef struct { char kind; uint64_t n; } mctx_t;
static bool model_cb(void *c, const char *key, size_t len, uint32_t data) {
    mctx_t *m = c; m->n++; nstat_t *s = nstat(m->kind, key, len); int k = classify(key, len); s->n++; s->cls[k]++;
    if (k && (memmem(key, len, "||", 2) || key[len - 1] == '|')) s->hit++;   // reused counter: key has an empty value part
    return true;
}
static int mode_model(void) {
    crf_t *crf = get_address_parser()->model.crf; V = get_address_parser()->vocab; uint64_t lv, sk; bool ok = true;
    mctx_t ms = {'S'}, mp = {'P'};
    ok &= fd_for_each_key(crf->state_features, model_cb, &ms, &lv, &sk); printf("state keys=%llu (leaves %llu skipped %llu)\n", (unsigned long long)ms.n, (unsigned long long)lv, (unsigned long long)sk);
    ok &= fd_for_each_key(crf->state_trans_features, model_cb, &mp, &lv, &sk); printf("trans keys=%llu (leaves %llu skipped %llu)\n", (unsigned long long)mp.n, (unsigned long long)lv, (unsigned long long)sk);
    printf("kind\tname\tkeys\tnovalue\tinD\tnotD\tempty_part\n");
    for (int i = 0; i < nns; i++) printf("%c\t%s\t%llu\t%llu\t%llu\t%llu\t%llu\n", NS[i].kind, NS[i].name, (unsigned long long)NS[i].n, (unsigned long long)NS[i].cls[0],
        (unsigned long long)NS[i].cls[1], (unsigned long long)NS[i].cls[2], (unsigned long long)NS[i].hit);
    return ok ? 0 : 3;
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    if (!strcmp(argv[1], "trace") && argc == 4) return mode_trace(argv[2], argv[3]);
    if (!strcmp(argv[1], "vocab") && argc == 3) return mode_vocab(argv[2]);
    if (!strcmp(argv[1], "model")) return mode_model();
    return 2;
}
