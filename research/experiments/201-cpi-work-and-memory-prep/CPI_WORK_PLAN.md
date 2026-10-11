# CPI remaining work plan: attribute token-span storage

## Scope and current evidence

This is a source/document review plan only. No AC power is available; no build,
model load, parser, or test was run. All 13 workloads are already observed, but
none are proof of behavior on every input. Promotion still requires exact
ordered output, one parsing thread, peak and steady footprint no higher than
the pinned original, and faster uninstrumented execution.

Experiment 187 recorded 4,039,344 physical searches and 404,890 later repeated
requests across 128,540 addresses. No repeated group had complete consumer
coverage; the 546,206 CPI memo reuses and 4,317,770 supplied feature IDs are
already existing work. Do not add a general lookup cache from these counts.
The trace is useful context, not a safe removal list (`187-cpi-reuse-flow/REPORT.md:5-15`,
`METRICS.md:27-55`).

Experiment 199 provides the only bounded candidate selected here: retain the
existing `EXP_TOKEN_SLICES` path and attribute its incremental token-storage
work against CPI with matched instrumentation and build inputs. In CPI,
`tokenized_string_add_token` calls `cstring_array_add_string_len` for each
retained token (`119-component-phrase-ids/sources/tokens.c:33-40`). In 199,
`address_parser_token_spans_are_separated` allows a view only when retained
spans have strict gaps (`199-token-span-storage/sources/address_parser.c:2410-2420`);
`tokenized_string_add_token_view` writes a terminator and pushes the existing
offset/length/type record (`sources/tokens.c:56-59`). Its accessor resolves a
view from `self->str` (`sources/tokens.c:86-92`). On touching or overlapping
spans, parsing keeps the original copy path (`sources/address_parser.c:2508-2535`).

This is a token-payload copy candidate, not a zero-copy parser. CPI's
`cstring_array_add_string_len` pushes one offset and appends token bytes plus a
NUL to its packed `char_array` (`199-token-span-storage/sources/string_utils.c:1241-1262`);
it does **not** allocate once per token. The packed buffer copies with
`memcpy` under `EXP_FAST_APPEND` (`sources/string_utils.c:950-977`); growth is
capacity-driven and amortized. 199 avoids those eligible token payload copies,
but still pushes token metadata and writes a NUL into the view buffer.

Both CPI and 199 still copy normalized bytes into the reusable `rs_str` buffer at
`address_parser_parse` (`sources/address_parser.c:2499-2505`; CPI source
`119-component-phrase-ids/sources/address_parser.c:2475-2482`). CPI's
`cstring_array` combines token bytes in a char array with a separate offsets
vector (`libpostal/src/string_utils.h:183-202`); clear resets used lengths
without releasing capacity (`199-token-span-storage/sources/string_utils.c:1229-1238`).
In 199's `EXP_REUSE` path, `rs_ready` still creates and retains the `cstring_array`
at setup, and parsing clears rather than removes it (`sources/address_parser.c:2452-2465,
2499-2505`). Thus the view path retains container objects and existing capacities
even when it stores no token payloads there. Byte-copy reduction alone is
insufficient: measure retained capacity and growth as well.

The 199 diagnostic screen reported 94.4% eligible spans on development and
golden-v1 (12,139 eligible / 724 fallback on golden); its counters were
candidate-only, so they do not quantify bytes saved against CPI. Its allocator-
only mimalloc v2 workload peak and steady-suite results were below O000, but CPI
also uses mimalloc and the builds were not a source-matched 199-versus-CPI copy
ablation. The separate ctypes API harness linked candidate archives into shared
libraries while O000 used its preserved dylib, so its higher candidate peak is
profile-mismatched. None of these results isolates an incremental 199 benefit
over CPI or supports a blanket memory claim.

## Bounded next candidate

Build one paired diagnostic comparison from the pinned CPI source and the
existing 199 source. The sole functional difference is `EXP_TOKEN_SLICES`;
compile the same instrumentation into both, with the same compiler flags,
scanner object, allocator, model, and input order. Keep `rs_str` and all parser
semantics unchanged. Instrument only `address_parser.c`, `tokens.c`, and the
container growth points in `string_utils.c` / vector helpers needed for actual
token-storage capacity. Reuse 199's source snapshot; do not introduce another
cache, token abstraction, normalization path, or public API change.

The separate `tokenized_string_new_from_str_view_size` constructor path is not a candidate here: it is only selected outside `EXP_REUSE` (`address_parser.c:2506-2516`), while the current CPI/199 build enables `EXP_REUSE` and uses `rs_ts`/`rs_str` instead (`address_parser.c:2491-2505`). A constructor-allocation draft would not change the measured CPI runtime; it is intentionally excluded.

Record matched per-address and aggregate counters with explicit denominators:

- Retained-token copy calls and payload bytes at
  `tokenized_string_add_token` / `cstring_array_add_string_len`, including
  terminator bytes; do not label each call as a heap allocation. Count view
  records and NUL writes at `tokenized_string_add_token_view`, and retain
  separate counters for vector pushes versus actual capacity-growth events.
- Eligible addresses/tokens, unsafe-span fallbacks, and fallback copy calls and
  bytes; compare views and copies over all retained tokens, not just eligible
  addresses.
- `rs_str` normalized payload-copy bytes and terminators, to show this common
  full-address copy remains unchanged.
- Token `cstring_array.str` used/capacity (`n`/`m`), offset-vector used/capacity,
  token-vector used/capacity, and `rs_str` used/capacity at each parse high-water;
  count growths and requested allocation bytes. Report retained maximum capacity
  separately from per-address use. Existing container capacities persist after
  `clear`, so allocation-call counts alone can hide their memory cost.
- Existing CPI whole-pipeline counters and exact output/thread checks, as
  cross-checks; do not treat 187's partial consumer links as proof of eliminated
  queries.

## Lifetime and fallback invariants

Token offsets and lengths come from the unchanged scanner on the normalized
string. The 199 guard skips separator/ignorable tokens and accepts only strictly
separated retained spans before any terminator write. Every consumer must use
`tokenized_string_get_token` while the current synchronous parse owns `rs_str`;
no view pointer may survive the parse or be retained across a later parse.
Response construction copies output bytes into its response block before the
scratch is reused (`address_parser.c:2424-2437, 2660-2709`). Preserve token
order, token type, separator bookkeeping, fallback behavior, and response
strings. Keep the `normalized` to `rs_str` copy in this comparison: removing it
would change ownership and could affect the single-phrase shortcut that still
reads `normalized` (`address_parser.c:2565-2615`).

## Decision rule and limits

If matched counters show token payload copies disappear on eligible spans but
offset/capacity growth, terminator writes, fallbacks, or retained high-water
capacity erase the byte/allocation benefit, stop this line. If work and capacity
are lower, that only justifies a separate release benchmark; it is not a speed
prediction. Require byte-identical output and one-thread checks on the existing
sets plus structural edge cases, then compare peak/steady memory and uninstrumented
speed against the pinned original. Finite workload results are not universal
proof.
