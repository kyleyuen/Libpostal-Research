# Experiment 162 — joint timing session for 158, 159, 160, 161

Date/time/timezone: 2026-10-05T20:12:05-0700 (created; decision rules pre-registered before the run)
Status: complete (session 2026-10-05T20:12–20:40-0700; 0 gate failures; all runs exit 0)

## Why one session

METHOD §Environment 5: baseline and candidates are always measured in the same session, and 034 showed parallel runs contaminate timing.
Each teammate's TIMING_REQUEST is therefore run here, serially, by the lead, with no teammate work running.

## Protocol

**Script.** `tools/run_session.sh`. This is 160's `run_timing.sh` with only two changes: the variant list was widened and the output directory moved here.

**bench_ext variants.** The SHA-256 values are in `raw/variant_hashes.txt` and equal the values the teammates reported.
- original (`015/bench_ext_baseline`)
- cpi (119)
- 158ctl
- 158fp
- 161fx
- 160ctl
- 160b
- 160c

**bench_ext runs.** Each variant is run on golden-v1 (`v1-all.tsv`) and OSM v3, for 5 rotating rounds × 20 passes. Every run is wrapped in `/usr/bin/time -l` and preceded by `env_gate.sh`. One untimed warm-up run per variant comes first, which leaves the page cache warm. Steady footprint and vmmap are read at pass 3, twice per variant.

**159.** Its `tools/run_timing.sh` runs unchanged afterwards in the same session. Its outputs go to `159/raw/timing`.

**Deviation.** 158's request named golden-v1 + novel through run_ext.sh. Here 158 uses golden-v1 + OSM through the common protocol, so all variants share one harness.

## Decision rules (pre-registered; copied from each experiment's KILL_RULE / TIMING_REQUEST)

**158 (R0).** 158fp vs 158ctl paired warm median and cold pass must lie within the session noise band, judged by the 158ctl vs cpi spread. Peak footprint must not be above 158ctl beyond allocator noise.

**161.** 161fx vs 158fp, same criteria.

**160.** Each of 160b and 160c is judged on its own.
- **Kill** if its median gain vs cpi is < +3% on golden-v1 **or** OSM, on the cold pass **or** on the warm passes.
- **Memory:** P = peak footprint + 1,117,339,520 B must be ≤ original peak − 20 MB. Max RSS is reported beside it.

**159.** Kill item 3 at P1 level: the exact phrase search must be clearly below cpi's context fill (css) on golden-v1 and OSM.

## Results (raw: `raw/analysis.txt` from `python3 tools/analyze.py`; 159: `159/raw/timing`)

**Speed vs cpi.** Each figure is the median of the per-round ratios over 5 rounds. Warm is the median of passes 2–20; cold is pass 1. The warm min–max is in `raw/analysis.txt`.

| Variant | v1 cold | v1 warm | OSM cold | OSM warm | v1 warm vs original | Peak footprint (MB) |
|---|---:|---:|---:|---:|---:|---:|
| cpi | 1.000 | 1.000 | 1.000 | 1.000 | 5.72× | 2,537 / 2,543 |
| 158ctl | 1.154 | 1.038 | 0.990 | 0.988 | 5.79× | same as cpi |
| 158fp | 1.102 | 1.042 | 0.975 | 0.998 | 5.83× | same as cpi |
| 161fx | 1.046 | 1.039 | 0.978 | 0.994 | 5.81× | same as cpi |
| 160ctl | 1.124 | 1.021 | 0.993 | 0.974 | 5.77× | same as cpi |
| 160b | 0.843 | **0.710** | 0.714 | **0.665** | 4.07× | 1,072 (P = 2,189) |
| 160c | 0.978 | **0.833** | 0.873 | **0.822** | 4.70× | 1,072 (P = 2,189) |

Noise band: the rebuild controls against cpi range from 0.97 to 1.13 warm and up to 1.15 cold on v1. The cpi cold pass on v1 (7.19 µs) was slower than all four rebuilds.

**159.** Phrase search per address, in µs. Pass 2–60 medians over 3 rounds. Exact r2 pass 1 (110.9 µs) is a page-fault outlier from the cold mmap.

| | golden-v1 exact | golden-v1 original trie | OSM exact | OSM original trie |
|---|---:|---:|---:|---:|
| Pass 2–60 medians | 1.45–1.82 | 0.73–0.74 | 0.65–0.83 | 0.50–0.57 |

cpi's whole context fill (css) is 1.79–2.12 cold / 0.39–0.40 warm on v1, and 1.07–1.30 / 0.35–0.36 on OSM.

**Steady footprint (pass 3).** original 2,492–2,493 MB; cpi, 158 and 161 1,927 MB. vmmap's mapped-file line was not captured for 160 (grep pattern mismatch), so steady memory for 160 is NOT measured.

## Decisions (by the pre-registered rules)

- **158: PASS.** 158fp vs 158ctl is within noise (warm 1.042 vs 1.038 on v1, 0.998 vs 0.988 on OSM) with equal peak footprint.
- **161: PASS.** 161fx vs 158fp is within noise (1.039 vs 1.042; 0.994 vs 0.998) with equal footprint. → **cpi + 158 + 161 (`161fx`) is a verified correctness-maintenance candidate with speed parity.** It has not been promoted to `final/` yet, and P4's reserved-set validation has not been run.
- **160: KILLED, both variants.** 160b is 29–34% slower than cpi warm and 160c is 17–18% slower, against the +3% bar. Memory would have passed (P ≈ 2,189 MB ≤ 2,594). 149's risk materialized: replacing cpi's structures with records costs more than it saves at this design.
- **159: KILLED (kill item 3).** The exact hash search is about 2× slower than the original trie warm on golden-v1 and about 1.3× slower on OSM, so it cannot beat cpi's context fill. 159b was not built, per the user's "time first" decision. P3 is therefore off.
