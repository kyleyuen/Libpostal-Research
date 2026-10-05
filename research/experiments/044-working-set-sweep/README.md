# Experiment 044 — time per address vs number of distinct addresses cycled (diagnostic, confounded)

Date/time/timezone: created 2026-10-02T17:12:56-0700; runs 2026-10-02T17:12:56-0700 to 2026-10-02T17:15:12-0700, America/Los_Angeles.
Status: complete (measurement only; **confounded, no conclusion drawn**)

Shipped `l3` bench_ext on the first N lines of the golden-v1 development split (`data/`, hashes `raw/subset_hashes.txt`), passes = max(20, 100000/N), two runs (ascending, descending), AC-gated, all exit 0 (`raw/run.log`). Median warm pass (`raw/analyze.stdout`):

    N=    30 working-set µs/address: r1 3.724 r2 3.744
    N=   100 working-set µs/address: r1 4.072 r2 4.074
    N=   300 working-set µs/address: r1 4.889 r2 4.887
    N=  1000 working-set µs/address: r1 5.743 r2 5.750
    N=  3000 working-set µs/address: r1 7.471 r2 7.454
    N= 10308 working-set µs/address: r1 7.874 r2 7.774

**Confound:** the development split is ordered by country and begins with Isle of Man addresses, so small-N subsets are geographically homogeneous (shared city/postcode/street data); the curve mixes working-set size with homogeneity. A seeded random-subset rerun would be needed before using it. **Interpretation note (advisor review):** the flat region from 3,000 to 10,308 indicates that at full scale the remaining first-touch cost is per-address traffic rather than reuse distance; it scales with distinct lines touched per address, so it cannot be used to argue that denser layouts would only slide along the flat end. Experiment 045 measures lines per address directly.
