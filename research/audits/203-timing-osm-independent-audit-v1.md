# Experiment 203 OSM timing cohort: independent audit

Audit scope: saved artifacts for `203-stream-osm-v3-v13-v1`; no workload or build was run for this review. This memo covers the 11-set OSM cohort only. The separate v14, steady-memory and live-thread evidence must still be completed before a full 203 decision.

## Capture and provenance checks

The invocation declares 11 datasets (102,814 input addresses), two roles, three rounds and 420 passes. I found 66 timing capture files; all 66 `.exit` records and run-log entries report exit 0, and all 66 per-capture gate records report AC power. The initial 180-second soft load-settle did not reach its threshold (recorded load averages 8.76/12.64/10.54); the run continued under the hard gate. Per-capture loads in the run log ranged from 2.07 to 7.94.

All 66 stdout files contain the configured 420 per-pass timing rows. I calculated parse duration as the sum of `us_per_addr × address_count` over those passes (microseconds converted to seconds), excluding setup time. The minimum capture duration is 10.552 seconds (`unseen-v11`, candidate, round 1); the maximum is 91.534 seconds. Thus all captures meet the declared 10-second parse-work floor.

All 33 candidate stderr files contain the exact active-stream marker `PC_COMPRESS_STREAM active=1 rows=5483892 edges=122300179 bytes=151033002`. The OSM manifest's three original and three candidate artifact hashes, seven model-file hashes, and suite runner hash match those in the golden/novel timing cohort manifest. The dataset input hashes and address counts are pinned in the OSM manifest. The separate correctness-only run `203-stream-output-v1` includes these OSM datasets; this OSM timing invocation itself ran only integrity, timing and report stages.

## Paired peak RSS and footprint

For each dataset, I paired candidate and original captures by round and parsed `maximum resident set size` and `peak memory footprint` from the respective `time -l` stderr. Each table cell below is `(RSS delta B / footprint delta B)`, candidate minus original:

| Dataset | Round 1 | Round 2 | Round 3 |
|---|---:|---:|---:|
| osm-v3 | -504,725,504 / -505,102,504 | -504,807,424 / -505,184,424 | -504,643,584 / -505,168,040 |
| unseen-v4 | -504,397,824 / -504,692,928 | -504,627,200 / -504,922,304 | -504,561,664 / -504,856,768 |
| unseen-v5 | -505,905,152 / -506,658,984 | -506,150,912 / -506,396,840 | -506,216,448 / -506,478,784 |
| unseen-v6 | -505,397,248 / -505,741,480 | -505,282,560 / -505,807,040 | -505,397,248 / -505,741,480 |
| unseen-v7 | -505,561,088 / -505,888,936 | -505,462,784 / -505,938,088 | -506,380,288 / -506,708,136 |
| unseen-v8 | -504,430,592 / -504,856,768 | -504,283,136 / -504,676,496 | -504,201,216 / -504,610,984 |
| unseen-v9 | -504,414,208 / -504,774,848 | -504,201,216 / -504,545,448 | -504,414,208 / -504,774,848 |
| unseen-v10 | -505,643,008 / -506,003,648 | -505,577,472 / -505,938,112 | -505,643,008 / -505,970,832 |
| unseen-v11 | -504,840,192 / -505,200,808 | -504,627,200 / -504,987,816 | -504,725,504 / -505,069,712 |
| unseen-v12 | -505,593,856 / -505,987,216 | -505,724,928 / -506,134,696 | -505,626,624 / -506,036,392 |
| unseen-v13 | -504,266,752 / -504,529,064 | -504,430,592 / -504,692,904 | -504,430,592 / -504,725,696 |

All 33 RSS deltas and all 33 footprint deltas are negative. Across this cohort, maximum peak RSS is 2,108,129,280 B for the candidate versus 2,613,362,688 B for original; maximum peak footprint is 2,107,476,008 B for the candidate versus 2,613,053,648 B for original. These cohort maxima may come from different datasets and are not a full-suite result.

## Scope and remaining checks

This audit verifies the OSM cohort's exits, power gates, active stream path, parse-duration floor, provenance consistency and paired peak measures. This timing invocation did not collect steady-state footprint or live thread samples. Do not use this cohort alone to claim experiment 203 passed: the v14 cohort and representative steady/thread checks remain separate requirements, and the earlier golden/novel cohort must remain visible in any final summary.
