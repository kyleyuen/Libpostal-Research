| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.92 | nan | 100% | 34.53 | 34.10 | 34.52 | nan (0) | 34.98 |
| novel | s3 | 90 | 9.85 | nan | 91% | 8.49 | 8.45 | 8.49 | 8.48 (8) | 8.57 |
| novel | m1 | 90 | 10.04 | nan | 93% | 8.45 | 8.41 | 8.45 | 8.42 (6) | 8.51 |
| novel | m2 | 90 | 9.10 | nan | 87% | 8.05 | 8.00 | 8.05 | 8.00 (12) | 8.10 |
| novel | m3 | 90 | 9.09 | nan | 90% | 8.02 | 7.97 | 8.02 | 7.97 (9) | 8.12 |
| v1-all | baseline | 90 | 2.94 | nan | 100% | 33.81 | 33.47 | 33.81 | nan (0) | 34.46 |
| v1-all | s3 | 90 | 9.83 | nan | 81% | 8.35 | 8.30 | 8.34 | 8.31 (17) | 8.46 |
| v1-all | m1 | 90 | 9.91 | nan | 92% | 8.32 | 8.27 | 8.32 | 8.31 (7) | 8.69 |
| v1-all | m2 | 90 | 9.18 | nan | 89% | 7.99 | 7.94 | 7.99 | 7.94 (10) | 8.05 |
| v1-all | m3 | 90 | 9.06 | nan | 84% | 7.95 | 7.88 | 7.95 | 7.89 (14) | 8.01 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | s3 | 4.065x | 4.037x | 4.065x | nanx | 4.081x |
| novel | m1 | 4.087x | 4.054x | 4.087x | nanx | 4.109x |
| novel | m2 | 4.289x | 4.265x | 4.289x | nanx | 4.317x |
| novel | m3 | 4.306x | 4.278x | 4.306x | nanx | 4.306x |
| v1-all | s3 | 4.052x | 4.033x | 4.052x | nanx | 4.074x |
| v1-all | m1 | 4.065x | 4.045x | 4.065x | nanx | 3.964x |
| v1-all | m2 | 4.230x | 4.216x | 4.230x | nanx | 4.282x |
| v1-all | m3 | 4.253x | 4.245x | 4.253x | nanx | 4.302x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   s3    : countries=17 min 3.319 (tw) median 4.034 max 4.447 (ru); below 1.05x: 0
  novel   m1    : countries=17 min 3.329 (tw) median 4.050 max 4.458 (ru); below 1.05x: 0
  novel   m2    : countries=17 min 3.485 (tw) median 4.215 max 4.628 (ru); below 1.05x: 0
  novel   m3    : countries=17 min 3.518 (tw) median 4.301 max 4.700 (ru); below 1.05x: 0
  v1-all  s3    : countries=17 min 3.382 (tw) median 4.016 max 4.385 (ru); below 1.05x: 0
  v1-all  m1    : countries=17 min 3.362 (tw) median 4.033 max 4.406 (ru); below 1.05x: 0
  v1-all  m2    : countries=17 min 3.391 (tw) median 4.066 max 4.420 (ru); below 1.05x: 0
  v1-all  m3    : countries=17 min 3.522 (tw) median 4.224 max 4.611 (ru); below 1.05x: 0
