// 159 P1 timing harness (156 ps_replay template). FOR THE LEAD'S SERIAL, GATED TIMING SESSION ONLY - teammates never run it timed.
// Replays each address's normalized + admin tokens (O1 types, O2 strings) through the three context-fill phrase searches:
//   variant "exact": ps_search_tokens_with_phrases on dict/comp/postal tables (mmap-loaded; load time reported)
//   variant "orig":  trie_search_tokens_with_phrases / search_address_dictionaries_tokens_with_phrases of the ORIGINAL library (142 ctl) on the real
//                    tries (reference only: cpi's own search adds EXP_FTT; cpi's whole context fill is measured separately by 119 css bench_split)
// Output: setup seconds, pass 1 (cold), median of passes 2..P, warm marginal (repeat-in-place), us/address; a phrase checksum (must be equal
// for both variants on the same corpus).
// usage: ps_bench {exact TABLEDIR|orig} ORACLE_DIR PASSES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "libpostal.h"
#include "address_parser.h"
#include "address_dictionary.h"
#include "../src/ps_exact.h"

static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static size_t unesc(char *s) {
    char *r = s, *w = s;
    while (*r) { if (*r == '\\' && r[1]) { r++; *w++ = *r == 't' ? '\t' : *r == 'n' ? '\n' : *r == 'r' ? '\r' : *r == '0' ? 0 : *r; r++; } else *w++ = *r++; }
    return (size_t)(w - s);
}
static int split(char *line, char **fld, int max) { int nf = 0; char *p = line; fld[nf++] = p; while ((p = strchr(p, '\t')) && nf < max) { *p++ = 0; fld[nf++] = p; } return nf; }
typedef struct { char *sn, *sa; token_array *tn, *ta; } addr_t;
static addr_t *A; static int nA;
static void load(const char *odir) {
    char cmd[4096]; snprintf(cmd, sizeof cmd, "gzip -dc '%s/O1.txt.gz'", odir); FILE *f1 = popen(cmd, "r");
    snprintf(cmd, sizeof cmd, "gzip -dc '%s/O2.txt.gz'", odir); FILE *f2 = popen(cmd, "r");
    char *l1 = NULL, *l2 = NULL; size_t c1 = 0, c2 = 0; ssize_t n2; int cap = 1 << 14; A = calloc(cap, sizeof *A);
    char *bn = malloc(1 << 20), *ba = malloc(1 << 20);
    while ((n2 = getline(&l2, &c2, f2)) > 0) {
        char *fld[16]; if (l2[n2 - 1] == '\n') l2[--n2] = 0; split(l2, fld, 16); size_t nt = (size_t)atol(fld[2]);
        if (getline(&l1, &c1, f1) <= 0) exit(4);
        token_array *tn = token_array_new(), *ta = token_array_new(); size_t on = 0, oa = 0;
        for (size_t t = 0; t < nt; t++) {
            ssize_t n1 = getline(&l1, &c1, f1); n2 = getline(&l2, &c2, f2); if (n1 <= 0 || n2 <= 0) exit(4);
            if (l1[n1 - 1] == '\n') l1[--n1] = 0;
            if (l2[n2 - 1] == '\n') l2[--n2] = 0;
            char *g[8]; split(l1, g, 8); uint16_t ty = (uint16_t)atoi(g[2]); split(l2, fld, 16);
            size_t ln = unesc(fld[2]), la = unesc(fld[3]); if (on + ln + 1 > (1 << 20) || oa + la + 1 > (1 << 20)) exit(5);
            token_array_push(tn, (token_t){on, ln, ty}); memcpy(bn + on, fld[2], ln); bn[on + ln] = 0; on += ln + 1;
            token_array_push(ta, (token_t){oa, la, ty}); memcpy(ba + oa, fld[3], la); ba[oa + la] = 0; oa += la + 1;
        }
        if (nA == cap) { cap *= 2; A = realloc(A, cap * sizeof *A); }
        A[nA].sn = malloc(on + 1); memcpy(A[nA].sn, bn, on); A[nA].sa = malloc(oa + 1); memcpy(A[nA].sa, ba, oa); A[nA].tn = tn; A[nA].ta = ta; nA++;
    }
    pclose(f1); pclose(f2);
}
static ps_table_t *T[3]; static trie_t *OT[3]; static int mode;   // 0 exact, 1 orig
static phrase_array *P[3]; static volatile uint64_t sink;
static void run_addr(int a) {
    if (A[a].tn->n == 0) return;
    for (int k = 0; k < 3; k++) phrase_array_clear(P[k]);
    if (mode == 0) {
        ps_search_tokens_with_phrases(T[0], A[a].sn, A[a].tn, &P[0]);
        ps_search_tokens_with_phrases(T[1], A[a].sa, A[a].ta, &P[1]);
        ps_search_tokens_with_phrases(T[2], A[a].sa, A[a].ta, &P[2]);
    } else {
        search_address_dictionaries_tokens_with_phrases(A[a].sn, A[a].tn, NULL, &P[0]);
        trie_search_tokens_with_phrases(OT[1], A[a].sa, A[a].ta, &P[1]);
        trie_search_tokens_with_phrases(OT[2], A[a].sa, A[a].ta, &P[2]);
    }
    for (int k = 0; k < 3; k++) for (size_t i = 0; i < P[k]->n; i++) sink = sink * 31 + P[k]->a[i].start * 7 + P[k]->a[i].len * 3 + P[k]->a[i].data;
}
static double pass(int twice) { double t0 = now(); for (int a = 0; a < nA; a++) { run_addr(a); if (twice) run_addr(a); } return now() - t0; }
int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: ps_bench {exact TABLEDIR|orig} ORACLE_DIR PASSES\n"); return 2; }
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);   // same scheduling class as bench_ext (no thread is created)
    mode = !strcmp(argv[1], "orig"); int ai = mode ? 2 : 3; if (argc < ai + 2) return 2;
    const char *odir = argv[ai]; int PS = atoi(argv[ai + 1]); if (PS < 2) PS = 2;
    for (int k = 0; k < 3; k++) P[k] = phrase_array_new();
    load(odir);   // corpus first, so table/trie loading is the last thing touched before pass 1
    double t0 = now();
    if (mode == 0) { const char *nm[3] = { "dict", "comp", "postal" }; char p[4096];
        for (int k = 0; k < 3; k++) { snprintf(p, sizeof p, "%s/%s.ps", argv[2], nm[k]); T[k] = ps_table_load(p); if (!T[k]) { fprintf(stderr, "load %s\n", p); return 2; } } }
    else { if (!libpostal_setup() || !libpostal_setup_parser()) return 2; address_parser_t *pr = get_address_parser(); OT[1] = pr->phrases; OT[2] = pr->postal_codes; OT[0] = get_address_dictionary()->trie; }
    double setup = now() - t0;
    double *c = malloc(sizeof(double) * PS), w[5];
    for (int p = 0; p < PS; p++) c[p] = pass(0) / nA * 1e6;
    for (int r = 0; r < 5; r++) { double t1 = pass(0), t2 = pass(1); w[r] = (t2 - t1) / nA * 1e6; }
    double first = c[0]; qsort(c + 1, PS - 1, sizeof(double), cmpd); qsort(w, 5, sizeof(double), cmpd);
    printf("%s %s addresses %d setup %.3f s | phrase search: pass1 %.3f  median p2..%d %.3f [%.3f-%.3f]  warm %.3f [%.3f-%.3f] us/addr | checksum %016llx\n",
           mode ? "orig" : "exact", odir, nA, setup, first, PS, c[1 + (PS - 1) / 2], c[1], c[PS - 1], w[2], w[0], w[4], (unsigned long long)sink);
    return 0;
}
