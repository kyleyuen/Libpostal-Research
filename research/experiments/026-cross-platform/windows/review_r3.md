# Windows r2 source review and r3 repair

Status: r3 source and paired harnesses cross-built successfully on macOS; native execution pending.

## Concrete finding

In `feature_dict.h`, the r2 `fd_int_split` path reserved its new arena and then called `fd_arena_ensure(d, d->arena_map)`. Here `arena_map` is the old dictionary's entire used arena rounded to 16 KiB, while the split writes only the keys that remain in the string dictionary. The call therefore committed every reserved page up front. On Windows, `MEM_COMMIT` charges process commit even before a page is accessed, whereas `MEM_RESERVE` does not. This can inflate peak commit and can make an otherwise viable split fall back after an unnecessary allocation failure.

The r3 source reserves the arena, then calls `fd_arena_ensure(d, off + entry)` immediately before each kept-key `memcpy`. The helper commits 64 KiB prefixes. This retains the same offsets and copied bytes. If commit fails, `fd_destroy(d)` releases the entire reservation and `fd_int_split` returns `NULL`; `crf_convert_int_keys` keeps the prior dictionaries. The 4 GiB initial dictionary uses the same per-insert commit path as r2.

`raw/commands/windows_overlay_candidate_r3.*` and `windows_build_candidate_r3.*` record the exact incremental build. It recompiled `address_parser.c` and `crf.c`, linked `libpostal-1.dll`, and exited 0. The r3 DLL and archive are in `artifacts/l4_r3/`; the matching benchmark and dumper links each exited 0. Compilation does not verify that Windows will reserve the address range, commit pages, preserve outputs, or meet the memory gate at runtime.

## Other reviewed paths

- `fd_win_reserve` rounds the requested size to 64 KiB, so a split arena below 64 KiB can still hold the first 64 KiB commit; the main `0xFFFFFFF0` request rounds to 4 GiB on x64 without `size_t` overflow.
- `fd_destroy` calls `VirtualFree(base, 0, MEM_RELEASE)`, releasing the full reservation. The unused tail stays uncommitted until then because ordinary `MEM_RELEASE` cannot release a subset of one reservation.
- Both harness executables import their colocated `libpostal-1.dll`. The PE import table confirms UCRT heap imports; mimalloc was not linked or proven active.
- The benchmark reads the identical TSV before the timer, uses the same parser API and ordered addresses for both variants, reports QPC wall time and process CPU time, and samples working set/commit at the end. Its thread snapshots happen after passes; a native external sampler during parsing remains required. Native output and performance remain untested.
- The Homebrew MinGW cross-binaries use UCRT imports. A native MSYS2 rebuild should record its CRT choice; testing the staged PE binaries avoids silently mixing C runtimes.
- The source intentionally matches packaged 021 `l4` apart from the Windows arena adapter. Experiment 025 proves that 021's feature-print API aborts, and `EXP_AUDIT_FIXES` was not included in this compile. A separate repaired variant would need its own source identity and Windows validation.

Microsoft API references: [VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc), [VirtualFree](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree), and [PROCESS_MEMORY_COUNTERS_EX](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex).
