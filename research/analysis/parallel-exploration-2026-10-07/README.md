# Parallel architecture exploration queue (2026-10-07)

Status: the 188–200 candidate queue is complete; its outcomes, failures, and limits are in [REPORT.md](REPORT.md). Experiment 201 measured token-span views against a source-matched CPI control and did not promote the candidate. The prior AC experiment 202 run stopped after 23/54 OSM captures; two battery preparations ended before timing captures. Final 202 validation covers 14 datasets / 130,341 rows: the source-weighted warm-parsing geomean is 5.893289× and the global peak maximum is 70,500,184 B below original. The Senzing subgroup peak still regressed by 58,982,640 B. Thread samples on golden-v1, osm-v3 and v14 observed one thread; full public API parity is limited by the known feature-print abort. See the [outcome report](../../OUTCOME_REPORT_2026-10-09.md) and [202 record](../../experiments/202-cpi-final-validation/README.md). Parser/model runs in these tracks were owned by the benchmark worker and run serially.

## Queue

| ID | Track | Candidate | Status |
|---|---|---|---|
| 188–197 | implementation181 | Ten incremental 181-derived structural candidates | Complete; see report and numbered records |
| 198–200 | new_architecture | Three distinct architecture candidates | Complete; 199 continued to bounded follow-up, with no promotion |
| 201 | CPI work and memory | Token-span views versus source-matched CPI | Bounded validation complete; no promotion |
| 202 | Accepted CPI validation | Fresh final release validation | Final 14-set timing/peak rollup complete; known Senzing peak and feature-print API failures documented |

The 181 queue is currently planned as: streamed score/key construction, canonical carry, direct-ID fewer-array-copies, exact unknown fallback, event capacity, pair/unary canonicalization, queue elimination, certifier optional, and one combined candidate. The candidate owner must keep the registered IDs and source histories aligned with their immutable handoffs. IDs 198–200 begin with distinct designs and may adapt to the measured evidence.

## Candidate handoff

Each ready candidate supplies one `raw/handoff.json` in its numbered experiment directory, matching [`candidate-handoff.schema.json`](candidate-handoff.schema.json). Required: source snapshot/patch and manifest hashes, exact build provenance, matching release and diagnostic artifact identity when available, binary hashes, model manifest identity, supported/unavailable metric declarations, and `parser_invoked:false`. A design-only record is not eligible for a parser queue slot. Workers may build and run non-model tests, but must not run a parser or load model data.

The coordinator rejects a handoff when paths are missing, hashes do not match, source/build identity is ambiguous, or the candidate shares mutable output paths with another build. No source changes are made to worker candidates by the queue coordinator.

## Sequential validation protocol

For every immutable candidate, execute a development screen against the preserved original 000 library, pinned release 119 CPI control, candidate release, and candidate diagnostic build, plus the unchanged 181 diagnostic control when the hypothesis is 181-derived and that control is build-compatible. Compare ordered output streams byte-for-byte; record one-thread samples, environment gates, `/usr/bin/time -l` resource logs, raw commands/stdout/stderr/exits, source/binary/model/input hashes and available operation counters. Metrics not emitted by a candidate are `null` with a reason, never zero.

Every runnable candidate then receives the full golden-v1 output check, including the frozen original reference. Candidates that remain viable after development and golden validation receive all-13 observed-workload output and diagnostic validation. All 13 datasets are already observed; no country/source-derived data, tables, thresholds or selection keys may enter candidate code. Each workload is gated before each parser command and processes run one at a time.

This is structural diagnostics and correctness screening, not a speed campaign. Process duration is retained as provenance only. No performance promotion follows from fewer diagnostic events. Any proposed promotion still needs the existing uninstrumented timing protocol and peak/steady memory evidence; RSS alone cannot satisfy memory acceptance.

## Shared inputs and controls

- Original 000: `research/experiments/000-baseline/artifacts/libpostal.a`, `libpostal.dylib`, and compatible dumper from experiment 182.
- Pinned 119 CPI and diagnostic helpers: experiment 185's verified `cpi` release/diagnostic pair and 119 source/hash provenance.
- Unchanged 181 control for attribution: use only a source-matched 181 control declared by the candidate descriptor; no silent substitution of the separately named `prototype181` build.
- Development screen: `profiling/golden-v1/development.txt`.
- Mandatory golden output: `profiling/golden-v1/all.txt` and frozen `baseline-outputs.jsonl`.
- Full observed set, for viable candidates: the registered all13 mode in the 186/187 configs.
- Model: `/Users/kyle/.cache/libpostal-profile/libpostal`, with the coordinator's verified manifest SHA-256.

## Raw batch records

`raw/` will contain coordinator preflight evidence, candidate handoff snapshots, per-command captures, comparisons, diagnostic summaries, archive hashes, and failed attempts. Each run folder is exclusive-create. Large diagnostics may be gzip archived losslessly only after exact decompression/hash verification; successful and unsuccessful records remain separate and are never overwritten.

The original preflight and sequential queue evidence are under `raw/`. Final statuses are in [`queue.json`](queue.json) and the linked reports; preserve every failed build or run alongside successful records.
