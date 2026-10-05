# Experiment 137 — model-derived hot tier of weight rows (EXP_HOT_TIER; the user's layered-model idea)

Date/time/timezone: created 2026-10-04T16:10:14-0700 (`raw/created.txt`); screen 2026-10-04T16:13:47-0700 → 2026-10-04T16:19:59-0700; America/Los_Angeles; commands in `raw/commands.txt`. Status: **rejected (slower)**

## Change
`raw/crf.diff`, `raw/address_parser.diff`; based on 136.
- **Building the tier (load).** Vocabulary words are ranked by the model's own training counts (ties by slot). Their role-feature weight rows (state word / prev / next / first word; transition word / prev word) are copied record for record into one compact region per weight table until **4 MB**, a budget fixed in advance from the hardware and not tuned on any dataset.
  - Result: 14,002 state rows (123,660 records) and 9,507 transition rows (286,389 records), 4.00 MB.
- **Routing.** The role table's entries for those rows are rewritten to virtual feature ids ≥ the table's row count. `crf_hot_row()` resolves any id to its records in the original rows or in the copy.
  - Every row reader on the active path uses it: lookup-stage prefetch, weight prefetch, rt8's every-line transition prefetch, both accumulation loops.
  - Same records, same order, so the arithmetic is unchanged.
- **Memory.** Load peak is unchanged (2.517 GB); bench peak RSS 2,543,108,096 B.

## Correctness (extensive)
- `hov` (+ `EXP_HOT_TIER_VERIFY`: every hot-row use is compared record by record with the original row) and `hot`: identical on golden-v1, novel, OSM, unseen v4, both frozen references and fuzz seeds 1 and 2. About 117.8M hot-row uses were checked (1.18M on the corpora, 116.6M on fuzz), 0 mismatches (`raw/verify_*`).
- `hot` is also identical to each set's original-library output on unseen v5, v6, v7, v8, v9, v10 and v11: 63,127 more addresses (`raw/extra_outputs/summary.txt`).

## Speed
Screen vs cpi (6 rounds; the machine was in interactive use, load average 3–6.6, see `raw/screen/load.log`; `raw/screen_analyze2.stdout`):
        v1 median cpi 5.817 hot 5.728; hot speedup per round [0.9895, 1.0156, 0.9712, 1.0647, 0.9929, 0.9887] median 0.9912; rounds faster 2/6
        v1 min    cpi 5.499 hot 5.533; hot speedup per round [0.9939, 0.9893, 0.9812, 1.007, 0.9972, 0.9868] median 0.9916; rounds faster 1/6
        osm median cpi 3.568 hot 3.63; hot speedup per round [0.9577, 0.967, 1.003, 0.9478, 1.0072, 0.9585] median 0.9627; rounds faster 2/6
        osm min    cpi 3.429 hot 3.497; hot speedup per round [0.9765, 0.9836, 0.9621, 0.9506, 1.0271, 0.9968] median 0.9801; rounds faster 1/6
Stage timers, 5 alternating rounds (`raw/split_analyze.stdout`):
    dev
       total_parse        cpi 5.899/2.532  hot 6.064/2.622  Δcold +0.165 Δwarm +0.090
       context_fill       cpi 1.856/0.388  hot 1.932/0.387  Δcold +0.076 Δwarm -0.001
       features           cpi 1.462/0.708  hot 1.495/0.703  Δcold +0.033 Δwarm -0.005
       lookups            cpi 0.697/0.206  hot 0.753/0.225  Δcold +0.056 Δwarm +0.019
       accumulate         cpi 0.856/0.484  hot 0.898/0.530  Δcold +0.042 Δwarm +0.046
       viterbi_and_rest   cpi 0.489/0.441  hot 0.487/0.438  Δcold -0.002 Δwarm -0.003
       accumulate cold Δ per round [0.061, 0.043, 0.04, 0.059, 0.02]
    osm
       total_parse        cpi 4.557/2.242  hot 4.007/2.253  Δcold -0.550 Δwarm +0.011
       context_fill       cpi 1.380/0.360  hot 1.084/0.347  Δcold -0.296 Δwarm -0.013
       features           cpi 1.050/0.588  hot 0.919/0.574  Δcold -0.131 Δwarm -0.014
       lookups            cpi 0.543/0.171  hot 0.473/0.188  Δcold -0.070 Δwarm +0.017
       accumulate         cpi 0.697/0.452  hot 0.670/0.486  Δcold -0.027 Δwarm +0.034
       viterbi_and_rest   cpi 0.435/0.395  hot 0.420/0.388  Δcold -0.015 Δwarm -0.007
       accumulate cold Δ per round [-0.073, -0.013, 0.044, -0.019, 0.009]
Development accumulation is slower in every round: +0.04 µs cold, +0.05 µs warm. OSM warm accumulation is +0.03 µs. The OSM cold totals were disturbed (cpi's context fill, untouched by this change, moved 0.3 µs).

## Decision
Rejected.
- **The hot rows already behave as hot.** They are by construction the most frequently used and evidently already stay cached across addresses. Packing them together brings no cold-time saving.
- **The routing check costs compute.** The per-row check and the extra start-table load add about 0.03–0.05 µs per address of warm compute.
- **Bloom filter part:** not built (136: few misses).
