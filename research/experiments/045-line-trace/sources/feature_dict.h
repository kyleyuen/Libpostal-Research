// Experiment 006: exact compact dictionary replacing a libpostal double-array trie for (name|value...) feature keys.
// Built once at load from the trie's own keys; every lookup verifies the full key, so hit/miss/id equal the trie's for ANY query.
// Layout: open-addressing table of 64-bit slots [tag:8 | id+1:24 | arena offset:32]; arena entries [name byte][rest bytes][NUL];
// name byte = name id (7 bits) | 0x80 if the key had a '|' separator. Names are the model keys' own prefixes before the first '|'.
#ifndef FEATURE_DICT_H
#define FEATURE_DICT_H
#include "trace_touch.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include "trie.h"

#define FD_MAX_NAMES 127
#define FD_SEP '|'

typedef struct fd_name { char *str; uint32_t len; uint64_t hash; } fd_name_t;
#ifdef EXP_INT_KEYS
struct fdi_slot;
#endif

typedef struct feature_dict {
    uint64_t *slots;
    uint64_t size;                 // number of slots (any size >= 2; indexed by fast range reduction, not a power of two)
    uint8_t *arena;                // mmap'd; only the used prefix is kept
    size_t arena_used, arena_map;
    fd_name_t names[FD_MAX_NAMES];
    uint32_t num_names;
    uint8_t name_table[256];       // open addressing over names: index+1, 0 = empty
    uint64_t num_keys;
    bool split;                    // true: keys are name|value (state/trans features); false: whole key is compared (vocabulary)
    bool wide_value;               // true: 32-bit value stored after the key in the arena (ids may exceed 2^24)
    uint64_t leaves_total, leaves_visited, skipped_unreachable;   // build statistics
    double build_seconds;
#ifdef EXP_INT_KEYS
    // Experiment 015: keys whose value parts are all vocabulary words live in an integer table instead of slots/arena (see 015 README).
    struct fdi_slot *islots;       // NULL = no integer table
    uint64_t isize, inum;
    int8_t arity[FD_MAX_NAMES];    // routed names: 1 or 2 value parts; 0 = not routed
    const struct feature_dict *vocab;
#endif
} feature_dict_t;

static inline uint64_t fd_mix(uint64_t h, uint64_t w) { h = (h ^ w) * 0xFF51AFD7ED558CCDULL; return h ^ (h >> 32); }

#ifdef EXP_FD_INLINE
// The last 0-7 bytes as one word without a libc call: overlapping fixed-size loads cover every byte. The hash function is only ever used inside this dictionary
// (built and probed with the same code), so changing it cannot change any lookup result, only the slot layout.
static inline uint64_t fd_tail_word(const char *s, size_t len) {
    if (len >= 4) { uint32_t a, b; memcpy(&a, s, 4); memcpy(&b, s + len - 4, 4); return (uint64_t)a | ((uint64_t)b << 32); }
    if (len >= 2) { uint16_t a, b; memcpy(&a, s, 2); memcpy(&b, s + len - 2, 2); return (uint64_t)a | ((uint64_t)b << 16); }
    return len ? (uint64_t)(unsigned char)s[0] : 0;
}
// Equality of n >= 1 bytes with fixed-size overlapping loads (exact: every byte is covered).
static inline int fd_eq(const void *pa, const void *pb, size_t n) {
    const char *a = (const char *)pa, *b = (const char *)pb;
    if (n >= 16) return memcmp(a, b, n) == 0;
    if (n >= 8) { uint64_t x1, y1, x2, y2; memcpy(&x1, a, 8); memcpy(&y1, b, 8); memcpy(&x2, a + n - 8, 8); memcpy(&y2, b + n - 8, 8); return x1 == y1 && x2 == y2; }
    if (n >= 4) { uint32_t x1, y1, x2, y2; memcpy(&x1, a, 4); memcpy(&y1, b, 4); memcpy(&x2, a + n - 4, 4); memcpy(&y2, b + n - 4, 4); return x1 == y1 && x2 == y2; }
    if (n >= 2) { uint16_t x1, y1, x2, y2; memcpy(&x1, a, 2); memcpy(&y1, b, 2); memcpy(&x2, a + n - 2, 2); memcpy(&y2, b + n - 2, 2); return x1 == y1 && x2 == y2; }
    return a[0] == b[0];
}
#else
static inline uint64_t fd_tail_word(const char *s, size_t len) { uint64_t w = 0; if (len) memcpy(&w, s, len); return w; }
static inline int fd_eq(const void *a, const void *b, size_t n) { return memcmp(a, b, n) == 0; }
#endif

