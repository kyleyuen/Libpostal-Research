#include "crf.h"
#include "log/log.h"
#ifdef EXP_FEATURE_DICT
#include "feature_dict.h"
#endif

#define CRF_SIGNATURE 0xCFCFCFCF


static inline bool crf_get_feature_id(crf_t *self, char *feature, uint32_t *feature_id) {
#ifdef EXP_FEATURE_DICT
    if (self->state_dict != NULL) return fd_lookup((feature_dict_t *)self->state_dict, feature, strlen(feature), feature_id);
#endif
    return trie_get_data(self->state_features, feature, feature_id);
}

static inline bool crf_get_state_trans_feature_id(crf_t *self, char *feature, uint32_t *feature_id) {
#ifdef EXP_FEATURE_DICT
    if (self->trans_dict != NULL) return fd_lookup((feature_dict_t *)self->trans_dict, feature, strlen(feature), feature_id);
#endif
    return trie_get_data(self->state_trans_features, feature, feature_id);
}

// Feature ids for one token are looked up as a batch (independent trie walks) before any weights are accumulated.
// Reused across calls like the parser context; the CRF is not used from multiple threads.
#define CRF_NO_FEATURE 0xFFFFFFFFu

static uint32_t *crf_feature_ids = NULL;
static size_t crf_feature_ids_capacity = 0;

static inline uint32_t *crf_feature_ids_reserve(size_t n) {
    if (n > crf_feature_ids_capacity || crf_feature_ids == NULL) {   // == NULL: an empty first batch must still allocate
        size_t capacity = crf_feature_ids_capacity ? crf_feature_ids_capacity : 256;
        while (capacity < n) capacity *= 2;
        uint32_t *ids = realloc(crf_feature_ids, capacity * sizeof(uint32_t));
        if (ids == NULL) return NULL;
        crf_feature_ids = ids;
        crf_feature_ids_capacity = capacity;
    }
    return crf_feature_ids;
}


#ifdef EXP_DEDUP_BATCH
bool trie_get_data_len(trie_t *self, char *key, size_t len, uint32_t *data);  // added to trie.h/trie.c by this patch

// Per-address scratch. All tokens' feature strings are copied here, each distinct string is looked up once, then weights
// are accumulated per token in the original order. Nothing survives between addresses except buffer capacity.
typedef struct crf_key_batch {
    char *arena;
    size_t arena_n, arena_m;
    uint32_t *off, *len, *id;
    uint64_t *hash;
    size_t n, m;
    uint32_t *tok_begin;
    size_t tok_m;
    uint32_t *table;
    size_t table_m;
} crf_key_batch_t;

static crf_key_batch_t crf_state_batch, crf_trans_batch;

static inline uint64_t crf_key_hash(const char *s, size_t len) {
    uint64_t h = 0x9E3779B97F4A7C15ULL ^ len;
    while (len >= 8) {
        uint64_t w;
        memcpy(&w, s, 8);
        h = (h ^ w) * 0xFF51AFD7ED558CCDULL;
        h ^= h >> 32;
        s += 8;
        len -= 8;
    }
    uint64_t w = 0;
    if (len) memcpy(&w, s, len);
    h = (h ^ w) * 0xC4CEB9FE1A85EC53ULL;
    return h ^ (h >> 29);
}

static bool crf_key_batch_begin(crf_key_batch_t *b, size_t num_tokens) {
    b->n = 0;
    b->arena_n = 0;
    if (num_tokens + 1 > b->tok_m) {
        size_t m = b->tok_m ? b->tok_m : 32;
        while (m < num_tokens + 1) m *= 2;
        uint32_t *p = realloc(b->tok_begin, m * sizeof(uint32_t));
        if (p == NULL) return false;
        b->tok_begin = p;
        b->tok_m = m;
    }
    return true;
}

