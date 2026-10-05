| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 95 | 2.93 | nan | 100% | 36.69 | 36.12 | 36.69 | nan (0) | 36.91 |
| novel | g | 95 | 6.69 | nan | 100% | 15.34 | 15.14 | 15.34 | nan (0) | 15.46 |
| novel | g2 | 95 | 6.70 | nan | 100% | 15.36 | 15.21 | 15.35 | nan (0) | 15.50 |
| novel | h1 | 95 | 6.74 | nan | 99% | 13.75 | 13.60 | 13.75 | 13.68 (1) | 13.86 |
| novel | h2 | 95 | 6.64 | nan | 100% | 15.18 | 15.03 | 15.17 | nan (0) | 15.22 |
| novel | h | 95 | 6.58 | nan | 99% | 13.56 | 13.45 | 13.55 | 13.38 (1) | 13.69 |
| v1-all | baseline | 95 | 2.91 | nan | 100% | 36.06 | 35.51 | 36.05 | nan (0) | 36.05 |
| v1-all | g | 95 | 6.76 | nan | 98% | 15.22 | 15.10 | 15.21 | 15.16 (2) | 15.29 |
| v1-all | g2 | 95 | 6.71 | nan | 100% | 15.25 | 15.05 | 15.25 | nan (0) | 15.30 |
| v1-all | h1 | 95 | 6.66 | nan | 99% | 13.78 | 13.64 | 13.77 | 13.65 (1) | 13.90 |
| v1-all | h2 | 95 | 6.66 | nan | 100% | 15.09 | 14.90 | 15.09 | nan (0) | 15.25 |
| v1-all | h | 95 | 6.72 | nan | 99% | 13.55 | 13.40 | 13.55 | 13.52 (1) | 13.76 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | g | 2.392x | 2.385x | 2.392x | nanx | 2.387x |
| novel | g2 | 2.389x | 2.375x | 2.390x | nanx | 2.381x |
| novel | h1 | 2.669x | 2.655x | 2.669x | nanx | 2.664x |
| novel | h2 | 2.418x | 2.403x | 2.418x | nanx | 2.425x |
| novel | h | 2.707x | 2.686x | 2.706x | nanx | 2.697x |
| v1-all | g | 2.370x | 2.352x | 2.369x | nanx | 2.357x |
| v1-all | g2 | 2.364x | 2.360x | 2.364x | nanx | 2.357x |
| v1-all | h1 | 2.617x | 2.604x | 2.617x | nanx | 2.593x |
| v1-all | h2 | 2.389x | 2.384x | 2.388x | nanx | 2.364x |
| v1-all | h | 2.661x | 2.651x | 2.660x | nanx | 2.619x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   g     : countries=17 min 2.221 (lt) median 2.377 max 2.638 (ru); below 1.05x: 0
  novel   g2    : countries=17 min 2.211 (lt) median 2.378 max 2.637 (ru); below 1.05x: 0
  novel   h1    : countries=17 min 2.259 (lt) median 2.642 max 2.862 (us); below 1.05x: 0
  novel   h2    : countries=17 min 2.238 (lt) median 2.407 max 2.643 (ru); below 1.05x: 0
  novel   h     : countries=17 min 2.268 (lt) median 2.651 max 2.918 (us); below 1.05x: 0
  v1-all  g     : countries=17 min 2.092 (lt) median 2.317 max 2.518 (ru); below 1.05x: 0
  v1-all  g2    : countries=17 min 2.098 (lt) median 2.342 max 2.532 (ru); below 1.05x: 0
  v1-all  h1    : countries=17 min 2.125 (lt) median 2.536 max 2.915 (gb); below 1.05x: 0
  v1-all  h2    : countries=17 min 2.115 (lt) median 2.372 max 2.548 (ru); below 1.05x: 0
  v1-all  h     : countries=17 min 2.161 (lt) median 2.559 max 2.994 (gb); below 1.05x: 0
