# Experiment 206 — v15: 7.27 million real addresses from 88 countries

Date/time/timezone: 2026-10-11 14:15–18:11 PDT (21:15 UTC 2026-10-11 – 01:11 UTC 2026-10-12)
Status: completed. Result: **PASS** on correctness, speed, memory and threads; **4.96× warm speedup** (95% CI 4.81–5.12; rounds 4.91–5.04); every one of the 88 countries above 3.5×
Evidence type: final benchmark (reviewer-style test set built from open data; no parser source change)
Protocol: METHOD.md, via `benchmark/run.sh --dataset` (the reviewer path from experiment 205)

## Question

Does the 203 candidate hold its speed, correctness and memory results on a much larger, independent test set covering every country in the earlier datasets? Plan B of the source scouting (`research/analysis/v15-source-scout-2026-10-11/`) asked for up to 100,000 real addresses per country for the 88 countries that occur in the 203 datasets, with at least 20,000 where the open data has them.

No parser source was changed. The candidate is the promoted 203 sources (CPI and the streamed postcode graph, PGO trained on golden-v1 development, full LTO, -O1 tokenizer, mimalloc), built fresh by `benchmark/run.sh`.

## Building the test set

All address data stays outside the repository (`~/v15-data`) because the sources carry their own licences: OpenAddresses per source, OpenStreetMap ODbL. This folder holds what is needed to rebuild it: the scripts, the selection, per-file SHA-256 values and the logs.

1. **OpenAddresses, 60 countries.** `select_sources.py` picks address layers from the OpenAddresses index (`batch.openaddresses.io/api/data`, fetched 2026-10-11; SHA-256 in `../../analysis/v15-source-scout-2026-10-11/raw/metadata-index-sha256.txt`), smallest first, until the estimated count reaches 150,000. The estimate uses 27 bytes per address, and the 150,000 leaves a margin for records without a street or number. A trial build (`raw/trial-oa-manifest.json`) kept fewer than 100,000 for South Korea (many exact duplicate strings) and Australia (many records without a house number), so their targets were raised to 600,000 and 400,000. Puerto Rico (`us/pr/statewide`) and Réunion (`fr/974/statewide`) are filed under other countries in OpenAddresses and are relabelled `pr` and `re`. The result is 288 files, 2.88 GB (`raw/oa-selection.json`), downloaded through `https://v2.openaddresses.io/batch-prod/job/<job>/source.geojson.gz` in 4.5 minutes; every file passed `gzip -t` (`raw/fetch_oa.log`).
2. **OpenStreetMap, 32 countries**, for the countries OpenAddresses lacks, plus Cyprus (its OpenAddresses file has no house numbers), Jamaica and the UAE (most of their records lack a street or number).
   - **Attempt 1, Overpass** (`fetch_osm.sh`, `osm2jsonl.py`). overpass-api.de answered 504 "server too busy", and the maps.mail.ru mirror rejected requests that asked for more time or memory. With default limits the mirror returned San Marino, Guatemala, Isle of Man and Suriname, then 504s again. overpass.kumi.systems returned 500, and overpass.osm.jp had an expired TLS certificate. Log: `raw/overpass-fetch-attempt.log`.
   - **Attempt 2, Geofabrik extracts** (`fetch_geofabrik.sh`, `opl2jsonl.py`; osmium-tool 1.19.1) for the other 28 countries. Each extract is clipped to the country's `boundary=administrative`, `admin_level=2`, `ISO3166-1=XX` relation, with its full geometry fetched from the OSM API. Without the clip, extracts that cover several countries (gcc-states, ireland-and-northern-ireland, malaysia-singapore-brunei) would mix countries. The selection matches the Overpass query: objects tagged with both `addr:housenumber` and `addr:street`. The extract URL, bytes, SHA-256, replication timestamp and boundary relation for each country are in `raw/osm-sources/`. As a check, the clipped counts match the earlier Overpass counts, exactly for North Macedonia 14,148, Tajikistan 19,379, Moldova 982,175, Georgia 126,150, Pakistan 101,078 and Bosnia and Herzegovina 170,926, and closely elsewhere (Ireland 383,382 vs 383,695, with Northern Ireland excluded).
