# Golden v4 — UNSEEN set from OpenStreetMap (built 2026-10-03 for the user-requested re-measure)

`build_unseen.py` = golden-v3's builder with a disjoint list of 58 cities (none appear in golden-v3; the Senzing-derived golden-v1 and novel sets are city-agnostic but their exact strings are removed). Fetched from the Overpass API 2026-10-02T22:55:24-0700 → 2026-10-02T23:52:22-0700 (`fetch.log`); Bristol and Porto failed after four retries and are absent. Exact duplicates of any golden-v1, novel or golden-v3 string are removed (0 occurred).

- `unseen.tsv` / `unseen.txt`: **10,293 addresses, 42 countries, 4,645 non-ASCII (45%)**; `raw/` Overpass JSON; `SHA256SUMS`.
- Purpose: never used for tuning, screening or selection before experiment 069. Not ground truth: outputs are compared with the original library's.

Data © OpenStreetMap contributors, ODbL, https://www.openstreetmap.org/copyright.
