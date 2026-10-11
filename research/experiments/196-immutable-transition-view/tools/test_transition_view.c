#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "crf_context.h"

static crf_context_t *make_context(void) {
    crf_context_t *c = crf_context_new(CRF_CONTEXT_VITERBI, 3, 4);
    assert(c);
    for (size_t i = 0; i < 12; i++) c->state->values[i] = (double)((int)(i % 7) - 3) / 4.0;
    for (size_t i = 0; i < 9; i++) c->trans->values[i] = (double)((int)(i % 5) - 2) / 8.0;
    for (size_t i = 0; i < 36; i++) c->state_trans->values[i] = (double)((int)(i % 9) - 4) / 16.0;
    return c;
}
int main(void) {
    crf_context_t *c = make_context();
    uint32_t reference[4] = {0}, candidate[4] = {0};
    double a = crf_context_viterbi(c, reference);
    double b = crf_context_viterbi_with_trans(c, candidate, c->trans->values);
    assert(a == b && memcmp(reference, candidate, sizeof(reference)) == 0);
    const double alternate[9] = { 1.0, -2.0, 0.5, 0.25, 1.25, -0.75, 2.0, 0.125, -1.0 };
    crf_context_t *d = make_context();
    memcpy(d->trans->values, alternate, sizeof(alternate));
    uint32_t owned[4] = {0}, viewed[4] = {0};
    double cscore = crf_context_viterbi(d, owned);
    double dscore = crf_context_viterbi_with_trans(d, viewed, alternate);
    assert(cscore == dscore && memcmp(owned, viewed, sizeof(owned)) == 0);
    crf_context_reset(d, CRF_CONTEXT_RESET_ALL);
    for (size_t i = 0; i < 9; i++) assert(d->trans->values[i] == 0.0);
    crf_context_destroy(d); crf_context_destroy(c);
    puts("transition-view selfcheck passed: old/new exact path+score, explicit alternate matrix, reset");
    return 0;
}
