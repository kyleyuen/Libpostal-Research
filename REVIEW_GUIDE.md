# Review guide

This guide is for a source and evidence review of the libpostal CPI optimization. Begin with the [project overview](README.md), then read the [promoted 203 candidate record](research/final/203-cpi-memory/README.md), its [build record](research/final/203-cpi-memory/BUILD.md), and the [measurement method](research/METHOD.md). To rerun the comparison yourself, including on your own address file, see [TESTING.md](TESTING.md). The [experiment ledger](research/experiments/README.md) is the chronological index of successful and unsuccessful work. The earlier [119 CPI build](research/final/119-cpi/BUILD.md) and [202 validation report](research/experiments/202-cpi-final-validation/FINAL_VALIDATION_REPORT.md) remain useful for understanding the original memory regression and historical results.

## What was checked

The 203 candidate output comparison covered 14 datasets and 130,341 rows. Candidate dumps were compared byte-for-byte with cached original-generated JSONLs. Thirteen references were older caches; the v14 original reference came from a fresh original run in 202, not a new original run in 203. Input and artifact hashes were checked, along with frozen golden-v1 and novel outputs. A negative control changed one output byte and confirmed the comparison detected it. These finite checks do not prove correctness for all addresses or API states. Two candidate fuzz jobs matched preserved original digest/status references, but did not rerun the original fuzz jobs or compare each parser output byte-by-byte.

The timing cohorts used three rounds and group-specific passes: 200 for Senzing, 420 for OSM, and 2,300 for v14. All 84 captures exited normally, met the 10-second minimum, and passed the AC-power gate; system load varied. Warm-parsing ratios were 6.11× for golden-v1, 5.83× for novel, 5.55× geometric mean across the 11 OSM datasets, and 5.72× for v14. These cohorts were timed separately, so there is no pooled 203 speed estimate. Every paired per-round ratio exceeded 1. Candidate startup was slower in each cohort: 9.64 s vs 2.79 s median, so the candidate only finishes sooner overall after about 260,000–550,000 addresses in one process (see the README [Startup cost](README.md#startup-cost) section).

Across 42 original/candidate pairs from 84 captures, candidate peak RSS and physical footprint did not exceed the corresponding original readings. Peak-footprint reductions ranged from 502,775,976 to 517,472,400 bytes. In the historical 202 check, 119 CPI passed the global maximum comparison by 70,500,184 bytes but failed all six Senzing per-pair footprint checks: golden-v1’s maximum delta was +131,236,152 bytes, and the Senzing subgroup maximum was +58,982,640 bytes (novel, round 2). The streaming 203 candidate passed the paired gate. Steady memory and thread evidence are representative samples, not measurements for every dataset and process state. Direct live samples for golden-v1, OSM v3, and v14 observed one parser thread. Each of the six baseline/candidate roles had six live samples after at least 50 parse passes; every sample showed one thread. Thread-capture processes were intentionally terminated after sampling.

## Limits to keep in view

- Public API ([experiment 204](research/experiments/204-public-api-parity/README.md)): on 4,356 addresses, 7 of 8 API families were byte-identical between fresh original and 203 builds. `libpostal_parser_print_features(true)` aborts in 203 by design (`EXP_ROW_PACK` guard), and upstream `libpostal_is_toponym_duplicate` crashes identically in both builds after 189 pairs. These are fresh setup builds, not the archived 203 binaries, and the inputs are finite.
- The 203 fuzz checks compare candidate digest/status streams with frozen original references. They are not per-result byte comparisons and did not rerun the original fuzz campaign.
- Do not describe all 14 as freshly parsed by both builds: 203 candidate dumps were compared against cached references; the v14 original reference was generated freshly in 202.
- The historical Senzing peak regression, slower startup, varying system load, and finite address coverage should remain visible alongside warm parsing speedups.
- For Senzing raw fields, an empty CSV cell is stored as an empty string; its real-world meaning is unspecified. A city yielding no usable OSM rows contributes no input row. Parser outputs are comparison references, not ground truth.

## 203 review evidence paths

From the repository root, run `python3 research/tools/verify_203_clone_evidence.py` to verify the included source/package hashes, all 14 output pairs, timing records, paired memory deltas, and live thread samples. This checks preserved evidence; it does not rerun parsing or validate the external model files. The original dataset manifest can also be checked with `cd profiling && shasum -a 256 -c SHA256SUMS`; in a clone, exactly 6 entries fail because those files (`golden-v1/records.jsonl` and the `parser_profile` binaries and their debug symbols) are too large or binary for git. Their sizes and hashes are listed in `research/EXCLUDED_FILES.sha256`.

- Promoted source and build identity: `research/final/203-cpi-memory/`.
- Inputs and dataset provenance: `profiling/`, including `profiling/golden-v14-osm-unseen/`.
- All 14 candidate output JSONLs: `research/suite/results/203-stream-output-v1/correctness/`; the 14 original JSONLs are in `research/suite/results/203-stream-output-v1/clone-evidence/original/`. `clone-evidence/manifest.json` maps each original cache to the candidate output and records rows and hashes.
- Raw timing stdout, stderr, and exit records for the 84 captures: `research/suite/results/203-stream-gold-novel-v1/timing/`, `research/suite/results/203-stream-osm-v3-v13-v1/timing/`, and `research/suite/results/203-stream-v14-v1/timing/`.
- Representative live steady-memory samples: `research/suite/results/203-stream-steady-golden-v1/`, `research/suite/results/203-stream-steady-osm-v3-v1/`, and `research/suite/results/203-stream-steady-v14-v1/`.
- Six live thread captures and their gate/environment records: `research/experiments/203-cpi-memory-regression/raw/stream-steady-thread-v1/`, including `thread-captures/results.json`.

The 203 final validation report links these paths and explains each gate. The report and files support the default-parser findings; public-API coverage is in experiment 204.

## Data and result formats

`profiling/golden-v1/all.txt` contains one address per line. The paired benchmark TSV files have no header and contain a lowercase two-letter country code, a tab, and the address; rows align with their text files. The Senzing CSV has 21 source fields, listed in [README.md](README.md). `records.jsonl` entries contain `id`, `address`, `country_codes`, `source_rows`, and `split`; nested `source_rows` carry source CSV fields. Frozen `baseline-outputs.jsonl` entries contain `id` and ordered `components` pairs (`label`, `component`). Live dump JSONL uses `address` and `components`.

For the source map and evidence paths, see [README.md](README.md). For a locally prepared environment, use [SETUP.md](SETUP.md). The repository is private and requires GitHub access. The 203 review uses the repository paths listed above; setup retrieves the external model only if building is needed.
