// 151: memory-access simulation of the feature/weight stage under two layouts, driven by the exact per-token keys
// and ids of the 142 oracles (O3k + O3i in lockstep) and the model's row sizes. Nothing here is timed.
//
// Layout P ("probe+row", today's design at its best): each emitted key costs one random probe line in a feature-key
//   hash table; each hit then reads its weight row in a packed co-located f64 record (feature-id order). Context-fill
//   trie walks are NOT included (045 measured them separately), which flatters P.
// Layout C ("compiled lexicon records"): rows grouped by the string they are keyed on -- one record per word/phrase
//   unit (word, prev word, next word, first word, P word, P prev word, phrase, phrase type+phrase, unambiguous phrase
//   type+phrase, commonly *, sub_word/prefix/suffix), one per word pair (prev word+word and word+next word share
//   it), one per remaining key (bias, postcode*, ...). One index line, then the record's lines (f32 value + 1/2-byte
//   label; legal only with certified argmax, see oracle_bound). Record size counts only rows this address uses
//   (lower bound). Placement pseudo-random in an arena of the packed-f32 model size.
// Output: distinct 128-B lines per address and fully associative LRU misses (16 / 48 MiB) on the last of PASSES
//   passes in corpus order, as in 045.
// usage: sim_layout MODEL.dat ORACLE_DIR PASSES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "crf.h"

static uint64_t fnv(const char *s, uint64_t h) { while (*s) { h ^= (unsigned char)*s++; h *= 0x100000001b3ull; } return h; }
static uint64_t mix(uint64_t x) { x ^= x >> 33; x *= 0xff51afd7ed558ccdull; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ull; x ^= x >> 33; return x; }

typedef struct { uint64_t *key; int32_t *prev, *next, *slot; uint32_t hmask; int32_t head, tail, n, cap; long miss; } lru_t;
static void lru_init(lru_t *c, int cap) {
    memset(c, 0, sizeof *c); c->cap = cap; c->key = malloc(8 * cap); c->prev = malloc(4 * cap); c->next = malloc(4 * cap);
    uint32_t hs = 1; while (hs < (uint32_t)cap * 2) hs <<= 1; c->hmask = hs - 1;
    c->slot = malloc(4 * (size_t)hs); for (uint32_t i = 0; i < hs; i++) c->slot[i] = -1; c->head = c->tail = -1;
}
static int32_t lru_find(lru_t *c, uint64_t k, uint32_t *pos) {
    uint32_t h = (uint32_t)mix(k) & c->hmask;
    for (;;) { int32_t s = c->slot[h]; if (s < 0) { *pos = h; return -1; } if (c->key[s] == k) { *pos = h; return s; } h = (h + 1) & c->hmask; }
}
static void unlink_(lru_t *c, int32_t s) {
    if (c->prev[s] >= 0) c->next[c->prev[s]] = c->next[s]; else c->head = c->next[s];
    if (c->next[s] >= 0) c->prev[c->next[s]] = c->prev[s]; else c->tail = c->prev[s];
}
static void push_front(lru_t *c, int32_t s) { c->prev[s] = -1; c->next[s] = c->head; if (c->head >= 0) c->prev[c->head] = s; c->head = s; if (c->tail < 0) c->tail = s; }
static void hdel(lru_t *c, uint64_t k) {      // backward-shift delete (linear probing)
    uint32_t pos; if (lru_find(c, k, &pos) < 0) return;
    uint32_t i = pos, j = pos;
    for (;;) {
        j = (j + 1) & c->hmask; if (c->slot[j] < 0) break;
        uint32_t h = (uint32_t)mix(c->key[c->slot[j]]) & c->hmask;
        int move = (i <= j) ? (h <= i || h > j) : (h <= i && h > j);
        if (move) { c->slot[i] = c->slot[j]; i = j; }
    }
    c->slot[i] = -1;
}
static void lru_access(lru_t *c, uint64_t k) {
    uint32_t pos; int32_t s = lru_find(c, k, &pos);
    if (s >= 0) { unlink_(c, s); push_front(c, s); return; }
    c->miss++;
    if (c->n < c->cap) s = c->n++;
    else { s = c->tail; unlink_(c, s); hdel(c, c->key[s]); lru_find(c, k, &pos); }
    c->key[s] = k; c->slot[pos] = s; push_front(c, s);
}

