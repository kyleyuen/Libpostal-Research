// 151: exact per-address workload + certified-argmax feasibility, from the 142 oracles and the model.
// Links the preserved ORIGINAL libpostal.a (000-baseline). Nothing here is timed.
//
// For every predicted address (O3i + O4 records):
//  1. rebuild each state row (S ids, emission order, original loop) and state-transition row (P ids) from the model
//     and require bit-identity with O4 (self-check: the work counted below is the original's real work);
//  2. run the ORIGINAL crf_context_viterbi on the O4 rows (reference labels); an in-file DP with the same ops gives
//     the smallest decision gap on that path (gap < 0 would mean our DP disagrees -> counted as error);
//  3. count the work: tokens, keys, hits, weight adds, distinct 128-B lines in the original CSR arrays and in a
//     packed co-located layout derived from the whole model in feature-id order (no data input);
//  4. certified argmax: error models E1 (double, any summation order), E2 (float32 weights and sums, double DP),
//     E3 (float32 everything, conservative) bound |approx - original| per score; intervals are propagated through
//     the original DP with monotone ops; an address is certified iff every decision on the original path is
//     strictly separated. Not certified => the runtime would fall back to the exact original-order path.
//
// usage: oracle_bound MODEL.dat ORACLE_DIR OUT_PER_ADDRESS.tsv   (summary on stdout; exit 1 on any mismatch)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include "crf.h"

#define L_MAX 32
static const double U64 = 1.1102230246251565e-16;   // 2^-53
static const double U32 = 5.9604644775390625e-08;   // 2^-24
static double gam(double n, double u) { return n <= 0 ? 0 : n * u / (1 - n * u); }

typedef struct { FILE *f; char *line; size_t cap; int have; long a; int t; char k; char *rest; } stream_t;
static void st_next(stream_t *s) {
    s->have = 0;
    if (getline(&s->line, &s->cap, s->f) <= 0) return;
    char *p = s->line, *e;
    s->a = strtol(p, &e, 10); p = e + 1;
    s->t = (int)strtol(p, &e, 10); p = e + 1;
    s->k = *p; s->rest = p + 1; if (*s->rest == '\t') s->rest++;
    size_t n = strlen(s->rest); if (n && s->rest[n - 1] == '\n') s->rest[n - 1] = 0;
    s->have = 1;
}
static FILE *gz(const char *dir, const char *name) {
    char cmd[4096]; snprintf(cmd, sizeof cmd, "gzip -dc '%s/%s.gz'", dir, name);
    FILE *f = popen(cmd, "r"); if (!f) { perror(cmd); exit(2); } return f;
}
static uint64_t bits(double d) { uint64_t u; memcpy(&u, &d, 8); return u; }
static double frombits(uint64_t u) { double d; memcpy(&d, &u, 8); return d; }

// per-address set of (array tag, line) ids
#define SETN (1 << 18)
static uint64_t setv[SETN]; static uint32_t setg[SETN]; static uint32_t gen = 1;
static int set_add(uint64_t k) {
    uint64_t h = (k * 0x9E3779B97F4A7C15ull) >> 40;
    for (;;) { h &= SETN - 1; if (setg[h] != gen) { setg[h] = gen; setv[h] = k; return 1; } if (setv[h] == k) return 0; h++; }
}
static long add_range(int tag, uint64_t b0, uint64_t b1) {   // bytes [b0,b1) of array `tag`
    long n = 0; if (b1 <= b0) return 0;
    for (uint64_t l = b0 >> 7; l <= (b1 - 1) >> 7; l++) n += set_add(((uint64_t)tag << 56) | l);
    return n;
}

typedef struct {
    int nS, nShit, nP, nPhit; long addS, addP;
    double s[L_MAX], sA[L_MAX]; int sk[L_MAX];
    double *st, *stA; int *stk;
    double o4s[L_MAX], *o4st;                      // O4 rows as dumped by the original
} tok_t;

static int L;
static uint32_t *ipS, *ixS, *ipP, *ixP; static double *dS, *dP; static double *TR;
static uint64_t *offS, *offP, *offS32, *offP32;

