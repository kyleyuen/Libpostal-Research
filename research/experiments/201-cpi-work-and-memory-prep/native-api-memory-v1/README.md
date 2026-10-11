# Native API memory harness (source-only handoff)

This versioned harness is prepared to compare native process footprint without
Python/ctypes in the measured process. It contains no parser result or memory
claim. Builds, model runs, thread checks, and comparisons are owned by the
benchmark worker.

## Fixture and API contract

`generate_fixtures.py` calls the existing `select_safe_cases` implementation
from `research/experiments/186-architecture-benchmark/tools/edge_api_dump.py`
on the frozen registry at `research/experiments/186-architecture-benchmark/edge-cases.json`.
It validates the pinned registry hash, requires the existing 78 selected cases,
and emits them in registry order into `fixtures.h`. It excludes the four
`safe_to_call: false` rows and includes the single explicit NULL-address case.

The fixture stores input and option UTF-8 bytes plus a terminating NUL. A
separate null flag preserves the difference between `NULL` and an empty string.
Embedded NUL bytes and their suffixes stay in the fixture; passing the pointer
to the C-string API therefore exposes the same prefix as the Python
`ctypes.c_char_p` path. This includes the registry's embedded-NUL input and
language/country options.

`api_memory_dump.c` calls the same setup functions once, parses all 78 cases in
the frozen order, destroys every non-NULL response, and tears down parser then
library state. Each stdout record includes case identity, input/options bytes,
response status, and component count. Following records serialize each ordered
label and component as byte length plus lowercase hex, so comparisons are
unambiguous at the byte level and do not depend on locale or JSON escaping.
Compile it once per library with the same harness source and original API
header (`research/experiments/186-architecture-benchmark/source/original/libpostal.h`).
The exact native outputs should be byte-compared to the 000 capture before
interpreting memory. The external runner should verify one thread per process.

## Frozen link inputs

| Role | Static archive | SHA-256 | Allocator link input |
|---|---|---|---|
| Original 000 | `research/experiments/000-baseline/artifacts/libpostal.a` | `30de6ef223346735ac3035654ff0d435e5e621dccc94dc77869807dbf146fe22` | Stock archive; do not add mimalloc |
| CPI matched profile | `research/experiments/119-component-phrase-ids/artifacts/cpi_201_matched_v1/libpostal.a` | `f7438a5f27bc2903bfdadea82ad3caf90778b0bc2b152fc2536c91c34d53a068` | `/opt/homebrew/opt/mimalloc/lib/mimalloc.o`, SHA-256 `03460b36edb0f104270109d33d96f8e7fc03c08d326cc5c2182c34b0a4900b82` |
| 199 token views | `research/experiments/199-token-span-storage/artifacts/release/libpostal.a` | `447e00b40cff2dbb2c4b388b8a844fd01bfc1610463f93432de3a2eb4960f576` | Same mimalloc object and link flags as matched CPI |

These are link inputs, not confirmation that any native harness was built.
The source-matched CPI artifact records O2/LTO, scanner O0, no PGO and the
canonical mimalloc object; the 199 archive is the candidate archive from the
same profile. Keep the original baseline stock-allocator build as the oracle.
Do not use the Python API peak as parser-only memory or replace the existing
release suite's peak/steady memory gate.

## Preparation checks and limits

The fixture generator was run only to materialize the C table (no compiler,
model setup, parser, or test was invoked). It selected 78 cases and excluded
four unsafe registry rows. Fixture and source hashes are in `manifest.json`.
This harness removes Python-host memory from the process measurement, but its
small C serializer is still part of each measured executable. Its identical
code and fixed registry make that overhead common across roles. Results remain
specific to this API harness and cannot establish parser-only allocation
attribution or universal output equivalence.
