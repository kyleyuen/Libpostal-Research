#include <assert.h>
#include <stdio.h>
#include "crf_context.h"

static void fill(double_matrix_t *m, double value) {
    for (size_t i = 0; i < m->m * m->n; i++) m->values[i] = value;
}
static void all_zero(double_matrix_t *m) {
    for (size_t i = 0; i < m->m * m->n; i++) assert(m->values[i] == 0.0);
}
static void all_value(double_matrix_t *m, double value) {
    for (size_t i = 0; i < m->m * m->n; i++) assert(m->values[i] == value);
}
int main(void) {
    crf_context_t *c = crf_context_new(CRF_CONTEXT_VITERBI, 3, 2);
    assert(c);
    fill(c->state, 5.0); fill(c->state_trans, 7.0);
    assert(crf_context_set_num_items(c, 2)); /* Existing API still clears scores. */
    all_zero(c->state); all_zero(c->state_trans);

    fill(c->state, 11.0); fill(c->state_trans, 13.0);
    fill(c->trans, 17.0); fill(c->alpha_score, 19.0); fill(c->beta_score, 23.0);
    assert(crf_context_set_num_items_for_reset(c, 2));
    all_value(c->state, 11.0); all_value(c->state_trans, 13.0);
    all_zero(c->alpha_score); all_zero(c->beta_score);
    for (size_t i = 0; i < c->backward_edges->m * c->backward_edges->n; i++) assert(c->backward_edges->values[i] == 0);
    crf_context_reset(c, CRF_CONTEXT_RESET_ALL);
    all_zero(c->state); all_zero(c->state_trans); all_zero(c->trans);

    fill(c->state, 29.0); fill(c->state_trans, 31.0);
    assert(crf_context_set_num_items_for_reset(c, 3)); /* Resize path also requires the next reset. */
    crf_context_reset(c, CRF_CONTEXT_RESET_ALL);
    all_zero(c->state); all_zero(c->state_trans); all_zero(c->trans);
    crf_context_destroy(c);

    c = crf_context_new(CRF_CONTEXT_MARGINALS, 3, 2);
    assert(c);
    fill(c->state, 37.0); fill(c->state_trans, 41.0);
    assert(crf_context_set_num_items_for_reset(c, 2));
    crf_context_reset(c, CRF_CONTEXT_RESET_ALL);
    all_zero(c->state); all_zero(c->state_trans); all_zero(c->trans);
    crf_context_destroy(c);
    puts("context-reset selfcheck passed: legacy zero-on-set and caller-reset paths");
    return 0;
}
