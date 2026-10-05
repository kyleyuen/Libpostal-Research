# Experiment 046 — cross-stage early prefetch of lookup traffic

Date/time/timezone: created 2026-10-02T17:22:44-0700; screen 2026-10-02T17:23:35-0700 to 2026-10-02T17:25:17-0700, America/Los_Angeles.
Status: **rejected (slower)**

## Change
Switch `EXP_EARLY_PREFETCH` (`sources/crf.c`, experiment copy of `l3`): after each token's features are generated, hash its new string keys and prefetch their dictionary slots; for integer keys already resolved to a feature id, prefetch the row-index entry; otherwise prefetch the integer slot. Intended to overlap lookup misses with the next tokens' feature generation (043: lookups + accumulation ≈1.33 µs first-touch). Prefetches only; the lookup stage recomputes its hashes unchanged.

## Results
Build `ep` (`l3` PGO/LTO/mimalloc): output byte-identical to the original on golden-v1 (sha 45574ee2…). Screen (`tools/screen.sh`, sequential alternating, golden-v1, 20 passes, AC-gated; median warm pass): `l3` 7.752 / 7.723 / 7.775 / 7.754 µs, `ep` 8.392 / 8.429 / 8.430 / 8.417 µs → **≈8.5% slower** in every round.

## Decision
Rejected without full validation. This implementation also recomputed `fd_split` and the hash of every string key (≈31 per address) a second time, so the result applies to this implementation only and is not general evidence that earlier prefetching cannot help.