static inline uint64_t fd_hash_bytes(uint64_t seed, const char *s, size_t len) {
    uint64_t h = seed ^ len;
    while (len >= 8) { uint64_t w; memcpy(&w, s, 8); h = fd_mix(h, w); s += 8; len -= 8; }
    uint64_t w = fd_tail_word(s, len);
    h = fd_mix(h, w);
    h *= 0xC4CEB9FE1A85EC53ULL;
    return h ^ (h >> 29);
}

// slot index from the HIGH bits of h (multiply-shift range reduction); the 8-bit tag uses the LOW bits, so they are independent
static inline uint64_t fd_index(const feature_dict_t *d, uint64_t h) { return (uint64_t)(((__uint128_t)h * d->size) >> 64); }
static inline uint64_t fd_next(const feature_dict_t *d, uint64_t i) { return i + 1 == d->size ? 0 : i + 1; }

static inline uint64_t fd_hash_key(uint8_t name_byte, const char *rest, size_t rest_len) {
    return fd_hash_bytes(0x9E3779B97F4A7C15ULL ^ ((uint64_t)name_byte << 56), rest, rest_len);
}

static inline int fd_find_name(const feature_dict_t *d, const char *name, size_t len) {
    uint64_t h = fd_hash_bytes(0x51ED270B9C3A1F27ULL, name, len);
    for (uint32_t i = (uint32_t)h & 255;; i = (i + 1) & 255) {
        uint8_t e = d->name_table[i];
        if (!e) return -1;
        const fd_name_t *n = &d->names[e - 1];
        if (n->hash == h && n->len == len && memcmp(n->str, name, len) == 0) return e - 1;
    }
}

static int fd_add_name(feature_dict_t *d, const char *name, size_t len) {
    int f = fd_find_name(d, name, len);
    if (f >= 0) return f;
    if (d->num_names >= FD_MAX_NAMES) return -1;
    fd_name_t *n = &d->names[d->num_names];
    n->str = malloc(len + 1); if (!n->str) return -1;
    memcpy(n->str, name, len); n->str[len] = 0; n->len = (uint32_t)len;
    n->hash = fd_hash_bytes(0x51ED270B9C3A1F27ULL, name, len);
    d->num_names++;
    for (uint32_t i = (uint32_t)n->hash & 255;; i = (i + 1) & 255) if (!d->name_table[i]) { d->name_table[i] = (uint8_t)d->num_names; break; }
    return d->num_names - 1;
}

// Splits a key into (name id, has separator, rest). Returns false if the name is unknown to the dictionary (=> no stored key can equal it).
static inline bool fd_split(const feature_dict_t *d, const char *key, size_t len, uint8_t *name_byte, const char **rest, size_t *rest_len) {
    if (!d->split) { *name_byte = 0; *rest = key; *rest_len = len; return true; }
    const char *p = memchr(key, FD_SEP, len);
    size_t nlen = p ? (size_t)(p - key) : len;
    int id = fd_find_name(d, key, nlen);
    if (id < 0) return false;
    *name_byte = (uint8_t)id | (p ? 0x80 : 0);
    *rest = p ? p + 1 : key + len;
    *rest_len = p ? len - nlen - 1 : 0;
    return true;
}

