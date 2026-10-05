
#ifdef EXP_BATCH
// =====================================================================================================================================
// Experiment 140: cross-address interleaving runtime ("software hyperthreading"). libpostal_parse_address_batch() keeps up to AP_BATCH_N addresses
// in flight on the calling thread, each in its own stackful coroutine (hand-written arm64 switch, experiment 139). At yield points placed right after
// a batch of prefetches, the running address saves its per-address state and another address resumes. Each address executes exactly the code and
// floating-point operations of a sequential parse; only the interleaving of different addresses changes. Nothing is shared or reused between
// addresses: every piece of per-address mutable state (inventoried with nm, see 140 README) is private to its slot.
// Controls (environment, read once): AP_BATCH_N (1..4 slots in use), AP_YIELD_MASK (bit k enables yield point k; default AP_YIELD_DEFAULT),
// AP_STRESS=seed (switch at a random half of the enabled yield points: verification only).
// =====================================================================================================================================
#ifndef AP_BATCH_N
#define AP_BATCH_N 4
#endif
#ifndef AP_YIELD_DEFAULT
#define AP_YIELD_DEFAULT 0u
#endif
typedef struct { void *sp; } ap_co_t;
void ap_co_switch(ap_co_t *from, ap_co_t *to);
void ap_co_trampoline(void);
__asm__(".text\n.globl _ap_co_switch\n.p2align 2\n_ap_co_switch:\n"
    "sub sp, sp, #160\n"
    "stp x19, x20, [sp, #0]\nstp x21, x22, [sp, #16]\nstp x23, x24, [sp, #32]\nstp x25, x26, [sp, #48]\nstp x27, x28, [sp, #64]\nstp x29, x30, [sp, #80]\n"
    "stp d8, d9, [sp, #96]\nstp d10, d11, [sp, #112]\nstp d12, d13, [sp, #128]\nstp d14, d15, [sp, #144]\n"
    "mov x9, sp\nstr x9, [x0]\nldr x9, [x1]\nmov sp, x9\n"
    "ldp x19, x20, [sp, #0]\nldp x21, x22, [sp, #16]\nldp x23, x24, [sp, #32]\nldp x25, x26, [sp, #48]\nldp x27, x28, [sp, #64]\nldp x29, x30, [sp, #80]\n"
    "ldp d8, d9, [sp, #96]\nldp d10, d11, [sp, #112]\nldp d12, d13, [sp, #128]\nldp d14, d15, [sp, #144]\n"
    "add sp, sp, #160\nret\n"
    ".globl _ap_co_trampoline\n.p2align 2\n_ap_co_trampoline:\nmov x0, x20\nblr x19\nbrk #0\n");
