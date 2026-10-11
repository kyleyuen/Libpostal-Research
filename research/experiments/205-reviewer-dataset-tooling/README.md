# Experiment 205 — reviewer bring-your-own-dataset tooling

Date/time/timezone: 2026-10-10 23:44 – 2026-10-11 00:10 PDT
Status: tooling checked. The full-run memory result is INCONCLUSIVE because the machine was paging; a repeat on a machine with free memory is pending.
Evidence type: tooling check (the inputs are repository data reused as stand-ins; these are not new speed or memory claims)
Protocol: METHOD.md

## Purpose and change

The goal is to let an outside reviewer test the optimized build on an address set they built themselves (a "v15") with one command, and get a plain PASS/FAIL answer. No parser source was changed. New or changed files:

- `benchmark/run.sh`:
  - `--dataset FILE [--name NAME]`, `--api`, `--api-only`, `--rebuild`.
  - Build reuse keyed on SHA-256 of every build input. The library is rebuilt only when library inputs change; harnesses are relinked when only they change.
  - The candidate variant is named `optimized` in new runs.
  - Every run ends by writing `VERDICT.md`.
- `benchmark/add_dataset.py`:
  - Copies the reviewer's file into the work directory. It accepts text or TSV, strips CR, and rejects invalid UTF-8 or NUL bytes with line numbers.
  - Gives text input the placeholder country `xx`. The harnesses use the country only to group timings; the parser never receives it.
  - Chooses passes so the optimized build parses for about 12 s per timed run.
  - Sets 3 rounds and caps the quiet-machine wait at 300 s.
- `benchmark/verdict.py` writes PASS / FAIL / INCONCLUSIVE checks for correctness, speed, memory, threads and API, plus startup and break-even. Its exit status is 0, 1 or 3.
- `research/suite/run_suite.py` has two small changes:
  - The thread probe runs at least the dataset's pass count, so a small dataset is still parsing while it is sampled. Repository datasets are unchanged at 200.
  - The settle wait is announced when it starts.

## Checks

| Check | Result |
|---|---|
| `add_dataset.py` on 300-line text with trailing blank lines; 50-line TSV with CRLF; a file with an invalid UTF-8 line; the name `bad name` | Imported, imported (CR stripped, country kept), rejected ("line 2"), rejected. Exit statuses 0, 0, 1, 1. |
| `verdict.py` on copies of the published 203 runs (gold+novel, OSM v3–v13, v14) | PASS; memory 6/6, 33/33 and 3/3 pairs; v14 speedup 5.73× and break-even 549,864, matching the published values |
| Build reuse | Runs 1–3 printed "reusing the identical … builds" after the build made by experiment 204 attempt 2. With `bin/harness.key` removed, `--setup-only` relinked only the harnesses (6 s; library not rebuilt). The relinked harness binaries had different SHA-256 values from the previous link, because macOS `clang -g` embeds a randomly named temporary object path in the debug map and UUID. Binaries are therefore not bit-reproducible across links; output parity is re-checked by every run. |
| Run 1: `--dataset v15.txt --quick` (300 addresses from v14) | 3.6 min, exit 0, verdict PASS. Its thread samples exposed the short-probe problem fixed above: 3 of 6 samples were taken after the process had exited. [raw/run-1-quick-v15-300/](raw/run-1-quick-v15-300/) |
| Run 2: full run, 2,555 addresses | Stopped by me after the suite waited silently for load < 2.5. A busy `icdd` system process kept load near 3.6, and the default wait is 30 min. This led to the 300 s cap and the wait message. [raw/run-2-interrupted-settle-wait/](raw/run-2-interrupted-settle-wait/) |
| Run 3: `--dataset v15-tooling-check.txt --api` (2,555 addresses = golden-v1 validation) | 17.6 min total, including a 300 s settle wait that gave up at load 4.7. See below. [raw/run-3-full-api-v15-2555/](raw/run-3-full-api-v15-2555/) |

### Run 3 result

- Correctness: PASS. The 2,555-address dump and both 2-million-input fuzz streams were byte-identical, and the negative control was detected.
- Speed: PASS, 7.35× (95% CI 5.59–9.66). The original measured 32.3 µs/address, about twice its quiet-machine value, at load 4–5; this ratio is not comparable with the published ones.
- Threads: PASS, 6/6 live samples with one thread.
- API: 7/8 families identical. `print_features` gave the documented abort and is reported as KNOWN LIMITATION, per experiment 204.
- Memory: peak footprint was lower in 3/3 pairs (optimized 2,012 MiB vs original 2,171–2,491 MiB). Peak RSS was higher in rounds 1 and 2 (+238,567,424 B and +90,783,744 B) and lower in round 3.

The original verdict printed FAIL; its output is in `run.sh.log`. I investigated before changing anything:

- On the published 203 golden-v1 captures, the original's peak RSS equals its peak footprint (ratio 1.000) in every round, and the candidate's ratio is 0.937–1.000.
- In run 3, the original's ratio was 0.686 and 0.733 in rounds 1 and 2. About 30% of its memory was compressed or paged out, and swap use was about 22.5 GB of 23.5 GB.
- The optimized build's footprint, 2,012 MiB, is unchanged from 203.

So the original's RSS was understated by paging. METHOD.md says a capture problem that makes the sign uncertain is inconclusive and must be repeated, without changing the criterion. `verdict.py` now classifies a positive RSS delta as INCONCLUSIVE only when either process's RSS was below 0.95× its footprint. Footprint increases, and RSS increases without paging, still FAIL. The 0.95 threshold was chosen after seeing this run. Rerunning the verdict on all three published 203 run directories still gives PASS for all 42 pairs. The regenerated `verdict.json` and `VERDICT.md` in the run 1 and run 3 directories come from the final `verdict.py`.

## Limits and next step

- These inputs are repository datasets reused to exercise the tooling, not a reviewer's new v15. They add no evidence beyond experiment 203.
- **Pending:** repeat run 3 on a machine with free memory, without the VM, Codex/ChatGPT and VS Code processes, to resolve the memory result. Until then the memory gate on this dataset is inconclusive, not passed.
- All runs used fresh setup builds in a scratch work directory, on the Apple M4 Pro described in experiment 204.