// Exact lookup starting the probe at slot `start` (fd_index(d, h) for a fresh probe). true and *id set iff the trie this dictionary was built from would return that id.
static inline bool fd_lookup_hashed_from(const feature_dict_t *d, uint8_t name_byte, const char *rest, size_t rest_len, uint64_t h, uint64_t start, uint32_t *id) {
    uint8_t tag = (uint8_t)h;
    for (uint64_t i = start;; i = fd_next(d, i)) {
        uint64_t s = d->slots[i]; TT(TS_FSLOT, &d->slots[i], 8);
        if (!s) return false;
        if ((uint8_t)(s >> 56) == tag) {
            const uint8_t *e = d->arena + (uint32_t)s; TT(TS_FARENA, e, rest_len + 2);
            if (e[0] == name_byte && fd_eq(e + 1, rest, rest_len + 1)) {
                if (d->wide_value) memcpy(id, e + 2 + rest_len, sizeof(uint32_t));
                else *id = (uint32_t)((s >> 32) & 0xFFFFFF) - 1;
                return true;
            }
        }
    }
}

static inline bool fd_lookup_hashed(const feature_dict_t *d, uint8_t name_byte, const char *rest, size_t rest_len, uint64_t h, uint32_t *id) {
    return fd_lookup_hashed_from(d, name_byte, rest, rest_len, h, fd_index(d, h), id);
}

// Slot index and arena entry of an exact key (rest must be NUL-terminated at rest[rl]).
static inline const uint8_t *fd_find_slot(const feature_dict_t *d, uint8_t nb, const char *rest, size_t rl, uint64_t *slot) {
    uint64_t h = fd_hash_key(nb, rest, rl); uint8_t tag = (uint8_t)h;
    for (uint64_t i = fd_index(d, h);; i = fd_next(d, i)) {
        uint64_t s = d->slots[i]; TT((const void *)d == trace_vocab ? TS_VSLOT : TS_FSLOT, &d->slots[i], 8);
        if (!s) return NULL;
        if ((uint8_t)(s >> 56) == tag) {
            const uint8_t *e = d->arena + (uint32_t)s; TT((const void *)d == trace_vocab ? TS_VARENA : TS_FARENA, e, rl + 2);
            if (e[0] == nb && fd_eq(e + 1, rest, rl + 1)) { *slot = i; return e; }
        }
    }
}
#ifdef EXP_INT_KEYS
#define FDI_NONE 0xFFFFFFFFu
#define FDI_WBITS 23
#define FDI_WNONE ((1u << FDI_WBITS) - 1)          // second part of a unary key; vocabulary slot indices are < FDI_WNONE (checked at build)
typedef struct __attribute__((packed)) fdi_slot { uint64_t key; uint32_t id1; } fdi_slot_t;   // id1 = feature id + 1, 0 = empty slot
static inline uint64_t fdi_key(uint32_t name, uint32_t w1, uint32_t w2) { return ((uint64_t)name << (2 * FDI_WBITS)) | ((uint64_t)w1 << FDI_WBITS) | w2; }
static inline uint64_t fdi_hash(uint64_t k) { k ^= k >> 33; k *= 0xFF51AFD7ED558CCDULL; k ^= k >> 33; k *= 0xC4CEB9FE1A85EC53ULL; return k ^ (k >> 33); }
static inline uint64_t fdi_index(const feature_dict_t *d, uint64_t k) { return (uint64_t)(((__uint128_t)fdi_hash(k) * d->isize) >> 64); }
static inline bool fdi_lookup(const feature_dict_t *d, uint64_t k, uint32_t *id) {
    for (uint64_t i = fdi_index(d, k);; i = i + 1 == d->isize ? 0 : i + 1) {
        const fdi_slot_t *s = &d->islots[i]; TT(TS_ISLOT, s, sizeof *s);
        if (!s->id1) return false;
        if (s->key == k) { *id = s->id1 - 1; return true; }
    }
}
// THE canonicalization used on both sides (model keys at load, emitted features at run time): the vocabulary slot index of s[0..len)
// if it is non-empty, contains no '|' and is a vocabulary key; FDI_NONE otherwise. s[len] must be readable; it need not be NUL.
static inline uint32_t fd_canon(const feature_dict_t *vocab, const char *s, size_t len) {
    if (len == 0 || memchr(s, FD_SEP, len) != NULL) return FDI_NONE;
    char buf[256]; char *heap = NULL; const char *p = s;
    if (s[len] != '\0') {
        char *t = len < sizeof buf ? buf : (heap = malloc(len + 1));
        if (t == NULL) abort();
        memcpy(t, s, len); t[len] = '\0'; p = t;
    }
    uint64_t slot; const uint8_t *e = fd_find_slot(vocab, 0, p, len, &slot);
    free(heap);
    return e ? (uint32_t)slot : FDI_NONE;
}
// Integer key of a (name byte, value) pair if it belongs to the integer table: routed name, value splits on '|' into exactly arity parts, each canonical.
static inline bool fd_int_classify(const feature_dict_t *d, uint8_t nb, const char *rest, size_t rl, uint64_t *key) {
    if (!(nb & 0x80)) return false;
    uint32_t name = nb & 0x7F; int a = d->arity[name];
    if (a == 0) return false;
    const char *bar = memchr(rest, FD_SEP, rl);
    if (a == 1) {
        if (bar) return false;
        uint32_t c = fd_canon(d->vocab, rest, rl); if (c == FDI_NONE) return false;
        *key = fdi_key(name, c, FDI_WNONE); return true;
    }
    if (!bar) return false;
    size_t l1 = (size_t)(bar - rest); const char *p2 = bar + 1; size_t l2 = rl - l1 - 1;
    if (memchr(p2, FD_SEP, l2)) return false;
    uint32_t c1 = fd_canon(d->vocab, rest, l1); if (c1 == FDI_NONE) return false;
    uint32_t c2 = fd_canon(d->vocab, p2, l2); if (c2 == FDI_NONE) return false;
    *key = fdi_key(name, c1, c2); return true;
}
#endif

