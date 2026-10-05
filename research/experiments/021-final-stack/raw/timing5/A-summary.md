| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.93 | nan | 100% | 34.53 | 34.10 | 34.52 | nan (0) | 35.00 |
| novel | m3 | 90 | 9.09 | nan | 90% | 8.01 | 7.97 | 8.01 | 8.01 (9) | 8.10 |
| novel | l3 | 90 | 8.61 | nan | 86% | 7.93 | 7.88 | 7.93 | 7.89 (13) | 8.01 |
| novel | l4 | 90 | 8.57 | nan | 92% | 7.92 | 7.86 | 7.92 | 7.86 (7) | 7.95 |
| v1-all | baseline | 90 | 3.01 | nan | 100% | 33.92 | 33.44 | 33.92 | nan (0) | 34.29 |
| v1-all | m3 | 90 | 9.12 | nan | 94% | 7.93 | 7.89 | 7.93 | 7.90 (5) | 7.98 |
| v1-all | l3 | 90 | 8.67 | nan | 87% | 7.89 | 7.82 | 7.89 | 7.88 (12) | 7.98 |
| v1-all | l4 | 90 | 8.55 | nan | 86% | 7.83 | 7.78 | 7.83 | 7.84 (13) | 7.90 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | m3 | 4.309x | 4.277x | 4.308x | nanx | 4.321x |
| novel | l3 | 4.353x | 4.324x | 4.352x | nanx | 4.369x |
| novel | l4 | 4.358x | 4.338x | 4.358x | nanx | 4.403x |
| v1-all | m3 | 4.277x | 4.237x | 4.277x | nanx | 4.297x |
| v1-all | l3 | 4.302x | 4.277x | 4.301x | nanx | 4.297x |
| v1-all | l4 | 4.330x | 4.296x | 4.330x | nanx | 4.338x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   m3    : countries=17 min 3.509 (tw) median 4.285 max 4.730 (ru); below 1.05x: 0
  novel   l3    : countries=17 min 3.587 (tw) median 4.348 max 4.813 (ru); below 1.05x: 0
  novel   l4    : countries=17 min 3.558 (tw) median 4.372 max 4.796 (ru); below 1.05x: 0
  v1-all  m3    : countries=17 min 3.381 (tw) median 4.103 max 4.503 (ru); below 1.05x: 0
  v1-all  l3    : countries=17 min 3.463 (tw) median 4.190 max 4.617 (ru); below 1.05x: 0
  v1-all  l4    : countries=17 min 3.497 (tw) median 4.224 max 4.631 (ru); below 1.05x: 0
