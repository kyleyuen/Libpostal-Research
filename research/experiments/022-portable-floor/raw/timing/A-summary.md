| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.92 | nan | 100% | 34.98 | 34.45 | 34.96 | nan (0) | 35.09 |
| novel | l4 | 90 | 8.60 | nan | 90% | 7.94 | 7.86 | 7.94 | 7.97 (9) | 8.00 |
| novel | pt | 90 | 9.55 | nan | 93% | 9.07 | 8.97 | 9.06 | 9.10 (6) | 9.23 |
| v1-all | baseline | 90 | 2.94 | nan | 100% | 34.03 | 33.42 | 34.03 | nan (0) | 34.45 |
| v1-all | l4 | 90 | 8.54 | nan | 93% | 7.90 | 7.80 | 7.90 | 7.86 (6) | 7.91 |
| v1-all | pt | 90 | 9.54 | nan | 93% | 9.03 | 8.93 | 9.03 | 8.94 (6) | 9.38 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | l4 | 4.406x | 4.381x | 4.404x | nanx | 4.386x |
| novel | pt | 3.858x | 3.843x | 3.857x | nanx | 3.800x |
| v1-all | l4 | 4.308x | 4.284x | 4.308x | nanx | 4.356x |
| v1-all | pt | 3.770x | 3.744x | 3.770x | nanx | 3.671x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   l4    : countries=17 min 3.625 (tw) median 4.382 max 4.844 (ru); below 1.05x: 0
  novel   pt    : countries=17 min 3.240 (tw) median 3.843 max 4.140 (ru); below 1.05x: 0
  v1-all  l4    : countries=17 min 3.526 (tw) median 4.249 max 4.612 (ru); below 1.05x: 0
  v1-all  pt    : countries=17 min 3.213 (tw) median 3.743 max 4.031 (ru); below 1.05x: 0
