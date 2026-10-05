# Experiment 083 — why pcp's gain over lf is smaller on unseen data (workload counters)

Date/time/timezone: created 2026-10-03T03:38:29-0700, America/Los_Angeles. Status: complete (diagnostic)

Question: pcp over lf is +4.8% golden-v1, +4.6% novel (082), +3.0% OSM (082) but +2.3% on unseen golden-v5 (080). Fewer prefetch targets on v5, or a selection effect from accepting on golden-v1/OSM screens?
Method: the counter builds 073 `fst` (FTT_STATS) and 076 `pcs` (PC_STATS) on each full set (`raw/fst_*.stderr`, `raw/pcs_*.stderr`).
    v1   addresses= 12863 tokens/addr=8.26 FTT-assisted tokens/addr=3.36 postcode-context calls/addr=1.43 decoded bytes/call=64
    nv   addresses= 12863 tokens/addr=8.02 FTT-assisted tokens/addr=3.25 postcode-context calls/addr=1.27 decoded bytes/call=60
    osm  addresses= 11405 tokens/addr=7.83 FTT-assisted tokens/addr=3.12 postcode-context calls/addr=1.43 decoded bytes/call=40
    u5   addresses=  8794 tokens/addr=7.92 FTT-assisted tokens/addr=3.17 postcode-context calls/addr=1.45 decoded bytes/call=52
Finding: v5 has nearly as many targets per address as golden-v1 (assisted tokens −6%, postcode calls equal), and its addresses are cheaper (3.9 vs 6.5 µs), so equal absolute savings would give v5 a *larger* relative gain. Target counts do not explain it. The split follows the data source: Senzing-derived sets (golden-v1, novel) gain 4.6–4.8%, OSM-derived sets (OSM, v5) 2.3–3.0%. UNVERIFIED interpretation: OSM-style sets repeat city names and postcodes within the run, so more of these lines are already cached and fewer misses remain to overlap; a selection effect (both changes were accepted on golden-v1/OSM screens) may contribute. The report quotes the unseen increment beside the screen numbers.

Addendum (085 stage timers): accumulation's first-touch share is 0.59 µs per address on development vs 0.24 µs on OSM (cold − warm), consistent with (not proof of) the cache-locality reading above.
