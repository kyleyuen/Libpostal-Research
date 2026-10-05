// Experiment 006 prototype (measurement only): builds the exact dictionary from the loaded baseline tries, verifies it against them
// exhaustively and by fuzzing, compares memory, and times trie vs dictionary lookups on recorded corpus keys. No library change.
// usage: dict_proto dev_features.tsv novel_features.tsv fuzz_count
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "libpostal.h"
#include "address_parser.h"
#include "crf.h"
#include "trie.h"
#include "feature_dict.h"

static inline uint64_t nowns(void) { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW); }
static uint64_t rs = 88172645463325252ULL;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }

typedef struct { trie_t *trie; feature_dict_t *d; uint64_t checked, mismatches; } verify_ctx_t;
static bool verify_cb(void *c, const char *key, size_t len, uint32_t data) {
    verify_ctx_t *v = c; uint32_t a = 0, b = 0;
    bool ta = trie_get_data(v->trie, (char *)key, &a), tb = fd_lookup(v->d, key, len, &b);
    v->checked++;
    if (!ta || !tb || a != data || b != data) v->mismatches++;
    return true;
}

typedef struct { char **k; size_t n; } keys_t;
static keys_t load_keys(const char *path, char kind) {
    FILE *f = fopen(path, "r"); keys_t r = {0}; size_t cap = 0; char *line = NULL; size_t lc = 0; ssize_t len;
    while ((len = getline(&line, &lc, f)) >= 0) {
        if (line[0] != kind) continue;
        while (len && line[len - 1] == '\n') line[--len] = 0;
        char *p = line; for (int t = 0; t < 3; t++) p = strchr(p, '\t') + 1;
        if (r.n == cap) { cap = cap ? cap * 2 : 1 << 16; r.k = realloc(r.k, cap * sizeof(char *)); }
        r.k[r.n++] = strdup(p);
    }
    fclose(f); return r;
}

static double time_trie(trie_t *t, char **k, size_t n, size_t *order, uint64_t *hits) {
    uint32_t id, h = 0; uint64_t t0 = nowns();
    for (size_t i = 0; i < n; i++) h += trie_get_data(t, k[order ? order[i] : i], &id);
    double ns = (double)(nowns() - t0); *hits = h; return ns / n;
}
static double time_dict(feature_dict_t *d, char **k, size_t n, size_t *order, uint64_t *hits) {
    uint32_t id, h = 0; uint64_t t0 = nowns();
    for (size_t i = 0; i < n; i++) { char *s = k[order ? order[i] : i]; h += fd_lookup(d, s, strlen(s), &id); }
    double ns = (double)(nowns() - t0); *hits = h; return ns / n;
}
// two-pass: hash + prefetch slots for a window of keys, then probe/verify
static double time_dict_batched(feature_dict_t *d, char **k, size_t n, size_t *order, uint64_t *hits, size_t win) {
    uint32_t id, h = 0; uint64_t t0 = nowns();
    uint8_t nb[256]; const char *rest[256]; size_t rl[256]; uint64_t hh[256]; bool ok[256];
    for (size_t base = 0; base < n; base += win) {
        size_t m = n - base < win ? n - base : win;
        for (size_t j = 0; j < m; j++) {
            char *s = k[order ? order[base + j] : base + j];
            ok[j] = fd_split(d, s, strlen(s), &nb[j], &rest[j], &rl[j]);
            if (ok[j]) { hh[j] = fd_hash_key(nb[j], rest[j], rl[j]); __builtin_prefetch(&d->slots[fd_index(d, hh[j])]); }
        }
        for (size_t j = 0; j < m; j++) if (ok[j]) h += fd_lookup_hashed(d, nb[j], rest[j], rl[j], hh[j], &id);
    }
    double ns = (double)(nowns() - t0); *hits = h; return ns / n;
}

static size_t trie_bytes(trie_t *t) { return t->nodes->n * sizeof(trie_node_t) + t->tail->n + t->data->n * sizeof(trie_data_node_t); }

