# 203 startup cost and break-even

Question: the 203 warm-parsing ratios exclude setup (library and model load). How many addresses must one process parse before the slower 203 startup is repaid?

Inputs: the 84 recorded 203 timing captures in `research/suite/results/203-stream-{gold-novel,osm-v3-v13,v14}-v1/timing/*.stdout`. No new parser runs were made. Each capture contributes `setup_seconds` and per-pass `us_per_addr`; warm cost is the median of passes 2..N.

Method: for each same-dataset, same-round pair, the break-even N solves `orig_setup + N × orig_warm = cand_setup + N × cand_warm`. The cold first pass is ignored, so this is a warm-state estimate. The script also reports the per-dataset speedup two ways: as the suite reports it (geometric mean of per-round ratios) and as [METHOD.md](../../METHOD.md) defines it (median over median).

Command, run from the repository root on 2026-10-11 UTC with Python 3.14.4 (exit 0):

```sh
python3 -I research/analysis/203-startup-breakeven/breakeven.py > research/analysis/203-startup-breakeven/output.txt
```

Result ([output.txt](output.txt)): 42 pairs; median setup 2.79 s for the original vs 9.64 s for the candidate, 6.87 s longer. Break-even ranges from 258,626 to 553,929 addresses, with a median of 477,308. By group it is 262k–272k for golden-v1, 259k–291k for novel, 442k–502k for OSM v3–v13, and 546k–554k for v14. Median-over-median speedups differ from the reported geometric means by at most 0.10× (novel: 5.93× vs 5.83×; golden-v1: 6.03× vs 6.11×).

Limits: there are three rounds per dataset on one machine, and system load varied as recorded in the suite environment files. The cold-pass slowdown is excluded.
