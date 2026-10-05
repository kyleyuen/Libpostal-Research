| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 114 | 3.01 | nan | 89% | 36.60 | 35.27 | 36.54 | 35.40 (13) | 36.77 |
| novel | c003 | 114 | 2.96 | nan | 89% | 31.81 | 30.53 | 31.78 | 30.41 (12) | 32.64 |
| novel | a5 | 114 | 2.99 | nan | 88% | 30.45 | 29.52 | 30.44 | 29.59 (14) | 31.53 |
| novel | dn | 114 | 6.04 | nan | 69% | 21.07 | 20.57 | 21.06 | 20.71 (35) | 21.22 |
| novel | d | 114 | 6.17 | nan | 82% | 20.33 | 19.85 | 20.32 | 19.86 (21) | 21.19 |
| v1-all | baseline | 114 | 2.94 | nan | 83% | 35.74 | 34.68 | 35.74 | 34.94 (19) | 36.47 |
| v1-all | c003 | 114 | 3.00 | nan | 88% | 31.32 | 30.32 | 31.31 | 30.39 (14) | 31.63 |
| v1-all | a5 | 114 | 2.97 | nan | 89% | 30.16 | 29.28 | 30.15 | 29.15 (13) | 30.22 |
| v1-all | dn | 114 | 5.96 | nan | 82% | 21.25 | 20.68 | 21.24 | 20.77 (20) | 21.38 |
| v1-all | d | 114 | 6.05 | nan | 81% | 20.10 | 19.60 | 20.09 | 19.75 (22) | 20.42 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | c003 | 1.151x | 1.156x | 1.150x | 1.164x | 1.126x |
| novel | a5 | 1.202x | 1.195x | 1.200x | 1.197x | 1.166x |
| novel | dn | 1.737x | 1.715x | 1.735x | 1.709x | 1.733x |
| novel | d | 1.800x | 1.777x | 1.798x | 1.783x | 1.735x |
| v1-all | c003 | 1.141x | 1.144x | 1.141x | 1.150x | 1.153x |
| v1-all | a5 | 1.185x | 1.185x | 1.185x | 1.199x | 1.207x |
| v1-all | dn | 1.682x | 1.677x | 1.682x | 1.682x | 1.706x |
| v1-all | d | 1.778x | 1.770x | 1.778x | 1.769x | 1.786x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   c003  : countries=17 min 1.091 (tw) median 1.123 max 1.150 (nl); below 1.05x: 0
  novel   a5    : countries=17 min 1.147 (lt) median 1.168 max 1.180 (ch); below 1.05x: 0
  novel   dn    : countries=17 min 1.550 (tw) median 1.650 max 1.794 (ru); below 1.05x: 0
  novel   d     : countries=17 min 1.654 (lt) median 1.757 max 1.930 (ru); below 1.05x: 0
  v1-all  c003  : countries=17 min 1.113 (ru) median 1.142 max 1.170 (hu); below 1.05x: 0
  v1-all  a5    : countries=17 min 1.160 (pl) median 1.185 max 1.220 (gb); below 1.05x: 0
  v1-all  dn    : countries=17 min 1.549 (lt) median 1.666 max 1.774 (ru); below 1.05x: 0
  v1-all  d     : countries=17 min 1.617 (lt) median 1.732 max 1.895 (ru); below 1.05x: 0