static inline bool fd_lookup(const feature_dict_t *d, const char *key, size_t len, uint32_t *id) {
    uint8_t nb; const char *rest; size_t rl;
    if (!fd_split(d, key, len, &nb, &rest, &rl)) return false;
#ifdef EXP_INT_KEYS
    uint64_t ik;
    if (d->islots != NULL && fd_int_classify(d, nb, rest, rl, &ik)) return fdi_lookup(d, ik, id);   // routed exactly as the load-time split
#endif
    return fd_lookup_hashed(d, nb, rest, rl, fd_hash_key(nb, rest, rl), id);
}

static inline void fd_destroy(feature_dict_t *d) {
    if (!d) return;
    for (uint32_t i = 0; i < d->num_names; i++) free(d->names[i].str);
    free(d->slots);
#ifdef EXP_INT_KEYS
    free(d->islots);
#endif
    if (d->arena) munmap(d->arena, d->arena_map);
    free(d);
}

static inline double fd_now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

static bool fd_insert(feature_dict_t *d, const char *key, size_t len, uint32_t data) {
    uint8_t nb; const char *rest; size_t rl;
    if (d->split) {
        const char *p = memchr(key, FD_SEP, len);
        size_t nlen = p ? (size_t)(p - key) : len;
        int id = fd_add_name(d, key, nlen);
        if (id < 0) return false;
        nb = (uint8_t)id | (p ? 0x80 : 0);
        rest = p ? p + 1 : key + len; rl = p ? len - nlen - 1 : 0;
    } else { nb = 0; rest = key; rl = len; }
    if (!d->wide_value && data >= 0xFFFFFE) return false;
    size_t entry = rl + 2 + (d->wide_value ? sizeof(uint32_t) : 0);
    if (d->arena_used + entry > d->arena_map || d->arena_used + entry > 0xFFFFFFF0ULL) return false;
    uint64_t h = fd_hash_key(nb, rest, rl);
    uint64_t i = fd_index(d, h);
    for (;; i = fd_next(d, i)) {                     // a trie never stores a key twice; refuse (=> keep the trie) if one appears
        uint64_t s = d->slots[i];
        if (!s) break;
        const uint8_t *e = d->arena + (uint32_t)s;
        if ((uint8_t)(s >> 56) == (uint8_t)h && e[0] == nb && fd_eq(e + 1, rest, rl + 1)) return false;
    }
    uint32_t off = (uint32_t)d->arena_used;
    d->arena[off] = nb; memcpy(d->arena + off + 1, rest, rl); d->arena[off + 1 + rl] = 0;
    if (d->wide_value) memcpy(d->arena + off + 2 + rl, &data, sizeof(uint32_t));
    d->arena_used += entry;
    d->slots[i] = (uint64_t)off | (d->wide_value ? (uint64_t)1 << 32 : (uint64_t)(data + 1) << 32) | ((uint64_t)(uint8_t)h << 56);
    d->num_keys++;
    return true;
}

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

