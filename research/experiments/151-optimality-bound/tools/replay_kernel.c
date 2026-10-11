// 151: measured floor of the BACK HALF (accumulate + Viterbi) on this machine, replaying the exact feature ids of the
// 142 oracles (O3i; token counts from O4) against the real model rows. The front half (tokenize, context fill, key
// build, key lookup) is given for free, so these are bounds, never candidates.
//   K0  original: CSR arrays, original loop order, double, library crf_context + crf_context_viterbi (reference)
//   K1  packed co-located f64 rows (label index + values in one record), same float order, double Viterbi
//   K2  certified f32: packed f32 rows, f32 Viterbi vectorizable over labels, per-address certificate on the decoded
//       path, exact K1 fallback when not certified
// Every variant's labels must equal K0's on every address (exit 1 otherwise).
// Timing: cold = PASSES sequential passes over the corpus (median of passes 2..P, as bench_ext); warm = marginal cost
// of an immediate repeat of the same address ((2x pass) - (1x pass)), median of 5.
// usage: replay_kernel MODEL.dat ORACLE_DIR PASSES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include "crf.h"

#define LL 20
#define TMAX 4096
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }

static int nA, *Tof; static uint32_t *tokBase, *tokS, *tokP, *sid, *pid;
static crf_t *crf; static uint32_t *ipS, *ixS, *ipP, *ixP; static double *dS, *dP;
static uint8_t *pkS, *pkP, *pk32S, *pk32P; static uint64_t *oS, *oP, *o32S, *o32P;
static float tr32[LL * LL], trmax; static double trD[LL * LL];

static FILE *gz(const char *dir, const char *name) { char cmd[4096]; snprintf(cmd, sizeof cmd, "gzip -dc '%s/%s.gz'", dir, name); return popen(cmd, "r"); }
static void load_ids(const char *dir) {
    char *line = NULL; size_t lc = 0; int cap = 1 << 16; Tof = calloc(cap, sizeof(int)); nA = 0;
    FILE *f = gz(dir, "O4.txt");
    while (getline(&line, &lc, f) > 0) {
        char *p = line, *e; long a = strtol(p, &e, 10); p = e + 1; int t = (int)strtol(p, &e, 10); p = e + 1;
        if (*p != 'S') continue;
        while (a >= cap) { Tof = realloc(Tof, sizeof(int) * cap * 2); memset(Tof + cap, 0, sizeof(int) * cap); cap *= 2; }
        if (t + 1 > Tof[a]) Tof[a] = t + 1; if (a + 1 > nA) nA = (int)a + 1;
    }
    pclose(f);
    tokBase = malloc(4 * (nA + 1)); uint32_t nt = 0;
    for (int a = 0; a < nA; a++) { tokBase[a] = nt; nt += Tof[a]; } tokBase[nA] = nt;
    tokS = malloc(4 * (nt + 1)); tokP = malloc(4 * (nt + 1));
    size_t cs = 1 << 20, cp = 1 << 18, ns = 0, np = 0; sid = malloc(4 * cs); pid = malloc(4 * cp);
    long cur = -1;                         // last token index whose start offsets are set
    f = gz(dir, "O3i.txt");
    while (getline(&line, &lc, f) > 0) {
        char *p = line, *e; long a = strtol(p, &e, 10); p = e + 1; int t = (int)strtol(p, &e, 10); p = e + 1; char k = *p; p += 2;
        p = strchr(p, '\t') + 1;
        if (a >= nA || t >= Tof[a]) { fprintf(stderr, "O3i record outside O4 tokens a=%ld t=%d\n", a, t); exit(4); }
        long tk = tokBase[a] + t;
        while (cur < tk) { cur++; tokS[cur] = (uint32_t)ns; tokP[cur] = (uint32_t)np; }
        if (*p == '-') continue;
        uint32_t id = (uint32_t)strtoul(p, NULL, 10);
        if (k == 'S') { if (ns == cs) { cs *= 2; sid = realloc(sid, 4 * cs); } sid[ns++] = id; }
        else { if (np == cp) { cp *= 2; pid = realloc(pid, 4 * cp); } pid[np++] = id; }
    }
    pclose(f);
    while (cur < (long)nt) { cur++; tokS[cur] = (uint32_t)ns; tokP[cur] = (uint32_t)np; }
}

