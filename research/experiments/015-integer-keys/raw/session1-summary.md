| dataset | variant | trials | setup s (median) | cold pass1 µs/addr | warm pass19 µs/addr | cold speedup | warm speedup | cold spread | warm spread |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| novel | baseline | 5 | 2.95 | 37.28 | 35.46 | 1.000x | 1.000x | 5.6% | 8.5% |
| novel | h | 5 | 6.66 | 13.46 | 13.14 | 2.770x | 2.699x | 2.9% | 2.3% |
| novel | h0 | 5 | 6.62 | 13.41 | 13.23 | 2.779x | 2.680x | 6.2% | 3.8% |
| novel | m | 5 | 6.65 | 13.60 | 13.30 | 2.742x | 2.665x | 34.8% | 2.4% |
| novel | k1 | 5 | 9.31 | 13.26 | 13.02 | 2.811x | 2.724x | 8.9% | 20.7% |
| novel | k2 | 5 | 12.13 | 13.40 | 13.06 | 2.783x | 2.714x | 6.8% | 7.8% |
| v1-all | baseline | 5 | 2.95 | 36.47 | 35.99 | 1.000x | 1.000x | 11.8% | 19.8% |
| v1-all | h | 5 | 6.67 | 13.64 | 13.27 | 2.673x | 2.712x | 15.2% | 1.6% |
| v1-all | h0 | 5 | 6.51 | 13.47 | 13.12 | 2.707x | 2.743x | 6.1% | 1.3% |
| v1-all | m | 5 | 6.66 | 13.39 | 13.18 | 2.724x | 2.731x | 4.3% | 2.0% |
| v1-all | k1 | 5 | 9.21 | 13.09 | 12.87 | 2.785x | 2.797x | 3.9% | 3.3% |
| v1-all | k2 | 5 | 11.95 | 13.31 | 12.92 | 2.741x | 2.785x | 3.2% | 6.7% |

Per-country speedup vs baseline (countries with >=100 addresses; medians over trials):
  novel   h          cold: countries=17 min 2.290 (lt) median 2.648 max 2.905 (us); #countries below 1.05x: 0
  novel   h          warm: countries=17 min 2.244 (lt) median 2.623 max 2.896 (us); #countries below 1.05x: 0
  novel   h0         cold: countries=17 min 2.277 (lt) median 2.641 max 2.914 (nl); #countries below 1.05x: 0
  novel   h0         warm: countries=17 min 2.207 (lt) median 2.612 max 2.878 (us); #countries below 1.05x: 0
  novel   m          cold: countries=17 min 2.251 (lt) median 2.618 max 2.897 (nl); #countries below 1.05x: 0
  novel   m          warm: countries=17 min 2.232 (lt) median 2.606 max 2.849 (nl); #countries below 1.05x: 0
  novel   k1         cold: countries=17 min 2.255 (lt) median 2.678 max 2.970 (nl); #countries below 1.05x: 0
  novel   k1         warm: countries=17 min 2.237 (lt) median 2.669 max 2.965 (nl); #countries below 1.05x: 0
  novel   k2         cold: countries=17 min 2.261 (lt) median 2.662 max 2.985 (nl); #countries below 1.05x: 0
  novel   k2         warm: countries=17 min 2.235 (lt) median 2.663 max 2.957 (nl); #countries below 1.05x: 0
  v1-all  h          cold: countries=17 min 2.130 (lt) median 2.564 max 3.003 (us); #countries below 1.05x: 0
  v1-all  h          warm: countries=17 min 2.213 (lt) median 2.594 max 3.024 (gb); #countries below 1.05x: 0
  v1-all  h0         cold: countries=17 min 2.167 (lt) median 2.585 max 3.048 (us); #countries below 1.05x: 0
  v1-all  h0         warm: countries=17 min 2.225 (lt) median 2.619 max 3.053 (us); #countries below 1.05x: 0
  v1-all  m          cold: countries=17 min 2.166 (lt) median 2.611 max 3.059 (us); #countries below 1.05x: 0
  v1-all  m          warm: countries=17 min 2.207 (lt) median 2.614 max 3.033 (gb); #countries below 1.05x: 0
  v1-all  k1         cold: countries=17 min 2.172 (lt) median 2.673 max 3.109 (us); #countries below 1.05x: 0
  v1-all  k1         warm: countries=17 min 2.233 (lt) median 2.681 max 3.123 (gb); #countries below 1.05x: 0
  v1-all  k2         cold: countries=17 min 2.141 (lt) median 2.658 max 3.020 (us); #countries below 1.05x: 0
  v1-all  k2         warm: countries=17 min 2.206 (lt) median 2.700 max 3.049 (gb); #countries below 1.05x: 0
