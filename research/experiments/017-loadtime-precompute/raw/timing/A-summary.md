| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.93 | nan | 100% | 34.24 | 33.77 | 34.24 | nan (0) | 34.60 |
| novel | c2s1 | 90 | 8.83 | nan | 93% | 10.14 | 10.07 | 10.14 | 10.08 (6) | 10.39 |
| novel | r0 | 90 | 8.85 | nan | 93% | 10.19 | 10.09 | 10.19 | 10.40 (6) | 10.37 |
| novel | u1 | 90 | 8.91 | nan | 91% | 10.16 | 10.06 | 10.15 | 10.19 (8) | 10.37 |
| novel | u2 | 90 | 8.86 | nan | 96% | 10.03 | 9.93 | 10.03 | 10.05 (4) | 10.19 |
| v1-all | baseline | 90 | 2.94 | nan | 100% | 33.64 | 33.22 | 33.63 | nan (0) | 33.95 |
| v1-all | c2s1 | 90 | 8.96 | nan | 96% | 10.03 | 9.91 | 10.03 | 10.05 (4) | 10.39 |
| v1-all | r0 | 90 | 8.84 | nan | 93% | 10.00 | 9.90 | 9.99 | 9.97 (6) | 10.22 |
| v1-all | u1 | 90 | 8.96 | nan | 90% | 9.97 | 9.89 | 9.97 | 9.95 (9) | 10.13 |
| v1-all | u2 | 90 | 8.88 | nan | 97% | 9.85 | 9.79 | 9.85 | 9.80 (3) | 9.97 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | c2s1 | 3.376x | 3.354x | 3.376x | nanx | 3.330x |
| novel | r0 | 3.360x | 3.345x | 3.361x | nanx | 3.337x |
| novel | u1 | 3.372x | 3.357x | 3.372x | nanx | 3.337x |
| novel | u2 | 3.415x | 3.400x | 3.415x | nanx | 3.394x |
| v1-all | c2s1 | 3.353x | 3.353x | 3.353x | nanx | 3.268x |
| v1-all | r0 | 3.366x | 3.356x | 3.366x | nanx | 3.322x |
| v1-all | u1 | 3.375x | 3.360x | 3.375x | nanx | 3.351x |
| v1-all | u2 | 3.417x | 3.395x | 3.416x | nanx | 3.406x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   c2s1  : countries=17 min 2.782 (tw) median 3.289 max 3.696 (ru); below 1.05x: 0
  novel   r0    : countries=17 min 2.806 (tw) median 3.310 max 3.739 (ru); below 1.05x: 0
  novel   u1    : countries=17 min 2.806 (tw) median 3.314 max 3.741 (ru); below 1.05x: 0
  novel   u2    : countries=17 min 2.787 (tw) median 3.320 max 3.735 (ru); below 1.05x: 0
  v1-all  c2s1  : countries=17 min 2.813 (tw) median 3.332 max 3.747 (ru); below 1.05x: 0
  v1-all  r0    : countries=17 min 2.829 (tw) median 3.329 max 3.718 (ru); below 1.05x: 0
  v1-all  u1    : countries=17 min 2.859 (tw) median 3.367 max 3.759 (ru); below 1.05x: 0
  v1-all  u2    : countries=17 min 2.850 (tw) median 3.425 max 3.807 (ru); below 1.05x: 0
