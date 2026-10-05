// 045 microbenchmark: cost of address-translation misses when the data itself is cached. A dependent pointer chase over K lines, either packed
// contiguously (few pages) or one line per 16 KB page spread over a large region (K pages). Also an independent-access variant (8 chains).
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <time.h>
#include <pthread.h>
#include <pthread/qos.h>
static inline uint64_t now(void) { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW); }
static void shuffle(size_t *a, size_t n, uint64_t *seed) { for (size_t i = n - 1; i > 0; i--) { *seed = *seed * 6364136223846793005ULL + 1442695040888963407ULL; size_t j = (*seed >> 33) % (i + 1); size_t t = a[i]; a[i] = a[j]; a[j] = t; } }
static double chase(char *base, size_t *off, size_t k, int chains, long iters) {
    for (size_t i = 0; i < k; i++) *(char **)(base + off[i]) = base + off[(i + 1) % k];
    char *p[8]; for (int c = 0; c < chains; c++) p[c] = base + off[(k / chains) * c];
    for (long i = 0; i < k * 4; i++) for (int c = 0; c < chains; c++) p[c] = *(char **)p[c];   // warm
    uint64_t t0 = now();
    for (long i = 0; i < iters; i++) for (int c = 0; c < chains; c++) p[c] = *(char **)p[c];
    uint64_t t1 = now();
    uintptr_t sink = 0; for (int c = 0; c < chains; c++) sink ^= (uintptr_t)p[c]; if (sink == 1) printf("!");
    return (double)(t1 - t0) / ((double)iters * chains);
}
int main(void) {
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    size_t region = 2048ull << 20; char *base = mmap(NULL, region, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    for (size_t i = 0; i < region; i += 16384) base[i] = 1;   // fault in
    size_t ks[] = {512, 2048, 8192, 32768}; uint64_t seed = 12345;
    for (int q = 0; q < 4; q++) {
        size_t k = ks[q]; size_t *off = malloc(k * sizeof *off);
        for (size_t i = 0; i < k; i++) off[i] = i * 128; shuffle(off, k, &seed);                       // packed: k lines in k*128 bytes
        double pk1 = chase(base, off, k, 1, 4000000), pk8 = chase(base, off, k, 8, 1000000);
        size_t npages = region / 16384, *pg = malloc(npages * sizeof *pg); for (size_t i = 0; i < npages; i++) pg[i] = i; shuffle(pg, npages, &seed);
        for (size_t i = 0; i < k; i++) off[i] = pg[i] * 16384 + ((i * 7) % 128) * 128; shuffle(off, k, &seed);   // spread: one line per page over 2 GB
        double sp1 = chase(base, off, k, 1, 4000000), sp8 = chase(base, off, k, 8, 1000000);
        printf("lines=%6zu data=%7zu KB | packed: dependent %.2f ns, 8 chains %.2f ns/access | spread over %zu pages: dependent %.2f ns, 8 chains %.2f ns/access\n",
            k, k * 128 / 1024, pk1, pk8, k, sp1, sp8);
        free(off); free(pg);
    }
    return 0;
}
