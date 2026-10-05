| dataset | variant | passes | setup s | system busy % | passes preempted | median µs | P10 µs | CPU-time median µs | clean-pass median µs (n) | cold pass1 µs |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| novel | baseline | 90 | 2.99 | nan | 100% | 34.55 | 33.92 | 34.54 | nan (0) | 35.23 |
| novel | k0 | 90 | 9.07 | nan | 98% | 12.65 | 12.49 | 12.65 | 12.47 (2) | 13.09 |
| novel | pgolto | 90 | 9.08 | nan | 98% | 10.65 | 10.45 | 10.65 | 10.62 (2) | 11.06 |
| novel | c2 | 90 | 8.89 | nan | 94% | 10.41 | 10.32 | 10.41 | 10.33 (5) | 10.56 |
| novel | c2s1 | 90 | 8.85 | nan | 94% | 10.20 | 10.11 | 10.19 | 10.09 (5) | 10.34 |
| novel | c2s2 | 90 | 8.98 | nan | 94% | 10.21 | 10.11 | 10.20 | 10.14 (5) | 10.53 |
| novel | c3 | 90 | 8.95 | nan | 97% | 10.18 | 10.09 | 10.18 | 10.09 (3) | 10.37 |
| novel | c4 | 90 | 8.94 | nan | 94% | 10.16 | 10.07 | 10.16 | 10.06 (5) | 10.39 |
| v1-all | baseline | 90 | 2.96 | nan | 100% | 34.42 | 33.59 | 34.39 | nan (0) | 35.12 |
| v1-all | k0 | 90 | 9.20 | nan | 99% | 12.70 | 12.54 | 12.69 | 12.74 (1) | 12.89 |
| v1-all | pgolto | 90 | 9.19 | nan | 96% | 10.43 | 10.35 | 10.43 | 10.34 (4) | 10.72 |
| v1-all | c2 | 90 | 9.04 | nan | 94% | 10.30 | 10.20 | 10.30 | 10.27 (5) | 10.78 |
| v1-all | c2s1 | 90 | 8.95 | nan | 96% | 10.15 | 10.03 | 10.15 | 10.16 (4) | 10.54 |
| v1-all | c2s2 | 90 | 8.96 | nan | 97% | 10.12 | 10.00 | 10.12 | 9.97 (3) | 10.36 |
| v1-all | c3 | 90 | 9.08 | nan | 97% | 10.14 | 10.00 | 10.14 | 10.30 (3) | 10.30 |
| v1-all | c4 | 90 | 8.97 | nan | 100% | 10.11 | 9.98 | 10.11 | nan (0) | 10.47 |

Speedup vs baseline (baseline µs / variant µs) by estimator:

| dataset | variant | median | P10 (interference-free) | CPU-time | clean passes | cold pass1 |
|---|---|---:|---:|---:|---:|---:|
| novel | k0 | 2.731x | 2.716x | 2.731x | nanx | 2.691x |
| novel | pgolto | 3.243x | 3.247x | 3.243x | nanx | 3.186x |
| novel | c2 | 3.318x | 3.288x | 3.318x | nanx | 3.335x |
| novel | c2s1 | 3.388x | 3.355x | 3.388x | nanx | 3.407x |
| novel | c2s2 | 3.385x | 3.355x | 3.386x | nanx | 3.344x |
| novel | c3 | 3.393x | 3.362x | 3.392x | nanx | 3.398x |
| novel | c4 | 3.399x | 3.368x | 3.398x | nanx | 3.389x |
| v1-all | k0 | 2.711x | 2.679x | 2.709x | nanx | 2.724x |
| v1-all | pgolto | 3.299x | 3.245x | 3.297x | nanx | 3.276x |
| v1-all | c2 | 3.341x | 3.292x | 3.339x | nanx | 3.257x |
| v1-all | c2s1 | 3.389x | 3.348x | 3.387x | nanx | 3.333x |
| v1-all | c2s2 | 3.400x | 3.360x | 3.398x | nanx | 3.389x |
| v1-all | c3 | 3.394x | 3.358x | 3.392x | nanx | 3.409x |
| v1-all | c4 | 3.405x | 3.364x | 3.403x | nanx | 3.354x |

Per-country speedup (last warm pass, median over processes; countries with >=100 addresses):
  novel   k0    : countries=17 min 2.265 (lt) median 2.689 max 2.969 (nl); below 1.05x: 0
  novel   pgolto: countries=17 min 2.812 (tw) median 3.245 max 3.668 (ru); below 1.05x: 0
  novel   c2    : countries=17 min 2.780 (tw) median 3.278 max 3.729 (ru); below 1.05x: 0
  novel   c2s1  : countries=17 min 2.803 (tw) median 3.329 max 3.783 (ru); below 1.05x: 0
  novel   c2s2  : countries=17 min 2.812 (tw) median 3.348 max 3.788 (ru); below 1.05x: 0
  novel   c3    : countries=17 min 2.819 (tw) median 3.348 max 3.805 (ru); below 1.05x: 0
  novel   c4    : countries=17 min 2.762 (tw) median 3.311 max 3.740 (ru); below 1.05x: 0
  v1-all  k0    : countries=17 min 2.161 (lt) median 2.652 max 3.054 (gb); below 1.05x: 0
  v1-all  pgolto: countries=17 min 2.833 (tw) median 3.274 max 3.617 (ru); below 1.05x: 0
  v1-all  c2    : countries=17 min 2.816 (tw) median 3.314 max 3.703 (ru); below 1.05x: 0
  v1-all  c2s1  : countries=17 min 2.802 (tw) median 3.310 max 3.717 (ru); below 1.05x: 0
  v1-all  c2s2  : countries=17 min 2.821 (tw) median 3.344 max 3.718 (ru); below 1.05x: 0
  v1-all  c3    : countries=17 min 2.805 (tw) median 3.332 max 3.732 (ru); below 1.05x: 0
  v1-all  c4    : countries=17 min 2.801 (tw) median 3.322 max 3.715 (ru); below 1.05x: 0
