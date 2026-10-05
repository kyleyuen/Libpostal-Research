# Experiment 026 — native Linux/Windows path and next speed work

Started: 2026-10-01T17:32:04-07:00 (America/Los_Angeles). Status: paused before further timing; no benchmark is running.

## Hypothesis and change

The 021 `l4` implementation contains structural gains that may carry to Linux and Windows, but its 4.3× result was measured only on an M4 Pro Mac. Port the packaged sources and benchmark without changing output semantics. Measure original and candidate natively where a runtime is available. Separately profile the actual 021 build before selecting any further speed optimization. Do not treat Mac-only proxy results or cross-compilation as native measurements.

Starting source: upstream libpostal commit `25099c506612b34b23b1bfe286ca6321fcf06f35`; preserved final source in `research/final/021-l4/sources/`. Starting original archive and outputs in `research/experiments/000-baseline/artifacts/` and `profiling/`. Source changes and new build files will be versioned here; 021 and all frozen inputs remain untouched.

## Reproduction

Every attempt, including failed builds, will capture exact command, timestamp, stdout, stderr, exit code, compiler and flags, source/model/input/binary hashes, and environment under `raw/commands/`. `tools/capture.sh` refuses to overwrite a prior attempt. Long-running builds/measurements run as background sessions so status can be reported while they proceed.

Initial host inventory: macOS 26.5.2 arm64 on an M4 Pro. Docker Desktop's Linux/aarch64 engine is available; Ubuntu 24.04 ARM64 was pulled for the Linux track. This is a Linux container/VM on Apple hardware, not a bare-metal Linux x86-64 result. The user confirmed that no Windows machine or VM is available. The Windows track will attempt cross-compilation and preserve a runnable package/instructions, without a Windows runtime or speed claim. The Windows toolchain setup and Linux image/build work may run concurrently; all timed runs will wait for a quiet CPU window.

Starting identities: `raw/commands/start_commit`, `package_integrity`, `input_hashes`, and `model_hashes` all exited 0. `model_hashes` rechecked every file listed in `profiling/manifest.json` against its recorded SHA-256. Independent Linux and Windows attempt logs are under `linux/`, `windows/`, and `raw/commands/`.

## Results in progress

- **Linux ARM64 in Docker Desktop's VM:** the rebuilt pinned original and the 021 `l4` source give byte-identical ordered output on golden-v1 (12,863), novel (12,863) and OSM (11,405). A deliberately changed label is detected, and the candidate's observed maximum thread count is one. These are functional results for Linux/aarch64, not Linux/x86-64. Raw commands and results are under `linux/` and `raw/commands/linux_*`.
- **Linux x86-64 under emulation:** the rebuilt pinned original and exact 021 source both compile with upstream SSE2 flags (`-DUSE_SSE -msse2 -mfpmath=sse`). Their x86-64 ELF dumpers give byte-identical ordered output on golden-v1 (12,863), novel (12,863) and OSM (11,405); golden-v1 and novel match the frozen references. This verifies an x86 code path functionally under emulation, **not x86 speed or memory**. Logs are under `linux/x86/` and `raw/commands/linux_x86_*`.
- **Linux memory:** on a 120-pass novel run, the pinned original's peak RSS was 1,960,681,472 bytes. The exact `l4` source used 2,347,384,832 bytes with glibc, 2,456,395,776 bytes with Ubuntu mimalloc 2.1.2, and 2,501,840,896 bytes with pinned mimalloc 3.5.3. Each exceeds the original and **fails the no-memory-increase gate**. A separate `candidate_dense` build removes only `EXP_LOADF=4` from 021's switches; it remains byte-identical on golden-v1, novel and OSM. Three paired 120-pass novel memory runs gave original peaks of 1,960,681,472 / 1,960,439,808 / 1,960,488,960 bytes and dense peaks of 1,945,812,992 / 1,945,743,360 / 1,945,669,632 bytes. Additional paired golden-v1 peaks were 1,960,550,400 vs 1,945,661,440 bytes (dense −14,888,960), and OSM peaks 1,959,571,456 vs 1,945,317,376 (dense −14,254,080). Late RSS was likewise lower on all three corpora, with one thread and zero swap. This supports the memory gate **on this Linux ARM64 VM**, subject to speed confirmation. Setup on v1 rose to about 10.1 s from 2.46 s. Peak RSS here is not the macOS footprint metric.
- **Windows x64:** the original and a Windows-adapted 021 candidate cross-compile to PE DLLs and paired harnesses. A source review found a split-arena commitment defect in the first successful port; corrected `l4_r3` builds and links, with the earlier attempts retained. No Windows machine or VM is available, so output, threads, speed and memory are unverified. The Windows binary still contains 021's separate feature-print defect; see [Windows runbook](windows/README.md) and [r3 review](windows/review_r3.md).
- **Mac profile:** a sample of the unchanged accepted 021 build attributed 16.7% exclusive samples to phrase trie traversal and 10.9% to CRF dictionary lookup. This is diagnostic only; it does not measure a speed gain. See [profile](profile/README.md).
- **Linux ARM64 speed diagnostic:** an AC-gated but heavily contended run stopped safely at 18:49:25 PDT after nine golden-v1 runs (three variants × three trials); no novel timing began. The dense variant's paired warm speedups were 3.5455/3.5225/3.1939× (median 3.5225×) against the native original, with about 15 MB less peak RSS. Only 1 of 447 original warm passes met the clean-pass rule. Its first pass was prewarmed with the other corpus, so it is not the required true cold check. This incomplete, noisy result is **not accepted Linux speed evidence**. The raw stop record and analysis are in [linux/timing-dense-3x150](linux/timing-dense-3x150/) and [Linux methods/results](linux/README.md).

Controlled Linux timing, novel timing, and a true cold check remain pending a quiet AC-powered host. There is **no accepted Linux or Windows speedup claim** yet, and the measured 4.330×/4.358× result remains specific to the M4 Pro macOS build.

## Correctness and interpretation

The unchanged 021 build remains the measured Mac deliverable. On any new platform, candidate output must be byte-identical to the platform's original library and golden-v1 reference, and parsing must stay single-threaded. Peak and steady memory plus cold/warm throughput are required for acceptance.

## Decision

Pending native evidence. Do not extrapolate the 4.3× Mac number. Linux ARM64 functional checks pass and the dense variant passes memory checks on all three corpora; the exact `l4` configurations fail Linux memory. Linux x86-64 output passes under emulation. Controlled Linux speed is pending. Windows remains compile-only.
