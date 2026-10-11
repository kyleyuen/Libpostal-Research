#include <stdint.h>
#include "diag.h"

int main(void) {
    static int dictionary;
    static const unsigned char key[] = {'m', 'a', 'i', 'n', '\0', 's', 't'};
    static const char address[] = "fixture/address";
    diag_address_begin_input(address, sizeof address - 1);
    diag_set_stage("fixture_producer");
    diag_set_family("fixture_dictionary");
    diag_lookup(&dictionary, "fixture_exact_bytes", "fixture_producer", "fixture_dictionary", key, sizeof key, 1);
    diag_count("feature_keys_materialized", "feature_generation", "feature_key", 2);
    diag_count("feature_key_bytes_materialized", "feature_generation", "feature_key", 19);
    diag_count("model_rows", "crf_state_lookup", "state_feature", 1);
    diag_count("weights_accumulated", "crf_state_lookup", "state_feature", 3);
    diag_count("allocation_calls", "feature_generation", "parser_alloc", 1);
    diag_count("allocation_requested_bytes", "feature_generation", "parser_alloc", 64);
    diag_set_stage("fixture_consumer");
    diag_lookup(&dictionary, "fixture_exact_bytes", "fixture_consumer", "fixture_dictionary", key, sizeof key, 0);
    diag_reuse(&dictionary, "fixture_exact_bytes", "fixture_consumer", "fixture_dictionary", key, sizeof key, 1);
    diag_address_end();
    return diag_trace_finish() == 0 ? 0 : 1;
}
