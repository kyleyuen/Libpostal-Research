| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.97 | nan | 100% | 34.77 | 33.91 | 34.76 | nan (0) | 35.27 |
| novel | k0 | 90 | 9.21 | nan | 98% | 12.71 | 12.53 | 12.71 | 12.64 (2) | 13.37 |
| novel | t4 | 90 | 9.23 | nan | 99% | 12.15 | 11.93 | 12.15 | 12.03 (1) | 12.53 |
| novel | t45 | 90 | 9.24 | nan | 97% | 11.68 | 11.51 | 11.68 | 11.48 (3) | 11.96 |
| novel | t6 | 90 | 9.20 | nan | 99% | 12.66 | 12.53 | 12.65 | 12.57 (1) | 12.84 |
| novel | t7 | 90 | 9.20 | nan | 99% | 12.89 | 12.61 | 12.88 | 12.72 (1) | 12.92 |
| novel | t8 | 90 | 9.25 | nan | 100% | 12.82 | 12.54 | 12.82 | nan (0) | 13.00 |
| novel | all | 90 | 9.18 | nan | 98% | 11.69 | 11.45 | 11.68 | 11.49 (2) | 11.84 |
| novel | all7 | 90 | 9.22 | nan | 99% | 11.65 | 11.38 | 11.65 | 11.38 (1) | 11.83 |
| novel | pgo | 90 | 9.04 | nan | 97% | 11.28 | 11.05 | 11.27 | 11.35 (3) | 11.63 |
| novel | pgolto | 90 | 9.20 | nan | 91% | 10.63 | 10.46 | 10.63 | 10.49 (8) | 10.90 |
| v1-all | baseline | 90 | 3.01 | nan | 100% | 34.54 | 33.50 | 34.50 | nan (0) | 35.34 |
| v1-all | k0 | 90 | 9.21 | nan | 99% | 12.73 | 12.57 | 12.73 | 12.55 (1) | 12.90 |
| v1-all | t4 | 90 | 9.36 | nan | 99% | 12.12 | 11.90 | 12.12 | 12.42 (1) | 12.17 |
| v1-all | t45 | 90 | 9.19 | nan | 99% | 11.45 | 11.37 | 11.45 | 11.85 (1) | 11.67 |
| v1-all | t6 | 90 | 9.29 | nan | 99% | 12.57 | 12.47 | 12.57 | 12.52 (1) | 12.87 |
| v1-all | t7 | 90 | 9.20 | nan | 99% | 12.82 | 12.59 | 12.82 | 12.82 (1) | 13.07 |
| v1-all | t8 | 90 | 9.39 | nan | 100% | 12.75 | 12.56 | 12.75 | nan (0) | 13.00 |
| v1-all | all | 90 | 9.20 | nan | 98% | 11.64 | 11.39 | 11.64 | 11.53 (2) | 11.88 |
| v1-all | all7 | 90 | 9.24 | nan | 94% | 11.62 | 11.40 | 11.61 | 11.40 (5) | 12.16 |
| v1-all | pgo | 90 | 9.27 | nan | 100% | 11.30 | 11.01 | 11.29 | nan (0) | 12.18 |
| v1-all | pgolto | 90 | 9.20 | nan | 99% | 10.59 | 10.35 | 10.59 | 10.66 (1) | 10.87 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | k0 | 2.735x | 2.707x | 2.734x | nanx | 2.638x |
| novel | t4 | 2.861x | 2.841x | 2.861x | nanx | 2.814x |
| novel | t45 | 2.976x | 2.945x | 2.975x | nanx | 2.949x |
| novel | t6 | 2.747x | 2.706x | 2.747x | nanx | 2.746x |
| novel | t7 | 2.698x | 2.689x | 2.698x | nanx | 2.730x |
| novel | t8 | 2.713x | 2.704x | 2.712x | nanx | 2.714x |
| novel | all | 2.975x | 2.962x | 2.975x | nanx | 2.979x |
| novel | all7 | 2.984x | 2.978x | 2.984x | nanx | 2.980x |
| novel | pgo | 3.082x | 3.069x | 3.084x | nanx | 3.032x |
| novel | pgolto | 3.271x | 3.240x | 3.270x | nanx | 3.235x |
| v1-all | k0 | 2.712x | 2.665x | 2.709x | nanx | 2.738x |
| v1-all | t4 | 2.849x | 2.815x | 2.847x | nanx | 2.903x |
| v1-all | t45 | 3.016x | 2.947x | 3.013x | nanx | 3.028x |
| v1-all | t6 | 2.747x | 2.686x | 2.744x | nanx | 2.746x |
| v1-all | t7 | 2.694x | 2.661x | 2.692x | nanx | 2.705x |
| v1-all | t8 | 2.708x | 2.667x | 2.706x | nanx | 2.718x |
| v1-all | all | 2.967x | 2.940x | 2.964x | nanx | 2.976x |
| v1-all | all7 | 2.972x | 2.939x | 2.970x | nanx | 2.907x |
| v1-all | pgo | 3.057x | 3.042x | 3.057x | nanx | 2.902x |
| v1-all | pgolto | 3.262x | 3.237x | 3.258x | nanx | 3.252x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   k0    : countries=17 min 2.240 (lt) median 2.609 max 2.908 (nl); below 1.05x: 0
  novel   t4    : countries=17 min 2.496 (lt) median 2.913 max 3.034 (nl); below 1.05x: 0
  novel   t45   : countries=17 min 2.594 (tw) median 2.961 max 3.324 (ru); below 1.05x: 0
  novel   t6    : countries=17 min 2.255 (lt) median 2.710 max 2.962 (nl); below 1.05x: 0
  novel   t7    : countries=17 min 2.241 (lt) median 2.675 max 2.956 (nl); below 1.05x: 0
  novel   t8    : countries=17 min 2.253 (lt) median 2.681 max 2.946 (nl); below 1.05x: 0
  novel   all   : countries=17 min 2.594 (tw) median 2.957 max 3.315 (ru); below 1.05x: 0
  novel   all7  : countries=17 min 2.633 (tw) median 3.013 max 3.354 (ru); below 1.05x: 0
  novel   pgo   : countries=17 min 2.698 (tw) median 3.089 max 3.443 (ru); below 1.05x: 0
  novel   pgolto: countries=17 min 2.853 (tw) median 3.271 max 3.681 (ru); below 1.05x: 0
  v1-all  k0    : countries=17 min 2.136 (lt) median 2.621 max 3.012 (gb); below 1.05x: 0
  v1-all  t4    : countries=17 min 2.313 (lt) median 2.763 max 3.010 (gb); below 1.05x: 0
  v1-all  t45   : countries=17 min 2.585 (tw) median 2.981 max 3.297 (ru); below 1.05x: 0
  v1-all  t6    : countries=17 min 2.125 (lt) median 2.607 max 2.984 (gb); below 1.05x: 0
  v1-all  t7    : countries=17 min 2.133 (lt) median 2.617 max 3.004 (gb); below 1.05x: 0
  v1-all  t8    : countries=17 min 2.131 (lt) median 2.576 max 2.987 (gb); below 1.05x: 0
  v1-all  all   : countries=17 min 2.569 (lt) median 2.936 max 3.224 (ru); below 1.05x: 0
  v1-all  all7  : countries=17 min 2.582 (lt) median 2.972 max 3.271 (ru); below 1.05x: 0
  v1-all  pgo   : countries=17 min 2.656 (tw) median 3.025 max 3.338 (ru); below 1.05x: 0
  v1-all  pgolto: countries=17 min 2.779 (tw) median 3.196 max 3.530 (ru); below 1.05x: 0