static void build_packed(void) {
    uint32_t mS = crf->weights->m, mP = crf->state_trans_weights->m; uint64_t o = 0, o32 = 0;
    oS = malloc(8 * (mS + 1ull)); o32S = malloc(8 * (mS + 1ull));
    for (uint32_t f = 0; f <= mS; f++) { oS[f] = o; o32S[f] = o32; if (f == mS) break; uint32_t n = ipS[f + 1] - ipS[f];
        o += 8 + ((n + 7) & ~7u) + 8ull * n; o32 += 8 + ((n + 3) & ~3u) + 4ull * n; }
    pkS = aligned_alloc(128, (o + 127) & ~127ull); pk32S = aligned_alloc(128, (o32 + 127) & ~127ull);
    for (uint32_t f = 0; f < mS; f++) { uint32_t n = ipS[f + 1] - ipS[f]; uint8_t *r = pkS + oS[f], *q = pk32S + o32S[f]; float as = 0;
        *(uint32_t *)r = n; *(uint32_t *)q = n;
        for (uint32_t c = 0; c < n; c++) { r[8 + c] = (uint8_t)ixS[ipS[f] + c]; q[8 + c] = (uint8_t)ixS[ipS[f] + c];
            ((double *)(r + 8 + ((n + 7) & ~7u)))[c] = dS[ipS[f] + c]; ((float *)(q + 8 + ((n + 3) & ~3u)))[c] = (float)dS[ipS[f] + c]; as += fabsf((float)dS[ipS[f] + c]); }
        ((float *)q)[1] = as * 1.0001f; }
    o = o32 = 0; oP = malloc(8 * (mP + 1ull)); o32P = malloc(8 * (mP + 1ull));
    for (uint32_t f = 0; f <= mP; f++) { oP[f] = o; o32P[f] = o32; if (f == mP) break; uint32_t n = ipP[f + 1] - ipP[f];
        o += 8 + ((2 * n + 7) & ~7u) + 8ull * n; o32 += 8 + ((2 * n + 3) & ~3u) + 4ull * n; }
    pkP = aligned_alloc(128, (o + 127) & ~127ull); pk32P = aligned_alloc(128, (o32 + 127) & ~127ull);
    for (uint32_t f = 0; f < mP; f++) { uint32_t n = ipP[f + 1] - ipP[f]; uint8_t *r = pkP + oP[f], *q = pk32P + o32P[f]; float as = 0;
        *(uint32_t *)r = n; *(uint32_t *)q = n;
        for (uint32_t c = 0; c < n; c++) { ((uint16_t *)(r + 8))[c] = (uint16_t)ixP[ipP[f] + c]; ((uint16_t *)(q + 8))[c] = (uint16_t)ixP[ipP[f] + c];
            ((double *)(r + 8 + ((2 * n + 7) & ~7u)))[c] = dP[ipP[f] + c]; ((float *)(q + 8 + ((2 * n + 3) & ~3u)))[c] = (float)dP[ipP[f] + c]; as += fabsf((float)dP[ipP[f] + c]); }
        ((float *)q)[1] = as * 1.0001f; }
}

