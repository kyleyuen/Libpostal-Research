// Experiment 002: calibration workloads to pin the meaning of the CPU Counters "bottleneck" columns.
// usage: counter_calib chase|alu|branch seconds
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static uint64_t rs = 88172645463325252ULL;
static inline uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    double secs = atof(argv[2]); uint64_t sink = 0;
    if (argv[1][0] == 'c') {                 // dependent pointer chase over 512 MiB (single random cycle, Sattolo)
        size_t n = (size_t)64 << 20; uint64_t *a = malloc(n * 8);
        for (size_t i = 0; i < n; i++) a[i] = i;
        for (size_t i = n - 1; i > 0; i--) { size_t j = rnd() % i; uint64_t t = a[i]; a[i] = a[j]; a[j] = t; }
        fprintf(stderr, "READY chase\n");
        uint64_t p = 0; double end = now() + secs; size_t steps = 0;
        while (now() < end) { for (int k = 0; k < 1000000; k++) p = a[p]; steps += 1000000; }
        sink = p; printf("chase steps=%zu sink=%llu\n", steps, (unsigned long long)sink);
    } else if (argv[1][0] == 'a') {          // independent ALU chains, everything in registers
        uint64_t x0 = 1, x1 = 2, x2 = 3, x3 = 4, x4 = 5, x5 = 6, x6 = 7, x7 = 8; double end = now() + secs; size_t it = 0;
        fprintf(stderr, "READY alu\n");
        while (now() < end) { for (int k = 0; k < 1000000; k++) {
            x0 = x0 * 3 + 1; x1 = x1 * 5 + 2; x2 = x2 * 7 + 3; x3 = x3 * 9 + 4; x4 = x4 * 11 + 5; x5 = x5 * 13 + 6; x6 = x6 * 15 + 7; x7 = x7 * 17 + 8; }
            it += 1000000; }
        sink = x0 ^ x1 ^ x2 ^ x3 ^ x4 ^ x5 ^ x6 ^ x7; printf("alu iters=%zu sink=%llu\n", it, (unsigned long long)sink);
    } else {                                 // random, unpredictable branches over a 1 MiB table (L2-resident)
        size_t n = 1 << 20; uint8_t *d = malloc(n); for (size_t i = 0; i < n; i++) d[i] = rnd() & 1;
        fprintf(stderr, "READY branch\n");
        uint64_t s = 0; double end = now() + secs; size_t it = 0;
        while (now() < end) { for (int rep = 0; rep < 100; rep++) for (size_t i = 0; i < n; i++) {
            if (d[i]) { s += i; __asm__ volatile("" ::: "memory"); } else { s ^= i * 3; __asm__ volatile("" ::: "memory"); } }
            it += 100 * n; }
        sink = s; printf("branch iters=%zu sink=%llu\n", it, (unsigned long long)sink);
    }
    return sink == 12345;
}
