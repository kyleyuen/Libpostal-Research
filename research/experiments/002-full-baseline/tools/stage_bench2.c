// Experiment 002 (derived from 001's stage_bench.c): measurement-only harness. Mirrors address_parser_parse() (address_parser.c:1661)
// and crf_tagger_score() (crf.c:15) step by step with timers between steps. Links the PRESERVED
// baseline libpostal.a; libpostal source is not modified. Single thread.
//
// usage: stage_bench2 corpus.txt passes
// Adds: original-order stage table, ASCII vs non-ASCII split, hit/miss lookup cost, trie step distributions.
// Phase A: per-stage wall time (whole pass over the corpus, `passes` times after 1 warm-up pass)
// Phase B: trie-lookup locality experiment (hot/cold/shuffled/repeat) + trie step counts
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "libpostal.h"
#include "address_parser.h"
#include "crf.h"
#include "trie.h"
#include "tokens.h"

bool trie_compare_tail(trie_t *self, char *str, size_t len, size_t tail_index);  // defined in trie.c, not in trie.h
static inline uint64_t nowns(void) { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW); }

enum { S_NORM, S_TOK, S_CTX, S_PRED_INIT, S_FEAT, S_LOOK_STATE, S_ACC_STATE, S_LOOK_TRANS, S_ACC_TRANS,
       S_VITERBI, S_LABELS, S_RESP, S_DESTROY, S_LA_STATE, S_LA_TRANS, S_N };
static const char *names[S_N] = {"normalize", "tokenize+build tokens", "context_fill (phrase/dict search)",
    "crf ctx reset+trans copy", "feature generation", "trie lookup: state features", "weight accumulate: state",
    "trie lookup: prev-tag features", "weight accumulate: prev-tag", "viterbi", "label strings", "response build",
    "free temporaries (response NOT included)", "ORIG-ORDER lookup+accumulate: state", "ORIG-ORDER lookup+accumulate: prev-tag"};

static uint64_t acc[S_N];
static uint64_t n_addr, n_shortcut, n_tokens, n_feat, n_feat_hit, n_tfeat, n_tfeat_hit;

// Optional recording of every feature string looked up (Phase B)
static char **rec_state = NULL, **rec_trans = NULL;
static size_t rec_state_n, rec_state_cap, rec_trans_n, rec_trans_cap;
static uint32_t *rec_state_addr;  // address index for each state feature (for subsetting)
static int recording;
static int interleave;   // 1 = original order: lookup then accumulate per feature (control)
static size_t cur_addr;

static void rec_push(char ***arr, size_t *n, size_t *cap, const char *s) {
    if (*n == *cap) { *cap = *cap ? *cap * 2 : 1 << 20; *arr = realloc(*arr, *cap * sizeof(char *)); }
    (*arr)[(*n)++] = strdup(s);
}