// K0: the original accumulate loop (crf.c) + library Viterbi
static void k0(int a, uint32_t *out) {
    int T = Tof[a]; crf_context_t *ctx = crf->context;
    crf_context_set_num_items(ctx, T); crf_context_reset(ctx, CRF_CONTEXT_RESET_ALL);
    double_matrix_copy(crf->trans_weights, ctx->trans);
    for (int t = 0; t < T; t++) {
        uint32_t tk = tokBase[a] + t; double *s = state_score(ctx, t), *st = state_trans_score_all(ctx, t);
        for (uint32_t q = tokS[tk]; q < tokS[tk + 1]; q++) { uint32_t f = sid[q]; for (uint32_t c = ipS[f]; c < ipS[f + 1]; c++) s[ixS[c]] += dS[c]; }
        for (uint32_t q = tokP[tk]; q < tokP[tk + 1]; q++) { uint32_t f = pid[q]; for (uint32_t c = ipP[f]; c < ipP[f + 1]; c++) st[ixP[c]] += dP[c]; }
    }
    crf_context_viterbi(ctx, out);
}
// K1: packed f64, same float order
static double sD[TMAX][LL], stD[TMAX][LL * LL], alD[TMAX][LL]; static uint8_t bk[TMAX][LL];
static void vitD(int T, uint32_t *out) {
    for (int j = 0; j < LL; j++) alD[0][j] = sD[0][j];
    for (int t = 1; t < T; t++) for (int j = 0; j < LL; j++) {
        double best = -DBL_MAX; int arg = -1;
        for (int i = 0; i < LL; i++) { double sc = alD[t - 1][i] + stD[t][i * LL + j] + trD[i * LL + j]; if (best < sc) { best = sc; arg = i; } }
        bk[t][j] = (uint8_t)arg; alD[t][j] = best + sD[t][j];
    }
    double best = -DBL_MAX; int arg = 0; for (int j = 0; j < LL; j++) if (best < alD[T - 1][j]) { best = alD[T - 1][j]; arg = j; }
    for (int t = T - 1; t >= 0; t--) { out[t] = arg; if (t) arg = bk[t][arg]; }
}
static void k1(int a, uint32_t *out) {
    int T = Tof[a];
    for (int t = 0; t < T; t++) {
        uint32_t tk = tokBase[a] + t; double *s = sD[t], *st = stD[t]; memset(s, 0, 8 * LL); memset(st, 0, 8 * LL * LL);
        for (uint32_t q = tokS[tk]; q < tokS[tk + 1]; q++) { const uint8_t *r = pkS + oS[sid[q]]; uint32_t n = *(const uint32_t *)r; const double *v = (const double *)(r + 8 + ((n + 7) & ~7u));
            for (uint32_t c = 0; c < n; c++) s[r[8 + c]] += v[c]; }
        for (uint32_t q = tokP[tk]; q < tokP[tk + 1]; q++) { const uint8_t *r = pkP + oP[pid[q]]; uint32_t n = *(const uint32_t *)r; const uint16_t *ix = (const uint16_t *)(r + 8); const double *v = (const double *)(r + 8 + ((2 * n + 7) & ~7u));
            for (uint32_t c = 0; c < n; c++) st[ix[c]] += v[c]; }
    }
    vitD(T, out);
}
// K2: certified f32 (+ K1 fallback)
static float sF[TMAX][LL], stF[TMAX][LL * LL], alF[TMAX][LL], errT[TMAX]; static long fallbacks;
static void k2(int a, uint32_t *out) {
    int T = Tof[a]; const float U = 5.9604645e-08f;
    for (int t = 0; t < T; t++) {
        uint32_t tk = tokBase[a] + t; float *s = sF[t], *st = stF[t]; memset(s, 0, 4 * LL); memset(st, 0, 4 * LL * LL);
        float as = 0; uint32_t k = 0;
        for (uint32_t q = tokS[tk]; q < tokS[tk + 1]; q++) { const uint8_t *r = pk32S + o32S[sid[q]]; uint32_t n = *(const uint32_t *)r; const float *v = (const float *)(r + 8 + ((n + 3) & ~3u));
            as += ((const float *)r)[1]; for (uint32_t c = 0; c < n; c++) s[r[8 + c]] += v[c]; k++; }
        for (uint32_t q = tokP[tk]; q < tokP[tk + 1]; q++) { const uint8_t *r = pk32P + o32P[pid[q]]; uint32_t n = *(const uint32_t *)r; const uint16_t *ix = (const uint16_t *)(r + 8); const float *v = (const float *)(r + 8 + ((2 * n + 3) & ~3u));
            as += ((const float *)r)[1]; for (uint32_t c = 0; c < n; c++) st[ix[c]] += v[c]; k++; }
        errT[t] = as * (k + 2) * U * 1.01f;   // per-entry bound: f32 rounding of weights and sums + the original's f64 rounding
    }
    for (int j = 0; j < LL; j++) alF[0][j] = sF[0][j];
    for (int t = 1; t < T; t++) {
        float best[LL]; uint8_t arg[LL];
        for (int j = 0; j < LL; j++) { best[j] = -FLT_MAX; arg[j] = 0; }
        for (int i = 0; i < LL; i++) { float p = alF[t - 1][i]; const float *row = &stF[t][i * LL], *trr = &tr32[i * LL];
            for (int j = 0; j < LL; j++) { float sc = (p + row[j]) + trr[j]; int m = best[j] < sc; best[j] = m ? sc : best[j]; arg[j] = m ? (uint8_t)i : arg[j]; } }
        for (int j = 0; j < LL; j++) { alF[t][j] = best[j] + sF[t][j]; bk[t][j] = arg[j]; }
    }
    float b = -FLT_MAX; int arg = 0; for (int j = 0; j < LL; j++) if (b < alF[T - 1][j]) { b = alF[T - 1][j]; arg = j; }
    for (int t = T - 1; t >= 0; t--) { out[t] = arg; if (t) arg = bk[t][arg]; }
    // certificate on the decoded path: cumulative bound E (score errors + trans rounding + f32 DP rounding); gap > 4E
    float E = errT[0] + 3 * U * (fabsf(alF[0][out[0]]) + 1); int ok = 1;
    for (int t = 1; t < T && ok; t++) {
        float amax = 0; for (int i = 0; i < LL; i++) amax = fmaxf(amax, fabsf(alF[t - 1][i]));
        E += errT[t] + U * trmax + 6 * U * (amax + fabsf(alF[t][out[t]]) + trmax);
        int j = out[t], is = out[t - 1]; float bs = (alF[t - 1][is] + stF[t][is * LL + j]) + tr32[is * LL + j], sec = -FLT_MAX;
        for (int i = 0; i < LL; i++) if (i != is) sec = fmaxf(sec, (alF[t - 1][i] + stF[t][i * LL + j]) + tr32[i * LL + j]);
        if (!(bs - sec > 4 * E)) ok = 0;
    }
    if (ok) { float sec = -FLT_MAX; for (int j = 0; j < LL; j++) if (j != (int)out[T - 1]) sec = fmaxf(sec, alF[T - 1][j]);
        if (!(alF[T - 1][out[T - 1]] - sec > 4 * E)) ok = 0; }
    if (!ok) { fallbacks++; k1(a, out); }
}

