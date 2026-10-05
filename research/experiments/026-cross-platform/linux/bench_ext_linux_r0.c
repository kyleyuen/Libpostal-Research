// Extended benchmark (experiment protocol v2): cold first pass, warm passes, per-country timing, setup time. One thread.
// usage: bench_ext target.tsv passes [warmup.tsv]   (files: country<TAB>address per line)   Times parse + response destroy only.
// If warmup.tsv is given it is parsed once, untimed, before pass 1: the model is paged in and exercised on OTHER data, so pass 1 on the
// target is 'first exposure to unseen addresses with a resident model' (the realistic new-dataset case), not a model page-in measurement.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include "libpostal.h"
static inline uint64_t nowns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (uint64_t)t.tv_sec * 1000000000ULL + (uint64_t)t.tv_nsec;
}
#define MAXC 256
int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) return 2;
    int passes = atoi(argv[2]);
    FILE *f = fopen(argv[1], "r"); if (!f) return 2;
    char **addr = NULL; int *cid = NULL; size_t n = 0, cap = 0; char *line = NULL; size_t lc = 0; ssize_t len;
    char cnames[MAXC][8]; int nc = 0;
    while ((len = getline(&line, &lc, f)) >= 0) {
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        char *tab = strchr(line, '\t'); if (!tab) return 2; *tab = 0;
        int c = -1; for (int i = 0; i < nc; i++) if (!strcmp(cnames[i], line)) { c = i; break; }
        if (c < 0) { if (nc == MAXC) return 2; strncpy(cnames[nc], line, 7); cnames[nc][7] = 0; c = nc++; }
        if (n == cap) { cap = cap ? cap * 2 : 1024; addr = realloc(addr, cap * sizeof *addr); cid = realloc(cid, cap * sizeof *cid); }
        addr[n] = strdup(tab + 1); cid[n] = c; n++;
    }
    uint64_t s0 = nowns();
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    printf("setup_seconds=%.3f corpus=%zu countries=%d\n", (nowns() - s0) / 1e9, n, nc);
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    if (argc == 4) {
        FILE *w = fopen(argv[3], "r"); char *wl = NULL; size_t wc = 0; ssize_t wn; size_t wcount = 0;
        if (!w) return 2;
        while ((wn = getline(&wl, &wc, w)) >= 0) {
            char *t = strchr(wl, '\t'); if (!t) continue;
            while (wn && (wl[wn - 1] == '\n' || wl[wn - 1] == '\r')) wl[--wn] = 0;
            libpostal_address_parser_response_t *r = libpostal_parse_address(t + 1, o);
            if (r) libpostal_address_parser_response_destroy(r);
            wcount++;
        }
        printf("warmup_addresses=%zu file=%s\n", wcount, argv[3]);
    }
    double *ct = calloc(nc, sizeof(double)); size_t *cn = calloc(nc, sizeof(size_t)); uint64_t comps = 0;
    for (int p = 1; p <= passes; p++) {
        memset(ct, 0, nc * sizeof(double)); memset(cn, 0, nc * sizeof(size_t));
        struct rusage ru0; getrusage(RUSAGE_SELF, &ru0);
        uint64_t t0 = nowns();
        for (size_t i = 0; i < n; i++) {
            uint64_t a = nowns();
            libpostal_address_parser_response_t *r = libpostal_parse_address(addr[i], o);
            if (!r || !r->num_components) return 3;
            comps += r->num_components; libpostal_address_parser_response_destroy(r);
            ct[cid[i]] += nowns() - a; cn[cid[i]]++;
        }
        double tot = (nowns() - t0) / 1e3;
        struct rusage ru1; getrusage(RUSAGE_SELF, &ru1);
        double cpu_us = (ru1.ru_utime.tv_sec - ru0.ru_utime.tv_sec) * 1e6 + (ru1.ru_utime.tv_usec - ru0.ru_utime.tv_usec)
                      + (ru1.ru_stime.tv_sec - ru0.ru_stime.tv_sec) * 1e6 + (ru1.ru_stime.tv_usec - ru0.ru_stime.tv_usec);
        printf("pass=%d us_per_addr=%.3f addresses_per_second=%.1f cpu_us_per_addr=%.3f nivcsw=%ld nvcsw=%ld\n", p, tot / n, n / (tot / 1e6), cpu_us / n,
               ru1.ru_nivcsw - ru0.ru_nivcsw, ru1.ru_nvcsw - ru0.ru_nvcsw);
        if (p == 1 || p == passes) for (int c = 0; c < nc; c++) printf("country pass=%d cc=%s n=%zu us_per_addr=%.3f\n", p, cnames[c], cn[c], ct[c] / 1e3 / cn[c]);
    }
    struct rusage u; getrusage(RUSAGE_SELF, &u);
    printf("components=%llu peak_rss_bytes=%ld\n", (unsigned long long)comps, u.ru_maxrss * 1024L);
    return 0;
}