static bool crf_key_batch_add_token(crf_key_batch_t *b, size_t t, cstring_array *keys) {
    b->tok_begin[t] = (uint32_t)b->n;
    size_t count = keys->indices->n;
    if (b->n + count > b->m) {
        size_t m = b->m ? b->m : 128;
        while (m < b->n + count) m *= 2;
        uint32_t *off = realloc(b->off, m * sizeof(uint32_t));
        if (off == NULL) return false;
        b->off = off;
        uint32_t *len = realloc(b->len, m * sizeof(uint32_t));
        if (len == NULL) return false;
        b->len = len;
        uint32_t *id = realloc(b->id, m * sizeof(uint32_t));
        if (id == NULL) return false;
        b->id = id;
        uint64_t *hash = realloc(b->hash, m * sizeof(uint64_t));
        if (hash == NULL) return false;
        b->hash = hash;
        b->m = m;
    }
    size_t need = b->arena_n + keys->str->n + count;
    if (need > b->arena_m) {
        size_t m = b->arena_m ? b->arena_m : 4096;
        while (m < need) m *= 2;
        char *arena = realloc(b->arena, m);
        if (arena == NULL) return false;
        b->arena = arena;
        b->arena_m = m;
    }
    for (size_t i = 0; i < count; i++) {
        char *s = keys->str->a + keys->indices->a[i];
        size_t len = strlen(s);
        memcpy(b->arena + b->arena_n, s, len + 1);
        b->off[b->n] = (uint32_t)b->arena_n;
        b->len[b->n] = (uint32_t)len;
        b->hash[b->n] = crf_key_hash(s, len);
        b->arena_n += len + 1;
        b->n++;
    }
    return true;
}

static bool crf_key_batch_lookup(crf_key_batch_t *b, trie_t *trie) {
    size_t n = b->n;
    size_t table_size = 64;
    while (table_size < 2 * n) table_size <<= 1;
    if (table_size > b->table_m) {
        uint32_t *p = realloc(b->table, table_size * sizeof(uint32_t));
        if (p == NULL) return false;
        b->table = p;
        b->table_m = table_size;
    }
    memset(b->table, 0, table_size * sizeof(uint32_t));
    size_t mask = table_size - 1;

    for (size_t k = 0; k < n; k++) {
        size_t slot = b->hash[k] & mask;
        bool duplicate = false;
        while (b->table[slot]) {
            size_t j = b->table[slot] - 1;
            if (b->hash[j] == b->hash[k] && b->len[j] == b->len[k] &&
                memcmp(b->arena + b->off[j], b->arena + b->off[k], b->len[k]) == 0) {
                b->id[k] = b->id[j];
                duplicate = true;
                break;
            }
            slot = (slot + 1) & mask;
        }
        if (!duplicate) {
            uint32_t feature_id;
            b->id[k] = trie_get_data_len(trie, b->arena + b->off[k], b->len[k], &feature_id) ? feature_id : CRF_NO_FEATURE;
            b->table[slot] = (uint32_t)(k + 1);
        }
    }
    return true;
}


#ifdef EXP_FEATURE_DICT
// Looks up every key of the batch in the dictionary: pass 1 splits/hashes a window of keys and prefetches their slots, pass 2 probes.
static void crf_key_batch_lookup_dict(crf_key_batch_t *b, const feature_dict_t *d) {
    enum { WIN = 64 };
    uint8_t nb[WIN]; const char *rest[WIN]; size_t rl[WIN]; uint64_t hh[WIN]; bool ok[WIN];
    for (size_t base = 0; base < b->n; base += WIN) {
        size_t m = b->n - base < WIN ? b->n - base : WIN;
        for (size_t j = 0; j < m; j++) {
            char *key = (char *)b->arena + b->off[base + j];
            ok[j] = fd_split(d, key, b->len[base + j], &nb[j], &rest[j], &rl[j]);
            if (ok[j]) {
                hh[j] = fd_hash_key(nb[j], rest[j], rl[j]);
                __builtin_prefetch(&d->slots[fd_index(d, hh[j])]);
            }
        }
        for (size_t j = 0; j < m; j++) {
            uint32_t feature_id;
            b->id[base + j] = ok[j] && fd_lookup_hashed(d, nb[j], rest[j], rl[j], hh[j], &feature_id) ? feature_id : CRF_NO_FEATURE;
        }
    }
}

