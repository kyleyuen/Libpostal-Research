| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.95 | nan | 100% | 35.61 | 35.16 | 35.58 | nan (0) | 37.28 |
| novel | h | 90 | 6.66 | nan | 100% | 13.21 | 13.09 | 13.21 | nan (0) | 13.46 |
| novel | h0 | 90 | 6.62 | nan | 100% | 13.26 | 13.15 | 13.25 | nan (0) | 13.41 |
| novel | m | 90 | 6.65 | nan | 100% | 13.24 | 13.13 | 13.23 | nan (0) | 13.60 |
| novel | k1 | 90 | 9.31 | nan | 100% | 12.99 | 12.76 | 12.98 | nan (0) | 13.26 |
| novel | k2 | 90 | 12.13 | nan | 100% | 13.09 | 12.91 | 13.07 | nan (0) | 13.40 |
| v1-all | baseline | 90 | 2.95 | nan | 100% | 35.50 | 34.75 | 35.47 | nan (0) | 36.47 |
| v1-all | h | 90 | 6.67 | nan | 100% | 13.30 | 13.16 | 13.27 | nan (0) | 13.64 |
| v1-all | h0 | 90 | 6.51 | nan | 100% | 13.26 | 13.11 | 13.24 | nan (0) | 13.47 |
| v1-all | m | 90 | 6.66 | nan | 100% | 13.23 | 13.07 | 13.22 | nan (0) | 13.39 |
| v1-all | k1 | 90 | 9.21 | nan | 100% | 12.93 | 12.78 | 12.91 | nan (0) | 13.09 |
| v1-all | k2 | 90 | 11.95 | nan | 100% | 12.93 | 12.82 | 12.92 | nan (0) | 13.31 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | h | 2.695x | 2.686x | 2.694x | nanx | 2.770x |
| novel | h0 | 2.686x | 2.674x | 2.685x | nanx | 2.779x |
| novel | m | 2.690x | 2.678x | 2.690x | nanx | 2.742x |
| novel | k1 | 2.741x | 2.756x | 2.741x | nanx | 2.811x |
| novel | k2 | 2.720x | 2.723x | 2.721x | nanx | 2.783x |
| v1-all | h | 2.669x | 2.641x | 2.672x | nanx | 2.673x |
| v1-all | h0 | 2.677x | 2.651x | 2.679x | nanx | 2.707x |
| v1-all | m | 2.683x | 2.658x | 2.683x | nanx | 2.724x |
| v1-all | k1 | 2.745x | 2.719x | 2.747x | nanx | 2.785x |
| v1-all | k2 | 2.746x | 2.712x | 2.745x | nanx | 2.741x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   h     : countries=17 min 2.244 (lt) median 2.623 max 2.896 (us); below 1.05x: 0
  novel   h0    : countries=17 min 2.207 (lt) median 2.612 max 2.878 (us); below 1.05x: 0
  novel   m     : countries=17 min 2.232 (lt) median 2.606 max 2.849 (nl); below 1.05x: 0
  novel   k1    : countries=17 min 2.237 (lt) median 2.669 max 2.965 (nl); below 1.05x: 0
  novel   k2    : countries=17 min 2.235 (lt) median 2.663 max 2.957 (nl); below 1.05x: 0
  v1-all  h     : countries=17 min 2.213 (lt) median 2.594 max 3.024 (gb); below 1.05x: 0
  v1-all  h0    : countries=17 min 2.225 (lt) median 2.619 max 3.053 (us); below 1.05x: 0
  v1-all  m     : countries=17 min 2.207 (lt) median 2.614 max 3.033 (gb); below 1.05x: 0
  v1-all  k1    : countries=17 min 2.233 (lt) median 2.681 max 3.123 (gb); below 1.05x: 0
  v1-all  k2    : countries=17 min 2.206 (lt) median 2.700 max 3.049 (gb); below 1.05x: 0
