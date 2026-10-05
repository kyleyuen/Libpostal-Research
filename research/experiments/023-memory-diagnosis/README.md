# Experiment 023 — Why the portable build (022 `pt`) uses more memory than `l4`

Date/time/timezone: 2026-10-01 ~11:50-12:10 PDT (diagnosis only; no source change, no new build)
Status: **diagnosed.** Cause: ~1.1 GB of load-time temporaries are freed, and the macOS system allocator keeps ~665 MB of those freed large blocks
as dirty memory charged to the process; mimalloc gives them back. Fix not yet implemented.

## Evidence

1. `raw/vmmap_pt.txt` vs `raw/vmmap_l4.txt` (steady state, 16 s in): `pt` has **MALLOC_LARGE (empty) 665.3 MB virtual, 573.6 MB dirty + 91.7 MB
   compressed** — freed large blocks still held by the allocator — plus 2.0 GB of live large blocks; `l4` (mimalloc) has no such regions.
   Footprint 3,029 MB vs 2,250 MB.
2. `raw/vmmap_pt_full.txt`: the empty regions are 10 blocks (128.0+40.9, 128.0+128.0+94.1, 54.5, 57.2+15.3+15.3, 4.0 MB) interleaved with live tables.
3. `raw/malloc_history_all.txt` / `raw/large_alloc_events.txt` (MallocStackLogging, live allocations only — this mode does not record frees): live
   allocations >= 10 MB sum to ~2.05 GB, all made during `libpostal_setup_parser` (weights, integer/string tables, role-id table, phrase and postcode
   tries, postcode graph 466.5 MB). The empty regions are therefore memory freed during load.
4. Source (021 `l4` sources): three load-time temporaries are built and freed — the two feature tries after conversion (`crf.c` `crf_convert_trie`),
   the vocabulary trie (`address_parser.c` `trie_destroy(vocab)`), and the two full feature dictionaries after the integer split
   (`crf.c` `fd_destroy(old[0]); fd_destroy(old[1])`).
5. `raw/dict_sizes_pt.txt` / `raw/dict_sizes_s0.txt` (`CRF_DICT_VERBOSE=1`): freed at load in `pt` — tries 322.0 + 53.9 + 94.6 MB, full dictionaries
   540.7 + 89.7 MB (~1.1 GB); in `s0` (load 0.75) the dictionaries are 307.3 + 47.3 MB. The full dictionaries are sized by the load factor, which is
   why the system-allocator gap grew from ~130 MB (s0 vs s2) to ~780 MB (pt vs l4).

## Conclusion

Not a leak and not extra live data: the live tables are the same as `l4`'s. The excess is freed load-time memory that the macOS system allocator
retains. Whether glibc (Linux) or the Windows heap retain it the same way is UNVERIFIED.

## Fix options (not implemented)

A. Return freed memory to the OS after load: one call per platform (`malloc_zone_pressure_relief` on macOS, `malloc_trim` on glibc, `HeapCompact` /
   `_heapmin` on Windows). Smallest change; steady memory should then match l4's, but peak still includes temporaries coexisting during load.
B. Allocate the large dictionary arrays with mmap/munmap (VirtualAlloc/VirtualFree on Windows) so frees always go back to the OS, independent of the
   allocator.
C. Remove the biggest temporary: build the split integer/string tables directly from the trie instead of building the full dictionary first and
   splitting it (630 MB never allocated). Lowers peak on every allocator, but is a larger code change in the exactness-critical build path.
