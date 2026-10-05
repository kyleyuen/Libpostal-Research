| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 95 | 2.91 | nan | 100% | 35.64 | 35.22 | 35.62 | nan (0) | 35.76 |
| novel | h | 95 | 6.58 | nan | 100% | 13.27 | 13.16 | 13.26 | nan (0) | 13.45 |
| novel | h0 | 95 | 6.59 | nan | 100% | 13.26 | 13.18 | 13.26 | nan (0) | 13.33 |
| novel | iA | 95 | 6.49 | nan | 100% | 13.38 | 13.28 | 13.38 | nan (0) | 13.64 |
| novel | iB | 95 | 6.53 | nan | 100% | 13.71 | 13.60 | 13.70 | nan (0) | 13.84 |
| novel | iC | 95 | 6.57 | nan | 100% | 13.48 | 13.39 | 13.47 | nan (0) | 13.70 |
| novel | i | 95 | 6.51 | nan | 100% | 13.62 | 13.51 | 13.62 | nan (0) | 13.76 |
| v1-all | baseline | 95 | 2.91 | nan | 100% | 35.14 | 34.69 | 35.13 | nan (0) | 35.31 |
| v1-all | h | 95 | 6.59 | nan | 100% | 13.25 | 13.15 | 13.25 | nan (0) | 13.43 |
| v1-all | h0 | 95 | 6.55 | nan | 100% | 13.23 | 13.11 | 13.23 | nan (0) | 13.41 |
| v1-all | iA | 95 | 6.47 | nan | 100% | 13.33 | 13.21 | 13.33 | nan (0) | 13.53 |
| v1-all | iB | 95 | 6.57 | nan | 100% | 13.60 | 13.46 | 13.59 | nan (0) | 13.79 |
| v1-all | iC | 95 | 6.57 | nan | 100% | 13.43 | 13.28 | 13.42 | nan (0) | 13.59 |
| v1-all | i | 95 | 6.47 | nan | 100% | 13.49 | 13.38 | 13.48 | nan (0) | 13.65 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | h | 2.686x | 2.676x | 2.686x | nanx | 2.660x |
| novel | h0 | 2.688x | 2.673x | 2.687x | nanx | 2.682x |
| novel | iA | 2.663x | 2.653x | 2.663x | nanx | 2.622x |
| novel | iB | 2.600x | 2.589x | 2.599x | nanx | 2.585x |
| novel | iC | 2.644x | 2.630x | 2.644x | nanx | 2.611x |
| novel | i | 2.617x | 2.606x | 2.616x | nanx | 2.599x |
| v1-all | h | 2.651x | 2.637x | 2.652x | nanx | 2.629x |
| v1-all | h0 | 2.655x | 2.647x | 2.655x | nanx | 2.633x |
| v1-all | iA | 2.636x | 2.627x | 2.636x | nanx | 2.610x |
| v1-all | iB | 2.584x | 2.577x | 2.585x | nanx | 2.560x |
| v1-all | iC | 2.617x | 2.613x | 2.617x | nanx | 2.598x |
| v1-all | i | 2.605x | 2.594x | 2.605x | nanx | 2.586x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   h     : countries=17 min 2.250 (lt) median 2.632 max 2.884 (us); below 1.05x: 0
  novel   h0    : countries=17 min 2.232 (lt) median 2.637 max 2.898 (us); below 1.05x: 0
  novel   iA    : countries=17 min 2.227 (lt) median 2.628 max 2.871 (us); below 1.05x: 0
  novel   iB    : countries=17 min 2.168 (lt) median 2.560 max 2.774 (us); below 1.05x: 0
  novel   iC    : countries=17 min 2.208 (lt) median 2.612 max 2.827 (nl); below 1.05x: 0
  novel   i     : countries=17 min 2.178 (lt) median 2.589 max 2.792 (gb); below 1.05x: 0
  v1-all  h     : countries=17 min 2.155 (lt) median 2.565 max 3.001 (gb); below 1.05x: 0
  v1-all  h0    : countries=17 min 2.161 (lt) median 2.565 max 2.982 (us); below 1.05x: 0
  v1-all  iA    : countries=17 min 2.145 (lt) median 2.545 max 2.965 (gb); below 1.05x: 0
  v1-all  iB    : countries=17 min 2.090 (lt) median 2.526 max 2.919 (gb); below 1.05x: 0
  v1-all  iC    : countries=17 min 2.119 (lt) median 2.565 max 2.948 (gb); below 1.05x: 0
  v1-all  i     : countries=17 min 2.133 (lt) median 2.562 max 2.919 (us); below 1.05x: 0