// returns response like address_parser_parse; timings accumulate in acc[] when timed != 0
static libpostal_address_parser_response_t *parse_staged(char *address, int timed) {
    address_parser_t *parser = get_address_parser();
    address_parser_context_t *context = parser->context;
    crf_t *self = parser->model.crf;
    uint64_t t0 = 0, t1;
#define T0() do { if (timed) t0 = nowns(); } while (0)
#define T1(s) do { if (timed) { t1 = nowns(); acc[s] += t1 - t0; t0 = t1; } } while (0)

    T0();
    char *normalized = address_parser_normalize_string(address);
    bool is_normalized = normalized != NULL;
    if (!is_normalized) normalized = address;
    T1(S_NORM);

    token_array *tokens = tokenize(normalized);
    tokenized_string_t *tokenized_str = tokenized_string_new_from_str_size(normalized, strlen(normalized), tokens->n);
    uint32_array_clear(context->separators);
    for (size_t i = 0; i < tokens->n; i++) {
        token_t token = tokens->a[i];
        if (ADDRESS_PARSER_IS_SEPARATOR(token.type)) {
            uint32_array_pop(context->separators);
            uint32_array_push(context->separators, ADDRESS_SEPARATOR_FIELD_INTERNAL);
            continue;
        } else if (ADDRESS_PARSER_IS_IGNORABLE(token.type)) {
            continue;
        }
        tokenized_string_add_token(tokenized_str, (const char *)normalized, token.len, token.type, token.offset);
        uint32_array_push(context->separators, ADDRESS_SEPARATOR_NONE);
    }
    T1(S_TOK);

    address_parser_context_fill(context, parser, tokenized_str, NULL, NULL);
    T1(S_CTX);

    libpostal_address_parser_response_t *response = NULL;
    phrase_t only_phrase = NULL_PHRASE;
    token_t token, prev_token;
    bool is_postal = false;
    if (context->component_phrases->n == 1) {
        only_phrase = context->component_phrases->a[0];
    } else if (context->postal_code_phrases->n == 1) {
        only_phrase = context->postal_code_phrases->a[0];
        is_postal = true;
    }
    if (only_phrase.start == 0 && only_phrase.len == tokenized_str->tokens->n && only_phrase.len > 0) {
        // Rare bypass path: defer to the library so behaviour is identical; time is charged to S_RESP.
        uint32_t mc = 0;
        if (!is_postal) mc = parser->phrase_types->a[only_phrase.data].most_common;
        if (is_postal || mc == ADDRESS_PARSER_BOUNDARY_CITY || mc == ADDRESS_PARSER_BOUNDARY_STATE ||
            mc == ADDRESS_PARSER_BOUNDARY_COUNTRY || mc == ADDRESS_PARSER_BOUNDARY_STATE_DISTRICT ||
            mc == ADDRESS_PARSER_BOUNDARY_COUNTRY_REGION || mc == ADDRESS_PARSER_BOUNDARY_SUBURB ||
            mc == ADDRESS_PARSER_BOUNDARY_CITY_DISTRICT || mc == ADDRESS_PARSER_BOUNDARY_WORLD_REGION) {
            token_array_destroy(tokens);
            tokenized_string_destroy(tokenized_str);
            if (is_normalized) free(normalized);
            if (timed && !interleave) n_shortcut++;
            response = libpostal_parse_address(address, libpostal_get_address_parser_default_options());
            T1(S_RESP);
            return response;
        }
    }

    cstring_array *token_labels = cstring_array_new_size(tokens->n);
    cstring_array *features = context->features, *prev_tag_features = context->prev_tag_features;
    size_t num_tokens = tokenized_str->tokens->n;
    if (timed && !interleave) n_tokens += num_tokens;

    crf_context_t *crf_context = self->context;
    crf_context_set_num_items(crf_context, num_tokens);
    crf_context_reset(crf_context, CRF_CONTEXT_RESET_ALL);
    double_matrix_copy(self->trans_weights, crf_context->trans);
    T1(S_PRED_INIT);

    uint32_t fidx;
    char *feature;
    static uint32_t ids[4096];
    static uint8_t hit[4096];
    for (uint32_t t = 0; t < num_tokens; t++) {
        T0();
        cstring_array_clear(features);
        cstring_array_clear(prev_tag_features);
        address_parser_features(parser, context, tokenized_str, t);
        T1(S_FEAT);

        double *state_scores = state_score(crf_context, t);
        uint32_t *indptr = self->weights->indptr->a, *indices = self->weights->indices->a;
        double *data = self->weights->data->a;

        size_t nf = 0;
        if (interleave && !recording) {
            cstring_array_foreach(features, fidx, feature, {
                uint32_t feature_id;
                if (!trie_get_data(self->state_features, feature, &feature_id)) continue;
                for (int col = indptr[feature_id]; col < indptr[feature_id + 1]; col++) state_scores[indices[col]] += data[col];
            })
            T1(S_LA_STATE);
            double *sts = state_trans_score_all(crf_context, t);
            uint32_t *ip2 = self->state_trans_weights->indptr->a, *ix2 = self->state_trans_weights->indices->a;
            double *d2 = self->state_trans_weights->data->a;
            cstring_array_foreach(prev_tag_features, fidx, feature, {
                uint32_t feature_id;
                if (!trie_get_data(self->state_trans_features, feature, &feature_id)) continue;
                for (int col = ip2[feature_id]; col < ip2[feature_id + 1]; col++) sts[ix2[col]] += d2[col];
            })
            T1(S_LA_TRANS);
            continue;
        }
        cstring_array_foreach(features, fidx, feature, {
            if (recording) { rec_push(&rec_state, &rec_state_n, &rec_state_cap, feature);
                             rec_state_addr = realloc(rec_state_addr, rec_state_cap * 4); rec_state_addr[rec_state_n - 1] = cur_addr; }
            hit[nf] = trie_get_data(self->state_features, feature, &ids[nf]);
            nf++;
        })
        T1(S_LOOK_STATE);
        if (timed && !interleave) n_feat += nf;
        for (size_t k = 0; k < nf; k++) {
            if (!hit[k]) continue;
            if (timed && !interleave) n_feat_hit++;
            for (int col = indptr[ids[k]]; col < indptr[ids[k] + 1]; col++) state_scores[indices[col]] += data[col];
        }
        T1(S_ACC_STATE);

        double *state_trans_scores = state_trans_score_all(crf_context, t);
        indptr = self->state_trans_weights->indptr->a;
        indices = self->state_trans_weights->indices->a;
        data = self->state_trans_weights->data->a;
        nf = 0;
        cstring_array_foreach(prev_tag_features, fidx, feature, {
            if (recording) rec_push(&rec_trans, &rec_trans_n, &rec_trans_cap, feature);
            hit[nf] = trie_get_data(self->state_trans_features, feature, &ids[nf]);
            nf++;
        })
        T1(S_LOOK_TRANS);
        if (timed && !interleave) n_tfeat += nf;
        for (size_t k = 0; k < nf; k++) {
            if (!hit[k]) continue;
            if (timed && !interleave) n_tfeat_hit++;
            for (int col = indptr[ids[k]]; col < indptr[ids[k] + 1]; col++) state_trans_scores[indices[col]] += data[col];
        }
        T1(S_ACC_TRANS);
    }

    uint32_array_resize_fixed(self->viterbi, num_tokens);
    crf_context_viterbi(crf_context, self->viterbi->a);
    T1(S_VITERBI);
    for (size_t i = 0; i < self->viterbi->n; i++)
        cstring_array_add_string(token_labels, cstring_array_get_string(self->classes, self->viterbi->a[i]));
    T1(S_LABELS);

    char *prev_label = NULL;
    response = malloc(sizeof(*response));
    size_t num_strings = cstring_array_num_strings(tokenized_str->strings);
    cstring_array *labels = cstring_array_new_size(num_strings);
    cstring_array *components = cstring_array_new_size(strlen(address) + num_strings);
    token_t *toks = tokenized_str->tokens->a;
    for (size_t i = 0; i < num_strings; i++) {
        char *str = tokenized_string_get_token(tokenized_str, i);
        char *label = cstring_array_get_string(token_labels, i);
        if (prev_label == NULL || strcmp(label, prev_label) != 0) {
            cstring_array_add_string(labels, label);
            cstring_array_start_token(components);
        }
        if (prev_label != NULL && strcmp(label, prev_label) == 0) {
            token = toks[i]; prev_token = toks[i - 1];
            if (token.offset > prev_token.offset + prev_token.len) cstring_array_cat_string(components, " ");
            cstring_array_cat_string(components, str);
        } else {
            cstring_array_append_string(components, str);
            cstring_array_terminate(components);
        }
        prev_label = label;
    }
    response->num_components = cstring_array_num_strings(components);
    response->components = cstring_array_to_strings(components);
    response->labels = cstring_array_to_strings(labels);
    T1(S_RESP);

    token_array_destroy(tokens);
    tokenized_string_destroy(tokenized_str);
    cstring_array_destroy(token_labels);
    if (is_normalized) free(normalized);
    T1(S_DESTROY);
    return response;
}