static double err(int model, int k, double A) {
    double e = model == 1 ? 2 * gam(k - 1, U64) * A : (U32 + gam(k - 1, U32) * (1 + U32) + gam(k - 1, U64)) * A;
    return 2 * e * 1.0001;    // [x-2e, x+2e] contains every runtime interval [x'-e, x'+e] (conservative)
}

static int interval_check(int T, tok_t *tk, const int *lab, int model, double *mingap_out) {
    static double plo[L_MAX], phi[L_MAX], clo[L_MAX], chi[L_MAX];
    double mingap = INFINITY; int ok = 1;
    for (int j = 0; j < L; j++) { double e = err(model, tk[0].sk[j], tk[0].sA[j]); plo[j] = tk[0].s[j] - e; phi[j] = tk[0].s[j] + e; }
    for (int t = 1; t < T; t++) {
        for (int j = 0; j < L; j++) {
            double best_lo = -DBL_MAX, best_hi = -DBL_MAX, ilo = -DBL_MAX, other_hi = -DBL_MAX; int istar = lab[t - 1];
            for (int i = 0; i < L; i++) {
                int c = i * L + j; double v = tk[t].st[c], tr = TR[c];
                double e = err(model, tk[t].stk[c], tk[t].stA[c]), etr = model == 1 ? 0 : 2 * U32 * fabs(tr) * 1.0001;
                double lo = (plo[i] + (v - e)) + (tr - etr), hi = (phi[i] + (v + e)) + (tr + etr);
                if (model == 3) { double w = 4 * U32 * (fabs(plo[i]) + fabs(phi[i]) + fabs(v) + fabs(tr)); lo -= w; hi += w; }
                if (lo > best_lo) best_lo = lo;
                if (hi > best_hi) best_hi = hi;
                if (i == istar) ilo = lo; else if (hi > other_hi) other_hi = hi;
            }
            if (j == lab[t]) { if (!(ilo > other_hi)) ok = 0; if (ilo - other_hi < mingap) mingap = ilo - other_hi; }
            double es = err(model, tk[t].sk[j], tk[t].sA[j]);
            clo[j] = best_lo + (tk[t].s[j] - es); chi[j] = best_hi + (tk[t].s[j] + es);
            if (model == 3) { double w = 4 * U32 * (fabs(clo[j]) + fabs(chi[j])); clo[j] -= w; chi[j] += w; }
        }
        memcpy(plo, clo, sizeof(double) * L); memcpy(phi, chi, sizeof(double) * L);
    }
    int js = lab[T - 1]; double oh = -DBL_MAX;
    for (int j = 0; j < L; j++) if (j != js && phi[j] > oh) oh = phi[j];
    if (!(plo[js] > oh)) ok = 0;
    if (plo[js] - oh < mingap) mingap = plo[js] - oh;
    *mingap_out = mingap;
    return ok;
}

// same ops as crf_context_viterbi; returns min over path decisions of (best - second best); <0 cannot happen if
// `lab` is this DP's argmax path, so a negative value flags a disagreement with the library.
static double exact_gap(int T, tok_t *tk, const int *lab, int *disagree) {
    static double prev[L_MAX], cur[L_MAX]; double g = INFINITY;
    for (int j = 0; j < L; j++) prev[j] = tk[0].s[j];
    for (int t = 1; t < T; t++) {
        for (int j = 0; j < L; j++) {
            double best = -DBL_MAX, second = -DBL_MAX; int arg = -1;
            for (int i = 0; i < L; i++) {
                double sc = prev[i] + tk[t].st[i * L + j] + TR[i * L + j];
                if (best < sc) { second = best; best = sc; arg = i; } else if (sc > second) second = sc;
            }
            if (j == lab[t]) { if (arg != lab[t - 1]) *disagree = 1; if (best - second < g) g = best - second; }
            cur[j] = best + tk[t].s[j];
        }
        memcpy(prev, cur, sizeof(double) * L);
    }
    double best = -DBL_MAX, second = -DBL_MAX; int arg = -1;
    for (int j = 0; j < L; j++) { if (best < prev[j]) { second = best; best = prev[j]; arg = j; } else if (prev[j] > second) second = prev[j]; }
    if (arg != lab[T - 1]) *disagree = 1;
    if (best - second < g) g = best - second;
    return g;
}

