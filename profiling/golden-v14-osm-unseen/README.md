# Golden v14 — fresh OpenStreetMap input

This is a new, one-use validation input prepared for experiment 202. It was
created without loading libpostal, consulting parser outputs, or timing a
candidate. The suite owner received the frozen files and hashes before any
parser run on this data.

## Selection and source

The builder contains a 67-city pool with names checked against the city lists
in the prior v3–v13 OSM builders. Acquisition time was bounded by selecting a
fixed, geographically broad subset of 15 cities across 14 countries; this
subset was chosen without parser outputs or candidate measurements. The first
few requests had already started when the subset was narrowed, so the complete
selection history and request outcomes are summarized in `FETCH_LOG.md`.

The source is OpenStreetMap through the public Overpass API. The builder queries
tagged objects within 700 m of each selected city centre, asks for at most 400
objects per city, assembles addresses with the existing country-format rules,
keeps at most 200 distinct strings per city, and sorts by SHA-256 of the
address. This method uses no model- or corpus-derived ordering. Raw API
responses are retained in `raw/`; OpenStreetMap data is available under the
ODbL.

## Frozen files

- `unseen14.txt`: 1,801 ordered address strings, one per line.
- `unseen14.tsv`: the same order and address strings with the country code in
  the first column, for `bench_ext`.
- 15 city responses were fetched. Thirteen yielded rows across 12 countries;
  two responses had no usable addresses. No city response was missing.
- The builder removed zero exact address candidates as duplicates of the 13
  prior sets: golden-v1, the novel set, and OSM v3–v13. The final output was
  separately checked for duplicates against all 13 inputs before handoff.

`provenance.json` records the selection, prior inputs, city/country/coordinate
list, raw response hashes and sizes, row counts by city and country, duplicate
count, and output hashes. `SHA256SUMS` pins the builder, provenance, outputs,
and raw responses. This is evidence for new-city and exact-string separation
from retained inputs; it is not evidence for a new source or universal
generalization.

## Reproduction

From the workspace root, `python3 profiling/golden-v14-osm-unseen/build_unseen14.py
build` regenerates the two frozen output files from retained responses and
updates `provenance.json`. `fetch` requests any selected city response that is
missing. Do not replace these frozen responses during experiment 202.
