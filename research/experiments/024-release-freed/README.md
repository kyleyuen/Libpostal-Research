# Experiment 024 — Fix option A: return freed load-time memory to the OS

Date/time/timezone: 2026-10-01 ~12:15-12:05 PDT
Status: **rejected (no effect on macOS).**

## Hypothesis and change

Experiment 023 found ~665 MB of freed load-time temporaries kept by the macOS system allocator. New switch `EXP_RELEASE_FREED` (019 candidate
`address_parser.c`, end of `address_parser_load`): after loading, call `malloc_zone_pressure_relief(NULL, 0)` on macOS, `malloc_trim(0)` on glibc,
`_heapmin()` on Windows. Build `pr` = 022 `pt` + this switch (`../022-portable-floor/tools/build_cc.sh pr clang none "-O2" "-O1" ""`, exit 0;
binary imports `_malloc_zone_pressure_relief`, `pt` does not).

## Result (AC, `raw/memory_pr_vs_pt.txt`)

| build | peak footprint | steady footprint | MALLOC_LARGE (empty) |
|---|---:|---:|---:|
| `pt` | 3.176 GB | 3,029 MB | 665.3 MB |
| `pr` | 3.175 GB | 3,029 MB | 665.3 MB |

No change: the macOS allocator (this macOS 26 system uses the xzone allocator; `_xzm_free` appears in profiles) does not release these blocks on a
pressure-relief call. Not verified for output or timed, since it was rejected on memory. Linux/Windows behaviour of `malloc_trim`/`_heapmin` untested.

## Decision

Rejected; the edit was reverted from the shared candidate source (the patched file is kept in `../022-portable-floor/artifacts/pr/src/`). Remaining options from 023: B (allocate the large tables directly with mmap/VirtualAlloc so frees always return to the OS) and C (never
build the 630 MB full dictionaries). Note: B only covers allocations our code makes; the 470 MB of tries are allocated by upstream trie code.
