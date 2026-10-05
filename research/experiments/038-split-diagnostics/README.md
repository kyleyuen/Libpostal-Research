# Experiment 038 — is phrase search miss-bound or instruction-bound, and where does feature generation's time go? (diagnostic)

Date/time/timezone: created 2026-10-02T16:45:38-0700; builds 16:45–16:47, measurements 2026-10-02T16:47:30-0700 to 2026-10-02T16:51:17-0700, America/Los_Angeles.
Status: complete (measurement only)

## Builds (experiment copy of `l3` sources; `raw/base_sources.sha256`)
- `dup`: `l3` + `EXP_DUP_SEARCH` — after the real searches, context fill repeats the component and postal-code trie searches into a discarded scratch array. Output byte-identical to the original on golden-v1 (sha 45574ee2…, `raw/dup_v1.jsonl`).
- `fs`: `l3` + `EXP_STAGE_STATS` + `EXP_FS_STATS` (eight timers inside `address_parser_features`). First attempt failed to compile (last marker landed in `ap_response_block`; `raw/build_fs_attempt1_marker.stderr`), fixed and rebuilt. Both with the `l3` PGO/LTO/mimalloc procedure.

## Results (`tools/run.sh`, sequential, AC-gated; all steps exit 0)

**Repeat-search test** (instructions/cycles per address, 2 vs 102 passes, 3 reps): `l3` 72,559 instructions / 31,285 cycles; `dup` 74,598 / 32,237. The extra component+postal search costs **+2,039 instructions and +952 cycles (~0.24 µs)** when its data is already cached, against ≈1.45 µs (≈5,800 cycles) for the first search of each address in 033's timers. The first search is therefore dominated by first-touch memory cost (cache and likely TLB misses in the 8.2M/7.5M-node tries), not by instruction count; the repeat runs at IPC ≈2.1.

**Feature-generation sections** (µs/address, 3 × 50 passes, development split; `raw/fs_r*.stdout`; features total 2.15):

| section | µs |
|---|---:|
| setup | 0.062 |
| address-dictionary phrase block | 0.086 |
| component-phrase block (incl. "commonly …") | 0.334 |
| postcode block (incl. postcode-context graph lookups) | 0.297 |
| word block (bias, prefix/suffix, n-grams, word, first word) | 0.377 |
| transition word | 0.067 |
| prev-word block | 0.203 |
| **next-word block** (next word, word+next word, first-word right context) | **0.679** |

Other stages in the same runs: context fill 2.15, lookups 1.16, accumulation 1.08, Viterbi/rest 1.00 µs.

## Interpretation
The next-word block is where each upcoming token's vocabulary/phrase data is first touched (later uses hit the per-address memos), and the postcode block includes the postcode-context graph. Together with the repeat-search result, the front half is mainly first-touch memory cost across several large model structures; reducing instruction counts or overlapping walks (037) does not address it. The lever is touching fewer distinct cache lines/pages per address (denser structures for the hot paths), within the steady-memory margin. TLB involvement is UNVERIFIED (no TLB counters).
