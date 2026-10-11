# v15 test-set source scouting (2026-10-11)

Goal (from the user): a reviewer-style test set of at least 2 million real addresses, covering all 88 countries in the 203 datasets. The target was at least 20,000 addresses per country, ideally 100,000, with a per-country warm-speed comparison. **No address data was downloaded.**

Only metadata was fetched:

- the OpenAddresses source index, `batch.openaddresses.io/api/data`, 1,409,133 bytes;
- job records for 18 borderline countries;
- the Geofabrik extract index, 512,426 bytes;
- the Overture addresses documentation page;
- 31 OpenStreetMap address *count* queries to the public Overpass API.

Hashes of the two index files are in `raw/metadata-index-sha256.txt`.

## libpostal's own coverage

- libpostal's language map (`resources/language/countries/country_language.tsv`) lists **251 country codes**. It ships address dictionaries for **60 languages**. Its parser was trained on OpenStreetMap worldwide plus OpenAddresses data for 42 countries (`resources/parser/data_sets/openaddresses.yaml`).
- The **88 countries** in this project are those present in the 203 datasets, not a libpostal limit. Testing all 251 codes at 20k each is not possible: many territories and microstates have fewer than 20,000 addresses in total.

## Result

How each count was obtained:

- **OpenAddresses:** exact `count` from job records where a country was borderline. Otherwise the file size divided by 27 bytes per address, calibrated on `ae/az/drm-ar` (19,642 addresses in 534,336 bytes). Size estimates are used only for countries estimated at 800k or more.
- **OpenStreetMap:** Overpass counts of objects tagged with both `addr:housenumber` and `addr:street`, for the 30 countries OpenAddresses does not cover.

| Group | Countries | Notes |
|---|---:|---|
| 100k or more available | 61 | 50 via OpenAddresses, 11 via OpenStreetMap |
| 20k–100k | 13 | e.g. Cyprus 80,934, Israel 81,562, Faroe Islands 26,472 |
| Under 20k (cannot reach 20k from these sources) | 9 | Liechtenstein 12,966 (whole country), San Marino 11,575, Isle of Man 6,856, Guatemala 6,766, Suriname 7,898, Ecuador 12,999, North Macedonia 14,148, Tajikistan 19,379; China 3,428 in OpenAddresses (Wuhan only), OSM count timed out |
| Unknown | 5 | UK, India, Malaysia: Overpass count timed out. Puerto Rico, Réunion: not country-level areas in OSM, so the query found none; needs a different query |

- With all available addresses capped at 20,000 per country, the total is **1,576,015**: 74 countries reach 20k, and the rest contribute what exists.
- Capped at 100,000 per country, the total is **6,897,178**.
- **2 million from all 88 countries with at least 20k each is not possible**, because 9 countries have fewer than 20k open addresses. **100k for every country is not possible** either; only 61 of 88 reach it.
- **About 2 million across 74 countries with at least 20k each is possible**, plus whatever exists for the other 14. For example, 27,000 per country × 74 is about 2.0 million.

**Download estimate** for the 58 OpenAddresses countries: about 2.6 GB for the 20k plan and 2.75 GB for the 100k plan. Most of it is single countrywide files that cannot be split: Mexico 849 MB, Netherlands 319 MB, Finland 256 MB, Portugal 203 MB, Chile 116 MB, Denmark 110 MB, Egypt 107 MB, Serbia 97 MB, Estonia 96 MB, Czechia 79 MB, Greece 78 MB. For several of these (NL, FI, PT, CL, DK, CZ, MX), the Overture GeoParquet release can be filtered by country with DuckDB, which may transfer less. That is unverified.

The 30 OSM-only countries would use Overpass queries that return only addresses. Each country is roughly 10–40 MB at 100k; that size is an estimate and has not been measured.

Full per-country table: [table.md](table.md). Source selection per country, which spreads across regions and takes the smallest files first: `raw/plan.json`.

## Limits

- The OpenStreetMap counts include every object with a house number and street, so some buildings are counted more than once. They were taken from the public server on 2026-10-11.
- The OpenAddresses size estimates assume 27 bytes per address. Sources with more fields per record would hold fewer addresses than estimated. Only countries estimated at 800k or more rely on the estimate.
- Licenses differ by source: OpenAddresses is per source (many attribution or share-alike), OpenStreetMap is ODbL, and Overture is per source. A test set built from them should stay outside the repository, or carry the matching attribution.
- Converting structured records to one-line addresses uses a simplified per-country order (`benchmark/build_dataset.py`). That affects how hard the test is, not whether the two builds agree.
