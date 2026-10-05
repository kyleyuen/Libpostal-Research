# Linux ARM64 validation and portability work

This is the Linux track of experiment 026. All timed conclusions remain pending. The ARM64 runs below used Docker Desktop's Linux VM on the same Apple M4 Pro Mac, not a bare-metal Linux host or an x86-64 machine. The separate x86-64 functional check under `x86/` ran under emulation and cannot establish x86-64 speed.

## Environment and source identity

- Pinned upstream libpostal commit: `25099c506612b34b23b1bfe286ca6321fcf06f35`. The original `libpostal/` checkout had no tracked diff (`raw/commands/linux_tracked_source_clean_r1.*`), but its status listed generated and `.DS_Store` files as untracked (`linux_source_status.*`). `prepare_source.sh` copied those files while excluding `.git`; `linux/source/` is therefore a source copy, not a Git checkout. It was subsequently configured inside Ubuntu 24.04 with `--disable-data-download`; see `raw/commands/linux_configure.*`. The first tracked-diff command mistakenly targeted the copy and failed with exit 129 (`linux_tracked_source_clean.*`), preserved as an audit correction.
- Exact 021 `l4` source overlay: `research/final/021-l4/sources/`, unchanged. The Linux benchmark port in `bench_ext_linux.c` uses `CLOCK_MONOTONIC_RAW`, omits the macOS QoS call, and converts Linux `ru_maxrss` KiB to bytes. It does not change the parser.
- Docker Engine 29.2.0, Linux/aarch64 image `sha256:21c50d089e171658759e896d9f6cfefc16c8637e1a34b779b45f0a146346aab3`; Ubuntu clang 18.1.3, LinuxKit kernel 6.12.67, 12 VM CPUs, about 8.2 GB VM RAM and 1 GB swap. The container itself has no smaller cgroup memory limit (`raw/commands/linux_container_limits.*`). An unrelated Postgres container was present.
- The model was mounted read-only at `/model`; all seven data-file SHA-256 values in `raw/commands/linux_model_hashes.stdout` match `profiling/manifest.json`. Input hashes are in `raw/commands/linux_input_hashes.stdout`; 021 overlay source hashes are in `raw/commands/linux_source_hashes.stdout`, and final Linux harness/build script hashes including `build_dense.sh` are in `linux_source_hashes_final_r3.stdout`. Post-session binary, input, model, and resolved Docker-image identities are in `linux_post_timing_{hashes,model_hashes,image}.*`; these were captured **after**, not at the start of, the interrupted diagnostic.
- The preserved `000-baseline/artifacts/libpostal.a` is a macOS archive and cannot execute in Linux. `baseline_upstream` is a fresh native rebuild of the same pinned original source: library `-O2`, scanner `-O0`. `baseline` is a matched-compiler original control with scanner `-O1`. Neither baseline applies any `EXP_*` switches. The common `build.sh` printed its candidate `switches=` list in baseline `build_settings.txt` despite not passing those flags to the baseline compiler; the command logs show the actual flags. `candidate` applies that list including `EXP_LOADF=4`; `candidate_dense` applies it without `EXP_LOADF=4`. Both use library `-O2`, scanner `-O1`, without PGO/LTO. Thus no Linux number can be called the Mac `l4` build's 4.3× result.

## Build attempts and output checks

All exact `docker` commands, timestamps, stdout, stderr, and exit codes are under `raw/commands/linux_*` (relative to the experiment root). Builds and failures are retained:

