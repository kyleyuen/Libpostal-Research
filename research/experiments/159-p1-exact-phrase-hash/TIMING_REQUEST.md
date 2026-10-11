# 159 timing request (for the lead: serial, on AC, `research/tools/env_gate.sh` gated, under `caffeinate -i`, no other agent's work running)

## What it decides

| Run | Measures | Decides |
|---|---|---|
| T1 `ps_bench exact` vs `ps_bench orig` (golden-v1, OSM v3; 3 rounds × 60 passes, alternating order) | 159's three exact phrase searches vs the ORIGINAL library's trie searches, on identical token streams | Like-for-like phrase-search gain (supporting data) |
| T2 119 `css` `bench_split` (golden-v1 all, OSM; 3 rounds × 3 passes) | cpi's whole context fill (CF), same session | Kill item 3 at P1: the exact phrase search must be clearly below cpi's CF (156's rule 3, now with an exact search) |
| `/usr/bin/time -l` on every run | peak RSS, setup | Indicative memory only (mmap'd tables count in RSS). P3 memory is judged in P3. |

The full context-fill gain (P3's ≥ +10% whole-parser gate) cannot be decided offline. A P1 pass here is necessary, not sufficient. Also, the simple table already fails the corrected memory baseline (README "Memory"). This timing tells whether a compact 159b is worth building.

## Artifacts (`artifacts/hashes.txt`)

| File | SHA-256 |
|---|---|
| `artifacts/ps_bench` (built by `tools/build_bench.sh`: `-O2`, `src/ps_exact.c` + `tools/ps_bench.c`, linked to 142 `ctl` `libpostal.a` for the orig variant) | `5b16cac968b275ac591a66eecdd7931e14f89338a380d4f10ee97581598820d7` |
| `artifacts/tables/dict.ps` | `2adb7f6816c12e0ccd76017c49b878f2d585852294412456157d6dddda254552` |
| `artifacts/tables/comp.ps` | `9ba8ca88279aae9f245362ffb431a0450326d32564f84528e8e28c047d1914f8` |
| `artifacts/tables/postal.ps` | `78ac9009e10aadd69ffdc0f8ef992db506847a3eb6a4f0e7791d52a554776970` |
| `src/ps_exact.c` / `tools/ps_bench.c` | `c6d2258a…` / `6218979d…` (full values in `artifacts/hashes.txt`) |
| `119-component-phrase-ids/artifacts/css/bench_split` | hashed by the script into `raw/timing/hashes.txt` |

Inputs: `research/oracles/v1/{golden-v1,osm-v3}` (O1/O2; ps_bench builds each address's normalized and admin token arrays before timing), and `profiling/golden-v2-novel/v1-all.tsv` and `profiling/golden-v3-osm/osm.tsv` for css. These are the same corpora as 155–157.

## Commands

```sh
cd "/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/159-p1-exact-phrase-hash"
caffeinate -i tools/run_timing.sh        # writes raw/timing/{run.log,gate.log,hashes.txt,<run>.stdout,<run>.stderr}
```

`tools/run_timing.sh` runs each run under `/usr/bin/time -l`, re-checks the gate before every run, and sleeps 2 s between runs:
- T1: for each round r = 1..3 and corpus c in {golden-v1, osm-v3}, it runs `ps_bench exact artifacts/tables <oracle c> 60` and `ps_bench orig <oracle c> 60`, with the order alternating by round.
- T2: then 3 rounds of `bench_split v1-all.tsv 3` and `bench_split osm.tsv 3`.

**Expected run time.** UNVERIFIED estimate: about 30–40 s per ps_bench run (table and corpus load plus 60 passes plus 10 warm passes) and the orig variant's model load. That is about 12 ps_bench runs plus 6 css runs.

## Reading the output

- **ps_bench line.** `setup s | pass1 (cold) | median p2..60 [min-max] | warm [min-max] us/addr | checksum`. Within a corpus the checksum **must be equal** between exact and orig: same phrases, same number of passes. A difference voids the round.
- **Cold pass 1 is asymmetric.** `exact` pass 1 includes the first-touch page faults of the mmapped tables, which `setup` does not count; the `orig` tries are already resident after the model load. Weigh the median and warm values alongside pass 1.
- **css stdout.** The context fill (CF) column, read the same way as 155/157.
- **Report.** Median over rounds of the pass-1 cold and warm values per variant. For css, CF scaled as in 155. Compare exact against cpi's CF and against orig. METHOD caveat, as in 156: each ps_bench process times only ~1–2 s of search per variant, below the 10 s per trial. Report that limitation with the numbers; if the margin is close, repeat with more rounds.
