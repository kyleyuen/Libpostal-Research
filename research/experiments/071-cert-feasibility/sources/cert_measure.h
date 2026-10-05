// Experiment 071 (measurement only): feasibility of certified early exit. Included once by crf.c.
#ifndef CERT_MEASURE_H
#define CERT_MEASURE_H
#include <math.h>
#define CM_G 6
#define CM_MAXL 32
static const char *cm_gname[CM_G] = {"pairs", "phrase-valued", "context (prev/next word + pairs)", "all transition features", "all state features except bias/word roles", "postcode"};
static uint8_t *cm_name_s = NULL, *cm_name_t = NULL; static size_t cm_rows_s = 0, cm_rows_t = 0;
static double cm_bs[128][CM_MAXL]; static double *cm_bt = NULL;   // [name][L*L]
static bool cm_skip_s[CM_G][128], cm_skip_t[CM_G][128];
static int cm_ready = 0;
static double *cm_marg = NULL, *cm_bnd[CM_G]; static size_t cm_mn = 0, cm_mcap = 0;
static unsigned long long cm_addr = 0, cm_cert[CM_G], cm_keys_total = 0, cm_keys_g[CM_G], cm_keys_cert[CM_G], cm_bad[CM_G];
static int cm_dcmp(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static void cm_report(void) {
    if (!cm_addr) return;
    if (cm_mn) { qsort(cm_marg, cm_mn, 8, cm_dcmp); fprintf(stderr, "MARGIN full-score best minus second-best path: p10=%.3f p50=%.3f p90=%.3f\n", cm_marg[cm_mn / 10], cm_marg[cm_mn / 2], cm_marg[cm_mn * 9 / 10]);
        for (int g = 0; g < CM_G; g++) { qsort(cm_bnd[g], cm_mn, 8, cm_dcmp); fprintf(stderr, "BOUND group=%d 2x path bound of skipped features: p10=%.3f p50=%.3f p90=%.3f\n", g, cm_bnd[g][cm_mn / 10], cm_bnd[g][cm_mn / 2], cm_bnd[g][cm_mn * 9 / 10]); } }
    fprintf(stderr, "CERT addresses=%llu keys=%llu (%.1f per address)\n", cm_addr, cm_keys_total, (double)cm_keys_total / cm_addr);
    for (int g = 0; g < CM_G; g++) fprintf(stderr, "CERT group=%d [%s] certified=%.1f%% group_keys=%.2f/addr skippable_keys=%.2f/addr (%.1f%% of all lookups) cert_but_wrong=%llu\n",
        g, cm_gname[g], 100.0 * cm_cert[g] / cm_addr, (double)cm_keys_g[g] / cm_addr, (double)cm_keys_cert[g] / cm_addr, 100.0 * cm_keys_cert[g] / (cm_keys_total ? cm_keys_total : 1), cm_bad[g]);
}
static void cm_map_dict(const feature_dict_t *d, uint8_t *map, size_t rows) {
    for (uint64_t i = 0; i < d->size; i++) { uint64_t s = d->slots[i]; if (!s) continue; const uint8_t *e = d->arena + (uint32_t)s;
        uint32_t id; if (d->wide_value) { size_t rl = strlen((const char *)e + 1); memcpy(&id, e + 2 + rl, 4); } else id = (uint32_t)((s >> 32) & 0xFFFFFF) - 1;
        if (id < rows) map[id] = e[0] & 0x7F; }
#ifdef EXP_INT_KEYS
    if (d->islots) for (uint64_t i = 0; i < d->isize; i++) { if (!d->islots[i].id1) continue; uint32_t id = d->islots[i].id1 - 1; if (id < rows) map[id] = (uint8_t)(d->islots[i].key >> (2 * FDI_WBITS)); }
#endif
}
static bool cm_name_is(const feature_dict_t *d, int nm, const char *s) { return nm >= 0 && (uint32_t)nm < d->num_names && strcmp(d->names[nm].str, s) == 0; }
static void cm_init(crf_t *self) {
    const feature_dict_t *sd = self->state_dict, *td = self->trans_dict; size_t L = self->num_classes;
    cm_rows_s = self->weights->indptr->n - 1; cm_rows_t = self->state_trans_weights->indptr->n - 1;
    cm_name_s = malloc(cm_rows_s); cm_name_t = malloc(cm_rows_t); memset(cm_name_s, 0xFF, cm_rows_s); memset(cm_name_t, 0xFF, cm_rows_t);
    cm_map_dict(sd, cm_name_s, cm_rows_s); cm_map_dict(td, cm_name_t, cm_rows_t);
    cm_bt = calloc(128 * L * L, sizeof(double));
    const uint32_t *ip = self->weights->indptr->a, *tp = self->state_trans_weights->indptr->a;
    for (size_t id = 0; id < cm_rows_s; id++) { uint8_t nm = cm_name_s[id]; if (nm >= 128) continue; for (uint32_t c = ip[id]; c < ip[id + 1]; c++) { double a = fabs(crf_pack_state[c].w); if (a > cm_bs[nm][crf_pack_state[c].cls]) cm_bs[nm][crf_pack_state[c].cls] = a; } }
    for (size_t id = 0; id < cm_rows_t; id++) { uint8_t nm = cm_name_t[id]; if (nm >= 128) continue; for (uint32_t c = tp[id]; c < tp[id + 1]; c++) { double a = fabs(crf_pack_trans[c].w); double *b = &cm_bt[nm * L * L + crf_pack_trans[c].cls]; if (a > *b) *b = a; } }
    for (int nm = 0; nm < 128; nm++) {
        cm_skip_s[0][nm] = cm_name_is(sd, nm, "prev word+word") || cm_name_is(sd, nm, "word+next word");
        cm_skip_s[1][nm] = nm < (int)sd->num_names && (strstr(sd->names[nm].str, "phrase") != NULL || strncmp(sd->names[nm].str, "commonly", 8) == 0);
        cm_skip_s[2][nm] = cm_skip_s[0][nm] || cm_name_is(sd, nm, "prev word") || cm_name_is(sd, nm, "next word");
        cm_skip_t[3][nm] = nm < (int)td->num_names;
        cm_skip_s[4][nm] = nm < (int)sd->num_names && !(cm_name_is(sd, nm, "bias") || cm_name_is(sd, nm, "word") || cm_name_is(sd, nm, "first word") || cm_name_is(sd, nm, "prev word") || cm_name_is(sd, nm, "next word"));
        cm_skip_s[5][nm] = nm < (int)sd->num_names && strncmp(sd->names[nm].str, "postcode", 8) == 0;
    }
    atexit(cm_report); cm_ready = 1;
}
// Viterbi (same recursion as crf_context_viterbi) on state[T][L], st[T][L*L] (t >= 1), trans[L*L]; returns best score, writes path.
static double cm_viterbi(size_t T, size_t L, const double *state, const double *st, const double *trans, uint32_t *path) {
    double a[64][CM_MAXL]; uint32_t bk[64][CM_MAXL];
    for (size_t j = 0; j < L; j++) a[0][j] = state[j];
    for (size_t t = 1; t < T; t++) for (size_t j = 0; j < L; j++) { double mx = -DBL_MAX; int am = -1;
        for (size_t i = 0; i < L; i++) { double sc = a[t - 1][i] + st[t * L * L + i * L + j] + trans[i * L + j]; if (mx < sc) { mx = sc; am = (int)i; } }
        bk[t][j] = am < 0 ? 0 : (uint32_t)am; a[t][j] = mx + state[t * L + j]; }
    double mx = -DBL_MAX; uint32_t am = 0; for (size_t i = 0; i < L; i++) if (a[T - 1][i] > mx) { mx = a[T - 1][i]; am = (uint32_t)i; }
    path[T - 1] = am; for (size_t t = T - 1; t > 0; t--) path[t - 1] = bk[t][path[t]];
    return mx;
}
// 2-best values over all paths (state/st/trans as above): v1 >= v2; path of the best in p1.
static void cm_viterbi2(size_t T, size_t L, const double *state, const double *st, const double *trans, double *v1, double *v2, uint32_t *p1) {
    double a[64][CM_MAXL][2]; uint32_t bi[64][CM_MAXL][2]; uint8_t br[64][CM_MAXL][2];
    for (size_t j = 0; j < L; j++) { a[0][j][0] = state[j]; a[0][j][1] = -INFINITY; }
    for (size_t t = 1; t < T; t++) for (size_t j = 0; j < L; j++) {
        double b0 = -INFINITY, b1 = -INFINITY; uint32_t i0 = 0, i1 = 0; uint8_t r0 = 0, r1 = 0;
        for (size_t i = 0; i < L; i++) for (int r = 0; r < 2; r++) { double sc = a[t - 1][i][r] + st[t * L * L + i * L + j] + trans[i * L + j];
            if (sc > b0) { b1 = b0; i1 = i0; r1 = r0; b0 = sc; i0 = (uint32_t)i; r0 = (uint8_t)r; } else if (sc > b1) { b1 = sc; i1 = (uint32_t)i; r1 = (uint8_t)r; } }
        a[t][j][0] = b0 + state[t * L + j]; a[t][j][1] = b1 + state[t * L + j]; bi[t][j][0] = i0; bi[t][j][1] = i1; br[t][j][0] = r0; br[t][j][1] = r1; }
    double b0 = -INFINITY, b1 = -INFINITY; uint32_t j0 = 0; uint8_t rr0 = 0;
    for (size_t j = 0; j < L; j++) for (int r = 0; r < 2; r++) { double sc = a[T - 1][j][r]; if (sc > b0) { b1 = b0; b0 = sc; j0 = (uint32_t)j; rr0 = (uint8_t)r; } else if (sc > b1) b1 = sc; }
    *v1 = b0; *v2 = b1; uint32_t j = j0; uint8_t r = rr0; p1[T - 1] = j;
    for (size_t t = T - 1; t > 0; t--) { uint32_t pi = bi[t][j][r]; uint8_t pr = br[t][j][r]; p1[t - 1] = pi; j = pi; r = pr; }
}
static double cm_path(size_t T, size_t L, const double *state, const double *st, const double *trans, const uint32_t *y) {
    double s = state[y[0]]; for (size_t t = 1; t < T; t++) s += st[t * L * L + y[t - 1] * L + y[t]] + trans[y[t - 1] * L + y[t]] + state[t * L + y[t]]; return s;
}
#endif
