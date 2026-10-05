| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.95 | nan | 100% | 34.53 | 34.17 | 34.52 | nan (0) | 35.00 |
| novel | s0 | 90 | 9.86 | nan | 92% | 8.62 | 8.58 | 8.62 | 8.60 (7) | 8.77 |
| novel | s1 | 90 | 10.01 | nan | 89% | 8.59 | 8.54 | 8.59 | 8.56 (10) | 8.74 |
| novel | s2 | 90 | 9.84 | nan | 88% | 8.53 | 8.48 | 8.53 | 8.50 (11) | 8.57 |
| novel | s3 | 90 | 9.96 | nan | 90% | 8.47 | 8.43 | 8.47 | 8.44 (9) | 8.80 |
| novel | s4 | 90 | 9.98 | nan | 90% | 8.60 | 8.56 | 8.60 | 8.57 (9) | 8.91 |
| v1-all | baseline | 90 | 2.88 | nan | 100% | 34.01 | 33.67 | 34.00 | nan (0) | 34.55 |
| v1-all | s0 | 90 | 9.82 | nan | 97% | 8.48 | 8.42 | 8.48 | 8.75 (3) | 8.60 |
| v1-all | s1 | 90 | 9.98 | nan | 90% | 8.44 | 8.39 | 8.44 | 8.41 (9) | 8.79 |
| v1-all | s2 | 90 | 9.92 | nan | 86% | 8.41 | 8.36 | 8.41 | 8.41 (13) | 8.60 |
| v1-all | s3 | 90 | 9.86 | nan | 90% | 8.33 | 8.30 | 8.33 | 8.31 (9) | 8.56 |
| v1-all | s4 | 90 | 9.97 | nan | 90% | 8.46 | 8.42 | 8.46 | 8.44 (9) | 8.75 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | s0 | 4.006x | 3.983x | 4.006x | nanx | 3.993x |
| novel | s1 | 4.019x | 3.998x | 4.020x | nanx | 4.004x |
| novel | s2 | 4.046x | 4.030x | 4.047x | nanx | 4.082x |
| novel | s3 | 4.076x | 4.052x | 4.076x | nanx | 3.978x |
| novel | s4 | 4.013x | 3.992x | 4.013x | nanx | 3.929x |
| v1-all | s0 | 4.010x | 3.999x | 4.009x | nanx | 4.017x |
| v1-all | s1 | 4.029x | 4.013x | 4.028x | nanx | 3.932x |
| v1-all | s2 | 4.045x | 4.028x | 4.045x | nanx | 4.017x |
| v1-all | s3 | 4.080x | 4.058x | 4.080x | nanx | 4.036x |
| v1-all | s4 | 4.020x | 3.999x | 4.019x | nanx | 3.948x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   s0    : countries=17 min 3.315 (tw) median 3.980 max 4.422 (ru); below 1.05x: 0
  novel   s1    : countries=17 min 3.290 (tw) median 3.983 max 4.408 (ru); below 1.05x: 0
  novel   s2    : countries=17 min 3.300 (tw) median 4.030 max 4.452 (ru); below 1.05x: 0
  novel   s3    : countries=17 min 3.294 (tw) median 4.023 max 4.438 (ru); below 1.05x: 0
  novel   s4    : countries=17 min 3.273 (tw) median 3.971 max 4.405 (ru); below 1.05x: 0
  v1-all  s0    : countries=17 min 3.338 (tw) median 3.980 max 4.376 (ru); below 1.05x: 0
  v1-all  s1    : countries=17 min 3.329 (tw) median 4.007 max 4.349 (ru); below 1.05x: 0
  v1-all  s2    : countries=17 min 3.317 (tw) median 4.021 max 4.359 (ru); below 1.05x: 0
  v1-all  s3    : countries=17 min 3.357 (tw) median 4.035 max 4.380 (ru); below 1.05x: 0
  v1-all  s4    : countries=17 min 3.262 (tw) median 3.942 max 4.272 (ru); below 1.05x: 0