// Converts a freshly read trie into an exact dictionary and frees the trie. Any doubt (fd_build returns NULL) keeps the trie.
static void crf_convert_trie(trie_t **trie, void **dict_out) {
    if (getenv("CRF_DICT_DISABLE") != NULL) return;     // experiment switch (differential testing / A-B in one binary)
    feature_dict_t *d = fd_build(*trie);
    if (d == NULL) return;
    if (getenv("CRF_DICT_VERBOSE") != NULL) {
        size_t old_bytes = (*trie)->nodes->n * sizeof(trie_node_t) + (*trie)->tail->n + (*trie)->data->n * sizeof(trie_data_node_t);
        fprintf(stderr, "crf dictionary: keys=%llu names=%u build=%.2fs dict=%.1f MB replaces trie=%.1f MB\n", (unsigned long long)d->num_keys, d->num_names,
                d->build_seconds, (d->size * 8 + d->arena_used) / 1048576.0, old_bytes / 1048576.0);
    }
    trie_destroy(*trie);
    *trie = NULL;
    *dict_out = d;
}
#endif

static bool crf_tagger_score_batched(crf_t *self, void *tagger, void *tagger_context, cstring_array *features, cstring_array *prev_tag_features, tagger_feature_function feature_function, tokenized_string_t *tokenized, crf_context_t *crf_context, size_t num_tokens) {
    if (!double_matrix_copy(self->trans_weights, crf_context->trans)) {
        return false;
    }

    crf_key_batch_t *sb = &crf_state_batch, *pb = &crf_trans_batch;
    if (!crf_key_batch_begin(sb, num_tokens) || !crf_key_batch_begin(pb, num_tokens)) return false;

    for (size_t t = 0; t < num_tokens; t++) {
        cstring_array_clear(features);
        cstring_array_clear(prev_tag_features);

        if (!feature_function(tagger, tagger_context, tokenized, (uint32_t)t)) {
            log_error("Could not add address parser features\n");
            return false;
        }

        if (!crf_key_batch_add_token(sb, t, features) || !crf_key_batch_add_token(pb, t, prev_tag_features)) return false;
    }
    sb->tok_begin[num_tokens] = (uint32_t)sb->n;
    pb->tok_begin[num_tokens] = (uint32_t)pb->n;

#ifdef EXP_FEATURE_DICT
    if (self->state_dict != NULL && self->trans_dict != NULL) {
        crf_key_batch_lookup_dict(sb, (feature_dict_t *)self->state_dict);
        crf_key_batch_lookup_dict(pb, (feature_dict_t *)self->trans_dict);
    } else
#endif
    if (!crf_key_batch_lookup(sb, self->state_features) || !crf_key_batch_lookup(pb, self->state_trans_features)) return false;

    for (size_t t = 0; t < num_tokens; t++) {
        double *state_scores = state_score(crf_context, (uint32_t)t);
        uint32_t *indptr = self->weights->indptr->a;
        uint32_t *indices = self->weights->indices->a;
        double *data = self->weights->data->a;

        for (uint32_t k = sb->tok_begin[t]; k < sb->tok_begin[t + 1]; k++) {
            uint32_t feature_id = sb->id[k];
            if (feature_id == CRF_NO_FEATURE) continue;
            for (int col = indptr[feature_id]; col < indptr[feature_id + 1]; col++) {
                state_scores[indices[col]] += data[col];
            }
        }

        double *state_trans_scores = state_trans_score_all(crf_context, (uint32_t)t);
        indptr = self->state_trans_weights->indptr->a;
        indices = self->state_trans_weights->indices->a;
        data = self->state_trans_weights->data->a;

        for (uint32_t k = pb->tok_begin[t]; k < pb->tok_begin[t + 1]; k++) {
            uint32_t feature_id = pb->id[k];
            if (feature_id == CRF_NO_FEATURE) continue;
            for (int col = indptr[feature_id]; col < indptr[feature_id + 1]; col++) {
                // Note: here there are L * L classes
                state_trans_scores[indices[col]] += data[col];
            }
        }
    }
    return true;
}
#endif


#ifdef EXP_DIRECT_KEYS
// Experiment 009: no per-token key copy. address_parser_features appends every token's keys to the same cstring_arrays (crf_features_accumulate),
// the keys are looked up straight out of those arrays (length from the offsets, no strlen), and the weight rows of hits are prefetched.
int crf_features_accumulate = 0;