static void tok_reset(tok_t *x) {
    x->nS = x->nShit = x->nP = x->nPhit = 0; x->addS = x->addP = 0;
    memset(x->s, 0, sizeof x->s); memset(x->sA, 0, sizeof x->sA); memset(x->sk, 0, sizeof x->sk);
    memset(x->st, 0, 8 * L * L); memset(x->stA, 0, 8 * L * L); memset(x->stk, 0, 4 * L * L);
    memset(x->o4s, 0, sizeof x->o4s); memset(x->o4st, 0, 8 * L * L);
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s MODEL ORACLE_DIR OUT.tsv\n", argv[0]); return 2; }
    crf_t *crf = crf_load(argv[1]);
    if (!crf) { fprintf(stderr, "crf_load failed\n"); return 2; }
    L = crf->num_classes; if (L > L_MAX) return 3;
    ipS = crf->weights->indptr->a; ixS = crf->weights->indices->a; dS = crf->weights->data->a;
    ipP = crf->state_trans_weights->indptr->a; ixP = crf->state_trans_weights->indices->a; dP = crf->state_trans_weights->data->a;
    TR = crf->trans_weights->values;
    uint32_t mS = crf->weights->m, mP = crf->state_trans_weights->m;
    // packed co-located record per row: nnz values + nnz label indices (S: 1 B, P: 2 B) + 2-byte count, aligned
    offS = malloc(8 * ((size_t)mS + 1)); offP = malloc(8 * ((size_t)mP + 1)); offS32 = malloc(8 * ((size_t)mS + 1)); offP32 = malloc(8 * ((size_t)mP + 1));
    uint64_t o = 0, o32 = 0; long hist[L_MAX + 2] = {0}; double nzS = 0;
    for (uint32_t f = 0; f < mS; f++) {
        uint32_t n = ipS[f + 1] - ipS[f]; offS[f] = o; offS32[f] = o32;
        o += ((uint64_t)n * 9 + 2 + 7) & ~7ull; o32 += ((uint64_t)n * 5 + 2 + 3) & ~3ull; hist[n > (uint32_t)L ? L + 1 : n]++; nzS += n;
    }
    offS[mS] = o; offS32[mS] = o32;
    uint64_t q = 0, q32 = 0;
    for (uint32_t f = 0; f < mP; f++) {
        uint32_t n = ipP[f + 1] - ipP[f]; offP[f] = q; offP32[f] = q32;
        q += ((uint64_t)n * 10 + 2 + 7) & ~7ull; q32 += ((uint64_t)n * 6 + 2 + 3) & ~3ull;
    }
    offP[mP] = q; offP32[mP] = q32;
    printf("model L=%d state_rows=%u state_nnz=%u (mean %.3f/row) state_trans_rows=%u state_trans_nnz=%u (mean %.2f/row)\n",
           L, mS, ipS[mS], nzS / mS, mP, ipP[mP], (double)ipP[mP] / mP);
    printf("model CSR MB: state %.1f (indptr %.1f, indices %.1f, data %.1f); state_trans %.1f\n",
           (4.0 * (mS + 1) + 12.0 * ipS[mS]) / 1e6, 4.0 * (mS + 1) / 1e6, 4.0 * ipS[mS] / 1e6, 8.0 * ipS[mS] / 1e6,
           (4.0 * (mP + 1) + 12.0 * ipP[mP]) / 1e6);
    printf("model packed MB: state f64 %.1f, f32 %.1f; state_trans f64 %.1f, f32 %.1f\n", o / 1e6, o32 / 1e6, q / 1e6, q32 / 1e6);
    printf("model state-row nnz histogram (all rows, n:count):");
    for (int n = 0; n <= L + 1; n++) printf(" %d:%ld", n, hist[n]);
    printf("\n");

    stream_t s3 = { .f = gz(argv[2], "O3i.txt") }, s4 = { .f = gz(argv[2], "O4.txt") };
    FILE *out = fopen(argv[3], "w");
    fprintf(out, "a\tT\tnS\tnShit\taddS\tnP\tnPhit\taddP\tlinesCSR\tlinesPacked64\tlinesPacked32\tcertE1\tcertE2\tcertE3\tgap\tgapE1\tgapE2\tgapE3\n");
    st_next(&s3); st_next(&s4);
    int cap = 64; tok_t *tk = calloc(cap, sizeof(tok_t));
    for (int i = 0; i < cap; i++) { tk[i].st = malloc(8 * L * L); tk[i].stA = malloc(8 * L * L); tk[i].stk = malloc(4 * L * L); tk[i].o4st = malloc(8 * L * L); }
    long nAddr = 0, badS = 0, badP = 0, badLab = 0, cert[4] = {0}, ties = 0;
    double sum[12] = {0};
    long hitHist[L_MAX + 2] = {0}; long nHits = 0;
    crf_context_t *ctx = crf->context; uint32_t *vl = NULL; int *lab = NULL; int labcap = 0;
    while (s3.have || s4.have) {
        long a = !s3.have ? s4.a : !s4.have ? s3.a : (s3.a < s4.a ? s3.a : s4.a);   // an address may have O4 rows but no O3i lines
        int T = 0; long lc = 0, lp = 0, lp32 = 0;
        gen++; if (gen == 0) { memset(setg, 0, sizeof setg); gen = 1; }
        while (s3.have && s3.a == a) {
            int t = s3.t;
            if (t >= cap) { int nc = cap; while (nc <= t) nc *= 2; tk = realloc(tk, nc * sizeof(tok_t));
                for (int i = cap; i < nc; i++) { memset(&tk[i], 0, sizeof(tok_t)); tk[i].st = malloc(8 * L * L); tk[i].stA = malloc(8 * L * L); tk[i].stk = malloc(4 * L * L); tk[i].o4st = malloc(8 * L * L); } cap = nc; }
            while (T <= t) tok_reset(&tk[T++]);
            tok_t *x = &tk[t];
            char *p = strchr(s3.rest, '\t'); char *idf = p ? p + 1 : s3.rest;
            if (s3.k == 'S') {
                x->nS++;
                if (*idf != '-') { uint32_t f = (uint32_t)strtoul(idf, NULL, 10); x->nShit++; nHits++;
                    uint32_t n = ipS[f + 1] - ipS[f]; hitHist[n > (uint32_t)L ? L + 1 : n]++;
                    for (uint32_t c = ipS[f]; c < ipS[f + 1]; c++) { x->s[ixS[c]] += dS[c]; x->sA[ixS[c]] += fabs(dS[c]); x->sk[ixS[c]]++; x->addS++; }
                    lc += add_range(0, 4ull * f, 4ull * f + 8) + add_range(1, 4ull * ipS[f], 4ull * ipS[f + 1]) + add_range(2, 8ull * ipS[f], 8ull * ipS[f + 1]);
                    lp += add_range(6, offS[f], offS[f + 1]); lp32 += add_range(8, offS32[f], offS32[f + 1]); }
            } else {
                x->nP++;
                if (*idf != '-') { uint32_t f = (uint32_t)strtoul(idf, NULL, 10); x->nPhit++;
                    for (uint32_t c = ipP[f]; c < ipP[f + 1]; c++) { x->st[ixP[c]] += dP[c]; x->stA[ixP[c]] += fabs(dP[c]); x->stk[ixP[c]]++; x->addP++; }
                    lc += add_range(3, 4ull * f, 4ull * f + 8) + add_range(4, 4ull * ipP[f], 4ull * ipP[f + 1]) + add_range(5, 8ull * ipP[f], 8ull * ipP[f + 1]);
                    lp += add_range(7, offP[f], offP[f + 1]); lp32 += add_range(9, offP32[f], offP32[f + 1]); }
            }
            st_next(&s3);
        }
        while (s4.have && s4.a == a) {
            int t = s4.t;
            if (t >= cap) { fprintf(stderr, "O4 token beyond capacity a=%ld t=%d\n", a, t); return 4; }
            // a token can have no feature records (no O3i lines) but still has O4 rows (all +0.0): extend T
            while (T <= t) tok_reset(&tk[T++]);
            if (s4.k == 'S') {
                char *p = s4.rest;
                for (int j = 0; j < L; j++) { uint64_t u = strtoull(p, &p, 16); if (*p == '\t') p++; tk[t].o4s[j] = frombits(u); if (u != bits(tk[t].s[j])) badS++; }
            } else {
                static uint64_t want[L_MAX * L_MAX]; memset(want, 0, sizeof want);
                char *p = s4.rest;
                while (*p) { unsigned long c = strtoul(p, &p, 10); p++; uint64_t u = strtoull(p, &p, 16); if (*p == '\t') p++; want[c] = u; }
                for (int c = 0; c < L * L; c++) { tk[t].o4st[c] = frombits(want[c]); if (want[c] != bits(tk[t].st[c])) badP++; }
            }
            st_next(&s4);
        }
        if (T == 0) continue;
        nAddr++;
        crf_context_set_num_items(ctx, T); crf_context_reset(ctx, CRF_CONTEXT_RESET_ALL);
        double_matrix_copy(crf->trans_weights, ctx->trans);
        for (int t = 0; t < T; t++) { memcpy(state_score(ctx, t), tk[t].o4s, 8 * L); memcpy(state_trans_score_all(ctx, t), tk[t].o4st, 8 * L * L); }
        if (T > labcap) { labcap = T * 2; vl = realloc(vl, 4 * labcap); lab = realloc(lab, sizeof(int) * labcap); }
        crf_context_viterbi(ctx, vl);                          // ORIGINAL library Viterbi on the oracle rows
        for (int t = 0; t < T; t++) lab[t] = (int)vl[t];
        int dis = 0; double g = exact_gap(T, tk, lab, &dis);
        double g1, g2, g3;
        int c1 = interval_check(T, tk, lab, 1, &g1), c2 = interval_check(T, tk, lab, 2, &g2), c3 = interval_check(T, tk, lab, 3, &g3);
        if (dis) badLab++;
        if (g == 0) ties++;
        cert[1] += c1; cert[2] += c2; cert[3] += c3;
        long v[8] = {0};
        for (int t = 0; t < T; t++) { v[0] += tk[t].nS; v[1] += tk[t].nShit; v[2] += tk[t].addS; v[3] += tk[t].nP; v[4] += tk[t].nPhit; v[5] += tk[t].addP; }
        fprintf(out, "%ld\t%d\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%d\t%d\t%d\t%.6g\t%.6g\t%.6g\t%.6g\n",
                a, T, v[0], v[1], v[2], v[3], v[4], v[5], lc, lp, lp32, c1, c2, c3, g, g1, g2, g3);
        sum[0] += T; for (int i = 0; i < 6; i++) sum[1 + i] += v[i]; sum[7] += lc; sum[8] += lp; sum[9] += lp32;
    }
    printf("addresses_predicted %ld  O4 state-bit mismatches %ld  O4 state-trans-bit mismatches %ld  label disagreements %ld  exact ties on path %ld\n",
           nAddr, badS, badP, badLab, ties);
    printf("per address: tokens %.3f  S keys %.3f  S hits %.3f  S adds %.2f  P keys %.3f  P hits %.3f  P adds %.2f\n",
           sum[0] / nAddr, sum[1] / nAddr, sum[2] / nAddr, sum[3] / nAddr, sum[4] / nAddr, sum[5] / nAddr, sum[6] / nAddr);
    printf("per address distinct weight lines (128 B): CSR %.2f  packed-f64 %.2f  packed-f32 %.2f\n", sum[7] / nAddr, sum[8] / nAddr, sum[9] / nAddr);
    printf("hit state-row nnz histogram (n:count):");
    for (int n = 0; n <= L + 1; n++) printf(" %d:%ld", n, hitHist[n]);
    printf("\ncertified (no fallback needed): E1 double-any-order %ld (%.4f%%)  E2 f32 weights+sums %ld (%.4f%%)  E3 f32 all %ld (%.4f%%)\n",
           cert[1], 100.0 * cert[1] / nAddr, cert[2], 100.0 * cert[2] / nAddr, cert[3], 100.0 * cert[3] / nAddr);
    fclose(out);
    return (badS || badP || badLab) ? 1 : 0;
}
