# Experiment 173 — share token identity between context fill and feature lookup

Date/time/timezone: 2026-10-05 22:53–23:10 PDT (America/Los_Angeles). Coverage and build markers in `raw/`; later checks lack captured per-command start/end markers.
Status: **rejected candidate; structural premise supported**.

## Hypothesis and change

Cpi's context fill resolves a vocabulary slot for each eligible admin-normalized token. Later, `ap_vm_get` looks up the corresponding regular-normalized token again. When the byte strings match and the earlier lookup hit, carry the slot forward and skip the second lookup. This probes one identity-sharing mechanism; it is not a rebuilt parser or integrated event-stream architecture.

Pre-registered screen: original-library O2 oracles from golden-v1, novel, OSM v3, fuzz-s1-200k; require at least 50% eligible on real corpora. Coverage: 49,239/75,659 (65.1%) golden, 46,627/73,763 (63.2%) novel, 40,368/61,157 (66.0%) OSM, 793,196/1,363,734 (58.2%) fuzz tokens. These are O2 upper bounds; runtime reuse counts below are authoritative. The 3,694,560-key vocabulary came from the model-derived 160 dump, not corpus-specific training.

Base Git commit `181737d30977adc9d287e3f7a1cfafd7277525d4`; source base `research/final/119-cpi/sources`. Only `address_parser.c` changes in the scratch copy; exact patch `raw/address_parser.patch`, full copy `sources/`, built source and binary hashes `artifacts/seed/hashes.txt`. The experiment adds a per-address slot array and guards reuse by length, bytes, delimiter, and successful prior lookup; otherwise it calls the old lookup. `AP_SEED_VERIFY=1` recomputes the old lookup and aborts on slot/pointer disagreement. Instrumentation counts reuse and fallback. No production candidate was changed.

## Reproduction

Hardware/OS/compiler: Apple Silicon arm64, Darwin 25.5.0, Apple clang 17.0.0. Input/script/binary SHA-256 in `raw/input-output-hashes.txt`; seven model file SHA-256 in `raw/model-hashes.txt`. Model directory `/Users/kyle/.cache/libpostal-profile/libpostal`. Source and build flags in `tools/build_cc.sh`, `switches.txt`, and `artifacts/seed/hashes.txt`. No PGO or LTO; lib `-O2`, scanner `-O1`, link `-O2`, mimalloc object. Coverage command and build command captured exactly in `raw/command.txt` and `raw/build.command.txt`; coverage 22:53:40–22:54:01 exit 0, build 22:56:30–23:00:39 exit 0. Their stdout/stderr/exits are under `raw/`. Build warnings retained; no build failure.

Original fuzz harness built with `clang -O2 -I libpostal/src research/tools/fuzz_pipeline.c research/experiments/000-baseline/artifacts/libpostal.a -lm -o research/experiments/173-shared-token-identity/artifacts/original_fuzz_pipeline` (exit 0; stdout/stderr retained). The original library is the preserved 000-baseline artifact. Verification used `research/tools/model_slot.sh` with the same model and input, seed 1 for fuzz. Exact replay of the already completed run commands and timestamps was not fully captured; raw outputs, errors, and binary hashes are retained, and this gap is explicit. `tools/verify.py` reproduces output checks, with results in `raw/verify-summary.txt` (exit 0).

## All measured trials

This was a correctness/work-removal test, not a valid throughput session: no environment gate, interleaved repeated trials, cold/warm protocol, or steady-state memory capture. These single-run times are diagnostic only.

| Trial | Binary | 2M fuzz wall/user seconds | Peak RSS bytes | Peak footprint bytes | Raw stderr |
|---|---|---:|---:|---:|---|
| 1 | candidate, verification off | 29.87 / 29.18 | 2,110,111,744 | 2,518,337,816 | `raw/verify/fuzz_s1_2m.candidate.stderr` |
| 2 | preserved original | 79.33 / 77.72 | 2,168,242,176 | 2,438,793,304 | `raw/verify/fuzz_s1_2m.original.stderr` |

Observed candidate peak footprint **+79,544,512 bytes** (+3.26%) relative to original on this paired workload; RSS happened to be lower. METHOD's environment protocol judges footprint, and no steady-state result was collected. The extra footprint cannot be attributed specifically to the slot buffer without a same-session cpi control. The strict no-memory-increase constraint is not demonstrated and this observed peak fails it for this candidate.

## Correctness and interpretation

Candidate `AP_SEED_VERIFY=1` final outputs: golden-v1 12,863/12,863, novel 12,863/12,863, OSM v3 11,405/11,405, fuzz seed 1 first 200,000/200,000 byte-identical to frozen/original references; 0 slot disagreements. Verification-off outputs matched on golden and first 200,000 fuzz, and full 2,000,000 fuzz outputs matched the freshly built preserved-original harness byte-for-byte. `tools/verify.py` checks these hashes and has a one-byte mutation negative control. Actual skipped lookups: 49,236 golden, 46,613 novel, 40,368 OSM, 791,836 first 200k fuzz, 7,908,725 full 2M fuzz; full fuzz fallback 5,699,618. Source patch contains no thread primitives; `ps -M` samples of original and candidate each show one thread in `raw/verify/*.psM.txt`.

Limitations: ordered O3i/O4 traces were not separately compared despite the pre-registered intent; final parse output and slot equivalence are checked. The experiment uses cpi's current architecture and includes counter/verification scaffolding. It does not measure an independent rebuilt parser, model retraining, speed gain, or memory-neutral implementation. A bounds concern in `normalized_tokens->a[j]` requires a guard before production use; the tested corpora did not expose a crash.

## Decision

**Reject this candidate** because paired full-fuzz peak footprint exceeds original and steady memory is unmeasured. Retain the narrower conclusion that shared token identity is common and can remove about 58–66% of token-level repeated lookup opportunities across the screened corpora while preserving tested final outputs. A different representation needs its own exact-output, one-thread, peak/steady-memory and full timing evidence. Ledger and activity log updated; professor report unchanged because no accepted improvement changed.
