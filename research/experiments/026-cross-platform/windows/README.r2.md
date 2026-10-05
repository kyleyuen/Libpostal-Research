# Windows x64 track — compile evidence only

2026-10-01, macOS arm64 host. **No Windows machine or VM was available, so none of these PE binaries has been executed.** There is no Windows output, throughput, thread-count, setup-time, or memory result. Cross-compilation must not be cited as a Windows speedup.

## Source and build identity

- Original: libpostal commit `25099c506612b34b23b1bfe286ca6321fcf06f35`, exported to `upstream/`. Its documented `windows/configure.ac` replaced the top-level `configure.ac` in this copied tree.
- Candidate: the same exported commit under `candidate/`, overlaid with every frozen 021 `l4` source (`sources/`) and one Windows-specific change to `feature_dict.h`. All other `l4` copies are byte-identical to `research/final/021-l4/sources/` (`raw/commands/windows_compare_l4_sources.*`); the final compiled candidate copy is identical to this track's source folder (`windows_compare_candidate_overlay_r1.*`).
- Candidate switches are exactly the 021 `l4` EXP list in `configure_candidate.sh`; `-O2` comes from upstream `src/Makefile.am`, scanner `-O0`. This x64 build has no ARM NEON execution and no Mac PGO/LTO profile. It is a portable-source compile, not the measured Mac binary.
- Source changes and new files are in `port.patch`. `src.Makefile.am` adds the Windows helper to the library. The generated `SHA256SUMS` covers sources, binaries, harnesses, model manifest and inputs; `SOURCE_COMMITS.txt` records source commits. The model file hashes to use on a Windows host are listed in `profiling/manifest.json`; no Windows model was loaded here.
- Compiler: Homebrew `mingw-w64 14.0.0_3`, `x86_64-w64-mingw32-gcc 16.2.0`. Install log: `raw/commands/windows_brew_install_mingw.*`. The DLLs and harness executables inspect as PE32+ x86-64; import lists are in `windows_inspect_binaries_r1.stdout`. `windows_verify_vm_symbols.stdout` confirms the r2 DLL contains the helper functions and imports the required WinAPI calls.

## Build and validation record

| Attempt | Exit | Result |
| --- | ---: | --- |
| `windows_bootstrap_original`, `windows_configure_original`, `windows_build_original` | 0 each | Pinned original `libpostal-1.dll` and static archive cross-built. |
| `windows_build_candidate` | 2 | `windows.h` in a library header collided with libpostal's `WORD` token macro. Failure retained. |
| `windows_bootstrap_candidate_r1`, `windows_configure_candidate_r1`, `windows_build_candidate_r1` | 0 each | WinAPI moved to `win_vm.c`, candidate DLL and static archive cross-built. The resulting `artifacts/l4/` snapshot has a reservation-size defect described below and is retained only as failed history. |
| `windows_overlay_candidate_r2`, `windows_build_candidate_r2`, `windows_stage_candidate_r2` | 0 each | Rounded the reservation size to 64 KiB, rebuilt the helper and DLL, and staged the corrected compile-only candidate in `artifacts/l4_r2/`. |
| `windows_compile_benchmark_r1`, `windows_compile_dumper`, `windows_link_*` | 0 each | Original/candidate benchmark and ordered-output dumper PE executables linked. The corrected `l4_r2` DLL was then linked again into `bench_ext_win_r2.exe` and `dump_outputs_win_r2.exe` (`windows_link_candidate_*_r2`), both exit 0. |
| `windows_configure_mimalloc`, `windows_build_mimalloc` | 0 each | Mimalloc 3.5.3 cross-built as a separate diagnostic object, not linked into either DLL. |

Each named attempt has a `.meta` file with exact argv, cwd, local timestamps and exit code, plus `.stdout` and `.stderr` under `../raw/commands/`. Two initial shell invocations were not captured by that wrapper: executing its nonexecutable file directly returned 126, then using a relative wrapper path from inside `upstream/` returned 127. Both were command setup mistakes and changed no source or build output.

## Windows arena adaptation

The 021 dictionary maps an almost 4 GiB sparse arena, writes model-derived key bytes, then unmaps the unused tail. MinGW-w64 lacks `sys/mman.h`. The copied `feature_dict.h` uses `fd_win_reserve` to reserve the virtual range, `fd_win_commit` to commit 64 KiB-aligned prefixes before **every** `fd_insert` write and before the `fd_int_split` copy loop, and `fd_win_release` on teardown. The r1 helper could reserve only 16 KiB for a small split arena while trying to commit 64 KiB; r2 rounds the reservation itself to 64 KiB first. The main `0xFFFFFFF0` byte request rounds safely to 4 GiB on x64. Allocation failure rejects the dictionary build and retains the original trie/older dictionary fallback. The unused tail remains reserved *address space* until teardown, but is never committed; normal `VirtualFree(MEM_RELEASE)` cannot release just a tail of a reservation. This preserves key offsets and the original lookup algorithm. It has compile proof only; runtime fallback and memory behavior remain unverified.

