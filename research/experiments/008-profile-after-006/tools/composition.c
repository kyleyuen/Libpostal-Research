#include <stdio.h>
#include <stdint.h>
#include "libpostal.h"
#include "address_parser.h"
#include "crf.h"
#include "trie.h"
#include "sparse_matrix.h"
#include "address_dictionary.h"
static double mb(size_t b) { return b / 1048576.0; }
static size_t trie_b(trie_t *t) { return t ? t->nodes->n * 8 + t->tail->n + t->data->n * 8 : 0; }
static size_t trie_cap(trie_t *t) { return t ? t->nodes->m * 8 + t->tail->m + t->data->m * 8 : 0; }
static size_t sm_b(sparse_matrix_t *m) { return m ? (m->indptr->n + m->indices->n) * 4 + m->data->n * 8 : 0; }
int main(void) {
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    address_parser_t *p = get_address_parser(); crf_t *c = p->model.crf;
    printf("crf state_features trie      %9.1f MB (capacity %.1f MB)  keys=%u\n", mb(trie_b(c->state_features)), mb(trie_cap(c->state_features)), c->state_features->num_keys);
    printf("crf state_trans_features trie%9.1f MB (capacity %.1f MB)\n", mb(trie_b(c->state_trans_features)), mb(trie_cap(c->state_trans_features)));
    printf("crf weights (CSR)            %9.1f MB  rows=%zu nnz=%zu  (indptr %.1f, indices %.1f, data %.1f MB)\n", mb(sm_b(c->weights)), (size_t)c->weights->m, (size_t)c->weights->data->n,
           mb(c->weights->indptr->n * 4), mb(c->weights->indices->n * 4), mb(c->weights->data->n * 8));
    printf("crf state_trans weights      %9.1f MB  nnz=%zu\n", mb(sm_b(c->state_trans_weights)), (size_t)c->state_trans_weights->data->n);
    printf("parser vocab trie            %9.1f MB  keys=%u\n", mb(trie_b(p->vocab)), p->vocab->num_keys);
    printf("parser phrases trie          %9.1f MB  keys=%u\n", mb(trie_b(p->phrases)), p->phrases->num_keys);
    printf("parser postal_codes trie     %9.1f MB  keys=%u\n", mb(trie_b(p->postal_codes)), p->postal_codes->num_keys);
    printf("phrase_types array           %9.1f MB  n=%zu\n", mb(p->phrase_types->n * sizeof(address_parser_types_t)), (size_t)p->phrase_types->n);
    printf("postal_code_contexts graph   n=%zu edges (see vmmap for size)\n", (size_t)0);
    return 0;
}
