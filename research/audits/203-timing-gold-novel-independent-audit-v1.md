# Experiment 203 golden-v1/novel timing cohort: independent audit

Audit scope: saved `203-stream-gold-novel-v1` timing artifacts only. No workload was run for this review. This memo covers two datasets, not the full experiment 203 campaign; OSM validation remains pending.

## Capture validity and environment

`invocation.json` declares the original 000 binary and `cpi-stream-203`, datasets golden-v1 and novel, and stages `integrity,timing,report`. `run.log` records all 12 planned captures (2 datasets × 2 roles × 3 rounds), each exiting 0. All 12 entries in `gate.log` report AC power. Each of the six candidate stderr files contains the active streaming marker `PC_COMPRESS_STREAM active=1 rows=5483892 edges=122300179 bytes=151033002`; original stderr files contain no such marker, as expected.

Every measured parse interval exceeds the 10-second minimum: I summed the per-pass `us_per_addr × address_count` values across each 200-pass capture. Durations range from 13.072 seconds (golden-v1 candidate, round 1) to 84.465 seconds. Setup is materially slower for the candidate: median 9.871 seconds vs 2.818 on golden-v1, and 9.849 vs 2.789 on novel. Load varied substantially: the recorded capture loads range 2.32–18.68 after the initial settle sample at 2.18. Results should retain this noise context.

## Paired timing and memory

The saved results report warm medians of 31.860 µs/address original vs 5.283 candidate on golden-v1, and 32.367 vs 5.456 on novel. The per-dataset geometric means of round speed ratios are 6.107× and 5.832×; their median round ratios are 6.043× and 5.932×. The two-source-weighted aggregate is 5.988× for this cohort only.

I parsed each timing stderr and paired candidate/original captures by dataset and round. All six paired peak RSS and peak-footprint deltas are negative:

| Dataset | Round | Peak RSS delta (B) | Peak footprint delta (B) |
|---|---:|---:|---:|
| golden-v1 | 1 | -635,338,752 | -502,775,976 |
| golden-v1 | 2 | -502,710,272 | -502,792,360 |
| golden-v1 | 3 | -502,775,808 | -502,857,896 |
| novel | 1 | -502,710,272 | -502,825,128 |
| novel | 2 | -502,890,496 | -503,021,760 |
| novel | 3 | -502,792,192 | -502,907,048 |

The larger golden-v1 round-1 RSS reduction reflects a higher original RSS capture; it is still a valid paired capture with zero swaps recorded. Cohort maxima agree with `results.json`: original RSS 2,614,149,120 B / footprint 2,613,348,584 B; candidate RSS 2,111,258,624 B / footprint 2,110,326,824 B. The run did not include a steady-memory or thread stage, so this memo makes no steady-state or thread-count claim.

## Scope

This evidence supports the paired timing and peak-memory result for golden-v1 and novel under this run's captures. It does not complete experiment 203 or override its earlier, separately measured 202 peak-footprint failures. OSM and other declared validation sets still need their own eligible paired captures before any full-campaign memory conclusion.