## Allocator limitation

The Mac 021 memory result depends on mimalloc. A MinGW cross-build of mimalloc tag `v3.5.3` produced `mimalloc.obj`, but `nm` shows `mi_malloc`/`mi_free` and **no** `malloc`/`free` override exports. Both paired DLLs import the UCRT heap (see `windows_inspect_binaries_r1.stdout`). Merely linking this object would not prove that libpostal uses mimalloc. Mimalloc's bundled Windows instructions specify a redirect DLL/import-order route for process-wide dynamic interposition; that has not been built or tested here. Thus even if the candidate runs, its no-memory-increase gate is open until native memory measurement and allocator verification.

## Rebuild on a macOS host with MinGW-w64

Use new output directories rather than overwriting this record. From the workspace root, replace `rebuild-original` and `rebuild-l4` with your chosen unused names:

```sh
W=research/experiments/026-cross-platform/windows
mkdir -p "$W/rebuild-original" "$W/rebuild-l4"
git -C libpostal archive 25099c506612b34b23b1bfe286ca6321fcf06f35 | tar -xf - -C "$W/rebuild-original"
git -C libpostal archive 25099c506612b34b23b1bfe286ca6321fcf06f35 | tar -xf - -C "$W/rebuild-l4"
cp "$W/rebuild-original/windows/configure.ac" "$W/rebuild-original/configure.ac"
cp "$W/rebuild-l4/windows/configure.ac" "$W/rebuild-l4/configure.ac"
cp "$W/sources/"* "$W/rebuild-l4/src/"
cp "$W/src.Makefile.am" "$W/rebuild-l4/src/Makefile.am"
(cd "$W/rebuild-original" && ./bootstrap.sh && ./configure --host=x86_64-w64-mingw32 --disable-data-download --datadir=C:/libpostal-profile && make -C src -j4 libpostal.la)
(cd "$W/rebuild-l4" && ./bootstrap.sh && bash ../configure_candidate.sh && make -C src -j4 libpostal.la)
```

Build the paired harnesses with `x86_64-w64-mingw32-gcc -std=gnu2x -O2 -Ilibpostal/src -c windows/bench_ext_win.c` (and `dump_outputs_win.c`), then link each object against the matching build's `src/.libs/libpostal.dll.a`; add `-lpsapi -lm` to the benchmark link. The exact completed commands are in `raw/commands/windows_compile_*` and `windows_link_*`.

## Native Windows acceptance still required

The staged `artifacts/original/` and **`artifacts/l4_r2/`** folders contain the paired DLLs and harnesses. Use `original/bench_ext_win_r1.exe` and `l4_r2/bench_ext_win_r2.exe` for the same-source benchmark, and `original/dump_outputs_win.exe` and `l4_r2/dump_outputs_win_r2.exe` for output checks. The earlier `artifacts/l4/` is retained as a failed attempt; do not use it for native validation. The configured model directory is `C:/libpostal-profile/libpostal`; copy the **same** model files listed in `profiling/manifest.json` there and check their hashes. On one Windows x64 machine, with AC power and no competing benchmark:

1. Run the paired dumpers named above on `profiling/golden-v1/all.txt` and `profiling/golden-v2-novel/novel.txt`. Compare ordered bytes between original and candidate. Use `research/experiments/015-integer-keys/tools/compare_frozen.py` against each frozen `baseline-outputs.jsonl`; include a deliberately altered output as a negative control. Run targeted API and fuzz checks as well.
2. Confirm one parsing thread through source inspection and repeated process-thread sampling while the candidate parses. The paired benchmarks sample their own thread counts after setup and each pass; an external continuous sampler is still needed for the strict gate.
3. Alternate original/candidate order for at least five trials on the same corpus and machine. Give each trial at least ten seconds, with the same different-corpus warmup. Preserve every pass. The benchmark reports cold pass, warm passes, per-country rates, setup, process CPU time, peak/steady working set and peak/steady private commit. Compare both memory metrics, and keep golden-v1 and novel timings separate.
4. Verify which allocator actually serves the DLL's allocations. If Windows memory rises above the original, the candidate fails regardless of speed. No 3.5× or 4.3× Windows claim is supported before these checks.
