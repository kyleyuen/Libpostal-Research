#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char dict_a, dict_b;

static void run_nested_labels(const char *out) {
    static const char key[] = "nested-label-key";
    diag_address_begin();
    diag_set_stage("context_fill");
    diag_set_family("state_feature");
    diag_set_operation("state_feature_lookup");
    const char *saved_stage = diag_current_stage();
    const char *saved_family = diag_current_family();
    const char *saved_operation = diag_current_operation();

    diag_set_stage("phrase_search");
    diag_set_family("address_phrase");
    diag_set_operation("phrase_tokens");
    if (strcmp(saved_stage, "context_fill") || strcmp(saved_family, "state_feature") ||
        strcmp(saved_operation, "state_feature_lookup")) exit(5);

    /* A second nested save and an address reset must not mutate saved values. */
    const char *nested_stage = diag_current_stage();
    const char *nested_family = diag_current_family();
    const char *nested_operation = diag_current_operation();
    diag_set_stage("crf_state_lookup");
    diag_set_family("model_row");
    diag_set_operation("row_lookup");
    if (strcmp(nested_stage, "phrase_search") || strcmp(nested_family, "address_phrase") ||
        strcmp(nested_operation, "phrase_tokens")) exit(6);
    diag_address_begin();
    if (strcmp(saved_stage, "context_fill") || strcmp(saved_family, "state_feature") ||
        strcmp(saved_operation, "state_feature_lookup")) exit(7);

    diag_set_stage(saved_stage);
    diag_set_family(saved_family);
    diag_set_operation(saved_operation);
    diag_lookup(&dict_a, diag_current_operation(), diag_current_stage(), diag_current_family(),
                key, sizeof key - 1, 1);
    diag_address_end();
    if (diag_write_metrics(out, 1, 0) != 0) exit(2);
}

static void run_label_too_long(const char *out) {
    char label[80];
    memset(label, 'x', sizeof label - 1);
    label[sizeof label - 1] = '\0';
    diag_address_begin();
    diag_set_operation(label);
    diag_address_end();
    if (diag_write_metrics(out, 1, 0) != -2) exit(8);
    puts("oversized diagnostic label rejected");
}

static void run_label_pool_full(const char *out) {
    char label[32];
    diag_address_begin();
    for (unsigned i = 0; i < 600; i++) {
        snprintf(label, sizeof label, "distinct-operation-%u", i);
        diag_set_operation(label);
    }
    diag_address_end();
    if (diag_write_metrics(out, 1, 0) != -2) exit(9);
    puts("diagnostic label pool exhaustion rejected");
}

static void run_basic(const char *out) {
    static const char key[] = "calibration-key";
    diag_address_begin();
    diag_set_stage("crf_state_lookup"); diag_set_family("state_feature"); diag_set_operation("state_feature_lookup");
    diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, sizeof key - 1, 0);
    diag_lookup(&dict_a, "state_feature_lookup", "feature_generation", "state_feature", key, sizeof key - 1, 1);
    diag_lookup(&dict_b, "state_feature_lookup", "crf_transition_lookup", "state_feature", key, sizeof key - 1, 1);
    diag_lookup(&dict_b, "state_feature_lookup", "crf_transition_lookup", "state_feature", key, sizeof key - 1, 1);
    diag_reuse(&dict_a, "state_feature_lookup", "context_fill", "state_feature", key, sizeof key - 1, 1);
    diag_address_end();

    /* Same key in the next address must start a fresh repeat identity set. */
    diag_address_begin();
    diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, sizeof key - 1, 1);
    diag_address_end();
    if (diag_write_metrics(out, 2, 0) != 0) exit(2);
}

static void run_query_overflow(const char *out) {
    char key[32];
    diag_address_begin();
    for (unsigned i = 0; i < 4096; i++) {
        int n = snprintf(key, sizeof key, "query-%u", i);
        diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, (size_t)n, 1);
    }
    int n = snprintf(key, sizeof key, "overflow-key");
    diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, (size_t)n, 1);
    diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, (size_t)n, 1);
    diag_address_end();
    if (diag_write_metrics(out, 1, 0) != 0) exit(2);
}

static void run_pattern_overflow(const char *out) {
    char key[32];
    diag_address_begin();
    for (unsigned i = 0; i < 129; i++) {
        int n = snprintf(key, sizeof key, "pattern-%u", i);
        diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, (size_t)n, 1);
        diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, (size_t)n, 1);
    }
    /* The 129th group is omitted; later observations must still enter the exact total. */
    int n = snprintf(key, sizeof key, "pattern-128");
    diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, (size_t)n, 1);
    diag_lookup(&dict_a, "state_feature_lookup", "crf_state_lookup", "state_feature", key, (size_t)n, 1);
    diag_address_end();
    if (diag_write_metrics(out, 1, 0) != 0) exit(2);
}

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "usage: %s basic|nested-labels|label-too-long|label-pool-full|query-overflow|pattern-overflow output.json\n", argv[0]); return 64; }
    if (!strcmp(argv[1], "basic")) run_basic(argv[2]);
    else if (!strcmp(argv[1], "nested-labels")) run_nested_labels(argv[2]);
    else if (!strcmp(argv[1], "label-too-long")) run_label_too_long(argv[2]);
    else if (!strcmp(argv[1], "label-pool-full")) run_label_pool_full(argv[2]);
    else if (!strcmp(argv[1], "query-overflow")) run_query_overflow(argv[2]);
    else if (!strcmp(argv[1], "pattern-overflow")) run_pattern_overflow(argv[2]);
    else return 64;
    return 0;
}
