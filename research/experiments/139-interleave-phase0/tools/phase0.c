// Experiment 139 (interleaving plan, Phase 0): microbenchmarks on this Mac. (1) cost of a hand-written arm64 user-space context switch;
// (2) memory-level parallelism: K independent random pointer chases interleaved in one loop; (3) the same K chases as K coroutines that
// prefetch the next node, switch, and dereference on resume (the mechanism the parser would use). Single thread throughout.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <pthread/qos.h>
typedef struct { void *sp; } co_t;
void co_switch(co_t *from, co_t *to);
__asm__(".text\n.globl _co_switch\n.p2align 2\n_co_switch:\n"
    "sub sp, sp, #160\n"
    "stp x19, x20, [sp, #0]\nstp x21, x22, [sp, #16]\nstp x23, x24, [sp, #32]\nstp x25, x26, [sp, #48]\nstp x27, x28, [sp, #64]\nstp x29, x30, [sp, #80]\n"
    "stp d8, d9, [sp, #96]\nstp d10, d11, [sp, #112]\nstp d12, d13, [sp, #128]\nstp d14, d15, [sp, #144]\n"
    "mov x9, sp\nstr x9, [x0]\nldr x9, [x1]\nmov sp, x9\n"
    "ldp x19, x20, [sp, #0]\nldp x21, x22, [sp, #16]\nldp x23, x24, [sp, #32]\nldp x25, x26, [sp, #48]\nldp x27, x28, [sp, #64]\nldp x29, x30, [sp, #80]\n"
    "ldp d8, d9, [sp, #96]\nldp d10, d11, [sp, #112]\nldp d12, d13, [sp, #128]\nldp d14, d15, [sp, #144]\n"
    "add sp, sp, #160\nret\n"
    ".globl _co_trampoline\n.p2align 2\n_co_trampoline:\nmov x0, x20\nblr x19\nbrk #0\n");
void co_trampoline(void);
static void co_init(co_t *c, void *stack, size_t size, void (*fn)(void *), void *arg) {
    uintptr_t top = ((uintptr_t)stack + size) & ~(uintptr_t)15; uint64_t *f = (uint64_t *)(top - 160); memset(f, 0, 160);
    f[0] = (uint64_t)(uintptr_t)fn; f[1] = (uint64_t)(uintptr_t)arg; f[10] = 0; f[11] = (uint64_t)(uintptr_t)co_trampoline; c->sp = f;
}
static double ns(uint64_t t) { static mach_timebase_info_data_t tb; if (!tb.denom) mach_timebase_info(&tb); return (double)t * tb.numer / tb.denom; }
static co_t main_co, a_co, b_co; static volatile uint64_t pp_n;
static void pp_a(void *x) { (void)x; for (;;) { pp_n++; co_switch(&a_co, &b_co); } }
static void pp_b(void *x) { (void)x; for (;;) { co_switch(&b_co, &a_co); if (pp_n >= 20000000) co_switch(&b_co, &main_co); } }
static uint64_t *arr; static size_t N;
enum { KMAX = 8 };
static co_t ch_co[KMAX]; static int ch_k; static uint64_t ch_steps, ch_sink[KMAX]; static volatile int ch_done;
static void chase_co(void *x) {
    int me = (int)(intptr_t)x; uint64_t idx = (uint64_t)me * 7919 % N;
    for (uint64_t s = 0; s < ch_steps; s++) { const uint64_t *p = &arr[idx]; __builtin_prefetch(p); co_switch(&ch_co[me], &ch_co[(me + 1) % ch_k]); idx = *p; }
    ch_sink[me] = idx; ch_done++;
    for (;;) co_switch(&ch_co[me], ch_done == ch_k ? &main_co : &ch_co[(me + 1) % ch_k]);
}
int main(void) {
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    void *sa = malloc(1 << 16), *sb = malloc(1 << 16); co_init(&a_co, sa, 1 << 16, pp_a, NULL); co_init(&b_co, sb, 1 << 16, pp_b, NULL);
    uint64_t t0 = mach_absolute_time(); co_switch(&main_co, &a_co); uint64_t t1 = mach_absolute_time();
    printf("switch: %.2f ns per switch (%llu round trips)\n", ns(t1 - t0) / (2.0 * pp_n), (unsigned long long)pp_n);
    N = (size_t)64 << 20; arr = malloc(N * sizeof *arr); uint32_t *perm = malloc(N * sizeof *perm);
    for (size_t i = 0; i < N; i++) perm[i] = (uint32_t)i;
    uint64_t r = 88172645463325252ULL; for (size_t i = N - 1; i > 0; i--) { r ^= r << 13; r ^= r >> 7; r ^= r << 17; size_t j = r % (i + 1); uint32_t t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
    for (size_t i = 0; i < N; i++) arr[perm[i]] = perm[(i + 1) % N]; free(perm);
    const uint64_t steps = 4000000;
    for (int k = 1; k <= KMAX; k *= 2) {
        uint64_t idx[KMAX]; for (int c = 0; c < k; c++) idx[c] = (uint64_t)c * 7919 % N;
        uint64_t s0 = mach_absolute_time();
        for (uint64_t s = 0; s < steps / k; s++) for (int c = 0; c < k; c++) idx[c] = arr[idx[c]];
        uint64_t s1 = mach_absolute_time(); uint64_t x = 0; for (int c = 0; c < k; c++) x ^= idx[c];
        printf("loop chase      k=%d: %.2f ns per access [%llu]\n", k, ns(s1 - s0) / (double)((steps / k) * k), (unsigned long long)(x & 1));
    }
    for (int k = 1; k <= KMAX; k *= 2) {
        ch_k = k; ch_steps = steps / k; ch_done = 0; void *stk[KMAX];
        for (int c = 0; c < k; c++) { stk[c] = malloc(1 << 16); co_init(&ch_co[c], stk[c], 1 << 16, chase_co, (void *)(intptr_t)c); }
        uint64_t s0 = mach_absolute_time(); co_switch(&main_co, &ch_co[0]); uint64_t s1 = mach_absolute_time();
        printf("coroutine chase k=%d: %.2f ns per access\n", k, ns(s1 - s0) / (double)(ch_steps * k));
        for (int c = 0; c < k; c++) free(stk[c]);
    }
    return 0;
}
