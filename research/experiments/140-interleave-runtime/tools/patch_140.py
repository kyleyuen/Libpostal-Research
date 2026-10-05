# Experiment 140: applies the interleaving runtime to the copy of cpi's sources in sources/ (run from the experiment directory).
# Every edit asserts its anchor; the runtime itself is tools/runtime_140.c, appended to address_parser.c.
S = "sources/"

# ---------------- crf.c: per-address state save/load + yield points ----------------
p = S + "crf.c"; s = open(p).read()
h = "static crf_wrec_t *crf_pack_state = NULL, *crf_pack_trans = NULL;"
assert s.count(h) == 1
s = s.replace(h, h + "\n#ifdef EXP_BATCH\nextern int ap_batch_active; void ap_batch_yield(int point);\n#define CRF_YIELD(pt) do { if (ap_batch_active) ap_batch_yield(pt); } while (0)\n#else\n#define CRF_YIELD(pt) ((void)0)\n#endif   // 140")
s += r'''
#ifdef EXP_BATCH
// Experiment 140 (interleaving runtime): this file's per-address state, saved / restored by the batch scheduler (address_parser.c) at every switch
// between in-flight addresses. Everything else here is model data, read-only after load.
typedef struct { uint64_t *ik_keys[2]; size_t ik_n[2], ik_m[2]; uint32_t *state_ids, *trans_ids, *state_tok, *trans_tok; size_t state_ids_m, trans_ids_m, tok_m;
    uint32_t *feature_ids; size_t feature_ids_capacity; size_t lean_cap; double_matrix_t *trans_saved; int features_accumulate; crf_context_t *ctx; } crf_xs_t;
size_t crf_xs_size(void) { return sizeof(crf_xs_t); }
void crf_xs_save(crf_t *c, void *v) { crf_xs_t *x = v;
    for (int w = 0; w < 2; w++) { x->ik_keys[w] = crf_ik_keys[w]; x->ik_n[w] = crf_ik_n[w]; x->ik_m[w] = crf_ik_m[w]; }
    x->state_ids = crf_state_ids; x->trans_ids = crf_trans_ids; x->state_tok = crf_state_tok; x->trans_tok = crf_trans_tok;
    x->state_ids_m = crf_state_ids_m; x->trans_ids_m = crf_trans_ids_m; x->tok_m = crf_tok_m; x->feature_ids = crf_feature_ids; x->feature_ids_capacity = crf_feature_ids_capacity;
    x->lean_cap = crf_lean_cap_items; x->trans_saved = crf_trans_saved; x->features_accumulate = crf_features_accumulate; x->ctx = c->context; }
void crf_xs_load(crf_t *c, const void *v) { const crf_xs_t *x = v;
    for (int w = 0; w < 2; w++) { crf_ik_keys[w] = x->ik_keys[w]; crf_ik_n[w] = x->ik_n[w]; crf_ik_m[w] = x->ik_m[w]; }
    crf_state_ids = x->state_ids; crf_trans_ids = x->trans_ids; crf_state_tok = x->state_tok; crf_trans_tok = x->trans_tok;
    crf_state_ids_m = x->state_ids_m; crf_trans_ids_m = x->trans_ids_m; crf_tok_m = x->tok_m; crf_feature_ids = x->feature_ids; crf_feature_ids_capacity = x->feature_ids_capacity;
    crf_lean_cap_items = x->lean_cap; crf_trans_saved = x->trans_saved; crf_features_accumulate = x->features_accumulate; c->context = x->ctx; }
void crf_xs_init_fresh(crf_t *c, void *v) {   // a new in-flight slot: empty scratch, its own CRF context
    crf_xs_t *x = v; memset(x, 0, sizeof *x); x->ctx = crf_context_new(CRF_CONTEXT_VITERBI | CRF_CONTEXT_MARGINALS, c->num_classes, CRF_CONTEXT_DEFAULT_NUM_ITEMS); }
#endif
'''   # functions appended at the end of crf.c, after every static they touch
# yield (2): direct scoring path only, after the weight-row prefetch loops, before accumulation
y = "    for (size_t t = 0; t < num_tokens; t++) {\n        double *state_scores = state_score(crf_context, (uint32_t)t);"
d0 = s.index("static bool crf_tagger_score_direct(")
assert s.count(y) == 2
k = s.index(y, d0)
s = s[:k] + "    CRF_YIELD(2);   // 140: weight rows were just prefetched\n" + s[k:]
# yield (1): crf_dict_lookup_array, between the slot-prefetch pass and the compare pass of each window
w1 = "        for (size_t j = 0; j < m; j++) {\n            uint32_t feature_id;\n#ifdef EXP_INT_KEYS\n            if (isint[j]) {"
assert s.count(w1) == 1, s.count(w1)
s = s.replace(w1, "        CRF_YIELD(1);   // 140: this window's dictionary slots were just prefetched\n" + w1)
open(p, "w").write(s)

# ---------------- crf_context.c: Viterbi buffer ----------------
p = S + "crf_context.c"; s = open(p).read()
a = "static double *vmo_max = NULL; static size_t vmo_cap = 0;"
assert s.count(a) == 1
s = s.replace(a, a + "\n#ifdef EXP_BATCH\nvoid crf_vmo_swap(double **m, size_t *c) { double *t = vmo_max; size_t u = vmo_cap; vmo_max = *m; vmo_cap = *c; *m = t; *c = u; }   // 140\n#endif")
open(p, "w").write(s)

# ---------------- address_parser.c ----------------
p = S + "address_parser.c"; s = open(p).read()
a = "        static int32_t *ftt_slots = NULL; static size_t ftt_cap = 0; size_t nt = normalized_admin_tokens->n;"
assert s.count(a) == 1
s = s.replace(a, "        int32_t *ftt_slots = ap_ftt_slots; size_t ftt_cap = ap_ftt_cap; size_t nt = normalized_admin_tokens->n;   // 140: file-scope (per in-flight address)")
b = "int32_t *q = realloc(ftt_slots, m * sizeof *q); if (q) { ftt_slots = q; ftt_cap = m; } }"
assert s.count(b) == 1
s = s.replace(b, "int32_t *q = realloc(ftt_slots, m * sizeof *q); if (q) { ftt_slots = q; ftt_cap = m; ap_ftt_slots = q; ap_ftt_cap = m; } }")
c = "void address_parser_context_fill(address_parser_context_t *context"
assert s.count(c) == 1
s = s.replace(c, "static int32_t *ap_ftt_slots = NULL; static size_t ap_ftt_cap = 0;   // 056 slot scratch (block-static before 140)\n" + c, 1)
d = "    bool have_component_phrases = trie_search_tokens_with_phrases(parser->phrases, normalized_str_admin, normalized_admin_tokens, &component_phrases);"
assert s.count(d) == 1
s = s.replace(d, "#ifdef EXP_BATCH\n    { extern int ap_batch_active; void ap_batch_yield(int point); if (ap_batch_active) ap_batch_yield(0); }   // 140: first-token entries / trie nodes just prefetched\n#endif\n" + d)
s += open("tools/runtime_140.c").read()
open(p, "w").write(s)
print("patched")