int main(int argc, char **argv) {
    if (argc != 4) return 2;
    size_t fuzz_n = strtoull(argv[3], 0, 10);
    double s0 = fd_now();
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    printf("setup_seconds=%.2f\n", fd_now() - s0);
    crf_t *crf = get_address_parser()->model.crf;
    struct { const char *name; trie_t *t; char kind; } tries[2] = {{"state_features", crf->state_features, 'S'}, {"state_trans_features", crf->state_trans_features, 'P'}};
    const char *files[2] = {argv[1], argv[2]}; const char *fnames[2] = {"dev", "novel"};
    for (int ti = 0; ti < 2; ti++) {
        trie_t *t = tries[ti].t;
        printf("\n=== %s: trie nodes=%zu tail=%zu data=%zu num_keys(header)=%u alphabet=%u  trie_bytes=%zu (%.1f MB)\n", tries[ti].name,
               (size_t)t->nodes->n, (size_t)t->tail->n, (size_t)t->data->n, t->num_keys, t->alphabet_size, trie_bytes(t), trie_bytes(t) / 1048576.0);
        feature_dict_t *d = fd_build(t);
        if (!d) { printf("fd_build returned NULL (would keep the trie)\n"); continue; }
        size_t dict_bytes = d->size * 8 + d->arena_used;
        printf("dict: keys=%llu leaves_total=%llu visited=%llu skipped_unreachable=%llu names=%u build_seconds=%.2f table=%.1f MB (%llu slots, load %.2f) arena=%.1f MB (mean %.2f B/key) total=%.1f MB = %.1f%% of trie\n",
               (unsigned long long)d->num_keys, (unsigned long long)d->leaves_total, (unsigned long long)d->leaves_visited, (unsigned long long)d->skipped_unreachable,
               d->num_names, d->build_seconds, d->size * 8 / 1048576.0, (unsigned long long)d->size, (double)d->num_keys / d->size, d->arena_used / 1048576.0,
               (double)d->arena_used / d->num_keys, dict_bytes / 1048576.0, 100.0 * dict_bytes / trie_bytes(t));
        printf("names:"); for (uint32_t i = 0; i < d->num_names; i++) printf(" [%s]", d->names[i].str); printf("\n");
        verify_ctx_t v = {t, d, 0, 0}; uint64_t lv, sk; double v0 = fd_now();
        fd_for_each_key(t, verify_cb, &v, &lv, &sk);
        printf("exhaustive round-trip: checked=%llu mismatches=%llu (%.1fs)\n", (unsigned long long)v.checked, (unsigned long long)v.mismatches, fd_now() - v0);
        keys_t dev = load_keys(files[0], tries[ti].kind);
        uint64_t fz = 0, fzm = 0, fz_hit = 0; char buf[1024];
        for (size_t it = 0; it < fuzz_n; it++) {
            char *src = dev.k[rnd() % dev.n]; size_t L = strlen(src); if (L == 0 || L > 900) continue;
            memcpy(buf, src, L + 1); int mode = rnd() % 8;
            if (mode == 1 && L > 1) { size_t p = rnd() % L; memmove(buf + p, buf + p + 1, L - p); }
            else if (mode == 2) { buf[rnd() % L] = (char)(1 + rnd() % 255); }
            else if (mode == 3) { size_t p = rnd() % (L + 1); memmove(buf + p + 1, buf + p, L - p + 1); buf[p] = (char)(1 + rnd() % 255); }
            else if (mode == 4) { buf[L] = (char)(32 + rnd() % 95); buf[L + 1] = 0; }
            else if (mode == 5 && L > 1) { buf[1 + rnd() % (L - 1)] = 0; }
            else if (mode == 6) { char *bar = strchr(buf, '|'); if (bar) *bar = (char)(33 + rnd() % 60); }
            else if (mode == 7) { size_t p = rnd() % L; buf[p] = '|'; }
            uint32_t a = 0, b = 0; size_t bl = strlen(buf);
            bool ta = trie_get_data(t, buf, &a), tb = fd_lookup(d, buf, bl, &b);
            fz++; if (ta) fz_hit++;
            if (ta != tb || (ta && a != b)) fzm++;
        }
        printf("fuzz: mutations=%llu (trie hits %llu) mismatches=%llu\n", (unsigned long long)fz, (unsigned long long)fz_hit, (unsigned long long)fzm);
        for (int fi = 0; fi < 2; fi++) {
            keys_t ks = load_keys(files[fi], tries[ti].kind);
            size_t *shuf = malloc(ks.n * sizeof(size_t)); for (size_t i = 0; i < ks.n; i++) shuf[i] = i;
            for (size_t i = ks.n - 1; i > 0; i--) { size_t j = rnd() % (i + 1); size_t x = shuf[i]; shuf[i] = shuf[j]; shuf[j] = x; }
            for (int rep = 0; rep < 3; rep++) {
                uint64_t h1, h2, h3, h4, h5;
                double a = time_trie(t, ks.k, ks.n, NULL, &h1), b = time_dict(d, ks.k, ks.n, NULL, &h2), c = time_dict_batched(d, ks.k, ks.n, NULL, &h3, 32);
                double a2 = time_trie(t, ks.k, ks.n, shuf, &h4), b2 = time_dict(d, ks.k, ks.n, shuf, &h5);
                printf("timing %s keys=%zu rep%d: corpus-order trie %.1f | dict %.1f | dict-batched(32) %.1f ns/lookup || shuffled trie %.1f | dict %.1f  (hits %llu/%llu/%llu %llu/%llu)\n",
                       fnames[fi], ks.n, rep, a, b, c, a2, b2, (unsigned long long)h1, (unsigned long long)h2, (unsigned long long)h3, (unsigned long long)h4, (unsigned long long)h5);
            }
            uint32_t id; size_t mm = 0; for (size_t i = 0; i < ks.n; i++) { bool x = trie_get_data(t, ks.k[i], &id); uint32_t id2 = 0; bool y = fd_lookup(d, ks.k[i], strlen(ks.k[i]), &id2); if (x != y || (x && id != id2)) mm++; }
            printf("corpus-key agreement %s: keys=%zu mismatches=%zu\n", fnames[fi], ks.n, mm);
        }
        fd_destroy(d);
    }
    return 0;
}
