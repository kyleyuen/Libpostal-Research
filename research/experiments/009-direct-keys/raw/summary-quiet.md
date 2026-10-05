| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 114 | 2.91 | nan | 100% | 36.84 | 36.47 | 36.84 | nan (0) | 36.98 |
| novel | d | 114 | 5.92 | nan | 100% | 20.24 | 20.02 | 20.23 | nan (0) | 20.29 |
| novel | e | 114 | 5.94 | nan | 100% | 18.94 | 18.78 | 18.94 | nan (0) | 19.18 |
| novel | f | 114 | 5.96 | nan | 99% | 17.39 | 17.19 | 17.38 | 16.96 (1) | 17.47 |
| v1-all | baseline | 114 | 2.92 | nan | 100% | 36.13 | 35.67 | 36.11 | nan (0) | 36.30 |
| v1-all | d | 114 | 5.93 | nan | 100% | 20.16 | 20.01 | 20.16 | nan (0) | 20.44 |
| v1-all | e | 114 | 5.95 | nan | 100% | 18.92 | 18.73 | 18.92 | nan (0) | 19.07 |
| v1-all | f | 114 | 6.02 | nan | 100% | 17.48 | 17.31 | 17.48 | nan (0) | 17.68 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | d | 1.820x | 1.822x | 1.821x | nanx | 1.823x |
| novel | e | 1.945x | 1.942x | 1.945x | nanx | 1.928x |
| novel | f | 2.119x | 2.122x | 2.119x | nanx | 2.117x |
| v1-all | d | 1.792x | 1.783x | 1.791x | nanx | 1.776x |
| v1-all | e | 1.909x | 1.904x | 1.909x | nanx | 1.903x |
| v1-all | f | 2.066x | 2.061x | 2.066x | nanx | 2.053x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   d     : countries=17 min 1.717 (lt) median 1.798 max 1.976 (ru); below 1.05x: 0
  novel   e     : countries=17 min 1.844 (lt) median 1.920 max 2.099 (ru); below 1.05x: 0
  novel   f     : countries=17 min 1.984 (lt) median 2.099 max 2.278 (ru); below 1.05x: 0
  v1-all  d     : countries=17 min 1.622 (nl) median 1.751 max 1.906 (ru); below 1.05x: 0
  v1-all  e     : countries=17 min 1.714 (nl) median 1.868 max 2.031 (ru); below 1.05x: 0
  v1-all  f     : countries=17 min 1.859 (lt) median 2.045 max 2.189 (ru); below 1.05x: 0
