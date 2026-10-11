# Experiment 163 — diagnostic design study: fused normalization/tokenization

Date/time/timezone: 2026-10-05 22:37 PDT (workspace environment labels current date 2026-10-06; shell clock captured in `raw/evidence.txt`).
Status: inconclusive — diagnostic probe completed; no candidate built/timed.

## Hypothesis and change

Measured opportunity: cpi's `normalize + tokenize + response` is 0.404/0.249 µs per address on golden-v1 and 0.366/0.274 µs on OSM (cold/warm; exp 141, profiling build). This is a small remaining bucket, but normalization and tokenization currently process bytes in separate passes and allocate/scan intermediates. A streaming transducer that performs the exact existing normalization decisions while delimiting tokens could avoid a pass and intermediate traffic.

Proposed architecture (not implemented): consume the same UTF-8 input once; preserve normalization's composition/lowercase/trim semantics and tokenizer's whitespace, punctuation, ignorable-token, and token-type behavior; emit normalized bytes into a growable buffer while recording token offsets and types. It must preserve current normalized string and token-array bytes/metadata exactly. The UTF-8 normalization library's composition/context rules may force buffering or fallback; this is the central feasibility risk.

Prior evidence: exp 011 introduced normalization fast paths; exp 141 says normalize/tokenize/response remains comparatively small. No claim that a fused pass is faster. A falsifiable first probe is a diagnostic equivalence harness over frozen O1/O2 oracle inputs plus pathological Unicode, followed by instruction/allocation counts on identical dev inputs; stop if exact token boundaries/types require repeated scans or fallback covers common input.

## Bounded corpus probe

Ran `python3 research/experiments/163-fused-normalize-tokenize/probe.py golden-v1 osm-v3 fuzz-s1-200k` (exit 0; stdout/stderr/exit in `raw/probe.*`). The script streamed frozen O2 rows and compared token text fields 2 and 3 (pre-normalized spelling and normalized spelling); no model was loaded. Input and script hashes: `raw/input_script_hashes.txt`.

| Corpus | Addresses | Token rows | ASCII rows | Raw differs from normalized | Duplicate raw → normalized occurrences within address |
|---|---:|---:|---:|---:|---:|
| golden-v1 | 12,863 | 75,659 | 62,556 | 24,733 (32.69%) | 1,189 → 595 |
| osm-v3 | 11,405 | 61,157 | 50,776 | 20,741 (33.91%) | 487 → 192 |
| fuzz-s1-200k | 178,919 | 1,363,734 | 1,159,897 | 471,781 (34.59%) | 212,592 → 192,740 |

Examples include `DDD → 588`, `DDDDD → 95801`, and `meriDian → meri3ian` (full list in stdout). This confirms substantial normalization work, but the oracle lacks original address spans and cannot establish that a fused scanner avoids passes or allocations.

## Reproduction and provenance

Hardware/OS: Apple Silicon arm64, Darwin 25.5.0 (shell `uname`, captured in `raw/evidence.txt`). Compiler/build: none; no build or benchmark. Base commit: `181737d30977adc9d287e3f7a1cfafd7277525d4`. cpi source SHA-256: `normalize.c` `57c4588989da4a560142068e876dde9db3938c3672dcf078973263348e36cf48`; `address_parser.c` `a3db12f9d2daf32903f986197a1738eef5d383a5528d94e8e8100b0804f18536`. Hashes and command status are in `raw/evidence.txt`.

## Trials and correctness

No candidate, model, binary, correctness comparison, or timing trial was run; no throughput or memory result exists. The corpus probe read existing O2 oracles. Golden-v1 output identity and single-thread parsing remain hard acceptance constraints. Preserve separate versioned inputs; do not mutate golden-v1.

## Decision

Inconclusive. The corpus data confirms a large normalized-token workload but does not show fusion is cheaper. Next probe: compare full normalized output and token metadata exactly on development and non-reserved inputs while counting intermediate bytes and passes; stop if common inputs need fallback or extra buffering. Ledger/report/activity log intentionally untouched per task scope.
