# Experiment 202 independent evidence audit

Audit time: 2026-10-10 07:19 UTC. Read-only review of retained timing, correctness,
thread and API records; no parser, model, benchmark or build was run.

## Timing and peak calculation

I recomputed each dataset's result from the captured pass logs for
`202-final-senzing-v2`, `202-ac-osm-v3-v13-resumed-v1`, and
`202-ac-unseen-v14-resumed-v1`. For each role and round, the warm value is the
median `us_per_addr` over passes 2 through the final pass. The paired round
ratio is original divided by packaged CPI; the dataset value is the median of
its three round ratios. This gives 14 sets: 12 `openstreetmap`, one `senzing`
(`golden-v1`), and one `senzing-style` (`novel`).

- Per-source geometric means: OSM 5.782694490x; Senzing 6.033569921x;
  Senzing-style 5.866359447x.
- Equal weighting of those three source-level geometric means: 5.893289233x.
- Unweighted geometric mean of all 14 dataset results: 5.806217304x.
- The 12 OSM datasets are 102,814 rows; Senzing and Senzing-style are 12,863
  rows each; v14 contributes 1,801 rows. Total: 14 datasets / 130,341 rows.
- The maximum recorded peak footprint is 2,613,905,616 B for original 000
  (`unseen-v8`, OSM) and 2,543,405,432 B for CPI (`osm-v3`, OSM), delta
  -70,500,184 B. The Senzing subgroup remains a +58,982,640 B regression;
  v14's pair delta is -94,027,608 B. Do not omit the Senzing subgroup result.

All 84 timing captures exited zero and contained the configured pass count:
12 Senzing captures at 200 passes, 66 OSM captures at 420, and 6 v14 captures
at 2,300. Parsing-only duration is the sum of every pass's `us_per_addr`
multiplied by that capture's `corpus` row count, divided by 1,000,000; setup is
excluded and pass 1 is included. The minimum is 10.791332902 s in OSM
`unseen-v11` CPI round 3. `/usr/bin/time -l real` includes setup and is not the
duration-floor calculation. OSM load varied, and the nine high-load captures
were retained rather than filtered. Source-weighted timing is a geometric
aggregation of per-dataset paired ratios; raw durations were not pooled and
no cross-group pooled confidence interval is supported.

Peak maxima above are the `peak_footprint_max` fields from those same eligible
capture groups. Steady-footprint samples and ordinary suite thread selectors
are representative only: golden-v1, osm-v3 and v14. Senzing and OSM
representative steady results are rounded to MB and do not replace peak
comparisons.

## v14 thread supplement

`raw/v14-thread-supplement-v1/manifest.json` records the v14 TSV SHA-256
`c795d62b629390c7ddf01d416f0d030e0bd9f075cb33d514ce10792b5b05e977`, an AC
gate exit 0 for each role, and the expected original and packaged-CPI
executable hashes (`a4968702…` and `c5612825…`). Each process had six samples
three seconds apart; all 12 samples found the process alive with exactly one
thread. Both processes were intentionally killed after sampling (wrapper
status -9), so this is thread-count evidence only, not a completed parser run
or output check. The capture used 5,000 passes as recorded in the script and
commands. The earlier suite `threads.json` zeroes were post-exit observations
and are not used as evidence.

## Correctness and public API scope

The experiment 202 documentation's default-parser output claim is scoped to
the checked address sets: the original 13 sets compare CPI output against
cached original-000 outputs, while v14 has fresh original/CPI dumps and a
bytewise comparison. Frozen golden and novel checks and the deliberate
negative-control detection are also recorded. The two 202 fuzz seeds compare
CPI digest/status streams with frozen original references; they do not rerun
the original fuzz pipeline or compare every mutated serialization byte.

Do not turn address-output parity into blanket public-API parity. Experiment
150's API probe records that CPI's `features` mode aborted with exit 134
(`EXP_ROW_PACK: only the direct scoring path is supported`), while default
parse and lifecycle probes passed. Separate experiment 201 archive-linked
`cpi-archive-mimalloc` evidence covers 78 edge API output rows with matching
SHA to original and one observed thread; this limited edge-output capture does
not show that the failing feature-print mode was repaired. The final summary
should disclose both facts and avoid saying that no prior API evidence exists:
no new full API suite ran for experiment 202, but limited 78-row API evidence
does exist.

The 202 output/fuzz evidence and all finite corpus checks remain bounded
observations, not proof of universal input or API-state identity. Direct v14
thread evidence is now available via the supplement above; do not describe
the v14 ordinary-suite zero samples as thread evidence.
