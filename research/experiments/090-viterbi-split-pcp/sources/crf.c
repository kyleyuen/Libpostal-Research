#include "crf.h"
#include <math.h>
#include "log/log.h"
#ifdef EXP_ROW_PACK
#if defined(EXP_ALLOC_LEAN)
#error "EXP_ROW_PACK is implemented for the direct path without EXP_ALLOC_LEAN"
#endif
#include "file_utils.h"
// Experiment 021 (C1-lite): each weight entry as one packed record {weight, class} instead of two parallel arrays (indices[], data[]); read straight from the
// model file in chunks (two passes with fseek: classes, then weights), so the original arrays are never allocated. Same doubles, same order.
typedef struct __attribute__((packed)) { double w; uint16_t cls; } crf_wrec_t;
static crf_wrec_t *crf_pack_state = NULL, *crf_pack_trans = NULL;
#ifdef EXP_BACK_NOCLEAR
// Experiment 072: 1 when every model weight is finite (checked once at load). Then Viterbi writes every backward-edge cell it later reads (t >= 1:
// each target label's maximum starts at -DBL_MAX and every candidate score is finite and larger), so clearing them per address is redundant.
static int crf_weights_finite = 0;
#endif
static sparse_matrix_t *crf_sparse_read_packed(FILE *f, crf_wrec_t **out) {
    sparse_matrix_t *sp = calloc(1, sizeof(sparse_matrix_t)); if (sp == NULL) return NULL;
    uint64_t len_indptr, len_indices, len_data; crf_wrec_t *recs = NULL; uint32_t *ib = NULL; double *db = NULL;
    if (!file_read_uint32(f, &sp->m) || !file_read_uint32(f, &sp->n) || !file_read_uint64(f, &len_indptr)) goto fail;
    sp->indptr = uint32_array_new_size((size_t)len_indptr); if (sp->indptr == NULL) goto fail;
    if (!file_read_uint32_array(f, sp->indptr->a, len_indptr)) goto fail;
    sp->indptr->n = (size_t)len_indptr;
    if (!file_read_uint64(f, &len_indices)) goto fail;
    off_t pos_i = ftello(f);
    if (pos_i < 0 || fseeko(f, pos_i + (off_t)(4 * len_indices), SEEK_SET) != 0 || !file_read_uint64(f, &len_data) || len_data != len_indices) goto fail;
    off_t pos_d = ftello(f);
    recs = malloc((len_indices ? len_indices : 1) * sizeof(crf_wrec_t)); ib = malloc(65536 * sizeof(uint32_t)); db = malloc(65536 * sizeof(double));
    if (recs == NULL || ib == NULL || db == NULL || fseeko(f, pos_i, SEEK_SET) != 0) goto fail;
    for (uint64_t j = 0; j < len_indices; ) {
        size_t k = len_indices - j < 65536 ? (size_t)(len_indices - j) : 65536;
        if (!file_read_uint32_array(f, ib, k)) goto fail;
        for (size_t q = 0; q < k; q++) { if (ib[q] > 65535) goto fail; recs[j + q].cls = (uint16_t)ib[q]; }
        j += k;
    }
    if (fseeko(f, pos_d, SEEK_SET) != 0) goto fail;
    for (uint64_t j = 0; j < len_data; ) {
        size_t k = len_data - j < 65536 ? (size_t)(len_data - j) : 65536;
        if (!file_read_double_array(f, db, k)) goto fail;
        for (size_t q = 0; q < k; q++) recs[j + q].w = db[q];
        j += k;
    }
    if (fseeko(f, pos_d + (off_t)(8 * len_data), SEEK_SET) != 0) goto fail;   // leave the stream where sparse_matrix_read leaves it
    free(ib); free(db); *out = recs; return sp;
fail:
    free(ib); free(db); free(recs); sparse_matrix_destroy(sp); return NULL;
}
#endif
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
#if defined(EXP_ALLOC_LEAN) || defined(EXP_TRANS_ALIAS)
    if (crf_context->trans != self->trans_weights && !double_matrix_copy(self->trans_weights, crf_context->trans)) {
#else
    if (!double_matrix_copy(self->trans_weights, crf_context->trans)) {
#endif
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

#ifdef EXP_INT_KEYS
// Experiment 015: routed features are emitted as an empty placeholder token plus an integer key in these side lists (one per feature array, in order).
int crf_ik_active = 0;
#ifdef EXP_ROLE_IDS
#define CRF_ROLE_N 6
uint32_t *crf_role_ids = NULL;   // [vocab slot][role]; NULL = not built
#endif
#ifdef EXP_CONST_IDS
// Experiment 025: state-feature ids of the value-less constant keys "bias" and "postcode have context", looked up once in the final dictionary.
uint32_t crf_const_ids[2] = {CRF_NO_FEATURE, CRF_NO_FEATURE};
int crf_const_ready = 0;
#endif
int crf_ik_name[2][6] = {{-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}};   // [0] state roles, [1] transition roles; -1 = not routed
static uint64_t *crf_ik_keys[2] = {NULL, NULL};
static size_t crf_ik_n[2] = {0, 0}, crf_ik_m[2] = {0, 0};
#ifdef EXP_INT_KEYS_VERIFY
cstring_array *crf_ik_verify_str[2] = {NULL, NULL};   // the string each placeholder stands for, in the same order
static feature_dict_t *crf_ik_full[2] = {NULL, NULL}; // the untouched dictionaries; every lookup is checked against them
#endif

void crf_ik_push(cstring_array *a, int which, uint64_t key) {
    if (crf_ik_n[which] == crf_ik_m[which]) {
        size_t m = crf_ik_m[which] ? crf_ik_m[which] * 2 : 256;
        uint64_t *p = realloc(crf_ik_keys[which], m * sizeof(uint64_t));
        if (p == NULL) { fprintf(stderr, "crf_ik_push: out of memory\n"); exit(EXIT_FAILURE); }
        crf_ik_keys[which] = p; crf_ik_m[which] = m;
    }
    crf_ik_keys[which][crf_ik_n[which]++] = key;
    cstring_array_start_token(a);
    char_array_push(a->str, '\0');
}

static const char *crf_ik_role_name[2][6] = {{"word", "prev word", "next word", "first word", "prev word+word", "word+next word"}, {"word", "prev word", NULL, NULL, NULL, NULL}};
static const int8_t crf_ik_role_arity[2][6] = {{1, 1, 1, 1, 2, 2}, {1, 1, 0, 0, 0, 0}};

// Called once after the vocabulary dictionary exists. Any doubt leaves the dictionaries of `h` in place (crf_ik_active stays 0).
bool crf_int_keys_enable(crf_t *crf, const feature_dict_t *vocab) {
    if (getenv("CRF_INT_KEYS_DISABLE") != NULL || crf == NULL || vocab == NULL || crf->state_dict == NULL || crf->trans_dict == NULL) return false;
    feature_dict_t *old[2] = {(feature_dict_t *)crf->state_dict, (feature_dict_t *)crf->trans_dict}, *nd[2] = {NULL, NULL};
    int names[2][6];
    for (int w = 0; w < 2; w++) {
        int8_t arity[FD_MAX_NAMES] = {0};
        for (int r = 0; r < 6; r++) {
            names[w][r] = -1;
            if (crf_ik_role_name[w][r] == NULL) continue;
#ifndef EXP_INT_KEYS_PAIRS
            if (crf_ik_role_arity[w][r] == 2) continue;
#endif
            int id = fd_find_name(old[w], crf_ik_role_name[w][r], strlen(crf_ik_role_name[w][r]));
            if (id < 0) continue;
            names[w][r] = id; arity[id] = crf_ik_role_arity[w][r];
        }
        nd[w] = fd_int_split(old[w], vocab, arity);
        if (nd[w] == NULL) { if (w == 1) fd_destroy(nd[0]); return false; }
    }
    if (getenv("CRF_DICT_VERBOSE") != NULL) {
        for (int w = 0; w < 2; w++) fprintf(stderr, "crf int keys [%s]: moved=%llu kept=%llu of %llu build=%.2fs int=%.1f MB string=%.1f MB (was %.1f MB)\n", w ? "trans" : "state",
            (unsigned long long)nd[w]->inum, (unsigned long long)nd[w]->num_keys, (unsigned long long)old[w]->num_keys, nd[w]->build_seconds,
            nd[w]->isize * sizeof(fdi_slot_t) / 1048576.0, (nd[w]->size * 8 + nd[w]->arena_used) / 1048576.0, (old[w]->size * 8 + old[w]->arena_used) / 1048576.0);
    }
#ifdef EXP_INT_KEYS_VERIFY
    crf_ik_full[0] = old[0]; crf_ik_full[1] = old[1];
    crf_ik_verify_str[0] = cstring_array_new(); crf_ik_verify_str[1] = cstring_array_new();
#else
    fd_destroy(old[0]); fd_destroy(old[1]);
#endif
    crf->state_dict = nd[0]; crf->trans_dict = nd[1];
    memcpy(crf_ik_name, names, sizeof names);
    crf_ik_active = 1;
#ifdef EXP_CONST_IDS
    { static const char *ck[2] = {"bias", "postcode have context"}; uint32_t id;
      for (int q = 0; q < 2; q++) crf_const_ids[q] = fd_lookup(nd[0], ck[q], strlen(ck[q]), &id) ? id : CRF_NO_FEATURE;
      crf_const_ready = 1; }
#endif
#ifdef EXP_ROLE_IDS
    // Experiment 023 (017 plan item 2): for every vocabulary slot w and each single-word role (state word / prev word / next word / first word,
    // transition word / prev word), the feature id the integer table returns for (role, w) — computed once here with fdi_lookup, so the emission
    // point can push the id itself instead of a key to look up. Same table, same answer.
    {
        size_t V = vocab->size; uint32_t *t = malloc(V * CRF_ROLE_N * sizeof(uint32_t));
        if (t != NULL) {
            for (size_t w = 0; w < V; w++) for (int r = 0; r < CRF_ROLE_N; r++) {
                int wh = r < 4 ? 0 : 1, rr = r < 4 ? r : r - 4, nm = names[wh][rr]; uint32_t id;
                t[w * CRF_ROLE_N + r] = (nm >= 0 && vocab->slots[w] != 0 && fdi_lookup(nd[wh], fdi_key((uint32_t)nm, (uint32_t)w, FDI_WNONE), &id)) ? id : CRF_NO_FEATURE;
            }
            crf_role_ids = t;
        }
    }
#endif
    return true;
}
#endif

// Looks up all keys of `a` in the dictionary d; ids[i] = feature id or CRF_NO_FEATURE. Prefetches the weight-row start of every hit.
#ifdef EXP_INT_KEYS_VERIFY
unsigned long long crf_ik_verified = 0;   // lookups checked against the full dictionaries (reported at exit)
__attribute__((destructor)) static void crf_ik_report(void) { if (crf_ik_active) fprintf(stderr, "EXP_INT_KEYS_VERIFY: %llu lookups checked, 0 mismatches\n", crf_ik_verified); }
#endif
#ifdef EXP_DUP_STATS
// Measurement only: per address, how many string-path keys repeat an earlier string key of the same address (and how many keys are integer/resolved).
static unsigned long long ds_str = 0, ds_dup = 0, ds_int = 0;
__attribute__((destructor)) static void ds_report(void) { if (ds_str + ds_int) fprintf(stderr, "DUP_STATS string_keys=%llu duplicates=%llu int_or_resolved=%llu\n", ds_str, ds_dup, ds_int); }
#endif
static void crf_dict_lookup_array(const feature_dict_t *d, cstring_array *a, uint32_t *ids, const uint32_t *indptr, int which) {
    enum { WIN = 64 };
    uint8_t nb[WIN]; const char *rest[WIN]; size_t rl[WIN]; uint64_t hh[WIN]; bool ok[WIN];
    size_t n = a->indices->n;
#ifdef EXP_INT_KEYS
    bool isint[WIN]; size_t ikp = 0;
    const uint64_t *ikeys = crf_ik_active ? crf_ik_keys[which] : NULL;
#ifdef EXP_INT_KEYS_VERIFY
    size_t ivp = 0;
#endif
#endif
    char *chars = a->str->a; uint32_t *off = a->indices->a; size_t chars_n = a->str->n;
    for (size_t base = 0; base < n; base += WIN) {
        size_t m = n - base < WIN ? n - base : WIN;
        for (size_t j = 0; j < m; j++) {
            size_t i = base + j;
            size_t len = (i + 1 < n ? off[i + 1] : chars_n) - off[i] - 1;
#ifdef EXP_CHECK_LEN
            if (strlen(chars + off[i]) != len) { fprintf(stderr, "EXP_CHECK_LEN mismatch: strlen=%zu computed=%zu\n", strlen(chars + off[i]), len); abort(); }
#endif
#ifdef EXP_INT_KEYS
            isint[j] = ikeys != NULL && len == 0;          // placeholder: real keys are never empty (a name is always present)
#ifdef EXP_DUP_STATS
            if (isint[j]) ds_int++;
#endif
            if (isint[j]) {
                if (ikp >= crf_ik_n[which]) { fprintf(stderr, "EXP_INT_KEYS: more placeholders than keys\n"); abort(); }
                hh[j] = ikeys[ikp++]; ok[j] = true;
#ifdef EXP_ROLE_IDS
                if (hh[j] >> 63) continue;                 // already resolved: nothing to prefetch in the table
#endif
                __builtin_prefetch(&d->islots[fdi_index(d, hh[j])]);
                continue;
            }
#endif
#ifdef EXP_DUP_STATS
            if (len > 0 && getenv("DUP_NAMES")) { const char *bar = memchr(chars + off[i], '|', len); fprintf(stderr, "NAME %.*s\n", (int)(bar ? (size_t)(bar - (chars + off[i])) : len), chars + off[i]); }
            if (len > 0) { ds_str++; for (size_t q = 0; q < i; q++) { size_t lq = (q + 1 < n ? off[q + 1] : chars_n) - off[q] - 1; if (lq == len && memcmp(chars + off[q], chars + off[i], len) == 0) { ds_dup++; break; } } }
#endif
#ifdef EXP_NAME_BYTE
            if (len >= 2 && (uint8_t)chars[off[i]] == 0x01) { nb[j] = (uint8_t)chars[off[i] + 1]; rest[j] = chars + off[i] + 2; rl[j] = len - 2; ok[j] = true; } else
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
#ifdef EXP_INT_KEYS
            if (isint[j]) {
#ifdef EXP_ROLE_IDS
                if (hh[j] >> 63) ids[base + j] = (uint32_t)hh[j]; else
#endif
                ids[base + j] = fdi_lookup(d, hh[j], &feature_id) ? feature_id : CRF_NO_FEATURE;
#ifdef EXP_WEIGHT_PREFETCH
                if (ids[base + j] != CRF_NO_FEATURE) __builtin_prefetch(&indptr[ids[base + j]]);
#endif
            } else
#endif
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
#ifdef EXP_INT_KEYS_VERIFY
            if (crf_ik_active) {
                const char *vk; size_t vl;
                if (isint[j]) {
                    cstring_array *vs = crf_ik_verify_str[which];
                    if (ivp >= vs->indices->n) { fprintf(stderr, "EXP_INT_KEYS_VERIFY: missing verify string\n"); abort(); }
                    vk = cstring_array_get_string(vs, (uint32_t)ivp); vl = strlen(vk); ivp++;
                } else {
                    size_t i = base + j; vk = chars + off[i]; vl = (i + 1 < n ? off[i + 1] : chars_n) - off[i] - 1;
                }
                uint32_t want; bool hit = fd_lookup(crf_ik_full[which], vk, vl, &want);
                uint32_t got = ids[base + j];
                if (hit != (got != CRF_NO_FEATURE) || (hit && want != got)) {
                    fprintf(stderr, "EXP_INT_KEYS_VERIFY MISMATCH [%s] key='%s' int=%d full_hit=%d full_id=%u got=%u\n", which ? "trans" : "state", vk, (int)isint[j], (int)hit, hit ? want : 0, got);
                    abort();
                }
                crf_ik_verified++;
            }
#endif
        }
    }
#ifdef EXP_INT_KEYS
    if (ikeys != NULL && ikp != crf_ik_n[which]) { fprintf(stderr, "EXP_INT_KEYS: %zu keys, %zu placeholders\n", crf_ik_n[which], ikp); abort(); }
#endif
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

#ifdef EXP_STAGE_STATS
#include <mach/mach_time.h>
extern uint64_t ss_t[16], ss_n[8];
#endif
#ifdef EXP_ALLOC_LEAN
// Experiment 016 T8. (C3) The context's transition matrix is only read by Viterbi and was re-copied from trans_weights for every address: point it at
// trans_weights instead (the context's own matrix is restored before the context is destroyed). (C2) state_trans is kept all-zero between addresses by
// re-zeroing exactly the cells the previous address wrote, instead of a memset of T x L x L doubles; any address that did not go through the recording
// path forces a full memset for the next one.
static double_matrix_t *crf_trans_saved = NULL;
static uint32_t *crf_st_touched = NULL; static size_t crf_st_n = 0, crf_st_m = 0;
static int crf_st_clean = 0;   // 1: every non-zero cell of state_trans is listed in crf_st_touched
static int crf_st_fail = 0;    // a realloc of the list failed during this address
static inline void crf_st_note(uint32_t cell) {
    if (crf_st_n == crf_st_m) { size_t m = crf_st_m ? crf_st_m * 2 : 1024; uint32_t *p = realloc(crf_st_touched, m * sizeof *p); if (p == NULL) { crf_st_fail = 1; return; } crf_st_touched = p; crf_st_m = m; }
    crf_st_touched[crf_st_n++] = cell;
}
#endif
#if defined(EXP_TRANS_ALIAS) && !defined(EXP_ALLOC_LEAN)
// Experiment 048: the context's transition matrix points at the model's (read-only for Viterbi) instead of being copied from it for every address;
// the context's own matrix is restored before the context is destroyed (same mechanism as 016 T8's C3, without its C2 cell bookkeeping).
static double_matrix_t *crf_trans_saved = NULL;
#endif
#ifdef EXP_NAME_BYTE
#include "fa_nb.h"
#endif
static bool crf_tagger_score_direct(crf_t *self, void *tagger, void *tagger_context, cstring_array *features, cstring_array *prev_tag_features, tagger_feature_function feature_function, tokenized_string_t *tokenized, crf_context_t *crf_context, size_t num_tokens) {
#if defined(EXP_ALLOC_LEAN) || defined(EXP_TRANS_ALIAS)
    if (crf_context->trans != self->trans_weights) {
        if (crf_context->trans->m != self->trans_weights->m || crf_context->trans->n != self->trans_weights->n) return false;
        crf_trans_saved = crf_context->trans; crf_context->trans = self->trans_weights;
    }
#else
    if (!double_matrix_copy(self->trans_weights, crf_context->trans)) return false;
#endif
    if (num_tokens + 1 > crf_tok_m) {
        size_t m = crf_tok_m ? crf_tok_m : 32;
        while (m < num_tokens + 1) m *= 2;
        uint32_t *a = realloc(crf_state_tok, m * sizeof(uint32_t)); if (a == NULL) return false; crf_state_tok = a;
        uint32_t *b = realloc(crf_trans_tok, m * sizeof(uint32_t)); if (b == NULL) return false; crf_trans_tok = b;
        crf_tok_m = m;
    }

    cstring_array_clear(features);
    cstring_array_clear(prev_tag_features);
#ifdef EXP_INT_KEYS
    crf_ik_n[0] = crf_ik_n[1] = 0;
#ifdef EXP_INT_KEYS_VERIFY
    if (crf_ik_active) { cstring_array_clear(crf_ik_verify_str[0]); cstring_array_clear(crf_ik_verify_str[1]); }
#endif
#endif
#ifdef EXP_NAME_BYTE
    fa_nb_dict[0] = self->state_dict; fa_nb_dict[1] = self->trans_dict; fa_nb_arr[0] = features; fa_nb_arr[1] = prev_tag_features;
#endif
    crf_features_accumulate = 1;
#ifdef EXP_STAGE_STATS
    uint64_t sa = mach_absolute_time();
#endif
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
#ifdef EXP_NAME_BYTE
    fa_nb_arr[0] = fa_nb_arr[1] = NULL;   // only the direct scoring pass reads the marker form
#endif
    crf_features_accumulate = 0;
    if (!ok) return false;
    crf_state_tok[num_tokens] = (uint32_t)features->indices->n;
    crf_trans_tok[num_tokens] = (uint32_t)prev_tag_features->indices->n;

    size_t ns = features->indices->n, np = prev_tag_features->indices->n;
#ifdef EXP_STAGE_STATS
    uint64_t sb = mach_absolute_time(); ss_t[5] += sb - sa; ss_n[2] += ns; ss_n[3] += np;
#endif
    if (ns > crf_state_ids_m) { size_t m = crf_state_ids_m ? crf_state_ids_m : 256; while (m < ns) m *= 2; uint32_t *p = realloc(crf_state_ids, m * sizeof(uint32_t)); if (p == NULL) return false; crf_state_ids = p; crf_state_ids_m = m; }
    if (np > crf_trans_ids_m) { size_t m = crf_trans_ids_m ? crf_trans_ids_m : 64; while (m < np) m *= 2; uint32_t *p = realloc(crf_trans_ids, m * sizeof(uint32_t)); if (p == NULL) return false; crf_trans_ids = p; crf_trans_ids_m = m; }

#ifdef EXP_ROW_PACK
    uint32_t *indptr = self->weights->indptr->a, *tindptr = self->state_trans_weights->indptr->a;
    const crf_wrec_t *srec = crf_pack_state, *trec = crf_pack_trans;
#else
    uint32_t *indptr = self->weights->indptr->a, *indices = self->weights->indices->a;
    double *data = self->weights->data->a;
    uint32_t *tindptr = self->state_trans_weights->indptr->a, *tindices = self->state_trans_weights->indices->a;
    double *tdata = self->state_trans_weights->data->a;
#endif

    crf_dict_lookup_array((feature_dict_t *)self->state_dict, features, crf_state_ids, indptr, 0);
    crf_dict_lookup_array((feature_dict_t *)self->trans_dict, prev_tag_features, crf_trans_ids, tindptr, 1);
#ifdef EXP_ROW_PACK
#ifdef EXP_WEIGHT_PREFETCH
    for (size_t k = 0; k < ns; k++) { uint32_t id = crf_state_ids[k]; if (id != CRF_NO_FEATURE) __builtin_prefetch(&srec[indptr[id]]); }
    for (size_t k = 0; k < np; k++) { uint32_t id = crf_trans_ids[k]; if (id != CRF_NO_FEATURE) __builtin_prefetch(&trec[tindptr[id]]); }
#endif
#else
    crf_prefetch_rows(crf_state_ids, ns, indptr, indices, data);
    crf_prefetch_rows(crf_trans_ids, np, tindptr, tindices, tdata);
#endif
#ifdef EXP_STAGE_STATS
    uint64_t sc = mach_absolute_time(); ss_t[6] += sc - sb;
#endif

    for (size_t t = 0; t < num_tokens; t++) {
        double *state_scores = state_score(crf_context, (uint32_t)t);
        for (uint32_t k = crf_state_tok[t]; k < crf_state_tok[t + 1]; k++) {
            uint32_t feature_id = crf_state_ids[k];
            if (feature_id == CRF_NO_FEATURE) continue;
#ifdef EXP_ROW_PACK
            for (int col = indptr[feature_id]; col < indptr[feature_id + 1]; col++) state_scores[srec[col].cls] += srec[col].w;
#else
            for (int col = indptr[feature_id]; col < indptr[feature_id + 1]; col++) state_scores[indices[col]] += data[col];
#endif
        }
        double *state_trans_scores = state_trans_score_all(crf_context, (uint32_t)t);
        for (uint32_t k = crf_trans_tok[t]; k < crf_trans_tok[t + 1]; k++) {
            uint32_t feature_id = crf_trans_ids[k];
            if (feature_id == CRF_NO_FEATURE) continue;
#ifdef EXP_ALLOC_LEAN
            uint32_t row0 = (uint32_t)(state_trans_scores - crf_context->state_trans->values);
            for (int col = tindptr[feature_id]; col < tindptr[feature_id + 1]; col++) { state_trans_scores[tindices[col]] += tdata[col]; crf_st_note(row0 + tindices[col]); }
#else
#ifdef EXP_ROW_PACK
            for (int col = tindptr[feature_id]; col < tindptr[feature_id + 1]; col++) state_trans_scores[trec[col].cls] += trec[col].w;   // L * L classes
#else
            for (int col = tindptr[feature_id]; col < tindptr[feature_id + 1]; col++) state_trans_scores[tindices[col]] += tdata[col];   // L * L classes
#endif
#endif
        }
    }
#ifdef EXP_ALLOC_LEAN
    crf_st_clean = !crf_st_fail;   // recording complete for this address
#endif
#ifdef EXP_STAGE_STATS
    ss_t[7] += mach_absolute_time() - sc;
#endif
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
    bool grew = false; (void)grew;
    if (T > crf_lean_cap_items) {
        grew = true;
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
#ifdef EXP_ALLOC_LEAN
    if (grew || !crf_st_clean) memset(self->state_trans->values, 0, crf_lean_cap_items * L * L * sizeof(double));   // establish the all-zero invariant
    else for (size_t i = 0; i < crf_st_n; i++) self->state_trans->values[crf_st_touched[i]] = 0.0;
    crf_st_n = 0; crf_st_clean = 0; crf_st_fail = 0;   // clean stays 0 unless the direct path records this address completely
#else
#ifdef EXP_ST_ROW0
    // Experiment 026: Viterbi reads state_trans rows t >= 1 only (it starts at t = 1), so row 0 never needs clearing; whatever it holds is never read.
    if (T > 1) memset(self->state_trans->values + L * L, 0, (T - 1) * L * L * sizeof(double));
#else
    memset(self->state_trans->values, 0, T * L * L * sizeof(double));
#endif
#endif
#ifdef EXP_BACK_NOCLEAR
    if (!crf_weights_finite)
#endif
    memset(self->backward_edges->values, 0, T * L * sizeof(uint32_t));
    self->num_items = T;
    return true;
}
#endif

bool crf_tagger_score(crf_t *self, void *tagger, void *tagger_context, cstring_array *features, cstring_array *prev_tag_features, tagger_feature_function feature_function, tokenized_string_t *tokenized, bool print_features) {
#ifdef EXP_VT_STATS
    uint64_t vt_enter = mach_absolute_time();
#endif
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

#ifdef EXP_VT_STATS
    ss_t[13] += mach_absolute_time() - vt_enter;
#endif
#ifdef EXP_DIRECT_KEYS
    if (!print_features && self->state_dict != NULL && self->trans_dict != NULL) {
        return crf_tagger_score_direct(self, tagger, tagger_context, features, prev_tag_features, feature_function, tokenized, crf_context, num_tokens);
    }
#endif
#ifdef EXP_ROW_PACK
    fprintf(stderr, "EXP_ROW_PACK: only the direct scoring path is supported (dictionaries disabled or print_features requested)\n"); abort();
#endif
#ifdef EXP_DEDUP_BATCH
    if (!print_features) {
        return crf_tagger_score_batched(self, tagger, tagger_context, features, prev_tag_features, feature_function, tokenized, crf_context, num_tokens);
    }
#endif

#if defined(EXP_ALLOC_LEAN) || defined(EXP_TRANS_ALIAS)
    if (crf_context->trans != self->trans_weights && !double_matrix_copy(self->trans_weights, crf_context->trans)) {
#else
    if (!double_matrix_copy(self->trans_weights, crf_context->trans)) {
#endif
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


#ifdef EXP_FAST_VITERBI
// Same recurrence, tie-breaking and floating-point association as crf_context_viterbi (crf_context.c), with the loops swapped (source label outer, target label inner)
// so the two accessor calls per (i, j) disappear and the inner loop runs over contiguous rows. For a fixed target j the candidates i are still visited in ascending order and
// replace the best only when strictly greater, so the chosen i, back[j] and cur[j] are identical, NaNs included.
#define CRF_FAST_VITERBI_MAX_L 256
static double crf_context_viterbi_fast(crf_context_t *self, uint32_t *labels) {
    const size_t T = self->num_items;
    const size_t L = self->num_labels;
    if (T == 0 || L > CRF_FAST_VITERBI_MAX_L) return crf_context_viterbi(self, labels);

    double best[CRF_FAST_VITERBI_MAX_L];
    int32_t arg[CRF_FAST_VITERBI_MAX_L];

    double *cur = alpha_score(self, 0);
    const double *state = state_score(self, 0);
    double_array_raw_copy(cur, state, L);

    const double *trans_all = self->trans->values;

    for (size_t t = 1; t < T; t++) {
        const double *prev = alpha_score(self, t - 1);
        cur = alpha_score(self, t);
        state = state_score(self, t);
        uint32_t *back = backward_edge_at(self, t);
        const double *st = state_trans_score_all(self, t);

        for (size_t j = 0; j < L; j++) { best[j] = -DBL_MAX; arg[j] = -1; }
        for (size_t i = 0; i < L; i++) {
            const double pi = prev[i];
            const double *sti = st + i * L;
            const double *tri = trans_all + i * L;
            for (size_t j = 0; j < L; j++) {
                double score = pi + sti[j] + tri[j];
                if (best[j] < score) { best[j] = score; arg[j] = (int32_t)i; }
            }
        }
        for (size_t j = 0; j < L; j++) {
            if (arg[j] >= 0) back[j] = (uint32_t)arg[j];
            cur[j] = best[j] + state[j];
        }
    }

    double max_score = -DBL_MAX;
    ssize_t argmax_score = -1;
    const double *last = alpha_score(self, T - 1);
    labels[T - 1] = 0;
    for (size_t i = 0; i < L; i++) {
        if (last[i] > max_score) { max_score = last[i]; argmax_score = (ssize_t)i; }
    }
    if (argmax_score >= 0) labels[T - 1] = (uint32_t)argmax_score;
    for (ssize_t t = (ssize_t)T - 2; t >= 0; t--) {
        uint32_t *back = backward_edge_at(self, (size_t)t + 1);
        labels[t] = back[labels[t + 1]];
    }
    return max_score;
}
#endif

bool crf_tagger_score_viterbi(crf_t *self, void *tagger, void *tagger_context, cstring_array *features, cstring_array *prev_tag_features, tagger_feature_function feature_function, tokenized_string_t *tokenized, double *score, bool print_features) {
    if (!crf_tagger_score(self, tagger, tagger_context, features, prev_tag_features, feature_function, tokenized, print_features)) {
        return false;
    }

    size_t num_tokens = tokenized->tokens->n;

    uint32_array_resize_fixed(self->viterbi, num_tokens);
#ifdef EXP_FAST_VITERBI
    double viterbi_score = crf_context_viterbi_fast(self->context, self->viterbi->a);
#else
#ifdef EXP_VT_STATS
    uint64_t vt0 = mach_absolute_time();
#endif
    double viterbi_score = crf_context_viterbi(self->context, self->viterbi->a);
#ifdef EXP_VT_STATS
    ss_t[14] += mach_absolute_time() - vt0;
#endif
#endif

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

#ifdef EXP_ROW_PACK
    crf->weights = crf_sparse_read_packed(f, &crf_pack_state);
#else
    crf->weights = sparse_matrix_read(f);
#endif
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

#ifdef EXP_ROW_PACK
    crf->state_trans_weights = crf_sparse_read_packed(f, &crf_pack_trans);
#else
    crf->state_trans_weights = sparse_matrix_read(f);
#endif
    if (crf->state_trans_weights == NULL) {
        goto exit_crf_created;
    }

    crf->trans_weights = double_matrix_read(f);
    if (crf->trans_weights == NULL) {
        goto exit_crf_created;
    }
#ifdef EXP_BACK_NOCLEAR
    if (crf->weights != NULL && crf->state_trans_weights != NULL && crf_pack_state != NULL && crf_pack_trans != NULL && crf->trans_weights != NULL) {
        int fin = 1; size_t ns_ = crf->weights->indptr->a[crf->weights->indptr->n - 1], nt_ = crf->state_trans_weights->indptr->a[crf->state_trans_weights->indptr->n - 1];
        for (size_t q = 0; q < ns_ && fin; q++) if (!isfinite(crf_pack_state[q].w) || fabs(crf_pack_state[q].w) > 1e100) fin = 0;
        for (size_t q = 0; q < nt_ && fin; q++) if (!isfinite(crf_pack_trans[q].w) || fabs(crf_pack_trans[q].w) > 1e100) fin = 0;
        for (size_t q = 0; q < crf->trans_weights->m * crf->trans_weights->n && fin; q++) if (!isfinite(crf->trans_weights->values[q]) || fabs(crf->trans_weights->values[q]) > 1e100) fin = 0;
        crf_weights_finite = fin;
    }
#endif

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
#ifdef EXP_ROW_PACK
    free(crf_pack_state); crf_pack_state = NULL; free(crf_pack_trans); crf_pack_trans = NULL;
#endif

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
#if defined(EXP_ALLOC_LEAN) || defined(EXP_TRANS_ALIAS)
        if (crf_trans_saved != NULL) { self->context->trans = crf_trans_saved; crf_trans_saved = NULL; }
#endif
        crf_context_destroy(self->context);
    }

    free(self);
}