typedef struct { FILE *f; char *line; size_t cap; int have; long a; int t; char k; char *key; } st_t;
static void nx(st_t *s) {
    s->have = 0; if (getline(&s->line, &s->cap, s->f) <= 0) return;
    char *p = s->line, *e; s->a = strtol(p, &e, 10); p = e + 1; s->t = (int)strtol(p, &e, 10); p = e + 1; s->k = *p; p += 2;
    char *q = strchr(p, '\t'); p = q ? q + 1 : p + strlen(p);
    size_t n = strlen(p); if (n && p[n - 1] == '\n') p[n - 1] = 0; s->key = p; s->have = 1;
}
static FILE *gz(const char *dir, const char *name) { char cmd[4096]; snprintf(cmd, sizeof cmd, "gzip -dc '%s/%s.gz'", dir, name); return popen(cmd, "r"); }

typedef struct { uint64_t *v; int n, cap; } vec_t;
static void push(vec_t *x, uint64_t v) { if (x->n == x->cap) { x->cap = x->cap ? x->cap * 2 : 256; x->v = realloc(x->v, 8 * x->cap); } x->v[x->n++] = v; }
static void range(vec_t *x, int tag, uint64_t b0, uint64_t nb) { if (!nb) return; for (uint64_t l = b0 >> 7; l <= (b0 + nb - 1) >> 7; l++) push(x, ((uint64_t)tag << 56) | l); }
static int cmpu(const void *a, const void *b) { uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b; return x < y ? -1 : x > y; }

