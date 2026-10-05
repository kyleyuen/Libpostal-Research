// Experiment 014 Q2 (synthetic arrays only, no libpostal): DRAM latency (lambda) and memory-level parallelism (M) on this machine.
// One random cyclic permutation (Sattolo) over N nodes, one node per 128-byte line (hw.cachelinesize = 128), array of `bytes` total.
// Variant "idx": each node stores ONLY the 32-bit index of the next node (no pointer-shaped data anywhere in the array).
// Variant "ptr": each node stores ONLY the absolute pointer to the next node (pointer-shaped data: exposes any data-dependent prefetcher).
// The array is rewritten before each variant pass so it holds only that variant's field (the other bytes are zero).
// k = 1..32 independent chains are advanced in one interleaved loop; chains start at evenly spaced positions on the cycle.
// lambda = ns per dependent load with k = 1; throughput(k) = loads/ns; M(k) = lambda * throughput(k) (Little's law).
// usage: chase BYTES LOADS_PER_MEASUREMENT TRIALS   (prints TSV: trial variant bytes k loads ns ns_per_step loads_per_ns)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread/qos.h>

#define LINE 128
static inline uint64_t nowns(void) { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW); }
static uint64_t rs = 0x9E3779B97F4A7C15ULL;
static inline uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }

static uint8_t *A; static size_t N;
static uint32_t *order;     // cycle order: order[p] = node at position p

#define DEF_IDX(K) static uint64_t chase_idx_##K(size_t steps, const uint32_t *st) { \
    uint32_t c[K]; for (int j = 0; j < K; j++) c[j] = st[j]; \
    for (size_t s = 0; s < steps; s++) for (int j = 0; j < K; j++) c[j] = *(const uint32_t *)(A + (size_t)c[j] * LINE); \
    uint64_t x = 0; for (int j = 0; j < K; j++) x += c[j]; return x; }
#define DEF_PTR(K) static uint64_t chase_ptr_##K(size_t steps, const uint32_t *st) { \
    const uint8_t *c[K]; for (int j = 0; j < K; j++) c[j] = A + (size_t)st[j] * LINE; \
    for (size_t s = 0; s < steps; s++) for (int j = 0; j < K; j++) c[j] = *(const uint8_t *const *)(c[j] + 8); \
    uint64_t x = 0; for (int j = 0; j < K; j++) x += (uintptr_t)c[j]; return x; }
#define ALL(M) M(1) M(2) M(3) M(4) M(5) M(6) M(7) M(8) M(9) M(10) M(11) M(12) M(13) M(14) M(15) M(16) \
    M(17) M(18) M(19) M(20) M(21) M(22) M(23) M(24) M(25) M(26) M(27) M(28) M(29) M(30) M(31) M(32)
#define KMAX 32
ALL(DEF_IDX) ALL(DEF_PTR)
typedef uint64_t (*chase_fn)(size_t, const uint32_t *);
#define REF_IDX(K) chase_idx_##K,
#define REF_PTR(K) chase_ptr_##K,
static chase_fn FI[KMAX] = {ALL(REF_IDX)}, FP[KMAX] = {ALL(REF_PTR)};

int main(int argc, char **argv) {
    if (argc != 4) return 2;
    size_t bytes = strtoull(argv[1], 0, 10), loads = strtoull(argv[2], 0, 10); int trials = atoi(argv[3]);
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);   // same scheduling class as bench_ext
    N = bytes / LINE; if (N < 64 || N > 0xFFFFFFFFu) return 2;
    A = aligned_alloc(16384, N * LINE); uint32_t *next = malloc(N * 4); order = malloc(N * 4);
    if (!A || !next || !order) { fprintf(stderr, "alloc failed\n"); return 3; }
    for (size_t i = 0; i < N; i++) next[i] = (uint32_t)i;
    for (size_t i = N - 1; i > 0; i--) { size_t j = rnd() % i; uint32_t t = next[i]; next[i] = next[j]; next[j] = t; }   // Sattolo: one cycle
    size_t p = 0; uint32_t cur = 0; do { order[p++] = cur; cur = next[cur]; } while (cur != 0 && p <= N);
    if (p != N) { fprintf(stderr, "not a single cycle: %zu of %zu\n", p, N); return 3; }
    fprintf(stderr, "nodes=%zu bytes=%zu single_cycle=yes base%%16384=%lu\n", N, N * LINE, (uintptr_t)A % 16384);
    printf("trial\tvariant\tbytes\tk\tloads\tns\tns_per_step\tloads_per_ns\n");
    uint64_t sink = 0; uint32_t st[KMAX];
    for (int t = 0; t < trials; t++) {
        for (int pass = 0; pass < 2; pass++) {
            int v = (t + pass) % 2;   // alternate which variant goes first in each trial
            memset(A, 0, N * LINE);
            for (size_t i = 0; i < N; i++) { if (v) *(uint8_t **)(A + i * LINE + 8) = A + (size_t)next[i] * LINE; else *(uint32_t *)(A + i * LINE) = next[i]; }
            for (int kk = 0; kk < KMAX; kk++) {
                int k = (t % 2) ? KMAX - kk : kk + 1;   // alternate k order between trials
                for (int j = 0; j < k; j++) st[j] = order[(size_t)j * (N / k)];
                size_t steps = loads / k; if (!steps) steps = 1;
                (v ? FP : FI)[k - 1](steps / 16 + 1, st);   // short warm-up of the same chains
                uint64_t t0 = nowns(); sink += (v ? FP : FI)[k - 1](steps, st); uint64_t ns = nowns() - t0;
                printf("%d\t%s\t%zu\t%d\t%zu\t%llu\t%.3f\t%.5f\n", t, v ? "ptr" : "idx", N * LINE, k, steps * k, (unsigned long long)ns, (double)ns / steps, (double)steps * k / ns);
                fflush(stdout);
            }
        }
    }
    fprintf(stderr, "sink=%llu\n", (unsigned long long)sink);
    return 0;
}
