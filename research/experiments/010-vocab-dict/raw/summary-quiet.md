| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 95 | 2.92 | nan | 100% | 36.78 | 36.17 | 36.77 | nan (0) | 37.04 |
| novel | f | 95 | 5.96 | nan | 100% | 17.41 | 17.24 | 17.40 | nan (0) | 17.52 |
| novel | f0 | 95 | 5.93 | nan | 100% | 17.52 | 17.33 | 17.52 | nan (0) | 17.66 |
| novel | fJ | 95 | 5.94 | nan | 99% | 16.73 | 16.54 | 16.73 | 16.34 (1) | 16.86 |
| novel | fC | 95 | 5.98 | nan | 100% | 17.14 | 16.96 | 17.13 | nan (0) | 17.36 |
| novel | fE | 95 | 5.96 | nan | 92% | 17.53 | 17.24 | 17.52 | 17.00 (8) | 17.82 |
| novel | fV | 95 | 6.65 | nan | 99% | 16.37 | 16.19 | 16.37 | 16.11 (1) | 16.55 |
| novel | g | 95 | 6.74 | nan | 97% | 15.34 | 15.15 | 15.34 | 15.28 (3) | 15.59 |
| v1-all | baseline | 95 | 2.92 | nan | 97% | 36.04 | 35.27 | 36.02 | 35.43 (3) | 35.79 |
| v1-all | f | 95 | 5.97 | nan | 99% | 17.42 | 17.23 | 17.42 | 16.89 (1) | 17.46 |
| v1-all | f0 | 95 | 5.96 | nan | 98% | 17.49 | 17.33 | 17.48 | 17.34 (2) | 17.52 |
| v1-all | fJ | 95 | 6.06 | nan | 100% | 16.79 | 16.66 | 16.79 | nan (0) | 17.02 |
| v1-all | fC | 95 | 5.96 | nan | 97% | 17.11 | 16.97 | 17.11 | 16.69 (3) | 17.14 |
| v1-all | fE | 95 | 5.95 | nan | 95% | 17.46 | 17.31 | 17.46 | 17.34 (5) | 17.66 |
| v1-all | fV | 95 | 6.65 | nan | 96% | 16.33 | 16.12 | 16.32 | 16.10 (4) | 16.48 |
| v1-all | g | 95 | 6.76 | nan | 82% | 15.18 | 14.89 | 15.18 | 14.99 (17) | 15.31 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | f | 2.113x | 2.098x | 2.113x | nanx | 2.114x |
| novel | f0 | 2.099x | 2.087x | 2.099x | nanx | 2.097x |
| novel | fJ | 2.198x | 2.187x | 2.199x | nanx | 2.197x |
| novel | fC | 2.146x | 2.132x | 2.147x | nanx | 2.134x |
| novel | fE | 2.098x | 2.098x | 2.099x | nanx | 2.079x |
| novel | fV | 2.247x | 2.234x | 2.247x | nanx | 2.238x |
| novel | g | 2.397x | 2.388x | 2.397x | nanx | 2.376x |
| v1-all | f | 2.069x | 2.047x | 2.068x | 2.098x | 2.049x |
| v1-all | f0 | 2.061x | 2.036x | 2.060x | 2.043x | 2.043x |
| v1-all | fJ | 2.146x | 2.117x | 2.145x | nanx | 2.103x |
| v1-all | fC | 2.106x | 2.078x | 2.105x | 2.123x | 2.088x |
| v1-all | fE | 2.064x | 2.038x | 2.063x | 2.043x | 2.026x |
| v1-all | fV | 2.208x | 2.188x | 2.206x | 2.201x | 2.171x |
| v1-all | g | 2.374x | 2.369x | 2.372x | 2.364x | 2.337x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   f     : countries=17 min 1.960 (lt) median 2.095 max 2.276 (ru); below 1.05x: 0
  novel   f0    : countries=17 min 1.931 (lt) median 2.072 max 2.248 (ru); below 1.05x: 0
  novel   fJ    : countries=17 min 2.050 (lt) median 2.179 max 2.348 (ru); below 1.05x: 0
  novel   fC    : countries=17 min 2.002 (lt) median 2.124 max 2.317 (ru); below 1.05x: 0
  novel   fE    : countries=17 min 1.944 (lt) median 2.074 max 2.257 (ru); below 1.05x: 0
  novel   fV    : countries=17 min 2.038 (tw) median 2.256 max 2.516 (ru); below 1.05x: 0
  novel   g     : countries=17 min 2.214 (lt) median 2.377 max 2.649 (ru); below 1.05x: 0
  v1-all  f     : countries=17 min 1.863 (lt) median 2.056 max 2.196 (ru); below 1.05x: 0
  v1-all  f0    : countries=17 min 1.867 (nl) median 2.045 max 2.171 (ru); below 1.05x: 0
  v1-all  fJ    : countries=17 min 1.936 (nl) median 2.125 max 2.252 (gb); below 1.05x: 0
  v1-all  fC    : countries=17 min 1.866 (lt) median 2.076 max 2.273 (tw); below 1.05x: 0
  v1-all  fE    : countries=17 min 1.852 (lt) median 2.040 max 2.162 (ru); below 1.05x: 0
  v1-all  fV    : countries=17 min 1.999 (lt) median 2.177 max 2.438 (ru); below 1.05x: 0
  v1-all  g     : countries=17 min 2.111 (lt) median 2.356 max 2.544 (ru); below 1.05x: 0