static bool fd_insert_cb(void *ctx, const char *key, size_t len, uint32_t data) { return fd_insert((feature_dict_t *)ctx, key, len, data); }

// Builds a dictionary from every key stored in `trie`. Returns NULL (caller keeps the trie) on any doubt.
static feature_dict_t *fd_build_ex(trie_t *trie, bool split, bool wide_value) {
    double t0 = fd_now();
    if (!trie || trie->alphabet_size != 256 || !trie->nodes || !trie->data || !trie->tail) return NULL;
    for (int i = 0; i < 256; i++) if (trie->alpha_map[(uint8_t)trie->alphabet[i]] != i) return NULL;      // bijective alphabet: no character aliasing
    uint32_t nn = (uint32_t)trie->nodes->n;
    trie_node_t *nodes = trie->nodes->a;
    uint64_t leaves_total = 0;
    for (uint32_t i = TRIE_POOL_BEGIN; i < nn; i++) if (nodes[i].check > 0 && nodes[i].base < 0) leaves_total++;
    if (leaves_total == 0 || (!wide_value && leaves_total >= 0xFFFFFE)) return NULL;

    feature_dict_t *d = calloc(1, sizeof *d); if (!d) return NULL;
    d->split = split; d->wide_value = wide_value;
#if defined(EXP_LOADF)
    uint64_t size = split ? leaves_total * EXP_LOADF + 2 : leaves_total * 2 + 2;                     // experiment 027: feature dictionaries at 1/EXP_LOADF, vocabulary 0.5
#elif defined(EXP_LOAD50)
    uint64_t size = leaves_total * 2 + 2;                                                            // experiment 026: load factor 0.5 (shorter miss probes)
#else
    uint64_t size = leaves_total * 4 / 3 + 2;                                                        // load factor 0.75
#endif
    d->size = size;
    d->slots = calloc(size, sizeof(uint64_t)); if (!d->slots) { free(d); return NULL; }
    d->arena_map = (size_t)0xFFFFFFF0ULL;
    d->arena = mmap(NULL, d->arena_map, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (d->arena == MAP_FAILED) { d->arena = NULL; fd_destroy(d); return NULL; }
    d->leaves_total = leaves_total;

    bool ok = fd_for_each_key(trie, fd_insert_cb, d, &d->leaves_visited, &d->skipped_unreachable);
    if (getenv("FD_VERBOSE")) fprintf(stderr, "fd_build: ok=%d leaves_total=%llu visited=%llu skipped=%llu inserted=%llu names=%u arena_used=%zu\n", ok,
        (unsigned long long)d->leaves_total, (unsigned long long)d->leaves_visited, (unsigned long long)d->skipped_unreachable, (unsigned long long)d->num_keys, d->num_names, d->arena_used);
    if (!ok || d->leaves_visited != d->leaves_total) { fd_destroy(d); return NULL; }        // enumeration must reach every valid leaf
    size_t page = 16384, keep = (d->arena_used + page - 1) / page * page;                   // give back the unused reservation
    if (keep < d->arena_map) munmap(d->arena + keep, d->arena_map - keep);
    d->arena_map = keep;
    d->build_seconds = fd_now() - t0;
    return d;
}
static feature_dict_t *fd_build(trie_t *trie) { return fd_build_ex(trie, true, false); }

#ifdef EXP_INT_KEYS
// Splits a built feature dictionary: keys that fd_int_classify accepts go to an integer table, all others to a new string dictionary with the same
// name ids. Returns NULL (caller keeps `old`) on any doubt: count mismatch, duplicate integer key, packing overflow.
static feature_dict_t *fd_int_split(const feature_dict_t *old, const feature_dict_t *vocab, const int8_t *arity) {
    double t0 = fd_now();
    if (!old || !old->split || old->wide_value || old->islots || !vocab || vocab->split || vocab->size >= FDI_WNONE || old->num_names > 127) return NULL;
    feature_dict_t *d = calloc(1, sizeof *d); if (!d) return NULL;
    d->split = true; d->wide_value = false; d->vocab = vocab;
    memcpy(d->arity, arity, sizeof d->arity);
    for (uint32_t i = 0; i < old->num_names; i++) {
        d->names[i] = old->names[i];
        d->names[i].str = malloc(old->names[i].len + 1);
        if (!d->names[i].str) { d->num_names = i; fd_destroy(d); return NULL; }
        memcpy(d->names[i].str, old->names[i].str, old->names[i].len + 1);
    }
    d->num_names = old->num_names;
    memcpy(d->name_table, old->name_table, sizeof d->name_table);
    uint64_t moved = 0, kept = 0, ik;
    for (uint64_t i = 0; i < old->size; i++) {                                     // pass 1: count
        uint64_t s = old->slots[i]; if (!s) continue;
        const uint8_t *e = old->arena + (uint32_t)s; const char *rest = (const char *)e + 1;
        if (fd_int_classify(d, e[0], rest, strlen(rest), &ik)) moved++; else kept++;
    }
    if (moved + kept != old->num_keys || moved == 0 || kept == 0) { fd_destroy(d); return NULL; }
#if defined(EXP_LOADF)
    d->size = kept * EXP_LOADF + 2; d->isize = moved * EXP_LOADF + 2;
#elif defined(EXP_LOAD50)
    d->size = kept * 2 + 2; d->isize = moved * 2 + 2;
#else
    d->size = kept * 4 / 3 + 2; d->isize = moved * 4 / 3 + 2;
#endif
    d->slots = calloc(d->size, sizeof(uint64_t)); d->islots = calloc(d->isize, sizeof(fdi_slot_t));
    size_t page = 16384; d->arena_map = (old->arena_used + page - 1) / page * page;
    d->arena = mmap(NULL, d->arena_map, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (d->arena == MAP_FAILED) d->arena = NULL;
    if (!d->slots || !d->islots || !d->arena) { fd_destroy(d); return NULL; }
    for (uint64_t i = 0; i < old->size; i++) {                                     // pass 2: insert
        uint64_t s = old->slots[i]; if (!s) continue;
        const uint8_t *e = old->arena + (uint32_t)s; const char *rest = (const char *)e + 1; size_t rl = strlen(rest);
        uint32_t id1 = (uint32_t)((s >> 32) & 0xFFFFFF);
        if (fd_int_classify(d, e[0], rest, rl, &ik)) {
            uint64_t j = fdi_index(d, ik);
            for (; d->islots[j].id1; j = j + 1 == d->isize ? 0 : j + 1) if (d->islots[j].key == ik) { fd_destroy(d); return NULL; }   // duplicate: refuse
            d->islots[j].key = ik; d->islots[j].id1 = id1; d->inum++;
        } else {
            size_t entry = rl + 2, off = d->arena_used;
            if (off + entry > d->arena_map) { fd_destroy(d); return NULL; }
            memcpy(d->arena + off, e, entry);
            uint64_t h = fd_hash_key(e[0], rest, rl), j = fd_index(d, h);
            while (d->slots[j]) j = fd_next(d, j);
            d->slots[j] = (uint64_t)off | ((uint64_t)id1 << 32) | ((uint64_t)(uint8_t)h << 56);
            d->arena_used += entry; d->num_keys++;
        }
    }
    if (d->inum != moved || d->num_keys != kept) { fd_destroy(d); return NULL; }
    size_t keep = (d->arena_used + page - 1) / page * page;
    if (keep < d->arena_map) { munmap(d->arena + keep, d->arena_map - keep); d->arena_map = keep; }
    d->leaves_total = old->num_keys;
    d->build_seconds = fd_now() - t0;
    return d;
}
#endif
#endif