static void ap_co_init(ap_co_t *c, void *stack, size_t size, void (*fn)(void *), void *arg) {
    uintptr_t top = ((uintptr_t)stack + size) & ~(uintptr_t)15; uint64_t *f = (uint64_t *)(top - 160); memset(f, 0, 160);
    f[0] = (uint64_t)(uintptr_t)fn; f[1] = (uint64_t)(uintptr_t)arg; f[11] = (uint64_t)(uintptr_t)ap_co_trampoline; c->sp = f;
}
size_t crf_xs_size(void); void crf_xs_save(crf_t *c, void *v); void crf_xs_load(crf_t *c, const void *v); void crf_xs_init_fresh(crf_t *c, void *v);
void crf_vmo_swap(double **m, size_t *c);
extern const void *fa_nb_arr[2];
typedef struct {   // this file's per-address state plus the exported per-address globals of trie_search.c, features.c and crf_context.c
    address_parser_context_t *context;
    int32_t *ftt_slots; size_t ftt_cap;
#ifdef EXP_VOCAB_MEMO
    ap_vmemo_t *vm; size_t vm_cap, vm_n; bool vm_on;
#endif
#if defined(EXP_WORD_MEMO)
    ap_wm_t *wm; size_t wm_cap, wm_n; bool wm_on;
#endif
#ifdef EXP_FA_LEN
    ap_pm_t *pm[2]; size_t pm_cap[2]; uint32_t pm_gen;
#endif
#ifdef EXP_REUSE
    token_array *rs_tokens; tokenized_string_t *rs_ts; char_array *rs_str; cstring_array *rs_token_labels, *rs_labels, *rs_components;
#endif
    const int32_t *trie_ftt_slot; size_t trie_ftt_n;
    const void *fa_nb_arr[2];
    double *vmo_max; size_t vmo_cap;
    unsigned char crf[256] __attribute__((aligned(16)));
} ap_xs_t;
// save copies the live globals into x. The Viterbi buffer lives in crf_context.c and is exchanged by swap: after a save the live pointer is NULL until
// the next load installs another slot's buffer, so save + load is exact.
static void ap_xs_save(ap_xs_t *x) {
    x->context = parser->context; x->ftt_slots = ap_ftt_slots; x->ftt_cap = ap_ftt_cap;
#ifdef EXP_VOCAB_MEMO
    x->vm = ap_vm; x->vm_cap = ap_vm_cap; x->vm_n = ap_vm_n; x->vm_on = ap_vm_on;
#endif
#if defined(EXP_WORD_MEMO)
    x->wm = ap_wm; x->wm_cap = ap_wm_cap; x->wm_n = ap_wm_n; x->wm_on = ap_wm_on;
#endif
#ifdef EXP_FA_LEN
    x->pm[0] = ap_pm[0]; x->pm[1] = ap_pm[1]; x->pm_cap[0] = ap_pm_cap[0]; x->pm_cap[1] = ap_pm_cap[1]; x->pm_gen = ap_pm_gen;
#endif
#ifdef EXP_REUSE
    x->rs_tokens = rs_tokens; x->rs_ts = rs_ts; x->rs_str = rs_str; x->rs_token_labels = rs_token_labels; x->rs_labels = rs_labels; x->rs_components = rs_components;
#endif
    x->trie_ftt_slot = trie_ftt_slot; x->trie_ftt_n = trie_ftt_n; x->fa_nb_arr[0] = fa_nb_arr[0]; x->fa_nb_arr[1] = fa_nb_arr[1];
    x->vmo_max = NULL; x->vmo_cap = 0; crf_vmo_swap(&x->vmo_max, &x->vmo_cap);
    crf_xs_save(parser->model.crf, x->crf);
}
static void ap_xs_load(ap_xs_t *x) {
    parser->context = x->context; ap_ftt_slots = x->ftt_slots; ap_ftt_cap = x->ftt_cap;
#ifdef EXP_VOCAB_MEMO
    ap_vm = x->vm; ap_vm_cap = x->vm_cap; ap_vm_n = x->vm_n; ap_vm_on = x->vm_on;
#endif
#if defined(EXP_WORD_MEMO)
    ap_wm = x->wm; ap_wm_cap = x->wm_cap; ap_wm_n = x->wm_n; ap_wm_on = x->wm_on;
#endif
#ifdef EXP_FA_LEN
    ap_pm[0] = x->pm[0]; ap_pm[1] = x->pm[1]; ap_pm_cap[0] = x->pm_cap[0]; ap_pm_cap[1] = x->pm_cap[1]; ap_pm_gen = x->pm_gen;
#endif
#ifdef EXP_REUSE
    rs_tokens = x->rs_tokens; rs_ts = x->rs_ts; rs_str = x->rs_str; rs_token_labels = x->rs_token_labels; rs_labels = x->rs_labels; rs_components = x->rs_components;
#endif
    trie_ftt_slot = x->trie_ftt_slot; trie_ftt_n = x->trie_ftt_n; fa_nb_arr[0] = x->fa_nb_arr[0]; fa_nb_arr[1] = x->fa_nb_arr[1];
    { double *m = x->vmo_max; size_t c = x->vmo_cap; crf_vmo_swap(&m, &c); }
    crf_xs_load(parser->model.crf, x->crf);
}
typedef struct { ap_co_t co; void *stack; ap_xs_t xs; int done; int used; } ap_slot_t;
static ap_slot_t ap_slots[AP_BATCH_N]; static int ap_slots_ready = 0, ap_cur = 0, ap_nslots = AP_BATCH_N; static ap_co_t ap_main_co;
static char **ap_bq_addrs; static size_t ap_bq_n, ap_bq_next; static libpostal_address_parser_response_t **ap_bq_out; static char *ap_bq_lang, *ap_bq_country;
int ap_batch_active = 0; static unsigned ap_yield_mask = 0; static int ap_stress = 0; static uint64_t ap_stress_rs = 1;
unsigned long long ap_yield_count = 0;
#ifndef AP_STACK_BYTES
#define AP_STACK_BYTES (8u << 20)   /* 140: as large as the main thread's default stack; untouched pages cost no memory */
#endif
enum { AP_STACK = AP_STACK_BYTES };
static int ap_pick_next(int from) {   // round robin over slots still working; -1 if none
    for (int k = 1; k <= ap_nslots; k++) { int c = (from + k) % ap_nslots; if (!ap_slots[c].done && ap_slots[c].used) return c; }
    return -1;
}
void ap_batch_yield(int point) {
    if (!ap_batch_active || !((ap_yield_mask >> point) & 1)) return;
    if (ap_stress) { ap_stress_rs ^= ap_stress_rs << 13; ap_stress_rs ^= ap_stress_rs >> 7; ap_stress_rs ^= ap_stress_rs << 17; if (ap_stress_rs & 1) return; }
    int nx = ap_pick_next(ap_cur); if (nx < 0 || nx == ap_cur) return;
    ap_yield_count++;
    int me = ap_cur; ap_xs_save(&ap_slots[me].xs); ap_xs_load(&ap_slots[nx].xs); ap_cur = nx;
    ap_co_switch(&ap_slots[me].co, &ap_slots[nx].co);
}
static void ap_slot_main(void *arg) {
    int me = (int)(intptr_t)arg;
    for (;;) { size_t i = ap_bq_next; if (i >= ap_bq_n) break; ap_bq_next = i + 1; ap_bq_out[i] = address_parser_parse(ap_bq_addrs[i], ap_bq_lang, ap_bq_country); }
    ap_slots[me].done = 1;
    int nx = ap_pick_next(me);
    ap_xs_save(&ap_slots[me].xs);
    if (nx < 0) { ap_xs_load(&ap_slots[0].xs); ap_cur = 0; ap_co_switch(&ap_slots[me].co, &ap_main_co); }   // all finished: slot 0's state back in place
    else { ap_xs_load(&ap_slots[nx].xs); ap_cur = nx; ap_co_switch(&ap_slots[me].co, &ap_slots[nx].co); }
    __builtin_trap();   // a finished slot is never resumed
}
static bool ap_slots_init(void) {
    if (ap_slots_ready) return true;
    if (crf_xs_size() > sizeof ap_slots[0].xs.crf || parser == NULL || parser->model_type != ADDRESS_PARSER_TYPE_CRF) return false;
    for (int k = 0; k < AP_BATCH_N; k++) { ap_slots[k].stack = malloc(AP_STACK); if (!ap_slots[k].stack) return false; }
    ap_xs_save(&ap_slots[0].xs); ap_xs_load(&ap_slots[0].xs);   // slot 0 = the parser's own contexts and scratch
    for (int k = 1; k < AP_BATCH_N; k++) { ap_xs_t *x = &ap_slots[k].xs; memset(x, 0, sizeof *x); x->context = address_parser_context_new(); if (!x->context) return false;
        crf_xs_init_fresh(parser->model.crf, x->crf); }
    const char *e = getenv("AP_BATCH_N"); if (e) { int v = atoi(e); if (v >= 1 && v <= AP_BATCH_N) ap_nslots = v; }
    e = getenv("AP_YIELD_MASK"); ap_yield_mask = e ? (unsigned)strtoul(e, NULL, 0) : AP_YIELD_DEFAULT;
    ap_stress = getenv("AP_STRESS") != NULL; if (ap_stress) ap_stress_rs = 0x9E3779B97F4A7C15ULL ^ (uint64_t)atoll(getenv("AP_STRESS"));
    ap_slots_ready = 1; return true;
}
bool libpostal_parse_address_batch(char **addrs, size_t n, libpostal_address_parser_options_t options, libpostal_address_parser_response_t **out) {
    if (parser == NULL || !ap_slots_init()) { for (size_t i = 0; i < n; i++) out[i] = address_parser_parse(addrs[i], options.language, options.country); return true; }
    ap_bq_addrs = addrs; ap_bq_n = n; ap_bq_next = 0; ap_bq_out = out; ap_bq_lang = options.language; ap_bq_country = options.country;
    for (int k = 0; k < ap_nslots; k++) { ap_slots[k].done = 0; ap_slots[k].used = 1; ap_co_init(&ap_slots[k].co, ap_slots[k].stack, AP_STACK, ap_slot_main, (void *)(intptr_t)k); }
    for (int k = ap_nslots; k < AP_BATCH_N; k++) { ap_slots[k].used = 0; ap_slots[k].done = 1; }
    ap_batch_active = 1; ap_cur = 0;   // globals hold slot 0's state
    ap_co_switch(&ap_main_co, &ap_slots[0].co);
    ap_batch_active = 0;
    return true;
}
#endif
