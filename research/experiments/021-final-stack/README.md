# Experiment 021 — Stack the remaining positives on 019 `q2`

Date/time/timezone: started 2026-10-01 05:20 PDT (unattended)
Status: **complete.** Best `s3` (q2 + constant ids + mimalloc): **4.08x golden-v1, 4.08x novel, 4.04x floor, 4.08x golden-v3-osm, 4.03x ASCII**; peak 2.113 GB (original 2.613 GB); byte-identical.

## Hypothesis and change

Session 2 (experiment 020) left two untested-together positives: mimalloc (+0.7-1.3% on `p1`) and a new switch `EXP_CONST_IDS` (experiment 025 item:
the value-less constant keys `bias` — once per token — and `postcode have context` are looked up once at load and pushed as resolved ids, like the role
ids; exact, same immutable dictionary answer). All builds use the same script (`tools/build_cc.sh`, Apple clang 17, whole-library PGO on golden-v1
development + LTO, tokenizer -O1) on 019's candidate sources with `q2`'s switches:
`s0` = q2 (reference rebuilt by this script), `s1` = + EXP_CONST_IDS, `s2` = + mimalloc, `s3` = + both. Prediction: none.

## Log of work

Builds `s0`-`s3` and `s4` (= s1 + 016 T6 two-byte phrase prefilter, added after g1 lost 2.5% to test the prefilter alone) all byte-identical
(dumps, frozen references, 2 x 2 M fuzz; `raw/verify_*`). Peak footprint (harness, `raw/peak_memory.txt`): s0 2.246 GB, s1 2.245, s2 2.113, s3 2.113.

## Results (AC, 05:15-05:58 PDT from raw/timing3/start.txt and DONE; corrected — first written as an estimate "~05:35-06:30", `raw/timing3/`): speedup vs the same-session original by median (all estimators agree)

| build | adds | golden-v1 | novel | floor | golden-v3-osm | ASCII |
|---|---|---:|---:|---:|---:|---:|
| `s0` | q2 (reference) | 4.010x | 4.006x | 3.962x | 3.979x | 3.956x |
| `s1` | + constant ids | 4.029x | 4.019x | 3.967x | 4.000x | 3.980x |
| `s2` | + mimalloc | 4.045x | 4.046x | 4.001x | 4.035x | 3.995x |
| **`s3`** | + both | **4.080x** | **4.076x** | **4.036x** | **4.078x** | **4.026x** |
| `s4` | s1 + 2-byte phrase prefilter | 4.020x | 4.013x | 3.958x | 3.987x | 3.958x |

Original µs/address this session: floor 32.81, golden-v3-osm 21.43, ASCII 31.56; `s3`: 8.13 / 5.26 / 7.84.

## Decision

**Accepted: `s3` is the new best candidate** (first build above 4x on every dataset). The two-byte prefilter is neutral here and is dropped.
Next: micro-batch on `s3` — skip clearing the unused transition row 0 (`EXP_ST_ROW0`) and dictionaries at load factor 0.5 (`EXP_LOAD50`).

## Micro-batch on `s3` (AC, 06:06-06:41 PDT from raw/timing4/start.txt and DONE; `raw/timing4/`)

`m1` = s3 + `EXP_ST_ROW0` (Viterbi never reads transition row 0, so it is not cleared); `m2` = s3 + `EXP_LOAD50` (all dictionaries at load factor 0.5
instead of 0.75: linear-probing misses — ~16% of feature lookups and every unknown word — probe ~2.5 instead of ~8.5 slots); `m3` = both.
All byte-identical (`raw/verify_m*`). Harness peak: m1 2.113 GB, m2/m3 2.264 GB (original 2.613 GB).

| build | golden-v1 | novel | floor | golden-v3-osm | ASCII |
|---|---:|---:|---:|---:|---:|
| `s3` | 4.052x | 4.065x | 4.010x | 4.044x | 4.014x |
| `m1` | 4.065x | 4.087x | 4.009x | 4.071x | 4.019x |
| `m2` | 4.230x | 4.289x | 4.214x | 4.222x | 4.210x |
| **`m3`** | **4.253x** | **4.306x** | **4.237x** | **4.252x** | **4.228x** |

Accepted: **`m3`** (4.25x / 4.31x / floor 4.24x / OSM 4.25x / ASCII 4.23x). The load factor is the biggest single lever since 019 — next: lower load
factors for the feature dictionaries (the vocabulary must stay >= 0.44 because its slot index is packed into 23 bits).

## Load-factor sweep on `m3` (AC, 06:47-07:14 PDT from raw/timing5/start.txt and DONE; `raw/timing5/`)

`EXP_LOADF=k` puts the feature dictionaries at load 1/k (vocabulary stays at 0.5: its slot index must fit in 23 bits and the role-id table scales with it).
`l3` = k 3, `l4` = k 4, both on m3's switches. Both byte-identical (`raw/verify_l*`). Harness peak: l3 2.380 GB, l4 2.471 GB (original 2.613 GB).

| build | golden-v1 | novel | floor | golden-v3-osm | ASCII |
|---|---:|---:|---:|---:|---:|
| `m3` | 4.277x | 4.309x | 4.214x | 4.235x | 4.225x |
| `l3` | 4.302x | 4.353x | 4.270x | 4.274x | 4.273x |
| **`l4`** | **4.330x** | **4.358x** | **4.269x** | **4.314x** | **4.303x** |

`l4` memory, measured after the timing: steady phys_footprint 2,250 MB vs the original's 2,492 MB (`raw/steady_footprint_l4.txt`); load time 8.7 s vs
3.4 s (`raw/load_time_l4.txt`). Gains flatten (l3 → l4 +0.3-0.9%) while memory headroom shrinks to 142 MB, so the sweep stops at k = 4.

**Accepted: `l4`, the final candidate**, packaged in `research/final/021-l4/` (sources, build script, hashes, BUILD.md).
