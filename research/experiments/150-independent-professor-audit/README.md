# Experiment 150 — independent second-opinion audit (no code change)

Date/time/timezone: audit began 2026-10-05 01:06:54 PDT; timed trials
01:12:00–01:41:35 PDT. Memory, API and archival checks followed.

Status: **completed audit**. This is a Codex second opinion, separate from the
student's optimization experiments. It is not a claim of human-professor review.

## Hypothesis and change

Question: Does the selected `cpi` build still meet the agreed **3.5×
single-threaded parser speedup without more memory** on newly sourced addresses,
with the same ordered output as the original?

Implementation change: **none**. Root checkout before this audit was
`181737d30977adc9d287e3f7a1cfafd7277525d4`. The reference libpostal
checkout was `25099c506612b34b23b1bfe286ca6321fcf06f35`. The tested
candidate is the preserved `research/final/119-cpi` build; no variant was
selected using this holdout. This record and `profiling/independent-audit-v1/`
are documentation and dataset additions only.

## Reproduction and provenance

- Host: Apple Silicon `Mac16,8`, macOS 26.5.2; Apple clang 17.0.0; Python 3.14.4.
- Corpus: [`profiling/independent-audit-v1`](../../../profiling/independent-audit-v1/README.md),
  3,220 RRAD real addresses plus 78 explicit edge cases, 3,298 total. Additional
  correctness-only addendum: 45 DAD international addresses. Dataset and raw
  source SHA-256 values are in its `SHA256SUMS` and metadata files. Zero exact
  RRAD strings overlapped golden-v1, novel or OSM v3–v11; reserved v12/v13 were
  not inspected.
- Model: the seven files in `benchmark/model_sha256.txt` all passed `shasum -a
  256 -c` against `/Users/kyle/.cache/libpostal-profile/libpostal`.
- Baseline `bench_ext_baseline` SHA-256:
  `c2ce250646155d6eda91337cac9b9ddab11f3605ff56b70ba59fc1529932c77e`.
  Candidate `bench_ext` SHA-256:
  `c5612825963a18be1f4e7869da2bb2dc6fedf3f098189d195bad765247b43c5a`.
  Correctness and fuzz executable hashes are in `raw/binary-hashes.txt`.
- No rebuild was part of the timed comparison. The preserved baseline and
  candidate executables were run with the same model, input order, harness and
  parser options. Candidate build flags are documented in experiment 119 and
  `benchmark/run.sh`; the original uses the preserved baseline build.
- Before **each** timed process, `research/tools/env_gate.sh` passed (30/30).
  `research/tools/env_snapshot.sh` recorded environment before and after every
  process in `raw/environment.txt`. Timing ran serially under `caffeinate -i`,
  with order reversed every round and two seconds between processes.

The exact timed invocation was `/usr/bin/time -l <bench_ext> <audit-input.tsv>
900`, once per variant per round, with stdout/stderr in `raw/timing/`. Each file
has corresponding start/end timestamps and gate output. This produces 900
passes over the same 3,298 inputs; candidate measured parsing lasted
12.21–13.80 seconds per trial, satisfying the protocol's ten-second minimum.
No formal trial was excluded. The earlier 40-pass calibration was **not** used
in these statistics; its raw files were inadvertently overwritten by round 01,
so only its console excerpt survives in the conversation. That is an audit-trail
limitation, not an invented result.

## All measured trials

Warm = median µs/address over passes 2–900 within each process. Speedup is the
baseline warm median divided by the candidate warm median in the same round.
Cold is pass 1. RSS delta is candidate minus original peak resident set size.

