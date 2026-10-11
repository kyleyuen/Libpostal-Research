# 201 paired token-storage diagnostic source

Status: source-only handoff for the benchmark owner. No compilation, parser call,
model load, or test was performed here.

## Frozen input and build modes

source-token-storage-v1/ is based on the immutable experiment 199 diagnostic
derivative research/analysis/parallel-exploration-2026-10-07/derivatives/199/source-v8/
(source-manifest SHA-256
7d47969a4396c18b5f520ecf948f217212c87f9eceb7edc2afccdede0218b4ff).
The two diagnostic builds must use this identical tree, common 119/CPI switches,
EXP_REUSE, EXP_FAST_APPEND, EXP185_DIAG, and the same CPI185 v9 base archive at
research/experiments/185-diagnostic-instrumentation/artifacts/build-20261007-1625-pattern-summary-fix/cpi/diagnostic/libpostal.a
(SHA-256 98275eb29cb6378632fc02f923b90e3c1505f191be696e0452cb19d57b2c69dc).
The only mode difference is EXP_TOKEN_SLICES: omit it for the ablation control
and define it for the 199 candidate. The control is the 199-source ablation with
the feature disabled, not a byte-for-byte 119 source build.

Compile and link the derivative address_parser.c, tokens.c, string_utils.c,
and diag.c; pair them with the copied tokens.h and diagnostic headers. Keep
all other CPI185 archive objects and build/link inputs identical. The exact source
manifest and patch are adjacent to this handoff.

## Added counters

- Existing token copy calls/payload bytes; added copy terminator stores, view
  records/payload bytes/NUL stores, and strict-gap eligibility/fallback counts
  in both modes (opportunity counts in the control, actual choice counts in 199).
- Exact tracked char_array realloc growth calls/requested bytes for token
  payload and normalized-text buffers. Tracking is scoped to each buffer while
  its token or normalized append runs and requires the common EXP_FAST_APPEND
  build switch.
- Offset-vector and token-record-vector growth calls/requested bytes at their
  respective push sites. A single push can grow each vector at most once.
- Run-wide maxima for used length and retained capacity of token payload bytes,
  offset entries, token records, and reused normalized text. The token payload
  used length includes one terminator per copied token; the normalized buffer
  length includes its NUL.
- Normalized-text copy calls, payload bytes, and NUL stores, to show this
  per-address copy remains common to both modes.

Copy calls/bytes do not mean heap allocations. The hooks do not count allocator
metadata, allocation cost outside these instrumented growth sites, or capacity
of unrelated parser buffers. The 185 diagnostic allocation counters remain
partial for token and string utility paths; use these direct growth counters for
the scoped arrays only.

## Source invariant review

The candidate guard skips separator/ignorable scanner records and permits views
only when each retained token begins strictly after the previous token end.
Touching/overlap selects the original copied-token path before any NUL store.
The write at each retained token end is inside the copied rs_str scratch: an
inter-token gap is available by the strict-gap test, while a final token ending
at the address length rewrites its existing terminal NUL. Parser consumers
request token strings through tokenized_string_get_token; context-fill's
normalizer receives the base pointer plus the scanner's bounded offset/length
record. The next parse can overwrite rs_str only after response strings have
been copied into the response block.

No concrete source-level lifetime/fallback defect was found in this review.
These invariants are not a universal runtime proof. Include structural
calibration inputs for separated retained words, touching/overlapping spans,
separator and ignorable spans between retained words, a final token at the
buffer terminator, and long gaps; confirm the scanner actually emits those
shapes before interpreting their counters. Exact outputs and single-thread
behavior remain benchmark gates.
