# Experiment 088 — warm bound of pcp: how fast is the deliverable when each address's data is already cached? (diagnostic)

Date/time/timezone: created 2026-10-03T04:50:43-0700; run 2026-10-03T04:50:44-0700 → 2026-10-03T04:54:28-0700, America/Los_Angeles. Status: complete (diagnostic)

041's harness (`../041-warm-bound/tools/bench_warm.c`, no library change) linked against pcp's own PGO/LTO `libpostal.a` with mimalloc (`tools/bench_warm_pcp`, `raw/hashes.txt`): every address is parsed once untimed and then again, timing only the second call; compared with pcp's normal `bench_ext`. golden-v1, OSM, unseen golden-v6; 10 passes, 3 alternating runs (`tools/run.sh`, 18 runs exit 0, AC-gated); median of passes 2–10.
    v1   normal 6.533 µs  warm-bound 3.008 µs  first-touch share 54%  | original 33.354 µs → normal 5.11×, warm bound 11.09×, 7.8× needs 4.28 µs
    osm  normal 4.220 µs  warm-bound 2.602 µs  first-touch share 38%  | original 21.042 µs → normal 4.99×, warm bound 8.09×, 7.8× needs 2.70 µs
    u6   normal 3.925 µs  warm-bound 2.540 µs  first-touch share 35%  | original 19.499 µs → normal 4.97×, warm bound 7.68×, 7.8× needs 2.50 µs
Original µs are from experiments 082 (golden-v1, OSM) and 086 (unseen v6), not this session, so the × figures are approximate.

Interpretation: the warm bound is the ceiling for any change that only removes memory stalls (prefetching, layout, smaller tables). It is 11.1× on golden-v1 but only **8.1× on OSM and 7.7× on unseen golden-v6**; 7.8× on OpenStreetMap-style data therefore cannot be reached by memory-side work at all and needs the warm computation itself cut by roughly a third together with nearly all first-touch cost.
