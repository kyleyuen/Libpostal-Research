# Diagnostic hook plan — 196 transition matrix view

Source-only derivative of the immutable 196 candidate over prototype181's 185 hooks. No build or parser run by implementation author.

Metrics: `transition_matrix_copy_calls_avoided` counts combined inference calls that skip the owned trans copy; `transition_matrix_copy_read_bytes_avoided` and `transition_matrix_copy_write_bytes_avoided` separately count `L*L*sizeof(double)` logical payload. They are not measured DRAM traffic. `viterbi_transition_matrix_model_view_calls` and `_owned_calls` identify selected source; `viterbi_transition_scalar_loads_from_model_view` counts one scalar model-view transition read at the innermost Viterbi predecessor loop. The earlier v1 metric incremented by label count L at each scalar load and is invalid as a read count. Standalone scoring still copies the owned matrix.
