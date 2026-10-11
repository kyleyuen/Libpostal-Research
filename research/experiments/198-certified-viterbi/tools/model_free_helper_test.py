#!/usr/bin/env python3
"""Extract the exact v3 decoder functions into a model-free synthetic-context test."""
from pathlib import Path
import hashlib

ROOT = Path(__file__).resolve().parents[4]
SRC = ROOT / "research/experiments/198-certified-viterbi/sources-v3/crf_context.c"
OUT = ROOT / "research/experiments/198-certified-viterbi/raw/helper-test.c"


def extract_function(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    raise ValueError(f"unterminated function: {signature}")


source = SRC.read_text()
reference = extract_function(source, "double crf_context_viterbi(crf_context_t *self, uint32_t *labels)")
candidate = extract_function(source, "bool crf_context_viterbi_f32_certified(crf_context_t *self, uint32_t *labels, double *score)")
scaffold = r'''#include <float.h>
#include <fenv.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma STDC FENV_ACCESS ON

typedef struct { double *values; } double_matrix_t;
typedef struct { uint32_t *values; } uint32_matrix_t;
typedef struct {
    size_t num_items, num_labels;
    double_matrix_t *state, *state_trans, *trans, *alpha_score;
    uint32_matrix_t *backward_edges;
} crf_context_t;

static inline double *alpha_score(crf_context_t *c, size_t t) { return c->alpha_score->values + t * c->num_labels; }
static inline double *state_score(crf_context_t *c, size_t t) { return c->state->values + t * c->num_labels; }
static inline double *state_trans_score_all(crf_context_t *c, size_t t) { return c->state_trans->values + t * c->num_labels * c->num_labels; }
static inline double *state_trans_score(crf_context_t *c, size_t t, size_t i) { return state_trans_score_all(c, t) + i * c->num_labels; }
static inline double *trans_score(crf_context_t *c, size_t i) { return c->trans->values + i * c->num_labels; }
static inline uint32_t *backward_edge_at(crf_context_t *c, size_t t) { return c->backward_edges->values + t * c->num_labels; }
static inline void double_array_raw_copy(double *dst, const double *src, size_t n) { memcpy(dst, src, n * sizeof(*dst)); }

'''

tests = r'''
enum { L = 20 };
typedef struct {
    crf_context_t c;
    double_matrix_t state, state_trans, trans, alpha;
    uint32_matrix_t back;
} fixture_t;

static bool fixture_new(fixture_t *f, size_t T, size_t L) {
    memset(f, 0, sizeof(*f));
    f->c.num_items = T; f->c.num_labels = L;
    f->state.values = calloc(T * L ? T * L : 1, sizeof(double));
    f->state_trans.values = calloc(T * L * L ? T * L * L : 1, sizeof(double));
    f->trans.values = calloc(L * L, sizeof(double));
    f->alpha.values = calloc(T * L ? T * L : 1, sizeof(double));
    f->back.values = calloc(T * L ? T * L : 1, sizeof(uint32_t));
    if (!f->state.values || !f->state_trans.values || !f->trans.values || !f->alpha.values || !f->back.values) return false;
    f->c.state = &f->state; f->c.state_trans = &f->state_trans; f->c.trans = &f->trans;
    f->c.alpha_score = &f->alpha; f->c.backward_edges = &f->back;
    return true;
}
static void fixture_free(fixture_t *f) { free(f->state.values); free(f->state_trans.values); free(f->trans.values); free(f->alpha.values); free(f->back.values); }
static void clear_scores(fixture_t *f) {
    size_t T=f->c.num_items, L=f->c.num_labels;
    memset(f->state.values,0,T*L*sizeof(double)); memset(f->state_trans.values,0,T*L*L*sizeof(double)); memset(f->trans.values,0,L*L*sizeof(double));
}
static void make_wide_winner(fixture_t *f) {
    size_t T=f->c.num_items, L=f->c.num_labels;
    clear_scores(f); f->state.values[0]=16.0;
    for (size_t t=1;t<T;t++) {
        double *st=f->state_trans.values+t*L*L;
        double *s=f->state.values+t*L;
        for (size_t j=0;j<L;j++) { st[j]=2.0; for(size_t i=1;i<L;i++) st[i*L+j]=-4.0; }
        s[0]=(t + 1 == T) ? 1000.0 : 20.0;
    }
}
static int compare_if_certified(const char *name, fixture_t *f, bool want_cert) {
    uint32_t exact[8192], got[8192]; double score=0;
    if (f->c.num_items > 8192) return 1;
    size_t T=f->c.num_items, L=f->c.num_labels;
    size_t sn=T*L*sizeof(double), stn=T*L*L*sizeof(double), trn=L*L*sizeof(double);
    double *state_copy=malloc(sn ? sn : 1), *st_copy=malloc(stn ? stn : 1), *tr_copy=malloc(trn ? trn : 1);
    if(!state_copy || !st_copy || !tr_copy) return 1;
    if(sn) memcpy(state_copy,f->state.values,sn); if(stn) memcpy(st_copy,f->state_trans.values,stn); memcpy(tr_copy,f->trans.values,trn);
    double exact_score=crf_context_viterbi(&f->c,exact);
    bool certified=crf_context_viterbi_f32_certified(&f->c,got,&score);
    int failed=0;
    if (want_cert && !certified) { fprintf(stderr,"%s: expected certificate, got fallback\n",name); failed=1; }
    if (certified && memcmp(exact,got,f->c.num_items*sizeof(uint32_t))!=0) { fprintf(stderr,"%s: accepted labels differ; exact score %.17g float-path score %.17g\n",name,exact_score,score); failed=1; }
    if (!certified) { uint32_t fallback[8192]; crf_context_viterbi(&f->c,fallback); if(memcmp(exact,fallback,T*sizeof(uint32_t))!=0) { fprintf(stderr,"%s: exact fallback reference differs\n",name); failed=1; } }
    bool unchanged=!((sn && memcmp(state_copy,f->state.values,sn)) || (stn && memcmp(st_copy,f->state_trans.values,stn)) || memcmp(tr_copy,f->trans.values,trn));
    if (!unchanged) { fprintf(stderr,"%s: input score matrices changed\n",name); failed=1; }
    printf("%s: certified=%d exact_labels_match=1 inputs_unchanged=%d\n",name,certified,unchanged);
    free(state_copy); free(st_copy); free(tr_copy); return failed;
}
static int expect_fallback(const char *name, fixture_t *f) { return compare_if_certified(name,f,false) ? 1 : 0; }

int main(void) {
    int bad=0; fixture_t f;
    if (!fixture_new(&f,1,L)) return 2;
    make_wide_winner(&f); bad |= compare_if_certified("one-item clear winner",&f,true); fixture_free(&f);
    if (!fixture_new(&f,4,L)) return 2;
    make_wide_winner(&f); bad |= compare_if_certified("multi-item clear winner",&f,true); fixture_free(&f);
    if (!fixture_new(&f,256,L)) return 2;
    make_wide_winner(&f); bad |= compare_if_certified("256-item clear winner",&f,true); fixture_free(&f);
    if (!fixture_new(&f,4096,L)) return 2;
    make_wide_winner(&f); bad |= compare_if_certified("long context, no hard length cutoff",&f,false); fixture_free(&f);
    if (!fixture_new(&f,2,L)) return 2;
    clear_scores(&f); bad |= expect_fallback("predecessor tie",&f); fixture_free(&f);
    if (!fixture_new(&f,1,L)) return 2;
    clear_scores(&f); bad |= expect_fallback("final-label tie",&f); fixture_free(&f);
    if (!fixture_new(&f,1,L)) return 2;
    clear_scores(&f); f.state.values[0]=1.0; f.state.values[1]=nextafter(1.0,2.0); bad |= expect_fallback("sub-f32 near tie",&f); fixture_free(&f);
    if (!fixture_new(&f,1,L)) return 2;
    clear_scores(&f); f.state.values[0]=NAN; bad |= expect_fallback("NaN operand",&f); fixture_free(&f);
    if (!fixture_new(&f,1,L)) return 2;
    clear_scores(&f); f.trans.values[0]=INFINITY; bad |= expect_fallback("infinite transition",&f); fixture_free(&f);
    if (!fixture_new(&f,1,L)) return 2;
    clear_scores(&f); f.state.values[0]=DBL_MIN; bad |= expect_fallback("operand below float normal range",&f); fixture_free(&f);
    if (!fixture_new(&f,1,L)) return 2;
    clear_scores(&f); f.state.values[0]=nextafter(0.0,1.0); bad |= expect_fallback("actual double subnormal",&f); fixture_free(&f);
    if (!fixture_new(&f,1,L)) return 2;
    clear_scores(&f); f.state.values[0]=DBL_MAX; bad |= expect_fallback("float conversion overflow",&f); fixture_free(&f);
    if (!fixture_new(&f,2,L)) return 2;
    clear_scores(&f); f.state.values[0]=3.0e38; for(size_t j=0;j<L;j++) f.state_trans.values[j]=3.0e38; bad |= expect_fallback("float intermediate overflow",&f); fixture_free(&f);
    if (!fixture_new(&f,2,L)) return 2;
    clear_scores(&f); f.state_trans.values[0]=NAN; bad |= expect_fallback("NaN state-transition operand",&f); fixture_free(&f);
    if (!fixture_new(&f,1,19)) return 2;
    clear_scores(&f); bad |= expect_fallback("unsupported label count",&f); fixture_free(&f);
    if (!fixture_new(&f,0,L)) return 2;
    bad |= expect_fallback("empty sequence",&f); fixture_free(&f);

#if defined(__aarch64__)
    if (!fixture_new(&f,1,L)) return 2;
    make_wide_winner(&f);
    if (fesetround(FE_DOWNWARD) != 0) { fprintf(stderr,"fesetround(FE_DOWNWARD) unavailable\n"); bad=1; }
    else { bad |= expect_fallback("non-RN fenv",&f); if (fesetround(FE_TONEAREST) != 0) bad=1; }
    uint64_t saved=0; __asm__ volatile("mrs %0, fpcr" : "=r"(saved));
    uint64_t masked=saved | (UINT64_C(1)<<24) | (UINT64_C(1)<<19);
    __asm__ volatile("msr fpcr, %0" :: "r"(masked));
    bad |= expect_fallback("FPCR FZ and FZ16",&f);
    __asm__ volatile("msr fpcr, %0" :: "r"(saved));
    fixture_free(&f);
#endif
    if (bad) return 1;
    puts("all model-free certifier helper checks passed"); return 0;
}
'''

OUT.write_text(scaffold + reference + "\n\n" + candidate + "\n\n" + tests)
print(f"generated {OUT}; source={hashlib.sha256(SRC.read_bytes()).hexdigest()}; harness={hashlib.sha256(OUT.read_bytes()).hexdigest()}")
