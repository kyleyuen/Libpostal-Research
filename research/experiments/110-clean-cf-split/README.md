# Experiment 110 — context-fill sub-stage split of psf on the cleaned machine (diagnostic)

Date/time/timezone: created 2026-10-04T00:07:12-0700 (`raw/created.txt`), America/Los_Angeles; commands in `raw/commands.txt`. Status: complete

Stats builds `cf2` / `cf3` = psf + EXP_STAGE_STATS + `EXP_CF2_STATS` (timers in `sources/address_parser.c`); 3 runs each, development and OSM (`raw/split_analyze.stdout`, `raw/split3_analyze.stdout`). Development, cold / warm / first-touch µs:

    normalize loop               0.132 / 0.086 / 0.046
    address dictionary search    0.231 / 0.059 / 0.172
    FTT block + prefix/suffix    0.421 / 0.080 / 0.341   (slot loop alone 0.338 / 0.033 / 0.305)
    component search             0.512 / 0.081 / 0.431
    postcode search (+077 hint)  0.437 / 0.059 / 0.378

OSM: 0.110/0.074, 0.171/0.057, 0.246/0.071, 0.233/0.077, 0.234/0.055. The vocabulary-slot loop (056/074) and the postcode search are the largest first-touch items after the component search. The slot loop's arena miss is what 075 already tried to prefetch (rejected, measured under load); not retried.
