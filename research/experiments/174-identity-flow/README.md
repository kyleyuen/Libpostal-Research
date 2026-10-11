# Experiment 174 — identity flow through the existing FTT slot buffer

Date/time/timezone: 2026-10-05 PDT (America/Los_Angeles), pre-registered before source edit.
Status: rejected candidate; identity path is correctness-verified.

## Hypothesis and change

173 established that admin-normalized tokens often have the same bytes as the regular normalized token later queried by `ap_vm_get`, but its separate `ap_seed_slots` array added a persistent allocation and had an unchecked cross-array index. The existing `EXP_FTT` component-phrase path already allocates one `int32_t` vocabulary-slot buffer per maximum address length and fills it during context construction. This experiment keeps that buffer live through feature scoring, resets its public pointer before the next context fill, and lets `ap_vm_get` reuse its matching slot only after explicit bounds, length, delimiter and byte-equality checks against both independently normalized streams. It otherwise performs the original lookup.

Expected effect: remove the second vocabulary probe for eligible tokens without a new per-address slot buffer. No throughput measurement is authorized in this experiment.

Base commit and source: `181737d30977adc9d287e3f7a1cfafd7277525d4`; copied from `research/final/119-cpi/sources` before editing. Candidate source and final patch will be retained under this directory.

## Acceptance screen

1. Exact-output oracle and final dumps must match the preserved original on development, golden-v1, novel, OSM v3 and fuzz samples; `AP_IDENTITY_VERIFY=1` must not abort.
2. The diff must contain no thread primitives and a live run must show one thread.
3. Peak RSS and peak footprint, and an explicit steady-state footprint observation, must be no larger than the preserved original. Any failure is recorded and rejects the candidate.
4. No speed timing or speed claim will be made.

## Reproduction

Hardware: Apple Silicon arm64, Darwin 25.5.0, Apple clang 17.0.0. Source base was copied at Git `181737d30977adc9d287e3f7a1cfafd7277525d4`; build flags and complete source patch are in `tools/build_cc.sh` and `raw/address_parser.patch`. Three build attempts are retained: build 1 lacked cpi switches, build 2 was invalidated when its script changed while executing, build 3 succeeded. `artifacts/original_*` links `000-baseline/artifacts/libpostal.a`; `artifacts/cpi` is a same-session, same-source build with only `EXP_SHARED_FTT_IDENTITY` removed. Commands, timestamps, stdout, stderr, exit codes and hashes are in `raw/`. Every model-running command is wrapped by `research/tools/model_slot.sh`.

## All measured trials

No speed trials were run.

| Observation | Original | Same-session cpi | Identity candidate | Raw evidence |
|---|---:|---:|---:|---|
| `/usr/bin/time -l` max RSS bytes | 1,293,860,864 | 2,831,253,504 | 2,689,138,688 | `raw/memory/*.stderr` |
| `/usr/bin/time -l` peak footprint bytes | 1,278,240 | 1,278,240 | 1,278,240 | `raw/memory/*.stderr` |
| steady `phys_footprint` MB after setup + one parse | 2,168 | 2,232 | 2,120 | `raw/memory/*.steady.txt` |

The time-tool peak footprint conflicts with the direct `footprint` samples and is not a credible process-footprint maximum here; raw values are retained rather than normalized. The observed candidate max RSS is above the preserved original by 1,395,277,824 bytes, so the strict peak-memory condition is not demonstrated and this candidate is rejected. The candidate's direct steady footprint was 48 MB below the original in this single serialized observation, but that does not repair the peak-RSS failure.

## Correctness and interpretation

Paired preserved-original output checks with `AP_IDENTITY_VERIFY=1` all passed: development 10,308; golden-v1 12,863; novel 12,863; OSM v3 11,405; deterministic fuzz seed 1, 200,000 generated inputs. All five original/candidate `cmp` checks exit 0 in `raw/verify-paired/summary.txt`; candidate verification did not abort. The formerly requested frozen golden JSONL uses a different `id`-based serialization than `dump_outputs`; its failed raw `cmp` is retained in `raw/verify/`, while the corrected direct original-versus-candidate comparator is authoritative.

The change occurs after context-fill's O2 state. Existing stage-oracle instrumentation targets an earlier source architecture and was not rebuilt; no O3/O4 trace claim is made. Final output is direct-compared to the preserved original instead.

`ps -M` samples identify the actual candidate executable PID, not its wrapper; each of original, cpi and candidate has one listed thread in `raw/memory/*.psM.txt`. The source diff has no thread or parallel primitives.

## Decision

Rejected under the hard peak-memory rule. The narrow supported result is that the existing FTT slot buffer can carry identity safely through the single-threaded parser flow with no added per-address slot array, while retaining byte-identical output on the screened corpora and fuzz. No performance claim is made.

Ledger, report and activity log updated: not by this subtask (owned by the parent agent).