typedef void (*kfn)(int, uint32_t *);
static uint32_t outbuf[TMAX]; static volatile uint32_t sink;
static double pass(kfn k, int twice) { double t0 = now(); for (int a = 0; a < nA; a++) { if (!Tof[a]) continue; k(a, outbuf); if (twice) k(a, outbuf); sink += outbuf[0]; } return now() - t0; }

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s MODEL ORACLE_DIR PASSES\n", argv[0]); return 2; }
    int P = atoi(argv[3]);
    crf = crf_load(argv[1]); if (!crf || crf->num_classes != LL) return 2;
    ipS = crf->weights->indptr->a; ixS = crf->weights->indices->a; dS = crf->weights->data->a;
    ipP = crf->state_trans_weights->indptr->a; ixP = crf->state_trans_weights->indices->a; dP = crf->state_trans_weights->data->a;
    for (int c = 0; c < LL * LL; c++) { trD[c] = crf->trans_weights->values[c]; tr32[c] = (float)trD[c]; trmax = fmaxf(trmax, fabsf(tr32[c])); }
    load_ids(argv[2]); build_packed();
    long nT = 0, bad1 = 0, bad2 = 0, n = 0; static uint32_t ref[TMAX], r1[TMAX], r2[TMAX];
    for (int a = 0; a < nA; a++) { int T = Tof[a]; if (!T) continue; if (T > TMAX) return 5; n++; nT += T;
        k0(a, ref); k1(a, r1); k2(a, r2);
        for (int t = 0; t < T; t++) { bad1 += r1[t] != ref[t]; bad2 += r2[t] != ref[t]; } }
    printf("addresses %ld tokens/addr %.3f  label mismatches vs K0: K1 %ld K2 %ld  K2 fallbacks %ld (%.4f%%)\n", n, (double)nT / n, bad1, bad2, fallbacks, 100.0 * fallbacks / n);
    fflush(stdout);
    kfn ks[3] = { k0, k1, k2 }; const char *nm[3] = { "K0 original CSR f64", "K1 packed f64 exact", "K2 certified f32" };
    for (int v = 0; v < 3; v++) {
        double *c = malloc(sizeof(double) * P), w[5];
        for (int p = 0; p < P; p++) c[p] = pass(ks[v], 0) / n * 1e6;
        for (int r = 0; r < 5; r++) { double t1 = pass(ks[v], 0), t2 = pass(ks[v], 1); w[r] = (t2 - t1) / n * 1e6; }
        double first = c[0]; qsort(c + 1, P - 1, sizeof(double), cmpd); qsort(w, 5, sizeof(double), cmpd);
        printf("%-22s pass1 %.3f  cold median p2..%d %.3f [%.3f-%.3f]  warm %.3f [%.3f-%.3f] us/addr\n", nm[v], first, P, c[1 + (P - 1) / 2], c[1], c[P - 1], w[2], w[0], w[4]);
        fflush(stdout); free(c);
    }
    printf("K2 fallbacks incl. timing passes: %ld\n", fallbacks);
    return (bad1 || bad2) ? 1 : 0;
}