3. **Conversion** with `benchmark/build_dataset.py` (`--per-country 100000 --min-per-country 20000`, seed 20261011, inputs in `raw/build-inputs.txt`). It keeps a seeded reservoir sample per country, deduplicated within the country. There were three attempts, all kept:
   - **Build 1:** 99 countries (`raw/build-attempt-1-99-countries-manifest.json`). OSM `addr:country` tags overrode the country given for the file: 489 records, mostly typos ("VI" inside Vietnam, "BS" inside Bosnia) plus 419 tagged HK inside China's boundary. Fix: a given `CC=` now wins over any country field in the records.
   - **Build 2:** 88 countries, 7,266,620 addresses (`raw/build-attempt-2-manifest.json`). The first benchmark run on it (`research/suite/results/20261011-165806-v15/`) passed correctness, then every timing run exited with status 3. `bench_ext.c:55` returns 3 when a parse has no components, and exactly one line did, in both builds: the Indonesian OSM record `- - -, - -` (house number "-", street "- -"). That run was stopped. Fix: records whose street or house number contains no letter or digit are skipped as placeholders. This is a data rule, not derived from parser output.
   - **Build 3 (used):** 7,266,489 addresses, 88 countries, 0 invalid lines, every address containing a letter or digit, per-country counts equal to the manifest (`raw/v15-manifest.json`, `raw/build-attempt-3.log`). SHA-256 `6ae33602…e23b` (`raw/v15.tsv.sha256`).

Per country: 62 countries have 100,000 addresses, 15 have 20,000–100,000 and 11 have fewer than 20,000. The 11 are az, bd, ec, gt, im, li, mk, sm, sr, tj and tz, and for each of them that is all the open data provides. Five countries unknown in the scouting were found: the UK and Malaysia (100,000 each), Puerto Rico and Réunion (100,000 each, from OpenAddresses) and India (65,213).

## Reproduction

- Hardware/OS/compiler: Apple M4 Pro (Mac16,8), macOS 26.5.2, Apple clang 17.0.0 (clang-1700.6.3.2), on AC power.
- Machine state: restarted before the run; swap 0 MB; load-settle wait passed (1.79 < 2.5 after 60 s). Non-essential user processes were paused with SIGSTOP for the run and resumed afterwards: Photos analysis, Spotlight indexing, Siri and suggestions, iCloud, Safari, Ollama, Finder and widgets. Per-run load averages are in the suite's `env.txt`: 1.07–2.00, where the benchmark itself is 1.0.
- Builds: `WORK_DIR=~/v15-data/work`, pinned libpostal `25099c50`, v1.0.0 model (hash-checked against `benchmark/model_sha256.txt`). `bench_ext` SHA-256 values: original `4b46d87e…51c8fb`, optimized `a756423c…842d01`.
- Command: `~/v15-data/rebuild-and-bench.sh`, which rebuilds v15.tsv, validates it, then runs `WORK_DIR=~/v15-data/work ./benchmark/run.sh --dataset ~/v15-data/v15.tsv --name v15 --model-dir ~/.cache/libpostal-profile/libpostal`.
- During correctness the suite's controller was paused (SIGSTOP), so the user could close apps before timing, then resumed. No timed process was paused.
- Run: `research/suite/results/20261011-172443-v15/` (`VERDICT.md`, `report.md`, `timing/`, `env.txt`).

## Results

| Check | Result |
|---|---|
| Correctness | PASS: all 7,266,489 parses byte-identical; 2 fuzz streams × 2,000,000 inputs identical; negative control detected |
| Speed | PASS: warm 4.96× (95% CI 4.81–5.12). Per round 5.04×, 4.91× and 4.94×. Original 14.632 µs/address, optimized 2.958 µs/address (median of passes 2–3) |
| Memory | PASS: peak RSS and peak footprint lower in 3 of 3 same-round pairs. Max peak footprint 3,022,326,280 B original vs 2,500,659,528 B optimized (−17.3%). Steady footprint 2,546 MB vs 2,314 MB. Not paged: swap 0 |
| Threads | PASS: 6 of 6 samples single-threaded for both builds |
| Startup | 2.65 s original vs 9.54 s optimized; break-even 590,457 addresses per process |

Per country (last timed pass, median over 3 rounds; startup and build excluded):
- 45 countries reach 5.00× or more, 39 are at 4–5×, and 4 are below 4× but above 3.5×: Kosovo 3.83×, South Korea 3.90×, Egypt 3.92× and the UAE 3.94×.
- The highest are Germany 6.17×, South Africa 6.12×, Ireland 6.00×, Peru 5.92× and the UK 5.84×.

