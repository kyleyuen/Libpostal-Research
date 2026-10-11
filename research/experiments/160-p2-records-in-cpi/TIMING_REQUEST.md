# Timing request — experiment 160 (P2, records inside cpi)

Written by teammate p2b on 2026-10-05. Nothing here has been timed: teammates do not time anything (team brief, rule 5). The lead should run it alone, on AC power, under `research/tools/env_gate.sh`.

## What to run

```sh
"/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/160-p2-records-in-cpi/tools/run_timing.sh"
```

Run it under `caffeinate -i` (METHOD, environment protocol 2), e.g. `caffeinate -i "…/tools/run_timing.sh"`.

It follows 121's `run_timing.sh`:
- **Runs.** 5 rotating rounds × 20 passes, golden-v1 (`profiling/golden-v2-novel/v1-all.tsv`) and OSM v3 (`profiling/golden-v3-osm/osm.tsv`). Every run is `/usr/bin/time -l <bench_ext> <tsv> 20` after `env_gate.sh`.
- **Steady memory.** Captured afterwards at pass 3, twice per variant, with `footprint -p` **and** `vmmap --summary`.
- **Output.** Everything goes to `raw/timing/`.

**Page cache.** The run starts with one untimed 1-pass run of every variant, so the model and the 1.1 GB record file are warm, as the model was in 121. A cold-cache setup measurement needs `sudo purge` before each run. That is optional, and must be reported separately if done.

## Artifacts (SHA-256, `raw/timing_artifact_hashes.txt`)

| Variant | bench_ext | Notes |
|---|---|---|
| original | `015-integer-keys/artifacts/bench_ext_baseline` `c2ce2506…c77e` | suite.json baseline |
| cpi | `119-component-phrase-ids/artifacts/cpi/bench_ext` `c5612825…c43c5a` | equals `final/119-cpi/build_hashes.txt` |
| 160ctl | `artifacts/160ctl/bench_ext` `58e63cf2…a2170` | cpi sources unchanged (all 15 src hashes = 119), own PGO; separates the rebuild effect |
| 160b | `artifacts/160b/bench_ext` `11abdbe9…faf8` | stage B: records file, string keys resolved to slots (`EXP_RECORDS`), lib `e03c8a60…06a7` |
| 160c | `artifacts/160c/bench_ext` `d3e0b157…aedfe` | stage C: emitters push slots directly (`EXP_RECORDS_DIRECT`), lib `4277dd86…5674` |

160b and 160c also need the record file at its compiled default path, `artifacts/records/address_parser_records.dat` (SHA-256 `3792d23d…0328`, 1,117,339,520 bytes). The script hashes it before timing. Do not move it.

## What each measurement decides (kill rule, `KILL_RULE.md`)

| Kill-rule item | Measurement | Decision, declared before timing |
|---|---|---|
| **Any output difference** | — | **Already decided: no difference** (README, correctness section). Not part of timing. |
| **Gain < +3% against cpi on golden-v1 and OSM** | bench_ext addresses/s: per-round speedup = variant / cpi; median over 5 rounds of the median of passes 2–20 (warm), and pass 1 (cold) separately | **Kill** a variant (160b or 160c, each judged on its own) if its median gain is < +3% on golden-v1 **or** on OSM, judged on the cold first pass **and** on the warm passes (METHOD, Generalization protocol §3): a variant must reach +3% on both. Report 160ctl / cpi so a rebuild effect is visible. 157 projected +4–8% (UNVERIFIED). |
| **Peak > original − 20 MB** | `/usr/bin/time -l`: "peak memory footprint" and "maximum resident set size", max over all runs of each variant | See the next section. **Kill** if the records-aware peak, P, of 160b or 160c is greater than the original's max peak footprint − 20 MB. |
| Steady memory (reported, not a kill item) | `footprint -p` + `vmmap --summary` "mapped file" resident at pass 3 | Compare footprint + mapped-file resident against cpi's and the original's footprint. |
| Load time ≤ cpi's 11.5 s (P2 goal; not in the kill list, but a goal) | bench_ext `setup_seconds=` | Report the median per variant, warm page cache. A cold-cache figure needs `purge`. |

### Why footprint alone cannot decide the memory item

- **What footprint counts.** macOS "peak memory footprint" (phys_footprint) counts dirty and compressed memory. It does **not** count clean file-backed pages.
- **How 160 loads the records.** 160b and 160c map the 1.1 GB record file read-only (`MAP_PRIVATE`) and touch every 16 KiB page at setup. So most of the arena and index is resident, yet absent from footprint.
- **Consequence.** Footprint alone would pass the −20 MB rule trivially. The decision therefore uses:

  **P = peak footprint + 1,117,339,520 B** (the whole record file, treated as resident; a conservative upper bound, since setup prefaults every page).

  Max RSS (which counts mapped pages) is reported beside it for every variant. If max RSS and P disagree about the kill, report both and mark the item inconclusive. **Do not drop the mapped bytes.**

**Indicative numbers only.** These were measured under parallel load with other teammates' model processes running, so they decide nothing (`raw/memory_indicative/`):
- **Load-only peak footprint:**
  - original 2,491 MB;
  - cpi 2,401 MB;
  - 160ctl 2,401 MB;
  - 160b 1,020 MB;
  - 160c 1,020 MB.
- **P for 160b and 160c:** ≈ 1,020 + 1,066 = 2,086 MiB, against original − 20 = 2,471 MiB. This passes if it holds under timing conditions (UNVERIFIED).
- **Max RSS** varied by ±350 MB between identical code (cpi 2,002 vs 160ctl 2,354), which is why it is noisy here.
- **Steady state on a live fuzz run:**
  - 160b: footprint 523 MB + mapped file 1.0 GB resident;
  - 160c: footprint 523 MB + mapped file 1.0 GB resident;
  - 160ctl: footprint 1.9 GB.

## Not requested

The novel set and unseen sets are not in the timing request: the kill rule names golden-v1 and OSM. If a variant passes, the M-milestone protocol (121/103: novel, unseen-v12 once) is the lead's next step, and v12 must not be opened before that.