static int same(libpostal_address_parser_response_t *a, libpostal_address_parser_response_t *b) {
    if (!a || !b || a->num_components != b->num_components) return 0;
    for (size_t i = 0; i < a->num_components; i++)
        if (strcmp(a->components[i], b->components[i]) || strcmp(a->labels[i], b->labels[i])) return 0;
    return 1;
}

// deterministic xorshift for the shuffle
static uint64_t rs = 88172645463325252ULL;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }

static double lookup_run(trie_t *trie, char **keys, size_t n, size_t *order, int repeat2, uint64_t *hits, uint64_t *cnt) {
    uint32_t id, h = 0; uint64_t c = 0;
    uint64_t t = nowns();
    for (size_t i = 0; i < n; i++) {
        char *k = keys[order ? order[i] : i];
        h += trie_get_data(trie, k, &id); c++;
        if (repeat2) { h += trie_get_data(trie, k, &id); c++; }
    }
    t = nowns() - t;
    *hits = h; *cnt = c;
    return (double)t;
}

// trie walk step counter, mirrors trie_get_from_index (trie.c:844)
static uint64_t trie_steps(trie_t *self, char *word, uint32_t *result) {
    size_t len = strlen(word);
    unsigned char *ptr = (unsigned char *)word;
    uint32_t node_id = ROOT_NODE_ID, next_id = 0;
    trie_node_t node = trie_get_node(self, node_id);
    uint64_t steps = 0;
    *result = NULL_NODE_ID;
    if (node.base == NULL_NODE_ID) return 0;
    for (size_t i = 0; i < len + 1; i++, ptr++, node_id = next_id) {
        next_id = trie_get_transition_index(self, node, *ptr);
        node = trie_get_node(self, next_id);
        steps++;
        if (node.check != node_id) return steps;
        if (node.base < 0) {
            trie_data_node_t dn = trie_get_data_node(self, node);
            char *q = *ptr ? (char *)ptr + 1 : (char *)ptr;
            if (dn.tail != 0 && trie_compare_tail(self, q, strlen(q) + 1, dn.tail)) *result = next_id;
            return steps;
        }
    }
    *result = next_id;
    return steps;
}

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    int passes = atoi(argv[2]);
    FILE *f = fopen(argv[1], "r");
    char **addr = NULL, *line = NULL; size_t cap = 0, n = 0, acap = 0; ssize_t len;
    while ((len = getline(&line, &cap, f)) >= 0) {
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        if (!len) continue;
        if (n == acap) { acap = acap ? acap * 2 : 1024; addr = realloc(addr, acap * sizeof(*addr)); }
        addr[n++] = strdup(line);
    }
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    libpostal_address_parser_options_t opts = libpostal_get_address_parser_default_options();
    crf_t *crf = get_address_parser()->model.crf;

    // --- correctness: staged replica vs library, plus recording pass ---
    uint64_t mism = 0;
    recording = 1;
    for (size_t i = 0; i < n; i++) {
        cur_addr = i;
        libpostal_address_parser_response_t *a = parse_staged(addr[i], 0), *b = libpostal_parse_address(addr[i], opts);
        if (!same(a, b)) mism++;
        libpostal_address_parser_response_destroy(a); libpostal_address_parser_response_destroy(b);
    }
    recording = 0;
    printf("replica_vs_library: checked=%zu mismatches=%llu shortcut_paths=%llu\n", n, (unsigned long long)mism, (unsigned long long)n_shortcut);
    n_shortcut = n_tokens = n_feat = n_feat_hit = n_tfeat = n_tfeat_hit = 0;

    // --- Phase A: per-stage timing ---
    // per pass: library call; staged untimed (batched); staged untimed (original order);
    // staged timed ORIGINAL order (interleave=1); staged timed BATCHED (interleave=0).
    static uint64_t acc_o[S_N], acc_b[S_N], acc_o_by[2][S_N], acc_b_by[2][S_N], destroy_o, destroy_b;
    static uint64_t addr_by[2];
    static char *is_ascii_arr; is_ascii_arr = malloc(n);
    size_t n_ascii = 0;
    for (size_t i = 0; i < n; i++) { int a = 1; for (char *c = addr[i]; *c; c++) if ((unsigned char)*c >= 0x80) { a = 0; break; }
        is_ascii_arr[i] = a; n_ascii += a; }
    printf("corpus: addresses=%zu ascii=%zu non_ascii=%zu\n", n, n_ascii, n - n_ascii);
    for (int p = 0; p <= passes; p++) {                      // p==0 is warm-up
        uint64_t t = nowns();
        for (size_t i = 0; i < n; i++) libpostal_address_parser_response_destroy(libpostal_parse_address(addr[i], opts));
        t = nowns() - t;
        uint64_t u = nowns();
        for (size_t i = 0; i < n; i++) libpostal_address_parser_response_destroy(parse_staged(addr[i], 0));
        u = nowns() - u;
        interleave = 1;
        uint64_t v = nowns();
        for (size_t i = 0; i < n; i++) libpostal_address_parser_response_destroy(parse_staged(addr[i], 0));
        v = nowns() - v;
        // timed, original order
        memset(acc, 0, sizeof acc); uint64_t dsum = 0, prev[S_N];
        uint64_t so = nowns();
        for (size_t i = 0; i < n; i++) {
            memcpy(prev, acc, sizeof acc);
            libpostal_address_parser_response_t *r = parse_staged(addr[i], 1);
            uint64_t d0 = nowns(); libpostal_address_parser_response_destroy(r); dsum += nowns() - d0;
            int a = is_ascii_arr[i]; addr_by[a]++;
            for (int k = 0; k < S_N; k++) if (p) acc_o_by[a][k] += acc[k] - prev[k];
        }
        so = nowns() - so;
        if (p) { for (int k = 0; k < S_N; k++) acc_o[k] += acc[k]; destroy_o += dsum; }
        interleave = 0;
        // timed, batched (approximation)
        memset(acc, 0, sizeof acc); dsum = 0;
        uint64_t sb = nowns();
        for (size_t i = 0; i < n; i++) {
            memcpy(prev, acc, sizeof acc);
            libpostal_address_parser_response_t *r = parse_staged(addr[i], 1);
            uint64_t d0 = nowns(); libpostal_address_parser_response_destroy(r); dsum += nowns() - d0;
            int a = is_ascii_arr[i];
            for (int k = 0; k < S_N; k++) if (p) acc_b_by[a][k] += acc[k] - prev[k];
        }
        sb = nowns() - sb;
        if (p) { for (int k = 0; k < S_N; k++) acc_b[k] += acc[k]; destroy_b += dsum;
            printf("pass %d: library=%.3f staged_untimed_batched=%.3f staged_untimed_original_order=%.3f staged_timed_original_order=%.3f staged_timed_batched=%.3f (us/addr)\n",
                   p, t / 1e3 / n, u / 1e3 / n, v / 1e3 / n, so / 1e3 / n, sb / 1e3 / n); }
        if (p == 0) { n_shortcut = n_tokens = n_feat = n_feat_hit = n_tfeat = n_tfeat_hit = 0; memset(addr_by, 0, sizeof addr_by); }
    }
    double D = (double)n * passes;
    printf("STAGE_TABLE original_order (us/addr, mean over %d passes; lookup+accumulate is ONE block per token)\n", passes);
    for (int k = 0; k < S_N; k++) if (k != S_LOOK_STATE && k != S_ACC_STATE && k != S_LOOK_TRANS && k != S_ACC_TRANS)
        printf("  %-46s %8.3f\n", names[k], acc_o[k] / 1e3 / D);
    printf("  %-46s %8.3f\n", "response destroy (outer loop)", destroy_o / 1e3 / D);
    printf("STAGE_TABLE batched_approximation (us/addr; lookup and accumulate separated -> ~13%% faster than library)\n");
    for (int k = 0; k < S_N; k++) if (k != S_LA_STATE && k != S_LA_TRANS)
        printf("  %-46s %8.3f\n", names[k], acc_b[k] / 1e3 / D);
    printf("  %-46s %8.3f\n", "response destroy (outer loop)", destroy_b / 1e3 / D);
    for (int a = 0; a < 2; a++) {
        double Da = (double)addr_by[a];
        printf("STAGE_TABLE original_order %s inputs (n=%llu addresses processed, us/addr)\n", a ? "ASCII" : "NON-ASCII", (unsigned long long)addr_by[a]);
        double tot = 0;
        for (int k = 0; k < S_N; k++) if (k != S_LOOK_STATE && k != S_ACC_STATE && k != S_LOOK_TRANS && k != S_ACC_TRANS) { tot += acc_o_by[a][k]; }
        for (int k = 0; k < S_N; k++) if (k != S_LOOK_STATE && k != S_ACC_STATE && k != S_LOOK_TRANS && k != S_ACC_TRANS)
            printf("  %-46s %8.3f\n", names[k], acc_o_by[a][k] / 1e3 / Da);
        printf("  %-46s %8.3f\n", "TOTAL of stages", tot / 1e3 / Da);
    }
    printf("counts_%d_passes: addresses=%.0f shortcut=%llu tokens=%llu state_feature_lookups=%llu (hits %llu) prevtag_lookups=%llu (hits %llu)\n",
           passes, D, (unsigned long long)n_shortcut, (unsigned long long)n_tokens, (unsigned long long)n_feat,
           (unsigned long long)n_feat_hit, (unsigned long long)n_tfeat, (unsigned long long)n_tfeat_hit);
    printf("per_address: tokens=%.2f state_lookups=%.2f (hit %.1f%%) prevtag_lookups=%.2f (hit %.1f%%)\n",
           n_tokens / D, n_feat / D, 100.0 * n_feat_hit / n_feat, n_tfeat / D, 100.0 * n_tfeat_hit / n_tfeat);
    printf("timer_resolution_note: CLOCK_UPTIME_RAW ticks are ~41.67 ns; timed passes add ~30-60 timer reads per address\n");

    // --- Phase B: lookup locality on recorded corpus features ---
    printf("recorded state_features=%zu prevtag_features=%zu\n", rec_state_n, rec_trans_n);
    size_t N = rec_state_n;
    size_t *shuf = malloc(N * sizeof(size_t));
    for (size_t i = 0; i < N; i++) shuf[i] = i;
    for (size_t i = N - 1; i > 0; i--) { size_t j = rnd() % (i + 1); size_t x = shuf[i]; shuf[i] = shuf[j]; shuf[j] = x; }
    size_t hotN = 0;
    while (hotN < N && rec_state_addr[hotN] < 50) hotN++;      // features of the first 50 addresses
    uint64_t h, c;
    for (int rep = 0; rep < 3; rep++) {
        double t;
        t = lookup_run(crf->state_features, rec_state, N, NULL, 0, &h, &c);
        printf("B1 corpus-order (all %zu): %.1f ns/lookup hits=%llu\n", N, t / c, (unsigned long long)h);
        t = lookup_run(crf->state_features, rec_state, N, shuf, 0, &h, &c);
        printf("B2 shuffled (no token locality): %.1f ns/lookup\n", t / c);
        t = lookup_run(crf->state_features, rec_state, N, NULL, 1, &h, &c);
        printf("B3 each lookup done twice back-to-back: %.1f ns/lookup (mean of 1st + 2nd)\n", t / c);
        uint64_t total = 0; double tt = 0;
        while (total < N) {                                     // hot set: first 50 addresses' features, repeated
            tt += lookup_run(crf->state_features, rec_state, hotN, NULL, 0, &h, &c); total += c;
        }
        printf("B4 hot working set (first 50 addrs, %zu features repeated): %.1f ns/lookup\n", hotN, tt / total);
    }
    // trie step statistics over the recorded features
    uint64_t st = 0, bad = 0; uint32_t r;
    size_t sample = N < 2000000 ? N : 2000000;
    for (size_t i = 0; i < sample; i++) {
        st += trie_steps(crf->state_features, rec_state[i], &r);
        if (r != trie_get(crf->state_features, rec_state[i])) bad++;
    }
    printf("trie steps per state-feature lookup: mean=%.2f over %zu (replica mismatches vs trie_get: %llu)\n", (double)st / sample, sample, (unsigned long long)bad);
    printf("state_features trie: nodes=%zu tail_bytes=%zu data=%zu | trans trie nodes=%zu\n",
           (size_t)crf->state_features->nodes->n, (size_t)crf->state_features->tail->n, (size_t)crf->state_features->data->n,
           (size_t)crf->state_trans_features->nodes->n);
    size_t total_len = 0; for (size_t i = 0; i < sample; i++) total_len += strlen(rec_state[i]);
    printf("mean state feature length=%.1f bytes\n", (double)total_len / sample);
    // hit vs miss lookup cost and step distributions
    {
        char **hk = malloc(N * sizeof(char *)), **mk = malloc(N * sizeof(char *)); size_t nh = 0, nm = 0;
        uint8_t *stepsH = malloc(N), *stepsM = malloc(N); uint32_t id;
        size_t hist_n = 0;
        for (size_t i = 0; i < N; i++) {
            uint32_t rr; uint64_t stp = trie_steps(crf->state_features, rec_state[i], &rr);
            if (stp > 255) stp = 255;
            if (trie_get_data(crf->state_features, rec_state[i], &id)) { hk[nh] = rec_state[i]; stepsH[nh++] = stp; }
            else { mk[nm] = rec_state[i]; stepsM[nm++] = stp; }
            hist_n++;
        }
        printf("hit/miss split of %zu recorded state lookups: hits=%zu (%.1f%%) misses=%zu (%.1f%%)\n", N, nh, 100.0 * nh / N, nm, 100.0 * nm / N);
        for (int rep = 0; rep < 3; rep++) {
            uint64_t hh, cc; double th = lookup_run(crf->state_features, hk, nh, NULL, 0, &hh, &cc);
            double tm = lookup_run(crf->state_features, mk, nm, NULL, 0, &hh, &cc);
            printf("B5 rep%d: hit-keys %.1f ns/lookup (corpus order within hits) | miss-keys %.1f ns/lookup\n", rep, th / nh, tm / nm);
        }
        for (int which = 0; which < 2; which++) {
            uint8_t *v = which ? stepsM : stepsH; size_t m = which ? nm : nh;
            uint32_t cnt[256] = {0}; uint64_t sum = 0;
            for (size_t i = 0; i < m; i++) { cnt[v[i]]++; sum += v[i]; }
            size_t acc2 = 0; int p50 = -1, p90 = -1, p99 = -1, mx = 0;
            for (int k = 0; k < 256; k++) { acc2 += cnt[k]; if (cnt[k]) mx = k;
                if (p50 < 0 && acc2 >= m * 0.50) p50 = k; if (p90 < 0 && acc2 >= m * 0.90) p90 = k; if (p99 < 0 && acc2 >= m * 0.99) p99 = k; }
            printf("steps %s: n=%zu mean=%.2f p50=%d p90=%d p99=%d max=%d | histogram(steps:count) ", which ? "MISS" : "HIT", m, (double)sum / m, p50, p90, p99, mx);
            for (int k = 1; k < 256; k++) if (cnt[k] && (k <= 40 || k % 10 == 0)) printf("%d:%u ", k, cnt[k]);
            printf("\n");
        }
    }
    libpostal_teardown_parser(); libpostal_teardown();
    return 0;
}
