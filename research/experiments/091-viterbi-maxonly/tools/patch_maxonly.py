# 091: applies the EXP_VITERBI_MAXONLY edits to sources/crf_context.c (copied from 077 pcp).
p='sources/crf_context.c'; s=open(p).read()
fn=s.index('double crf_context_viterbi(crf_context_t *self, uint32_t *labels) {')
s=s[:fn]+'''#ifdef EXP_VITERBI_MAXONLY
// Experiment 091: for L = 20 the forward pass keeps only the running maxima (same compare/select, same values) and stores each step's maximum before
// the state score is added; the backtrack recovers each step's argmax for the one label on the path as the first i whose score, recomputed with the
// identical expression, equals that maximum (= the first strictly greater score of the original scan), or 0 if no score ever beat -DBL_MAX (the
// original then left the zeroed backward edge, 0).
static double *vmo_max = NULL; static size_t vmo_cap = 0;
#endif
'''+s[fn:]
fn=s.index('double crf_context_viterbi(crf_context_t *self, uint32_t *labels) {')
def ins_after(anchor, text):
    global s; i=s.index(anchor,fn)+len(anchor); s=s[:i]+text+s[i:]
def ins_before(anchor, text):
    global s; i=s.index(anchor,fn); s=s[:i]+text+s[i:]
ins_after('    double_array_raw_copy(cur, state, L);\n','''#ifdef EXP_VITERBI_MAXONLY
    bool vmo_used = false;
    if (L == 20) {
        if (T * 20 > vmo_cap) { size_t m = vmo_cap ? vmo_cap : 64 * 20; while (m < T * 20) m *= 2; double *q = realloc(vmo_max, m * sizeof *q); if (q) { vmo_max = q; vmo_cap = m; } }
        vmo_used = vmo_max != NULL && T * 20 <= vmo_cap;
    }
#endif
''')
ins_before('        if (L == 20) {\n#define V20_DECL(k)','''#ifdef EXP_VITERBI_MAXONLY
        if (vmo_used) {
#define M20_DECL(k) float64x2_t m##k = vdupq_n_f64(-DBL_MAX);
#define M20_STEP(k) { float64x2_t sc = vaddq_f64(vaddq_f64(p, vld1q_f64(sr + 2 * k)), vld1q_f64(tr + 2 * k)); uint64x2_t gt = vcltq_f64(m##k, sc); \\
                      m##k = vbslq_f64(gt, sc, m##k); }
#define M20_OUT(k) { vst1q_f64(mrow + 2 * k, m##k); vst1q_f64(cur + 2 * k, vaddq_f64(m##k, vld1q_f64(state + 2 * k))); }
            M20_DECL(0) M20_DECL(1) M20_DECL(2) M20_DECL(3) M20_DECL(4) M20_DECL(5) M20_DECL(6) M20_DECL(7) M20_DECL(8) M20_DECL(9)
            for (i = 0; i < 20; i++) {
                const double *sr = state_trans_score(self, t, i), *tr = trans_score(self, i);
                const float64x2_t p = vdupq_n_f64(prev[i]);
                M20_STEP(0) M20_STEP(1) M20_STEP(2) M20_STEP(3) M20_STEP(4) M20_STEP(5) M20_STEP(6) M20_STEP(7) M20_STEP(8) M20_STEP(9)
            }
            double *mrow = vmo_max + (size_t)t * 20;
            M20_OUT(0) M20_OUT(1) M20_OUT(2) M20_OUT(3) M20_OUT(4) M20_OUT(5) M20_OUT(6) M20_OUT(7) M20_OUT(8) M20_OUT(9)
#undef M20_DECL
#undef M20_STEP
#undef M20_OUT
            continue;
        }
#endif
''')
ins_before('''    /* Tag labels by tracing the backward links. */''','''#ifdef EXP_VITERBI_MAXONLY
    if (vmo_used) {
        for (t = (int)T - 1; t >= 1; t--) {
            const uint32_t jl = labels[t]; const double target = vmo_max[(size_t)t * 20 + jl]; const double *pv = alpha_score(self, t - 1);
            uint32_t am = 0;
            if (target != -DBL_MAX) {
                for (i = 0; i < 20; i++) { double sc = pv[i] + state_trans_score(self, t, i)[jl] + trans_score(self, i)[jl]; if (sc == target) { am = (uint32_t)i; break; } }
#ifdef EXP_VITERBI_MAXONLY_VERIFY
                if (i == 20) { fprintf(stderr, "EXP_VITERBI_MAXONLY: no i matched\\n"); abort(); }
#endif
            }
            labels[t - 1] = am;
        }
        return max_score;
    }
#endif
''')
open(p,'w').write(s)
