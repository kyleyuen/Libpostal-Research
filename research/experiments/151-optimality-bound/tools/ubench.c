// 151: machine constants on this Mac (replaces LIMITS' UNVERIFIED clock / FP-pipe assumptions). Median of 9 runs each.
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <arm_neon.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static int cmpd(const void *a, const void *b) { double x = *(double *)a, y = *(double *)b; return x < y ? -1 : x > y; }
#include <stdlib.h>
#define N 200000000L
#define REP 9
static double med(double *v) { qsort(v, REP, sizeof(double), cmpd); return v[REP / 2]; }
int main(void) {
    double r[REP]; volatile uint64_t sink = 0; volatile double fs = 0;
    for (int k = 0; k < REP; k++) { uint64_t x = k; double t = now();
        for (long i = 0; i < N; i++) __asm__ volatile("add %0, %0, #1" : "+r"(x));   // 1-cycle dependent chain
        r[k] = (now() - t) / N; sink += x; }
    double cyc = med(r); printf("dependent int add: %.4f ns/op => effective clock %.3f GHz\n", cyc * 1e9, 1 / cyc / 1e9);
    for (int k = 0; k < REP; k++) { double a = 1.0 + k * 1e-9, b = 1e-12; double t = now();
        for (long i = 0; i < N / 4; i++) __asm__ volatile("fadd %d0, %d0, %d1" : "+w"(a) : "w"(b));
        r[k] = (now() - t) / (N / 4); fs += a; }
    double lat = med(r); printf("dependent f64 fadd latency: %.3f ns = %.2f cycles\n", lat * 1e9, lat / cyc);
    for (int k = 0; k < REP; k++) { float64x2_t a0 = vdupq_n_f64(1), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0, b = vdupq_n_f64(1e-12); double t = now();
        for (long i = 0; i < N / 8; i++) { __asm__ volatile("fadd %0.2d, %0.2d, %8.2d\n fadd %1.2d, %1.2d, %8.2d\n fadd %2.2d, %2.2d, %8.2d\n fadd %3.2d, %3.2d, %8.2d\n fadd %4.2d, %4.2d, %8.2d\n fadd %5.2d, %5.2d, %8.2d\n fadd %6.2d, %6.2d, %8.2d\n fadd %7.2d, %7.2d, %8.2d"
            : "+w"(a0), "+w"(a1), "+w"(a2), "+w"(a3), "+w"(a4), "+w"(a5), "+w"(a6), "+w"(a7) : "w"(b)); }
        r[k] = (now() - t) / (N / 8 * 8); fs += vgetq_lane_f64(a0, 0) + vgetq_lane_f64(a7, 1); }
    double thr = med(r); printf("independent 128-bit fadd (8 chains): %.4f ns/instr = %.2f instr/cycle (f64x2: %.2f f64 adds/cycle, f32x4 same pipe: %.2f f32 adds/cycle)\n",
        thr * 1e9, cyc / thr, 2 * cyc / thr, 4 * cyc / thr);
    return (int)(sink & 0) + (fs == 12345.0);
}
