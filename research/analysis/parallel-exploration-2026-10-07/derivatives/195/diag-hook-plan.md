# Diagnostic hook plan — 195 single context zero pass

Source-only derivative of the immutable 195 candidate over prototype181's 185 hooks. No build or parser run by implementation author.

Metrics: `reset_duplicate_zero_bytes_avoided` counts logical double elements omitted by the setter branch: `(T*L + T*L*L) * sizeof(double)`; `reset_duplicate_zero_passes_avoided` counts the two omitted matrix-zero calls when `clear_state_scores` is false. This is logical store volume, not observed DRAM traffic. Subsequent RESET_ALL writes remain and are not counted as avoided.