static int group_of(const char *fam) {   // 0 unit record, 1 pair record, 2 own-key record
    static const char *unit[] = { "word", "prev word", "next word", "first word", "phrase", "phrase type+phrase",
        "unambiguous phrase type+phrase", "sub_word", "prefix", "suffix", NULL };
    if (!strncmp(fam, "commonly ", 9)) return 0;
    for (int i = 0; unit[i]; i++) if (!strcmp(fam, unit[i])) return 0;
    if (!strcmp(fam, "prev word+word") || !strcmp(fam, "word+next word")) return 1;
    return 2;
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s MODEL ORACLE_DIR PASSES\n", argv[0]); return 2; }
    int passes = atoi(argv[3]);
    crf_t *crf = crf_load(argv[1]); if (!crf) return 2;
    uint32_t *ipS = crf->weights->indptr->a, *ipP = crf->state_trans_weights->indptr->a;
    uint32_t mS = crf->weights->m, mP = crf->state_trans_weights->m;
    uint64_t *offS = malloc(8 * ((size_t)mS + 1)), *offP = malloc(8 * ((size_t)mP + 1)), o = 0, arena32 = 0;
    for (uint32_t f = 0; f < mS; f++) { uint32_t n = ipS[f + 1] - ipS[f]; offS[f] = o; o += ((uint64_t)n * 9 + 2 + 7) & ~7ull; arena32 += n * 5ull + 2; }
    offS[mS] = o; o = 0;
    for (uint32_t f = 0; f < mP; f++) { uint32_t n = ipP[f + 1] - ipP[f]; offP[f] = o; o += ((uint64_t)n * 10 + 2 + 7) & ~7ull; arena32 += n * 6ull + 2; }
    offP[mP] = o;
    const uint64_t KEYLINES = ((uint64_t)(mS + mP) * 16 * 2) / 128, IDXLINES = KEYLINES / 4;

    st_t k = { .f = gz(argv[2], "O3k.txt") }, d = { .f = gz(argv[2], "O3i.txt") };
    nx(&k); nx(&d);
    int nA = 0, capA = 0; vec_t *AP = NULL, *AC = NULL;
    long probesP = 0, rowsP = 0, recC = 0, recLinesC = 0;
    enum { RN = 4096 }; static uint64_t rk[RN], rb[RN]; static int rused[RN]; static int rlist[RN]; int nr;
    while (k.have) {
        long a = k.a;
        if (nA == capA) { capA = capA ? capA * 2 : 1024; AP = realloc(AP, capA * sizeof(vec_t)); AC = realloc(AC, capA * sizeof(vec_t)); }
        memset(&AP[nA], 0, sizeof(vec_t)); memset(&AC[nA], 0, sizeof(vec_t)); nr = 0;
        while (k.have && k.a == a) {
            if (!d.have || d.a != k.a || d.t != k.t || d.k != k.k) { fprintf(stderr, "O3k/O3i out of step at a=%ld\n", a); return 3; }
            const char *key = k.key, *bar = strchr(key, '|');
            char fam[128]; size_t fl = bar ? (size_t)(bar - key) : strlen(key); if (fl > 127) fl = 127; memcpy(fam, key, fl); fam[fl] = 0;
            int hit = d.key[0] != '-'; uint32_t id = hit ? (uint32_t)strtoul(d.key, NULL, 10) : 0;
            push(&AP[nA], (1ull << 56) | (mix(fnv(key, k.k == 'P' ? 0x9ae16a3b2f90404full : 0xcbf29ce484222325ull)) % KEYLINES)); probesP++;
            uint64_t rowbytes = 0;
            if (hit) {
                if (k.k == 'S') { range(&AP[nA], 2, offS[id], offS[id + 1] - offS[id]); rowbytes = (ipS[id + 1] - ipS[id]) * 5ull; }
                else            { range(&AP[nA], 3, offP[id], offP[id + 1] - offP[id]); rowbytes = (ipP[id + 1] - ipP[id]) * 6ull; }
                rowsP++;
            }
            int g = group_of(fam); const char *arg = bar ? bar + 1 : key; uint64_t rkey;
            if (g == 0) { const char *last = strrchr(arg, '|'); rkey = fnv(last ? last + 1 : arg, 0x1111); }
            else if (g == 1) rkey = fnv(arg, 0x2222);
            else rkey = fnv(key, 0x3333);
            uint32_t h = (uint32_t)mix(rkey) & (RN - 1);
            while (rused[h] && rk[h] != rkey) h = (h + 1) & (RN - 1);
            if (!rused[h]) { rused[h] = 1; rk[h] = rkey; rb[h] = 2; rlist[nr++] = h; }
            rb[h] += rowbytes;
            nx(&k); nx(&d);
        }
        for (int i = 0; i < nr; i++) {
            int h = rlist[i];
            push(&AC[nA], (4ull << 56) | (mix(rk[h]) % IDXLINES));
            int before = AC[nA].n; range(&AC[nA], 5, mix(rk[h] ^ 0x5555) % arena32, rb[h]); recLinesC += AC[nA].n - before; recC++;
            rused[h] = 0;
        }
        nA++;
    }
    double dP = 0, dC = 0;
    for (int i = 0; i < nA; i++) for (int w = 0; w < 2; w++) {
        vec_t *x = w ? &AC[i] : &AP[i]; if (!x->n) continue;
        uint64_t *t = malloc(8 * x->n); memcpy(t, x->v, 8 * x->n); qsort(t, x->n, 8, cmpu);
        int u = 1; for (int p = 1; p < x->n; p++) if (t[p] != t[p - 1]) u++;
        if (w) dC += u; else dP += u; free(t);
    }
    printf("addresses %d  passes %d\n", nA, passes);
    printf("layout P (probe + packed-f64 row): probes/addr %.2f  rows/addr %.2f  distinct lines/addr %.2f\n", (double)probesP / nA, (double)rowsP / nA, dP / nA);
    printf("layout C (compiled records, f32): records/addr %.2f  record lines/addr %.2f  distinct lines/addr incl. index %.2f\n",
           (double)recC / nA, (double)recLinesC / nA, dC / nA);
    int caps[] = { 16 << 20, 48 << 20 };
    for (int ci = 0; ci < 2; ci++) for (int w = 0; w < 2; w++) {
        lru_t c; lru_init(&c, caps[ci] / 128); long m0 = 0;
        for (int p = 0; p < passes; p++) {
            if (p == passes - 1) m0 = c.miss;
            for (int i = 0; i < nA; i++) { vec_t *x = w ? &AC[i] : &AP[i]; for (int q = 0; q < x->n; q++) lru_access(&c, x->v[q]); }
        }
        printf("LRU %2d MiB layout %c: last-pass misses/addr %.2f\n", caps[ci] >> 20, w ? 'C' : 'P', (double)(c.miss - m0) / nA);
    }
    return 0;
}
