# 196 diagnostic source v2 — source-only correction

This is a fresh frozen source snapshot derived from `derivatives/196/source` after the v1 diagnostic counter was found to overcount. In `crf_context.c`, the loop reads one `trans[j]` scalar for each predecessor and now increments `viterbi_transition_scalar_loads_from_model_view` by 1 at that exact site. The earlier `viterbi_transition_cells_from_model_view` incremented by L inside the innermost predecessor loop and is retired as a read metric.

No release source or binary changed. No build, parser, or model run was performed for this revision. Existing v1 builds/captures and their source hashes remain preserved as historical evidence; v2 measurements are unavailable.
