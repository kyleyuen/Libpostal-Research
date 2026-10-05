| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.91 | nan | 100% | 34.51 | 34.19 | 34.51 | nan (0) | 35.32 |
| novel | u2 | 90 | 8.86 | nan | 93% | 10.06 | 9.99 | 10.06 | 10.16 (6) | 10.41 |
| novel | r1 | 90 | 8.93 | nan | 97% | 10.03 | 9.97 | 10.03 | 10.00 (3) | 10.33 |
| novel | v1 | 90 | 9.04 | nan | 96% | 10.04 | 9.97 | 10.04 | 10.00 (4) | 10.17 |
| novel | n1 | 90 | 8.84 | nan | 93% | 10.00 | 9.93 | 10.00 | 9.98 (6) | 10.37 |
| novel | f1 | 90 | 8.61 | nan | 91% | 9.67 | 9.59 | 9.66 | 9.63 (8) | 9.82 |
| novel | a1 | 90 | 8.53 | nan | 92% | 9.71 | 9.64 | 9.71 | 9.69 (7) | 9.87 |
| novel | u3 | 90 | 8.98 | nan | 91% | 9.34 | 9.29 | 9.34 | 9.30 (8) | 9.41 |
| novel | b1 | 90 | 8.49 | nan | 94% | 8.99 | 8.95 | 8.99 | 8.98 (5) | 9.11 |
| novel | b2 | 90 | 8.64 | nan | 89% | 8.98 | 8.90 | 8.98 | 8.94 (10) | 9.12 |
| novel | h1 | 90 | 8.49 | nan | 92% | 8.93 | 8.89 | 8.93 | 8.93 (7) | 9.00 |
| novel | p1 | 90 | 8.47 | nan | 92% | 8.97 | 8.93 | 8.97 | 8.94 (7) | 9.04 |
| v1-all | baseline | 90 | 2.94 | nan | 100% | 33.93 | 33.51 | 33.93 | nan (0) | 34.35 |
| v1-all | u2 | 90 | 8.84 | nan | 96% | 9.84 | 9.77 | 9.83 | 9.88 (4) | 10.16 |
| v1-all | r1 | 90 | 8.90 | nan | 96% | 9.84 | 9.78 | 9.84 | 9.83 (4) | 10.02 |
| v1-all | v1 | 90 | 8.94 | nan | 90% | 9.85 | 9.79 | 9.85 | 9.81 (9) | 9.97 |
| v1-all | n1 | 90 | 8.85 | nan | 94% | 9.84 | 9.76 | 9.83 | 9.94 (5) | 10.11 |
| v1-all | f1 | 90 | 8.65 | nan | 98% | 9.45 | 9.40 | 9.45 | 9.56 (2) | 9.64 |
| v1-all | a1 | 90 | 8.48 | nan | 96% | 9.53 | 9.47 | 9.53 | 9.51 (4) | 9.74 |
| v1-all | u3 | 90 | 9.14 | nan | 94% | 9.19 | 9.13 | 9.19 | 9.15 (5) | 9.28 |
| v1-all | b1 | 90 | 8.45 | nan | 90% | 8.88 | 8.82 | 8.87 | 8.84 (9) | 9.06 |
| v1-all | b2 | 90 | 8.55 | nan | 90% | 8.82 | 8.75 | 8.82 | 8.78 (9) | 8.95 |
| v1-all | h1 | 90 | 8.42 | nan | 91% | 8.81 | 8.74 | 8.80 | 8.76 (8) | 8.89 |
| v1-all | p1 | 90 | 8.47 | nan | 94% | 8.84 | 8.78 | 8.84 | 8.79 (5) | 8.91 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | u2 | 3.430x | 3.423x | 3.431x | nanx | 3.392x |
| novel | r1 | 3.441x | 3.429x | 3.441x | nanx | 3.419x |
| novel | v1 | 3.438x | 3.429x | 3.438x | nanx | 3.474x |
| novel | n1 | 3.451x | 3.443x | 3.451x | nanx | 3.406x |
| novel | f1 | 3.570x | 3.565x | 3.570x | nanx | 3.595x |
| novel | a1 | 3.555x | 3.547x | 3.555x | nanx | 3.578x |
| novel | u3 | 3.694x | 3.681x | 3.694x | nanx | 3.753x |
| novel | b1 | 3.838x | 3.820x | 3.838x | nanx | 3.877x |
| novel | b2 | 3.843x | 3.842x | 3.844x | nanx | 3.874x |
| novel | h1 | 3.863x | 3.846x | 3.862x | nanx | 3.922x |
| novel | p1 | 3.846x | 3.829x | 3.846x | nanx | 3.905x |
| v1-all | u2 | 3.450x | 3.431x | 3.450x | nanx | 3.381x |
| v1-all | r1 | 3.447x | 3.426x | 3.448x | nanx | 3.429x |
| v1-all | v1 | 3.444x | 3.424x | 3.444x | nanx | 3.447x |
| v1-all | n1 | 3.450x | 3.431x | 3.450x | nanx | 3.397x |
| v1-all | f1 | 3.591x | 3.564x | 3.591x | nanx | 3.564x |
| v1-all | a1 | 3.559x | 3.536x | 3.560x | nanx | 3.528x |
| v1-all | u3 | 3.691x | 3.669x | 3.691x | nanx | 3.700x |
| v1-all | b1 | 3.823x | 3.799x | 3.823x | nanx | 3.792x |
| v1-all | b2 | 3.848x | 3.828x | 3.847x | nanx | 3.840x |
| v1-all | h1 | 3.854x | 3.833x | 3.854x | nanx | 3.863x |
| v1-all | p1 | 3.839x | 3.817x | 3.839x | nanx | 3.855x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   u2    : countries=17 min 2.797 (tw) median 3.382 max 3.841 (ru); below 1.05x: 0
  novel   r1    : countries=17 min 2.849 (tw) median 3.409 max 3.870 (ru); below 1.05x: 0
  novel   v1    : countries=17 min 2.844 (tw) median 3.401 max 3.878 (ru); below 1.05x: 0
  novel   n1    : countries=17 min 2.815 (tw) median 3.415 max 3.849 (ru); below 1.05x: 0
  novel   f1    : countries=17 min 2.943 (tw) median 3.544 max 3.981 (ru); below 1.05x: 0
  novel   a1    : countries=17 min 2.919 (tw) median 3.527 max 3.950 (ru); below 1.05x: 0
  novel   u3    : countries=17 min 3.068 (tw) median 3.660 max 4.160 (ru); below 1.05x: 0
  novel   b1    : countries=17 min 3.157 (tw) median 3.813 max 4.248 (ru); below 1.05x: 0
  novel   b2    : countries=17 min 3.163 (tw) median 3.832 max 4.276 (ru); below 1.05x: 0
  novel   h1    : countries=17 min 3.159 (tw) median 3.827 max 4.253 (ru); below 1.05x: 0
  novel   p1    : countries=17 min 3.141 (tw) median 3.808 max 4.258 (ru); below 1.05x: 0
  v1-all  u2    : countries=17 min 2.844 (tw) median 3.412 max 3.813 (ru); below 1.05x: 0
  v1-all  r1    : countries=17 min 2.835 (tw) median 3.402 max 3.806 (ru); below 1.05x: 0
  v1-all  v1    : countries=17 min 2.829 (tw) median 3.421 max 3.818 (ru); below 1.05x: 0
  v1-all  n1    : countries=17 min 2.846 (tw) median 3.416 max 3.772 (ru); below 1.05x: 0
  v1-all  f1    : countries=17 min 2.972 (tw) median 3.547 max 3.934 (ru); below 1.05x: 0
  v1-all  a1    : countries=17 min 2.930 (tw) median 3.524 max 3.862 (ru); below 1.05x: 0
  v1-all  u3    : countries=17 min 3.055 (tw) median 3.681 max 4.084 (ru); below 1.05x: 0
  v1-all  b1    : countries=17 min 3.132 (tw) median 3.783 max 4.127 (ru); below 1.05x: 0
  v1-all  b2    : countries=17 min 3.166 (tw) median 3.822 max 4.164 (ru); below 1.05x: 0
  v1-all  h1    : countries=17 min 3.208 (tw) median 3.843 max 4.189 (ru); below 1.05x: 0
  v1-all  p1    : countries=17 min 3.138 (tw) median 3.804 max 4.159 (ru); below 1.05x: 0