| Round | Original warm µs | Candidate warm µs | Warm speedup | Cold speedup | RSS delta MB |
|---:|---:|---:|---:|---:|---:|
| 01 | 27.074 | 4.051 | 6.683× | 4.555× | −63.5 |
| 02 | 28.531 | 4.658 | 6.125× | 5.104× | −64.1 |
| 03 | 26.553 | 4.029 | 6.590× | 7.065× | −64.0 |
| 04 | 27.223 | 4.042 | 6.735× | 5.112× | −63.8 |
| 05 | 27.829 | 4.435 | 6.275× | 5.350× | −64.1 |
| 06 | 27.721 | 4.469 | 6.203× | 5.235× | −64.0 |
| 07 | 25.788 | 4.045 | 6.375× | 4.631× | −64.0 |
| 08 | 26.625 | 4.034 | 6.600× | 5.154× | −64.1 |
| 09 | 26.658 | 4.029 | 6.617× | 4.687× | −64.0 |
| 10 | 26.591 | 4.038 | 6.585× | 6.162× | −64.7 |
| 11 | 26.497 | 4.025 | 6.583× | 4.822× | −64.0 |
| 12 | 26.627 | 4.011 | 6.638× | 4.476× | −64.0 |
| 13 | 26.703 | 4.024 | 6.636× | 5.155× | −64.0 |
| 14 | 26.669 | 4.034 | 6.611× | 6.188× | −63.8 |
| 15 | 26.743 | 4.078 | 6.558× | 5.744× | −64.0 |

Across the 15 paired rounds, median warm speedup was **6.590×** (range
6.125–6.735×); the geometric mean was **6.519×** (log-ratio Student-t 95%
interval 6.416–6.623×, treating rounds as the units). Median cold speedup was
5.154× (range 4.476–7.065×). The weakest hand-built category was the four
`MALFORM` strings: median last-pass ratio 4.046×, minimum 3.884×; this tiny
category is diagnostic, not a population estimate. `raw/analysis.json` includes
every process and category result. All 30 timed processes exited successfully,
with 900 numbered passes and the same component count in each pair.

Peak RSS was lower for the candidate in **all 15 pairs**, by 63.5–64.7 MB.
Independent steady-footprint samples on this corpus gave original **2,491 MB**
and candidate **1,929 MB** (`raw/memory/`). Six `ps -M` samples of each running
parser showed **one thread** each. The steady-memory probe processes were
intentionally terminated after sampling; their signal exits are not benchmark
failures.

## Correctness and other API checks

- `dump_outputs` produced **byte-identical JSONL** for all 3,298 timed inputs
  and, separately, the 45 DAD international addresses (`raw/correctness/`).
- The baseline and candidate `fuzz_pipeline` outputs matched for **15 distinct
  seeds × 100,000 mutations = 1,500,000** cases. These are equality checks of
  64-bit hashes of ordered parser outputs, with exact dump comparisons on the
  unmutated sets. Both compressed output streams for each seed, their raw hashes,
  line counts and the empty stderr files are documented in `raw/fuzz/`.
- A fresh probe compiled from the repository's existing
  `research/experiments/025-audit-next/tools/api_probe.c` passed default parse,
  parser lifecycle and lifecycle-with-disabled-integer-keys for both builds;
  their stdout was identical. **But the public feature-print API regresses:**
  the original's `features` probe exited 0; `cpi` exited **134 (abort)** with
  `EXP_ROW_PACK: only the direct scoring path is supported`. The relevant final
  source is `research/final/119-cpi/sources/crf.c:793`.

The probe was built outside the repository with `clang -O2 -Ilibpostal/src
research/experiments/025-audit-next/tools/api_probe.c <libpostal.a> [-for-cpi:
/opt/homebrew/opt/mimalloc/lib/mimalloc.o] -lm -o <temp-output>`. Its preserved
stdout/stderr is in `raw/correctness/`; the feature-print failure was not a
timing observation.

## Interpretation and limitations

Under the **agreed default-parser speed/memory/single-thread contract**, this
independent holdout supports the target with a large margin. It does **not** establish a fully
drop-in replacement: the supported feature-print API abort should be repaired
before that broader claim.

Candidate median setup was 11.480 s versus 2.681 s for the original. For one
pass over this 3,298-address holdout, setup plus parsing is about 11.49 s
candidate versus 2.77 s original. On these per-address costs, the extra setup
amortizes only after roughly **389,000 addresses** in one process. The stated
3.5× result concerns parsing after setup; setup is reported separately.

This is one Mac and an RRAD US-only timing corpus with 78 deliberately chosen
edge strings. The 45 additional international strings were correctness-only.
Repeated passes test warm throughput; the cold first pass helps bound that
limitation. Neither these tests nor the 15 mutation seeds exhaust all possible
parser inputs. A fresh-clone suite smoke report from another workspace was
reviewed, not counted among this audit's 15 trials.

Decision: **contract passed; general API parity not passed**. No candidate
code was changed. Preserve this record as a second opinion alongside, not
merged into, the earlier performance suite.
