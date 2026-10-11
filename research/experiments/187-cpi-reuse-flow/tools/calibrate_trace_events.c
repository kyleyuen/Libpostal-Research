#include "cpi/diag.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static unsigned char dict_a, dict_b;

int main(int argc, char **argv) {
    if (argc != 2) return 64;
    if (setenv("TRACE_JSONL", argv[1], 1) ||
        setenv("TRACE_CASE_ID", "runtime-hook-calibration", 1) ||
        setenv("TRACE_EXPECTED_ADDRESSES", "1", 1) ||
        setenv("TRACE_SOURCE_MANIFEST_SHA256", "0000000000000000000000000000000000000000000000000000000000000000", 1)) return 2;
    static const char address[] = "fixture";
    static const char key[] = "same-key";
    diag_address_begin_input(address, sizeof address - 1);
    diag_set_stage("calibration");
    diag_set_family("state_feature");
    diag_set_operation("feature_lookup");
    diag_set_site("calibration.physical");
    uint64_t first = diag_lookup_result(&dict_a, "feature_lookup", "calibration", "state_feature",
                                        key, sizeof key - 1, 1, "feature_id", 17);
    diag_consume(&dict_a, first, "calibration.consumer", "calibration", "state_row", "feature_id", 17);
    uint64_t reused = diag_reuse_linked(&dict_a, "feature_lookup", "calibration", "state_feature",
                                        key, sizeof key - 1, 1, "feature_id", 17,
                                        "fixture_pointer_memo", first);
    diag_consume(&dict_a, reused, "calibration.memo_consumer", "calibration", "state_row", "feature_id", 17);
    (void)diag_lookup_result(&dict_b, "feature_lookup", "calibration", "state_feature",
                             key, sizeof key - 1, 1, "feature_id", 19);
    (void)diag_lookup_result(&dict_a, "other_operation", "calibration", "state_feature",
                             key, sizeof key - 1, 1, "feature_id", 23);
    const char *old_site = diag_current_site();
    diag_set_site("calibration.precomputed_id");
    (void)diag_precomputed_id("feature_lookup", "setup_answer", "state_feature", 29, "fixture_setup_id");
    diag_set_site(old_site);
    uint64_t miss = diag_lookup_result(&dict_a, "word_vocab_lookup", "calibration", "word_vocab",
                                       "absent", 6, 0, "vocab_frequency", 0);
    diag_consume(&dict_a, miss, "calibration.vocab_miss_consumer", "calibration",
                 "vocab_frequency_return", "vocab_frequency", 0);
    diag_address_end();
    return diag_trace_finish() ? 3 : 0;
}