1. `linux_build_baseline` failed because libpostal's local `features.h` shadowed glibc's `<features.h>` under `-I`; changed local header search to `-iquote`.
2. `linux_build_baseline_r1` compiled the large scanner object but exited 2 when a live edit to its shell script produced an unexpected EOF. `linux_finish_baseline` completed the archive/link without recompiling it. This was an orchestration error; both logs are preserved.
3. `linux_build_baseline_upstream`, `linux_build_candidate`, and `linux_build_candidate_dense` exited 0. The dense variant uses the 021 sources and switches except `EXP_LOADF=4`, returning the dictionary tables to the source's default 0.75 load factor. It is a distinct variant.
4. Mimalloc 3.5.3 was fetched from the [official release](https://github.com/microsoft/mimalloc/releases/tag/v3.5.3), tag commit `d4881d338125e1cb7c47ba4cfb398d6f7c0c8d45`, archive SHA-256 `3b4a15153a59905995f7070296ed604bb5ccc00cabb8b93446931aff77224d47`. It was built with Clang Release, `MI_OVERRIDE=ON`, and linked as `mimalloc.o` into the separate `candidate_mi3` binaries. `nm` shows `malloc` and `mi_malloc` at the same address (`raw/commands/linux_mi3_symbols.stdout`), proving the allocator override is linked. Ubuntu's separate mimalloc 2.1.2 trial used `LD_PRELOAD` and is identified as `candidate_mi` in memory records.

`baseline_upstream` matches the frozen golden-v1 and novel references exactly, 12,863/12,863 each. A deliberate one-label mutation caused the comparator to fail, as required. The exact 021 Linux candidate, its mimalloc 3.5.3 relink, and the dense variant each matched `baseline_upstream` byte for byte on golden-v1 (12,863), novel (12,863), and OSM (11,405); golden-v1 and novel also matched their frozen references. All candidate dump and comparison exit codes were 0. The OSM set has no frozen baseline-output file; it was compared to the newly rebuilt native original dump. These checks use the unchanged `dump_outputs.c` and `compare_frozen.py` tools from earlier experiments.

## Peak and steady memory, one thread

The initial memory series parsed novel.tsv for 120 passes, with the macOS host AC gate before each run. `check_runtime.sh` sampled each Linux process's `/proc/PID/status` and task directory about four times per second; all exited 0, showed one thread, and had zero `VmSwap`. The benchmark's `getrusage` peak RSS is used below; sampled late RSS is in `raw/commands/linux_analyze_memory.stdout`. Separate paired golden-v1/all and OSM runs follow the first table.

| Variant | Peak RSS, bytes | Result vs original |
| --- | ---: | --- |
| Native original, three runs | 1,960,681,472 / 1,960,439,808 / 1,960,488,960 | reference |
| Exact 021 `l4`, glibc | 2,347,384,832 | +386,703,360; fails memory |
| Exact 021 `l4`, Ubuntu mimalloc 2.1.2 | 2,456,395,776 | +495,714,304; fails memory |
| Exact 021 `l4`, pinned mimalloc 3.5.3 | 2,501,840,896 | +541,159,424; fails memory |
| Dense tables, glibc, three runs | 1,945,812,992 / 1,945,743,360 / 1,945,669,632 | paired differences −14,868,480 / −14,696,448 / −14,819,328 |

The dense variant's maximum peak RSS is 14,868,480 bytes below the original's maximum, and its late sampled RSS is also about 14.8 MB lower. Additional paired 120-pass checks (`raw/commands/linux_analyze_memory_more.stdout`) found:

| Input | Original peak RSS | Dense peak RSS | Dense minus original | Original setup | Dense setup |
| --- | ---: | ---: | ---: | ---: | ---: |
| golden-v1/all | 1,960,550,400 B | 1,945,661,440 B | −14,888,960 B | 2.464 s | 10.121 s |
| OSM | 1,959,571,456 B | 1,945,317,376 B | −14,254,080 B | 1.772 s | 10.177 s |

Late sampled RSS was also lower by 14.8 MB on golden-v1 and 14.3 MB on OSM. Each process exited 0, used one thread, and showed no `VmSwap`; the AC gate and host environment snapshots are retained. The margin is narrow, but all three tested corpora meet the measured Linux VM peak/late RSS no-increase gate. Linux RSS is not macOS physical footprint, so do not compare these numbers directly to the Mac 2.471/2.613 GB result. `/proc` samples, benchmark stdout, host AC gates, and host environment snapshots are preserved in `linux/runtime/` and `raw/commands/linux_memory_*`.

## x86-64 Linux functional check under emulation

An Ubuntu 24.04 amd64 image (`sha256:170c970c37aa7e8dbbcd306412a4062827a0fe590f2560f68aa982c715be132a`) ran under Docker Desktop's x86-64 translation on the ARM64 Mac. `x86/source/` is a fresh pinned-source copy configured inside that image. Its generated Makefile calls for `-mfpmath=sse -msse2 -DUSE_SSE`; `x86/build.sh` applied those flags to **both** original and candidate libraries. Both used Clang 18.1.3, library `-O2`, scanner `-O0` for feasible functional testing. These are x86-64 ELF builds, with hashes and flags under `x86/artifacts/` and exact logs `raw/commands/linux_x86_*`.

Both builds exited 0. The candidate's ordered output matched the rebuilt original byte for byte on golden-v1 (12,863), novel (12,863), and OSM (11,405); golden-v1 and novel also matched their frozen references. `linux_x86_verify_session` and every comparison exited 0. This specifically exercises the x86 `USE_SSE` build path. There is no representative x86-64 speed, memory, or thread measurement from emulation.

## Timing status and limits

An AC-gated, interleaved Linux ARM64 Docker timing session began at 18:36:54 PDT on 2026-10-01. Because the host remained busy (about 37–48% CPU idle with unrelated processes consuming several cores), it was explicitly labeled a **noisy diagnostic**, not accepted speed evidence. The user needed to close the laptop; a watcher stopped the runner at 18:49:25 PDT after all nine golden-v1 runs completed (three variants × three 150-pass trials), before any novel run. `timing-dense-3x150/STOPPED` records the reason; `raw/commands/linux_timing_dense_session.meta` records the deliberate exit 143, while `linux_timing_stop_at_boundary.meta` records watcher exit 0. Every completed benchmark run exited 0, and all start/end times, commands, stdout, stderr, AC gates, and environment snapshots remain in `timing-dense-3x150/`.

The partial golden-v1 analysis (`raw/commands/linux_analyze_timing_dense_partial.*`) found warm median 37.394 µs/address for the rebuilt original, 10.263 for exact l4 glibc, and 10.662 for dense glibc. Paired per-trial speedups were 3.7366/3.6704/3.4448× for exact l4 (median 3.6704×) and 3.5455/3.5225/3.1939× for dense (median 3.5225×). The dense run's max peak was 1,945,767,936 B versus 1,960,665,088 B for original (−14,897,152 B); exact l4 peaked at 2,347,380,736 B and fails the memory gate. These figures are **diagnostic only**: the matrix lacks novel timing, the original had only 1 clean warm pass out of 447, and the per-run `host-cpu` sampler files were empty despite pre/post host snapshots documenting contention.

The harness supplied the other TSV as an untimed warmup before pass 1. Its `prewarmed_first_pass_median_us` and first-pass country data are therefore *not* METHOD's true no-warmup cold-stream check. `run_timing_dense_r1.sh` and `run_true_cold.sh` are unexecuted plans for a fresh quiet AC session; they snapshot binary/model/input SHA-256 values and the resolved Docker image ID at session start and end, and the latter omits all address warmup. After the diagnostic, `bench_ext_linux.c` was fixed to fail if a warmup line lacks a tab or parsing returns no components; `bench_ext_linux_r0.c` preserves the exact source used by the current binaries (SHA-256 `ca8486174ec23c82eb6c7dddcb67efae004d299f2b810582092b012ba2a7a50d`). The fix passed a syntax check (`linux_bench_r1_syntax.*`), but the benchmark binaries have **not** been rebuilt or retested with it. Rebuild and recheck them before future accepted timing. `run_timing.sh` is an unused mimalloc-2 draft and `run_timing_mi3.sh` a separate pinned-mimalloc-3 draft; neither supplies measured evidence.

The dense variant is output-correct and passed this VM's repeated memory checks on novel, golden-v1/all, and OSM. It is **not accepted as a Linux speed deliverable** until the full controlled timing and cold checks complete. There is no representative Linux x86-64 or Windows speed claim here.
