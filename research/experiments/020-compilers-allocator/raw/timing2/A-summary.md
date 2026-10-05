| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.99 | nan | 100% | 34.38 | 33.85 | 34.37 | nan (0) | 35.13 |
| novel | p1 | 90 | 8.49 | nan | 93% | 9.00 | 8.94 | 9.00 | 9.00 (6) | 9.07 |
| novel | q1 | 90 | 9.84 | nan | 91% | 8.65 | 8.61 | 8.65 | 8.61 (8) | 8.75 |
| novel | q2 | 90 | 9.84 | nan | 89% | 8.60 | 8.57 | 8.60 | 8.58 (10) | 8.82 |
| novel | g1 | 90 | 9.83 | nan | 91% | 8.83 | 8.79 | 8.83 | 8.79 (8) | 8.94 |
| novel | g2 | 90 | 13.14 | nan | 92% | 8.93 | 8.88 | 8.93 | 8.88 (7) | 9.17 |
| novel | z1 | 90 | 8.61 | nan | 96% | 9.09 | 9.05 | 9.09 | 9.07 (4) | 9.20 |
| novel | z2 | 90 | 9.39 | nan | 93% | 9.61 | 9.54 | 9.61 | 9.55 (6) | 9.74 |
| novel | z3 | 90 | 8.47 | nan | 93% | 8.89 | 8.86 | 8.89 | 8.88 (6) | 8.94 |
| v1-all | baseline | 90 | 2.94 | nan | 100% | 33.85 | 33.39 | 33.84 | nan (0) | 34.37 |
| v1-all | p1 | 90 | 8.48 | nan | 89% | 8.86 | 8.80 | 8.86 | 8.84 (10) | 8.96 |
| v1-all | q1 | 90 | 9.85 | nan | 87% | 8.49 | 8.43 | 8.49 | 8.43 (12) | 8.76 |
| v1-all | q2 | 90 | 9.89 | nan | 89% | 8.53 | 8.44 | 8.52 | 8.45 (10) | 8.65 |
| v1-all | g1 | 90 | 9.85 | nan | 92% | 8.70 | 8.67 | 8.70 | 8.68 (7) | 8.81 |
| v1-all | g2 | 90 | 13.01 | nan | 92% | 8.79 | 8.74 | 8.78 | 8.76 (7) | 8.88 |
| v1-all | z1 | 90 | 8.54 | nan | 87% | 8.97 | 8.91 | 8.97 | 8.92 (12) | 9.08 |
| v1-all | z2 | 90 | 9.38 | nan | 93% | 9.52 | 9.45 | 9.52 | 9.50 (6) | 9.62 |
| v1-all | z3 | 90 | 8.47 | nan | 90% | 8.80 | 8.74 | 8.80 | 8.74 (9) | 8.89 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | p1 | 3.820x | 3.786x | 3.819x | nanx | 3.873x |
| novel | q1 | 3.975x | 3.933x | 3.975x | nanx | 4.016x |
| novel | q2 | 3.997x | 3.950x | 3.996x | nanx | 3.982x |
| novel | g1 | 3.893x | 3.853x | 3.891x | nanx | 3.929x |
| novel | g2 | 3.849x | 3.814x | 3.848x | nanx | 3.832x |
| novel | z1 | 3.782x | 3.740x | 3.781x | nanx | 3.818x |
| novel | z2 | 3.577x | 3.547x | 3.577x | nanx | 3.607x |
| novel | z3 | 3.865x | 3.822x | 3.865x | nanx | 3.928x |
| v1-all | p1 | 3.818x | 3.793x | 3.818x | nanx | 3.835x |
| v1-all | q1 | 3.987x | 3.960x | 3.987x | nanx | 3.923x |
| v1-all | q2 | 3.970x | 3.958x | 3.970x | nanx | 3.972x |
| v1-all | g1 | 3.889x | 3.853x | 3.889x | nanx | 3.899x |
| v1-all | g2 | 3.853x | 3.821x | 3.853x | nanx | 3.869x |
| v1-all | z1 | 3.774x | 3.747x | 3.774x | nanx | 3.784x |
| v1-all | z2 | 3.555x | 3.535x | 3.555x | nanx | 3.572x |
| v1-all | z3 | 3.845x | 3.819x | 3.845x | nanx | 3.867x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   p1    : countries=17 min 3.101 (tw) median 3.774 max 4.207 (ru); below 1.05x: 0
  novel   q1    : countries=17 min 3.223 (tw) median 3.938 max 4.385 (ru); below 1.05x: 0
  novel   q2    : countries=17 min 3.247 (tw) median 3.945 max 4.392 (ru); below 1.05x: 0
  novel   g1    : countries=17 min 3.197 (tw) median 3.857 max 4.249 (ru); below 1.05x: 0
  novel   g2    : countries=17 min 3.112 (tw) median 3.854 max 4.214 (ru); below 1.05x: 0
  novel   z1    : countries=17 min 3.071 (tw) median 3.720 max 4.121 (ru); below 1.05x: 0
  novel   z2    : countries=17 min 2.982 (tw) median 3.547 max 3.823 (ru); below 1.05x: 0
  novel   z3    : countries=17 min 3.141 (tw) median 3.815 max 4.253 (ru); below 1.05x: 0
  v1-all  p1    : countries=17 min 3.187 (tw) median 3.778 max 4.125 (ru); below 1.05x: 0
  v1-all  q1    : countries=17 min 3.258 (tw) median 3.913 max 4.287 (ru); below 1.05x: 0
  v1-all  q2    : countries=17 min 3.311 (tw) median 3.920 max 4.296 (ru); below 1.05x: 0
  v1-all  g1    : countries=17 min 3.252 (tw) median 3.806 max 4.155 (ru); below 1.05x: 0
  v1-all  g2    : countries=17 min 3.177 (tw) median 3.811 max 4.169 (ru); below 1.05x: 0
  v1-all  z1    : countries=17 min 3.126 (tw) median 3.726 max 4.077 (ru); below 1.05x: 0
  v1-all  z2    : countries=17 min 3.025 (tw) median 3.548 max 3.768 (ru); below 1.05x: 0
  v1-all  z3    : countries=17 min 3.179 (tw) median 3.817 max 4.171 (ru); below 1.05x: 0