| Code | Country | Source | Addresses | Original µs/addr | Optimized µs/addr | Speedup (median) | Min–max over rounds |
|---|---|---|---:|---:|---:|---:|---|
| de | Germany | OpenAddresses | 100,000 | 12.321 | 2.015 | 6.17× | 5.99–6.61 |
| za | South Africa | OpenAddresses | 100,000 | 9.833 | 1.613 | 6.12× | 6.02–6.12 |
| ie | Ireland | OSM (Geofabrik) | 100,000 | 11.247 | 1.874 | 6.00× | 6.00–6.00 |
| gb | United Kingdom | OSM (Geofabrik) | 100,000 | 20.72 | 3.532 | 5.84× | 5.75–6.33 |
| nz | New Zealand | OpenAddresses | 100,000 | 13.018 | 2.259 | 5.74× | 5.73–6.34 |
| au | Australia | OpenAddresses | 100,000 | 16.963 | 3.022 | 5.66× | 5.61–5.92 |
| br | Brazil | OpenAddresses | 100,000 | 11.294 | 2.009 | 5.62× | 5.49–5.67 |
| nl | Netherlands | OpenAddresses | 100,000 | 15.857 | 2.828 | 5.61× | 5.60–5.76 |
| ru | Russia | OpenAddresses | 100,000 | 18.911 | 3.383 | 5.56× | 5.52–5.80 |
| pl | Poland | OpenAddresses | 100,000 | 11.148 | 2.014 | 5.55× | 5.38–5.55 |
| ch | Switzerland | OpenAddresses | 100,000 | 10.41 | 1.888 | 5.50× | 5.49–5.51 |
| be | Belgium | OpenAddresses | 100,000 | 15.142 | 2.743 | 5.48× | 5.39–5.77 |
| cl | Chile | OpenAddresses | 100,000 | 16.154 | 3.024 | 5.40× | 5.30–5.42 |
| ua | Ukraine | OpenAddresses | 100,000 | 11.653 | 2.184 | 5.37× | 5.32–5.48 |
| pr | Puerto Rico | OpenAddresses | 100,000 | 14.763 | 2.754 | 5.35× | 5.34–5.37 |
| by | Belarus | OpenAddresses | 100,000 | 13.308 | 2.49 | 5.34× | 5.30–5.40 |
| mx | Mexico | OpenAddresses | 100,000 | 23.27 | 4.351 | 5.33× | 5.31–5.35 |
| ca | Canada | OpenAddresses | 100,000 | 10.315 | 1.955 | 5.28× | 5.24–5.34 |
| it | Italy | OpenAddresses | 100,000 | 10.268 | 1.95 | 5.27× | 5.22–5.33 |
| kw | Kuwait | OSM (Geofabrik) | 100,000 | 10.244 | 1.964 | 5.24× | 5.16–5.26 |
| us | United States | OpenAddresses | 100,000 | 14.011 | 2.687 | 5.22× | 5.18–5.48 |
| lu | Luxembourg | OpenAddresses | 100,000 | 15.962 | 3.077 | 5.21× | 5.17–5.21 |
| es | Spain | OpenAddresses | 100,000 | 20.899 | 4.029 | 5.19× | 4.68–5.28 |
| fr | France | OpenAddresses | 100,000 | 17.163 | 3.342 | 5.14× | 5.06–5.80 |
| sg | Singapore | OpenAddresses | 100,000 | 16.659 | 3.253 | 5.12× | 5.12–5.17 |
| fi | Finland | OpenAddresses | 100,000 | 14.968 | 2.916 | 5.11× | 4.78–5.37 |
| kz | Kazakhstan | OpenAddresses | 100,000 | 10.615 | 2.067 | 5.10× | 5.01–5.64 |
| ar | Argentina | OpenAddresses | 100,000 | 14.062 | 2.752 | 5.09× | 5.03–5.14 |
| uy | Uruguay | OpenAddresses | 100,000 | 15.374 | 3.038 | 5.05× | 5.00–5.27 |
| pt | Portugal | OpenAddresses | 100,000 | 26.275 | 5.215 | 5.04× | 4.89–5.11 |
| at | Austria | OpenAddresses | 100,000 | 10.953 | 2.233 | 5.00× | 4.89–5.04 |
| cz | Czechia | OpenAddresses | 100,000 | 15.969 | 3.151 | 4.98× | 4.89–5.44 |
| jp | Japan | OpenAddresses | 100,000 | 20.566 | 4.134 | 4.97× | 4.93–5.03 |
| md | Moldova | OSM (Geofabrik) | 100,000 | 15.867 | 3.184 | 4.97× | 4.82–5.03 |
| re | Reunion | OpenAddresses | 100,000 | 19.044 | 3.845 | 4.95× | 4.95–4.99 |
| ro | Romania | OSM (Geofabrik) | 100,000 | 13.428 | 2.768 | 4.95× | 4.57–5.26 |
| no | Norway | OpenAddresses | 100,000 | 11.627 | 2.362 | 4.92× | 4.80–5.25 |
| se | Sweden | OpenAddresses | 100,000 | 10.853 | 2.232 | 4.86× | 4.85–5.06 |
| ee | Estonia | OpenAddresses | 100,000 | 12.709 | 2.618 | 4.85× | 4.82–4.95 |
| dk | Denmark | OpenAddresses | 100,000 | 15.464 | 3.22 | 4.84× | 4.80–5.15 |
| my | Malaysia | OSM (Geofabrik) | 100,000 | 12.063 | 2.503 | 4.83× | 4.76–4.89 |
| co | Colombia | OpenAddresses | 100,000 | 6.954 | 1.443 | 4.81× | 4.80–4.91 |
| ba | Bosnia and Herzegovina | OSM (Geofabrik) | 100,000 | 12.653 | 2.67 | 4.74× | 4.60–5.12 |
| tr | Turkiye | OSM (Geofabrik) | 100,000 | 12.233 | 2.606 | 4.73× | 4.65–4.79 |
| hu | Hungary | OSM (Geofabrik) | 100,000 | 14.113 | 2.99 | 4.72× | 4.63–4.73 |
| bg | Bulgaria | OpenAddresses | 100,000 | 16.63 | 3.569 | 4.67× | 4.64–4.70 |
| lt | Lithuania | OpenAddresses | 100,000 | 17.163 | 3.683 | 4.66× | 4.63–4.70 |
| rs | Serbia | OpenAddresses | 100,000 | 9.957 | 2.12 | 4.65× | 4.60–4.76 |
| am | Armenia | OSM (Geofabrik) | 100,000 | 12.045 | 2.65 | 4.60× | 4.54–4.64 |
| is | Iceland | OpenAddresses | 100,000 | 7.952 | 1.747 | 4.57× | 4.52–4.60 |
| lv | Latvia | OpenAddresses | 100,000 | 26.77 | 5.859 | 4.56× | 4.53–4.59 |
| sk | Slovakia | OpenAddresses | 100,000 | 14.654 | 3.225 | 4.54× | 4.52–4.66 |
| hr | Croatia | OpenAddresses | 100,000 | 16.791 | 3.701 | 4.54× | 4.49–4.82 |
| uz | Uzbekistan | OSM (Geofabrik) | 100,000 | 12.557 | 2.811 | 4.47× | 4.43–4.75 |
| qa | Qatar | OpenAddresses | 100,000 | 5.075 | 1.145 | 4.43× | 4.28–4.57 |
| si | Slovenia | OpenAddresses | 100,000 | 14.579 | 3.323 | 4.39× | 4.35–4.40 |
| tw | Taiwan | OpenAddresses | 100,000 | 22.126 | 5.09 | 4.35× | 4.34–4.41 |
| gr | Greece | OpenAddresses | 100,000 | 18.004 | 4.307 | 4.18× | 4.01–4.36 |
| ae | United Arab Emirates | OSM (Geofabrik) + OpenAddresses | 100,000 | 13.72 | 3.486 | 3.94× | 3.91–3.98 |
| eg | Egypt | OpenAddresses | 100,000 | 24.631 | 6.278 | 3.92× | 3.73–4.00 |
| kr | South Korea | OpenAddresses | 100,000 | 16.379 | 4.201 | 3.90× | 3.89–4.01 |
| xk | Kosovo | OpenAddresses | 100,000 | 13.805 | 3.603 | 3.83× | 3.79–3.89 |
| cn | China | OSM (Geofabrik) + OpenAddresses | 95,576 | 17.389 | 3.766 | 4.64× | 4.61–4.66 |
| ge | Georgia | OSM (Geofabrik) | 94,164 | 12.254 | 2.562 | 4.76× | 4.73–5.09 |
| pk | Pakistan | OSM (Geofabrik) | 93,442 | 15.038 | 2.888 | 5.21× | 5.19–5.22 |
| kg | Kyrgyzstan | OSM (Geofabrik) | 91,845 | 10.514 | 1.96 | 5.39× | 5.36–5.41 |
| tt | Trinidad and Tobago | OpenAddresses | 88,446 | 13.158 | 2.262 | 5.82× | 5.78–5.92 |
| il | Israel | OpenAddresses | 80,719 | 8.013 | 1.455 | 5.50× | 5.43–5.54 |
| vn | Vietnam | OSM (Geofabrik) | 68,034 | 11.215 | 2.369 | 4.74× | 4.70–4.77 |
| in | India | OSM (Geofabrik) | 65,213 | 24.654 | 4.981 | 4.96× | 4.95–4.96 |
| nc | New Caledonia | OpenAddresses | 51,702 | 12.218 | 2.261 | 5.40× | 5.39–5.83 |
| ph | Philippines | OSM (Geofabrik) | 50,148 | 14.855 | 2.79 | 5.32× | 5.27–5.33 |
| id | Indonesia | OSM (Geofabrik) | 43,488 | 14.898 | 3.066 | 4.83× | 4.79–4.89 |
| pe | Peru | OpenAddresses | 28,222 | 12.125 | 2.056 | 5.92× | 5.89–5.93 |
| cy | Cyprus | OSM (Geofabrik) + OpenAddresses | 27,194 | 11.691 | 2.576 | 4.54× | 4.49–4.61 |
| fo | Faroe Islands | OpenAddresses | 26,471 | 10.136 | 2.056 | 4.92× | 4.88–5.09 |
| jm | Jamaica | OSM (Geofabrik) + OpenAddresses | 20,938 | 16.6 | 3.233 | 5.13× | 5.05–5.25 |
| az | Azerbaijan | OSM (Geofabrik) | 19,051 | 11.118 | 2.483 | 4.50× | 4.46–4.53 |
| bd | Bangladesh | OSM (Geofabrik) | 18,121 | 17.633 | 3.888 | 4.53× | 4.45–4.75 |
| tj | Tajikistan | OSM (Geofabrik) | 17,806 | 14.209 | 2.917 | 4.92× | 4.87–4.98 |
| tz | Tanzania | OSM (Geofabrik) | 15,957 | 10.764 | 2.122 | 5.06× | 5.02–5.08 |
| mk | North Macedonia | OSM (Geofabrik) | 13,010 | 12.248 | 2.674 | 4.61× | 4.50–4.65 |
| li | Liechtenstein | OpenAddresses | 12,966 | 10.568 | 2.054 | 5.14× | 5.10–5.20 |
| ec | Ecuador | OSM (Geofabrik) | 11,741 | 13.7 | 2.624 | 5.22× | 5.13–5.25 |
| sm | San Marino | OSM (Overpass) | 11,514 | 15.638 | 2.876 | 5.41× | 5.34–5.48 |
| sr | Suriname | OSM (Overpass) | 7,628 | 9.651 | 1.896 | 5.20× | 5.09–5.21 |
| im | Isle of Man | OSM (Overpass) | 6,708 | 11.509 | 2.142 | 5.42× | 5.25–5.44 |
| gt | Guatemala | OSM (Overpass) | 6,385 | 12.76 | 2.517 | 5.08× | 5.05–5.45 |

## Interpretation and limits

- The overall speedup, 4.96×, is lower than on the 203 OSM-derived sets (5.5–5.7×). The original is also faster per address here (14.6 µs vs 18.8 µs on osm-v3), so this mix of addresses is cheaper to parse. The four countries below 4× are mostly scripts or formats with little Latin text (Korean, Arabic) or a single short-address source (Kosovo). Why they gain less was not investigated.
- Converting structured records to one line uses a simplified per-country order (`build_dataset.py`). That changes how hard the parse is, not whether the two builds agree.
- Samples are not uniform within a country. OpenAddresses files were chosen smallest first, so some countries come from a few regions (for example, the US from 100 small county files). OSM data covers whatever has been mapped.
- PGO was trained on golden-v1 development data, not on any of these sources. No tuning was done on v15.
- Per-country timings are the per-address times measured inside the last pass. Countries with a few thousand addresses (Isle of Man, Guatemala) have wider ranges.
- One machine and one dataset. The 95% interval rests on 3 rounds.

Decision: record. The 203 candidate meets the 3.5× contract on a 7.27-million-address, 88-country open-data test set, with byte-identical output, lower memory and one thread.
