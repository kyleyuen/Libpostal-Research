# Golden v3 — independent held-out set from OpenStreetMap

Built 2026-10-01 by `build_osm.py` (source of truth; `fetch` then `build`). Independent of golden-v1 and the novel set, which both come from the
Senzing libpostal test data: these addresses are fetched directly from OpenStreetMap through the Overpass API (maps.mail.ru mirror; the main server
returned 504s), 700 m around the centre of 64 listed cities on six continents, and assembled from each object's `addr:*` tags in the country's usual
order (templates in the script). At most 200 distinct addresses per city (chosen by SHA-256 order); exact duplicates of golden-v1 or novel strings
are removed (0 occurred). Rome's fetch failed after four retries and is absent.

- `osm.tsv` (`country<TAB>address`) and `osm.txt`: **11,405 addresses, 49 countries, 5,154 non-ASCII (45%)**; mean 5.1 words / 34 bytes
  (golden-v1: 5.6 / 39). Thin cities: Mumbai 7, Dubai 33, Houston 113 addresses.
- `raw/`: the Overpass JSON responses as downloaded; `fetch.log`, `build.log`; `SHA256SUMS`.
- Not ground truth: the parser's output on these strings is compared with the original library's, not with the OSM tags.

Data © OpenStreetMap contributors, available under the Open Database License (ODbL), https://www.openstreetmap.org/copyright.
