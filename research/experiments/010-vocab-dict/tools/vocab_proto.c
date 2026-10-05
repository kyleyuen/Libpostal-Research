// Experiment 010 prototype (measurement only): exact dictionary for the vocabulary trie. Exhaustive round trip, fuzz, size, timing.
// usage: vocab_proto dev_features.tsv fuzz_count
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "libpostal.h"
#include "address_parser.h"
#include "trie.h"
#include "feature_dict.h"
static inline uint64_t nowns(void) { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW); }
static uint64_t rs = 88172645463325252ULL;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
typedef struct { trie_t *t; feature_dict_t *d; uint64_t checked, mism; } vctx_t;
static bool vcb(void *c, const char *key, size_t len, uint32_t data) {
    vctx_t *v = c; uint32_t a = 0, b = 0; bool ta = trie_get_data(v->t, (char *)key, &a), tb = fd_lookup(v->d, key, len, &b);
    v->checked++; if (!ta || !tb || a != data || b != data) v->mism++; return true;
}
int main(int argc, char **argv) {
    if (argc != 3) return 2; size_t fuzz_n = strtoull(argv[2], 0, 10);
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    trie_t *t = get_address_parser()->vocab;
    size_t tb = t->nodes->n * 8 + t->tail->n + t->data->n * 8;
    printf("vocab trie: nodes=%zu tail=%zu data=%zu keys(header)=%u alphabet=%u bytes=%.1f MB\n", (size_t)t->nodes->n, (size_t)t->tail->n, (size_t)t->data->n, t->num_keys, t->alphabet_size, tb / 1048576.0);
    feature_dict_t *d = fd_build_ex(t, false, true);
    if (!d) { printf("fd_build_ex returned NULL (would keep the trie)\n"); return 0; }
    size_t db = d->size * 8 + d->arena_used;
    printf("dict: keys=%llu leaves_total=%llu visited=%llu skipped=%llu build=%.2fs table=%.1f MB (%llu slots) arena=%.1f MB (mean %.2f B/key) total=%.1f MB = %.1f%% of trie\n",
           (unsigned long long)d->num_keys, (unsigned long long)d->leaves_total, (unsigned long long)d->leaves_visited, (unsigned long long)d->skipped_unreachable, d->build_seconds,
           d->size * 8 / 1048576.0, (unsigned long long)d->size, d->arena_used / 1048576.0, (double)d->arena_used / d->num_keys, db / 1048576.0, 100.0 * db / tb);
    vctx_t v = {t, d, 0, 0}; uint64_t lv, sk; fd_for_each_key(t, vcb, &v, &lv, &sk);
    printf("exhaustive round-trip: checked=%llu mismatches=%llu\n", (unsigned long long)v.checked, (unsigned long long)v.mism);
    // words = single-value parts of recorded 'word|X' features
    FILE *f = fopen(argv[1], "r"); char *line = NULL; size_t lc = 0; ssize_t len; char **w = NULL; size_t n = 0, cap = 0;
    while ((len = getline(&line, &lc, f)) >= 0) {
        if (line[0] != 'S') continue; while (len && line[len - 1] == '\n') line[--len] = 0;
        char *p = line; for (int k = 0; k < 3; k++) p = strchr(p, '\t') + 1;
        char *bar = strchr(p, '|'); if (!bar || strchr(bar + 1, '|')) continue;
        if (n == cap) { cap = cap ? cap * 2 : 1 << 16; w = realloc(w, cap * sizeof(char *)); } w[n++] = strdup(bar + 1);
    }
    printf("query words: %zu\n", n);
    uint64_t fz = 0, fzm = 0, fzh = 0; char buf[512];
    for (size_t it = 0; it < fuzz_n; it++) {
        char *src = w[rnd() % n]; size_t L = strlen(src); if (L == 0 || L > 400) continue; memcpy(buf, src, L + 1); int mode = rnd() % 6;
        if (mode == 1 && L > 1) { size_t p = rnd() % L; memmove(buf + p, buf + p + 1, L - p); }
        else if (mode == 2) buf[rnd() % L] = (char)(1 + rnd() % 255);
        else if (mode == 3) { size_t p = rnd() % (L + 1); memmove(buf + p + 1, buf + p, L - p + 1); buf[p] = (char)(1 + rnd() % 255); }
        else if (mode == 4) { buf[L] = (char)(32 + rnd() % 95); buf[L + 1] = 0; }
        else if (mode == 5 && L > 1) buf[1 + rnd() % (L - 1)] = 0;
        uint32_t a = 0, b = 0; bool x = trie_get_data(t, buf, &a), y = fd_lookup(d, buf, strlen(buf), &b);
        fz++; if (x) fzh++; if (x != y || (x && a != b)) fzm++;
    }
    printf("fuzz: mutations=%llu (trie hits %llu) mismatches=%llu\n", (unsigned long long)fz, (unsigned long long)fzh, (unsigned long long)fzm);
    size_t *sh = malloc(n * sizeof(size_t)); for (size_t i = 0; i < n; i++) sh[i] = i; for (size_t i = n - 1; i > 0; i--) { size_t j = rnd() % (i + 1); size_t x = sh[i]; sh[i] = sh[j]; sh[j] = x; }
    for (int rep = 0; rep < 3; rep++) for (int ord = 0; ord < 2; ord++) {
        uint32_t id, h1 = 0, h2 = 0; uint64_t t0 = nowns();
        for (size_t i = 0; i < n; i++) h1 += trie_get_data(t, w[ord ? sh[i] : i], &id); double a = (double)(nowns() - t0) / n; t0 = nowns();
        for (size_t i = 0; i < n; i++) { char *s = w[ord ? sh[i] : i]; h2 += fd_lookup(d, s, strlen(s), &id); } double b = (double)(nowns() - t0) / n;
        printf("timing %s rep%d: trie %.1f ns/lookup | dict %.1f ns/lookup (hits %u/%u)\n", ord ? "shuffled" : "corpus-order", rep, a, b, h1, h2);
    }
    return 0;
}