// Looks up all keys of `a` in the dictionary d; ids[i] = feature id or CRF_NO_FEATURE. Prefetches the weight-row start of every hit.
static void crf_dict_lookup_array(const feature_dict_t *d, cstring_array *a, uint32_t *ids, const uint32_t *indptr) {
    enum { WIN = 64 };
    uint8_t nb[WIN]; const char *rest[WIN]; size_t rl[WIN]; uint64_t hh[WIN]; bool ok[WIN];
    size_t n = a->indices->n;
    char *chars = a->str->a; uint32_t *off = a->indices->a; size_t chars_n = a->str->n;
    for (size_t base = 0; base < n; base += WIN) {
        size_t m = n - base < WIN ? n - base : WIN;
        for (size_t j = 0; j < m; j++) {
            size_t i = base + j;
            size_t len = (i + 1 < n ? off[i + 1] : chars_n) - off[i] - 1;
#ifdef EXP_CHECK_LEN
            if (strlen(chars + off[i]) != len) { fprintf(stderr, "EXP_CHECK_LEN mismatch: strlen=%zu computed=%zu\n", strlen(chars + off[i]), len); abort(); }
#endif
            ok[j] = fd_split(d, chars + off[i], len, &nb[j], &rest[j], &rl[j]);
            if (ok[j]) {
                hh[j] = fd_hash_key(nb[j], rest[j], rl[j]);
                __builtin_prefetch(&d->slots[fd_index(d, hh[j])]);
            }
        }
#ifdef EXP_ARENA_PREFETCH
        // pass 1b: find each key's first candidate slot (empty, or tag match) and prefetch that key's arena entry before comparing
        uint64_t cand[WIN];
        for (size_t j = 0; j < m; j++) {
            cand[j] = 0;
            if (!ok[j]) continue;
            uint8_t tag = (uint8_t)hh[j];
            uint64_t i = fd_index(d, hh[j]);
            for (;; i = fd_next(d, i)) {
                uint64_t sl = d->slots[i];
                if (!sl || (uint8_t)(sl >> 56) == tag) { if (sl) __builtin_prefetch(d->arena + (uint32_t)sl); break; }
            }
            cand[j] = i;
        }
#endif
        for (size_t j = 0; j < m; j++) {
            uint32_t feature_id;
#ifdef EXP_ARENA_PREFETCH
            if (ok[j] && fd_lookup_hashed_from(d, nb[j], rest[j], rl[j], hh[j], cand[j], &feature_id)) {
#else
            if (ok[j] && fd_lookup_hashed(d, nb[j], rest[j], rl[j], hh[j], &feature_id)) {
#endif
                ids[base + j] = feature_id;
#ifdef EXP_WEIGHT_PREFETCH
                __builtin_prefetch(&indptr[feature_id]);
#endif
            } else {
                ids[base + j] = CRF_NO_FEATURE;
            }
        }
    }
}

static inline void crf_prefetch_rows(const uint32_t *ids, size_t n, const uint32_t *indptr, const uint32_t *indices, const double *data) {
#ifdef EXP_WEIGHT_PREFETCH
    for (size_t k = 0; k < n; k++) {
        uint32_t id = ids[k];
        if (id == CRF_NO_FEATURE) continue;
        uint32_t start = indptr[id];
        __builtin_prefetch(&indices[start]);
        __builtin_prefetch(&data[start]);
    }
#endif
}

static uint32_t *crf_state_ids = NULL, *crf_trans_ids = NULL, *crf_state_tok = NULL, *crf_trans_tok = NULL;
static size_t crf_state_ids_m = 0, crf_trans_ids_m = 0, crf_tok_m = 0;

static bool crf_tagger_score_direct(crf_t *self, void *tagger, void *tagger_context, cstring_array *features, cstring_array *prev_tag_features, tagger_feature_function feature_function, tokenized_string_t *tokenized, crf_context_t *crf_context, size_t num_tokens) {
    if (!double_matrix_copy(self->trans_weights, crf_context->trans)) return false;
    if (num_tokens + 1 > crf_tok_m) {
        size_t m = crf_tok_m ? crf_tok_m : 32;
        while (m < num_tokens + 1) m *= 2;
        uint32_t *a = realloc(crf_state_tok, m * sizeof(uint32_t)); if (a == NULL) return false; crf_state_tok = a;
        uint32_t *b = realloc(crf_trans_tok, m * sizeof(uint32_t)); if (b == NULL) return false; crf_trans_tok = b;
        crf_tok_m = m;
    }

    cstring_array_clear(features);
    cstring_array_clear(prev_tag_features);
    crf_features_accumulate = 1;
    bool ok = true;
    for (size_t t = 0; t < num_tokens; t++) {
        crf_state_tok[t] = (uint32_t)features->indices->n;
        crf_trans_tok[t] = (uint32_t)prev_tag_features->indices->n;
        if (!feature_function(tagger, tagger_context, tokenized, (uint32_t)t)) {
            log_error("Could not add address parser features\n");
            ok = false;
            break;
        }
    }
    crf_features_accumulate = 0;
    if (!ok) return false;
    crf_state_tok[num_tokens] = (uint32_t)features->indices->n;
    crf_trans_tok[num_tokens] = (uint32_t)prev_tag_features->indices->n;

    size_t ns = features->indices->n, np = prev_tag_features->indices->n;
    if (ns > crf_state_ids_m) { size_t m = crf_state_ids_m ? crf_state_ids_m : 256; while (m < ns) m *= 2; uint32_t *p = realloc(crf_state_ids, m * sizeof(uint32_t)); if (p == NULL) return false; crf_state_ids = p; crf_state_ids_m = m; }
    if (np > crf_trans_ids_m) { size_t m = crf_trans_ids_m ? crf_trans_ids_m : 64; while (m < np) m *= 2; uint32_t *p = realloc(crf_trans_ids, m * sizeof(uint32_t)); if (p == NULL) return false; crf_trans_ids = p; crf_trans_ids_m = m; }

    uint32_t *indptr = self->weights->indptr->a, *indices = self->weights->indices->a;
    double *data = self->weights->data->a;
    uint32_t *tindptr = self->state_trans_weights->indptr->a, *tindices = self->state_trans_weights->indices->a;
    double *tdata = self->state_trans_weights->data->a;

    crf_dict_lookup_array((feature_dict_t *)self->state_dict, features, crf_state_ids, indptr);
    crf_dict_lookup_array((feature_dict_t *)self->trans_dict, prev_tag_features, crf_trans_ids, tindptr);
    crf_prefetch_rows(crf_state_ids, ns, indptr, indices, data);
    crf_prefetch_rows(crf_trans_ids, np, tindptr, tindices, tdata);

    for (size_t t = 0; t < num_tokens; t++) {
        double *state_scores = state_score(crf_context, (uint32_t)t);
        for (uint32_t k = crf_state_tok[t]; k < crf_state_tok[t + 1]; k++) {
            uint32_t feature_id = crf_state_ids[k];
            if (feature_id == CRF_NO_FEATURE) continue;
            for (int col = indptr[feature_id]; col < indptr[feature_id + 1]; col++) state_scores[indices[col]] += data[col];
        }
        double *state_trans_scores = state_trans_score_all(crf_context, (uint32_t)t);
        for (uint32_t k = crf_trans_tok[t]; k < crf_trans_tok[t + 1]; k++) {
            uint32_t feature_id = crf_trans_ids[k];
            if (feature_id == CRF_NO_FEATURE) continue;
            for (int col = tindptr[feature_id]; col < tindptr[feature_id + 1]; col++) state_trans_scores[tindices[col]] += tdata[col];   // L * L classes
        }
    }
    return true;
}
#endif


#ifdef EXP_LEAN_CONTEXT
// Viterbi-only replacement for crf_context_set_num_items. The library version resizes and zeroes 8 matrices per address (alpha, beta, state, state_trans,
// four marginal arrays, backward edges) and, because double_matrix_resize compares against the *current* dimensions, re-allocates whenever an address is longer
// than the previous one. Viterbi decoding reads only state, state_trans, trans (written elsewhere), alpha_score (every cell it reads is written first) and
// backward_edges (kept zeroed, as before). Capacity is tracked here so a matrix is only reallocated when it really has to grow.
static size_t crf_lean_cap_items = 0;

static bool crf_context_set_num_items_lean(crf_context_t *self, size_t T) {
    const size_t L = self->num_labels;
    if (T > crf_lean_cap_items) {
        size_t cap = crf_lean_cap_items ? crf_lean_cap_items : 32;
        while (cap < T) cap *= 2;
        if (!double_matrix_resize(self->state, cap, L) || !double_matrix_resize(self->state_trans, cap, L * L) ||
            !double_matrix_resize(self->alpha_score, cap, L) || !uint32_matrix_resize(self->backward_edges, cap, L)) return false;
        crf_lean_cap_items = cap;
    }
    self->state->m = T; self->state->n = L;
    self->state_trans->m = T; self->state_trans->n = L * L;
    self->alpha_score->m = T; self->alpha_score->n = L;
    self->backward_edges->m = T; self->backward_edges->n = L;
    memset(self->state->values, 0, T * L * sizeof(double));
    memset(self->state_trans->values, 0, T * L * L * sizeof(double));
    memset(self->backward_edges->values, 0, T * L * sizeof(uint32_t));
    self->num_items = T;
    return true;
}
#endif

bool crf_tagger_score(crf_t *self, void *tagger, void *tagger_context, cstring_array *features, cstring_array *prev_tag_features, tagger_feature_function feature_function, tokenized_string_t *tokenized, bool print_features) {
    if (self == NULL || feature_function == NULL || tokenized == NULL ) {
        return false;
    }
    size_t num_tokens = tokenized->tokens->n;

    crf_context_t *crf_context = self->context;
#ifdef EXP_LEAN_CONTEXT
    if (!crf_context_set_num_items_lean(crf_context, num_tokens)) return false;
#else
    crf_context_set_num_items(crf_context, num_tokens);
#endif
#ifndef EXP_NO_RESET
    crf_context_reset(crf_context, CRF_CONTEXT_RESET_ALL);
#endif
    // EXP_NO_RESET: crf_context_set_num_items already zeroed state and state_trans, trans is overwritten by the copy below,
    // and the marginal arrays reset() would also clear are never read by Viterbi decoding.

#ifdef EXP_DIRECT_KEYS
    if (!print_features && self->state_dict != NULL && self->trans_dict != NULL) {
        return crf_tagger_score_direct(self, tagger, tagger_context, features, prev_tag_features, feature_function, tokenized, crf_context, num_tokens);
    }
#endif
#ifdef EXP_DEDUP_BATCH
    if (!print_features) {
        return crf_tagger_score_batched(self, tagger, tagger_context, features, prev_tag_features, feature_function, tokenized, crf_context, num_tokens);
    }
#endif

    if (!double_matrix_copy(self->trans_weights, crf_context->trans)) {
        return false;
    }

    for (uint32_t t = 0; t < num_tokens; t++) {
        cstring_array_clear(features);
        cstring_array_clear(prev_tag_features);

        if (!feature_function(tagger, tagger_context, tokenized, t)) {
            log_error("Could not add address parser features\n");
            return false;
        }

        uint32_t fidx;
        char *feature;

        if (print_features) {
            printf("{ ");
            size_t num_features = cstring_array_num_strings(features);
            cstring_array_foreach(features, fidx, feature, {
                printf("%s", feature);
                if (fidx < num_features - 1) printf(", ");
            })
            size_t num_prev_tag_features = cstring_array_num_strings(prev_tag_features);
            if (num_prev_tag_features > 0) {
                printf(", ");
            }
            cstring_array_foreach(prev_tag_features, fidx, feature, {
                printf("prev tag+%s", feature);
                if (fidx < num_prev_tag_features - 1) printf(", ");
            })
            printf(" }\n");
        }

        uint32_t feature_id;

        double *state_scores = state_score(crf_context, t);

        uint32_t *indptr = self->weights->indptr->a;
        uint32_t *indices = self->weights->indices->a;
        double *data = self->weights->data->a;

        uint32_t *feature_ids = crf_feature_ids_reserve(features->indices->n);
        if (feature_ids == NULL) return false;

        cstring_array_foreach(features, fidx, feature, {
            feature_ids[fidx] = crf_get_feature_id(self, feature, &feature_id) ? feature_id : CRF_NO_FEATURE;
        })

        for (fidx = 0; fidx < features->indices->n; fidx++) {
            feature_id = feature_ids[fidx];
            if (feature_id == CRF_NO_FEATURE) {
                continue;
            }

            for (int col = indptr[feature_id]; col < indptr[feature_id + 1]; col++) {
                uint32_t class_id = indices[col];
                state_scores[class_id] += data[col];
            }
        }

        double *state_trans_scores = state_trans_score_all(crf_context, t);

        indptr = self->state_trans_weights->indptr->a;
        indices = self->state_trans_weights->indices->a;
        data = self->state_trans_weights->data->a;

        feature_ids = crf_feature_ids_reserve(prev_tag_features->indices->n);
        if (feature_ids == NULL) return false;

        cstring_array_foreach(prev_tag_features, fidx, feature, {
            feature_ids[fidx] = crf_get_state_trans_feature_id(self, feature, &feature_id) ? feature_id : CRF_NO_FEATURE;
        })

        for (fidx = 0; fidx < prev_tag_features->indices->n; fidx++) {
            feature_id = feature_ids[fidx];
            if (feature_id == CRF_NO_FEATURE) {
                continue;
            }

            for (int col = indptr[feature_id]; col < indptr[feature_id + 1]; col++) {
                // Note: here there are L * L classes
                uint32_t class_id = indices[col];
                state_trans_scores[class_id] += data[col];
            }
        }

    }
    return true;
}

bool crf_tagger_score_viterbi(crf_t *self, void *tagger, void *tagger_context, cstring_array *features, cstring_array *prev_tag_features, tagger_feature_function feature_function, tokenized_string_t *tokenized, double *score, bool print_features) {
    if (!crf_tagger_score(self, tagger, tagger_context, features, prev_tag_features, feature_function, tokenized, print_features)) {
        return false;
    }

    size_t num_tokens = tokenized->tokens->n;

    uint32_array_resize_fixed(self->viterbi, num_tokens);
    double viterbi_score = crf_context_viterbi(self->context, self->viterbi->a);

    *score = viterbi_score;

    return true;
}


bool crf_tagger_predict(crf_t *self, void *tagger, void *context, cstring_array *features, cstring_array *prev_tag_features, cstring_array *labels, tagger_feature_function feature_function, tokenized_string_t *tokenized, bool print_features) {
    double score;

    if (labels == NULL) return false;
    if (!crf_tagger_score_viterbi(self, tagger, context, features, prev_tag_features, feature_function, tokenized, &score, print_features)) {
        return false;
    }

    uint32_t *viterbi = self->viterbi->a;

    for (size_t i = 0; i < self->viterbi->n; i++) {
        char *predicted = cstring_array_get_string(self->classes, viterbi[i]);
        cstring_array_add_string(labels, predicted);
    }

    return true;
}


bool crf_write(crf_t *self, FILE *f) {
    if (self == NULL || f == NULL || self->weights == NULL || self->classes == NULL ||
        self->state_features == NULL || self->state_trans_features == NULL) {
        log_info("something was NULL\n");
        return false;
    }

    if (!file_write_uint32(f, CRF_SIGNATURE) ||
        !file_write_uint32(f, self->num_classes)) {
        log_info("error writing header\n");
        return false;
    }

    uint64_t classes_str_len = (uint64_t) cstring_array_used(self->classes);
    if (!file_write_uint64(f, classes_str_len)) {
        log_info("error writing classes_str_len\n");
        return false;
    }

    if (!file_write_chars(f, self->classes->str->a, classes_str_len)) {
        log_info("error writing chars\n");
        return false;
    }

    if (!trie_write(self->state_features, f)) {
        log_info("error state_features\n");
        return false;
    }

    if (!sparse_matrix_write(self->weights, f)) {
        log_info("error weights\n");
        return false;
    }

    if (!trie_write(self->state_trans_features, f)) {
        log_info("error state_trans_features\n");
        return false;
    }

    if (!sparse_matrix_write(self->state_trans_weights, f)) {
        log_info("error state_trans_weights\n");
        return false;
    }

    if (!double_matrix_write(self->trans_weights, f)) {
        log_info("error trans_weights\n");
        return false;
    }

    return true;
}


bool crf_save(crf_t *self, char *filename) {
    if (self == NULL || filename == NULL) {
        log_info("crf or filename was NULL\n");
        return false;
    }
    FILE *f = fopen(filename, "wb");
    if (f == NULL) return false;
    bool ret_val = crf_write(self, f);
    fclose(f);
    return ret_val;
}


crf_t *crf_read(FILE *f) {
    if (f == NULL) return NULL;

    uint32_t signature;

    if (!file_read_uint32(f, &signature) || signature != CRF_SIGNATURE) {
        return NULL;
    }

    crf_t *crf = calloc(1, sizeof(crf_t));
    if (crf == NULL) return NULL;

    if (!file_read_uint32(f, &crf->num_classes) ||
        crf->num_classes == 0) {
        free(crf);
        return NULL;
    }

    uint64_t classes_str_len;

    if (!file_read_uint64(f, &classes_str_len)) {
        goto exit_crf_created;
    }

    char_array *array = char_array_new_size(classes_str_len);

    if (array == NULL) {
        goto exit_crf_created;
    }

    if (!file_read_chars(f, array->a, classes_str_len)) {
        char_array_destroy(array);
        goto exit_crf_created;
    }

    array->n = classes_str_len;

    crf->classes = cstring_array_from_char_array(array);
    if (crf->classes == NULL) {
        goto exit_crf_created;
    }

    crf->state_features = trie_read(f);
    if (crf->state_features == NULL) {
        goto exit_crf_created;
    }
#ifdef EXP_FEATURE_DICT
    crf_convert_trie(&crf->state_features, &crf->state_dict);
#endif

    crf->weights = sparse_matrix_read(f);
    if (crf->weights == NULL) {
        goto exit_crf_created;
    }

    crf->state_trans_features = trie_read(f);
    if (crf->state_trans_features == NULL) {
        goto exit_crf_created;
    }
#ifdef EXP_FEATURE_DICT
    crf_convert_trie(&crf->state_trans_features, &crf->trans_dict);
#endif

    crf->state_trans_weights = sparse_matrix_read(f);
    if (crf->state_trans_weights == NULL) {
        goto exit_crf_created;
    }

    crf->trans_weights = double_matrix_read(f);
    if (crf->trans_weights == NULL) {
        goto exit_crf_created;
    }

    crf->viterbi = uint32_array_new();
    if (crf->viterbi == NULL) {
        goto exit_crf_created;
    }

    crf->context = crf_context_new(CRF_CONTEXT_VITERBI | CRF_CONTEXT_MARGINALS, crf->num_classes, CRF_CONTEXT_DEFAULT_NUM_ITEMS);
    if (crf->context == NULL) {
        goto exit_crf_created;
    }

    return crf;

exit_crf_created:
    crf_destroy(crf);
    return NULL;
}

crf_t *crf_load(char *filename) {
    if (filename == NULL) return NULL;
    FILE *f = fopen(filename, "rb");
    if (f == NULL) return NULL;
    crf_t *crf = crf_read(f);
    fclose(f);
    return crf;
}

void crf_destroy(crf_t *self) {
    if (self == NULL) return;

    if (self->classes != NULL) {
        cstring_array_destroy(self->classes);
    }

    if (self->state_features != NULL) {
        trie_destroy(self->state_features);
    }
#ifdef EXP_FEATURE_DICT
    fd_destroy((feature_dict_t *)self->state_dict);
    fd_destroy((feature_dict_t *)self->trans_dict);
#endif

    if (self->weights != NULL) {
        sparse_matrix_destroy(self->weights);
    }

    if (self->state_trans_features != NULL) {
        trie_destroy(self->state_trans_features);
    }

    if (self->state_trans_weights != NULL) {
        sparse_matrix_destroy(self->state_trans_weights);
    }

    if (self->trans_weights != NULL) {
        double_matrix_destroy(self->trans_weights);
    }

    if (self->viterbi != NULL) {
        uint32_array_destroy(self->viterbi);
    }

    if (self->context != NULL) {
        crf_context_destroy(self->context);
    }

    free(self);
}
