# Audit 203: Senzing peak-memory regression evidence

## Finding

The Senzing subgroup’s `+58,982,640 B` regression is a **peak process footprint** result, not a peak RSS result. In all six paired timing captures—two datasets by three rounds—the CPI peak footprint exceeded the original peak footprint from the same dataset and round. The subgroup maximum is the novel dataset’s round 2 pair.

The historical 14-dataset global maximum remains lower for CPI: original `2,613,905,616 B`, CPI `2,543,405,432 B`, difference `−70,500,184 B`. This satisfies the historical global-maximum comparison, while the Senzing per-dataset peak comparisons fail. The global result does not erase the subgroup regression.

## Paired timing captures

Values are bytes. Peak footprint is the macOS `/usr/bin/time -l` “peak memory footprint” field used by the method’s acceptance metric. Peak RSS is the separate “maximum resident set size” field and is retained as supplemental evidence.

| Dataset / round | Original peak footprint | CPI peak footprint | CPI − original |
| --- | ---: | ---: | ---: |
| golden-v1 / r1 | 2,383,972,392 | 2,537,343,352 | +153,370,960 |
| golden-v1 / r2 | 2,349,729,832 | 2,537,343,352 | +187,613,520 |
| golden-v1 / r3 | 2,406,107,200 | 2,537,343,352 | +131,236,152 |
| novel / r1 | 2,450,114,648 | 2,537,343,352 | +87,228,704 |
| novel / r2 | 2,478,360,712 | 2,537,343,352 | +58,982,640 |
| novel / r3 | 2,474,674,312 | 2,537,343,352 | +62,669,040 |

CPI recorded the same peak-footprint value in all six captures. Original peak footprint varied by `56,377,368 B` across golden-v1 rounds and `28,246,064 B` across novel rounds. The six paired comparisons therefore do not reduce to one anomalous CPI peak sample.

Peak RSS has a different pattern. Per-round original/CPI RSS values were:

- golden-v1: r1 `2,383,380,480 / 2,051,260,416`; r2 `2,349,137,920 / 2,438,922,240`; r3 `2,324,086,784 / 2,292,432,896`.
- novel: r1 `2,449,489,920 / 2,210,512,896`; r2 `2,211,233,792 / 2,537,046,016`; r3 `2,474,016,768 / 2,313,355,264`.

Only golden-v1 r2 and novel r2 have higher CPI RSS than their paired original. RSS must not be substituted for the peak-footprint gate.

## Separate steady-footprint sample

`memory.json` reports steady footprint on golden-v1 only, after three warm passes: original `2172 MB`, CPI `1927 MB`. This is a separate steady-state measurement. It does not offset the six higher CPI peak-footprint readings; there is no paired steady sample for the novel dataset in this run.

## Evidence and method

The pairing table was read from these raw `/usr/bin/time -l` records:

- `research/suite/results/202-final-senzing-v2/timing/golden-v1__{original-000,cpi-packaged}__r{1,2,3}.stderr` and matching `.stdout` files.
- `research/suite/results/202-final-senzing-v2/timing/novel__{original-000,cpi-packaged}__r{1,2,3}.stderr` and matching `.stdout` files.
- `research/suite/results/202-final-senzing-v2/results.json` and `report.md` contain per-dataset maxima and the subgroup summary; `memory.json` contains the separate steady samples.
- The historical full-suite maxima are in `research/experiments/202-cpi-final-validation/FINAL_VALIDATION_REPORT.md` and its linked provenance.

The method defines peak memory difference as `max(candidate peak footprint) − max(baseline peak footprint)` and treats RSS as supplemental. It also retains peak and steady footprint as constraints. This audit reads existing artifacts only; it adds no build, parser run, or new acceptance result. A fix remains pending.
