# Experiment 203 timing evidence: independent final audit

Audit scope: saved timing artifacts for the golden/novel, OSM and v14 cohorts. No parser, benchmark, build, or model workload was run for this audit. Steady-memory and live-thread evidence is separate and is not covered here.

## v14 cohort

`research/suite/results/203-stream-v14-v1/` declares one dataset (1,801 rows), two roles, three rounds and 2,300 passes per capture. All six capture files and run-log entries show exit 0; all six gate records report AC power. Each of the three candidate stderr files has the active-stream marker `PC_COMPRESS_STREAM active=1 rows=5483892 edges=122300179 bytes=151033002`.

I summed `us_per_addr × address_count` over each capture's per-pass timing lines. All six contain 2,300 pass rows. Candidate parse times are 10.704, 10.966 and 10.976 seconds; original times are 62.104, 63.048 and 63.096 seconds. The minimum exceeds the 10-second floor. Captured load averages range 2.38–3.27. Candidate setup median is 9.687 seconds vs 2.828 seconds for original; startup remains slower.

Each cell is `(RSS delta B / peak-footprint delta B)`, candidate minus original, paired by round:

| Dataset | Round 1 | Round 2 | Round 3 |
|---|---:|---:|---:|
| unseen-v14 | -87,834,624 / -517,292,200 | -517,013,504 / -517,472,400 | -516,833,280 / -517,308,584 |

All three paired deltas are negative for both measures. The round-1 RSS delta is smaller than the other two because original round 1 recorded lower maximum RSS; its paired peak-footprint delta is also negative. This run stages integrity, timing and report only; it provides no v14 steady-state or thread sample.

## Cross-cohort checks

The three timing manifests have identical original/candidate artifact hashes, all seven model hashes, and suite-runner hash. The candidate binary identity is the same in all cohorts. The experiment 203 source files are pinned in the candidate artifact hash record; output audit memo `203-output-independent-audit-v1.md` independently checked the candidate source/artifact hashes and the tested input hashes.

The timing cohorts contain 84 captures total: golden-v1/novel 12, OSM v3–v13 66, and v14 6. Their per-pass settings differ by design (200, 420 and 2,300, respectively), so raw capture durations are not pooled. All 84 saved capture exit codes are zero, all 84 capture gates are AC, and all 42 candidate stderr files across the groups show the active-stream marker. The minimum summed per-pass parse duration across the 84 captures is 10.552 seconds, from the OSM cohort. Every capture therefore meets the 10-second floor.

Paired peak RSS and peak-footprint deltas are nonpositive in the independently audited groups: golden/novel has six negative pairs, OSM has 33 negative pairs, and v14 has three negative pairs for each measure. Detailed pair values are retained in `203-timing-gold-novel-independent-audit-v1.md` and `203-timing-osm-independent-audit-v1.md`.

## Limits

This memo verifies timing-capture integrity and paired peak measures across the three cohorts. It does not report a combined speed estimate, steady-memory result, or live-thread result. The separate 203 correctness run covers exact serialized outputs on its finite 14-dataset set; that evidence does not certify arbitrary inputs. Do not claim whole-experiment 203 acceptance until the separate steady-memory and live-thread evidence is audited, and keep historical 202 failures visible in the final decision.
