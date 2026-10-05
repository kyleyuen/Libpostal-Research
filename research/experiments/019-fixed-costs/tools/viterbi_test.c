// 019: scalar vs NEON Viterbi inner step on random inputs with many exact ties, -inf and NaN. Exit 1 on any difference.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <arm_neon.h>
static uint64_t rs = 88172645463325252ULL; static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static double val(void) { unsigned r = rnd() % 100; if (r < 3) return -INFINITY; if (r < 4) return NAN; if (r < 50) return (double)(int)(rnd() % 7) - 3; return ((double)(rnd() % 2000000) - 1e6) / 997.0; }
int main(void) {
    enum { L = 20 }; double prev[L], st[L * L], tr[L * L], state[L]; long bad = 0;
    for (long it = 0; it < 5000000; it++) {
        for (int k = 0; k < L; k++) { prev[k] = val(); state[k] = val(); } for (int k = 0; k < L * L; k++) { st[k] = val(); tr[k] = val(); }
        uint32_t b1[L], b2[L]; double c1[L], c2[L]; memset(b1, 0, sizeof b1); memset(b2, 0, sizeof b2);
        for (int j = 0; j < L; j++) { double mx = -DBL_MAX; long am = -1; for (int i = 0; i < L; i++) { double sc = prev[i] + st[i * L + j] + tr[i * L + j]; if (mx < sc) { mx = sc; am = i; } } if (am >= 0) b1[j] = am; c1[j] = mx + state[j]; }
        float64x2_t mx[10]; int64x2_t ag[10];
        for (int v = 0; v < 10; v++) { mx[v] = vdupq_n_f64(-DBL_MAX); ag[v] = vdupq_n_s64(-1); }
        for (int i = 0; i < L; i++) { float64x2_t p = vdupq_n_f64(prev[i]); int64x2_t iv = vdupq_n_s64(i);
            for (int v = 0; v < 10; v++) { float64x2_t sc = vaddq_f64(vaddq_f64(p, vld1q_f64(st + i * L + 2 * v)), vld1q_f64(tr + i * L + 2 * v)); uint64x2_t gt = vcltq_f64(mx[v], sc); mx[v] = vbslq_f64(gt, sc, mx[v]); ag[v] = vbslq_s64(gt, iv, ag[v]); } }
        for (int v = 0; v < 10; v++) { double mm[2]; int64_t aa[2]; vst1q_f64(mm, mx[v]); vst1q_s64(aa, ag[v]); for (int k = 0; k < 2; k++) { int jj = 2 * v + k; if (aa[k] >= 0) b2[jj] = aa[k]; c2[jj] = mm[k] + state[jj]; } }
        if (memcmp(b1, b2, sizeof b1) || memcmp(c1, c2, sizeof c1)) { if (bad++ < 3) printf("MISMATCH at iteration %ld\n", it); }
    }
    printf("%s: 5,000,000 random steps (ties, -inf, NaN), %ld mismatches\n", bad ? "FAIL" : "OK", bad); return bad != 0;
}
