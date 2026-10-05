# Experiment 148 — warm line profile of 122's integer bigram path (ipr) vs cpi: where does the extra emission cost go? (diagnostic; decides R1)

Date/time/timezone: created 2026-10-04T21:39:28-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete (diagnostic) — rule outcome: canon group 67% of the extra cost → R1 built (149)

## Declared rule (written before recording)

**Inputs.**
- Binaries: `122-int-pairs-retest/artifacts/ipr/bench_ext` and `119-component-phrase-ids/artifacts/cpi/bench_ext`, both existing and hash-recorded.
- Method (124/132): each binary runs `bench_ext` cycling the same 300 development addresses (warm), while xctrace's CPU Profiler records 10 s. Leaf PCs are resolved to their innermost inlined function with `atos -i`.
- Deltas (ipr − cpi) are grouped by function:
  - **canon:** `ap_canon_part`, `fd_canon`, `ap_vm_get`, the word-memo canon, and `fd_find_slot` reached from them;
  - **int machinery:** `crf_ik_push`, `fdi_*`, the integer batch resolution;
  - **string:** `feature_array_add`, `char_array_*`, `memmove`, `strlen`, `fd_lookup*` on strings.

**Decision.**
- If the **canon** group carries ≥ 50% of ipr's extra (non-negative) cost, a per-token unit cache can remove it. **Build R1**, keeping 122's batched, prefetched pair probe.
- Otherwise the integer machinery itself costs about as much as the string emission it replaces. **Stop the bigram card** and record why.

Caveat: interactive load (Chrome, WindowServer) was present. Sampling shares within a process are less sensitive to it than absolute times, and the load is recorded per run.

## Reproduction

- `tools/profile.sh`: cpi, ipr, then cpi again (`cpi2`, repeatability), each recorded with xctrace CPU Profiler for 10 s while `bench_ext` cycles `raw/dev300.tsv`.
  - Attempt 1 (4,000 passes) ended before the recording did and is invalid (`raw/attempt1/`).
  - Attempt 2 (60,000 passes) has ≈41.7k samples each (`raw/load.log`; load 3.8–4.0, swap 9.9 GB).
- Analysis: `tools/diff_profile.py` → `raw/diff_profile.stdout`, innermost inlined function via `atos -i`.

## Results (warm µs per address; ipr − mean(cpi, cpi2); cpi − cpi2 ≤ 0.01 for the functions below)

| Group | Δ | Main contributors |
|---|---:|---|
| string emission saved | ≈ −0.135 | `feature_array_add_nb` −0.046, `fj_copy` −0.029, `fd_lookup_hashed_from` −0.023, memmove −0.017, strlen −0.009 |
| canon added | ≈ +0.084 | `ap_canon_part` +0.033, `fd_find_slot` +0.025, `ap_vm_get` +0.007, memchr +0.006, `fd_eq` +0.006 |
| int machinery added | ≈ +0.042 | `fdi_lookup` +0.021, score loop +0.011, `fdi_hash` +0.004, `crf_ik_push` +0.003 |

Totals: ipr 3.577, cpi 3.580 / 3.636 µs warm. The net is ≈0, consistent with 122 and 147.

By the declared rule (canon ≥ 50% of the extra cost) R1 was built: experiment 149. Note on the outcome: in 149 the content-validated cache did **not** reduce the extra cost. The canon-group cost is therefore the memo path itself, not repeated canonicalization of the same strings.
