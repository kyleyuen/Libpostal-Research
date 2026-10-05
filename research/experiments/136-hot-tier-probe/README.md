# Experiment 136 — how much of each address's weight-row reads would a model-derived hot tier cover? (diagnostic, read-only)

Date/time/timezone: created 2026-10-04T16:04:57-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: complete

Idea (user, 2026-10-04): a layered model. A small "main" tier holds the most-used parts, the full model sits behind it, and a Bloom filter could rule out lookups that miss.
Rule (user): the hot set must not be chosen from, or tuned to, any dataset.
- **Ranking:** vocabulary words by the model's own training counts.
- **Rows covered:** each word's role-feature rows (word / prev word / next word / first word, transition word / prev word), via the existing slot → feature-id table.
- **Budget:** fixed in advance at 4 MB (fits in the shared L2). 1 MB and 16 MB are printed for context only.
- **Use of datasets:** only to measure coverage afterwards.
Build `hp3` = cpi + `EXP_HOT_PROBE` (counters in `sources/crf.c`, sets built in `sources/address_parser.c`), no PGO.
**Attempt 1 was invalid and is kept** in `raw/attempt1_wrong_count_field/`: it ranked by a dictionary slot field that is not the count, so every word looked equally frequent.

Results, attempt 2 (`raw/probe_dev.stderr`, `raw/probe_osm.stderr`):
- Top words by training count: `DDDDD` (330.8M), `DD` (249.2M), `DDD` (183.3M), i.e. digit-shape placeholders.
- 4 MB holds the role rows of the 4,855 most frequent words (1 MB: 993; 16 MB: 27,936).
- Per address: development reads 38.6 state rows (478 records) and 8.3 transition rows (615 records); OSM 35.9 (455) and 7.5 (587).
- Coverage at **4 MB**: development state 20.6% of rows (28.3% of records), transition 61.7% (87.1%); OSM state 21.4% (29.0%), transition 66.7% (88.6%). At 1 MB / 16 MB: state 18–25%, transition 53–80% of rows.
- **Bloom question:** state keys not found 7.0 of 45.6 per address (15.4%) on development, 4.2 of 40.2 (10.5%) on OSM; transition about 0.2–0.4. A filter could only save a few lookups per address, each of which costs about what one Bloom probe does → not pursued.
Interpretation: a 4 MB model-derived hot tier would cover about 87–89% of transition-weight records and about 28% of state-weight records read per address, consistently on both sets. Whether that saves time depends on whether these rows already stay cached on their own; that can only be measured by building it (relocation plus direct row references).
