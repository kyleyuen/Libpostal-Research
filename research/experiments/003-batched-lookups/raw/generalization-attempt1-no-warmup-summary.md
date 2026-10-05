| dataset | variant | trials | setup s (median) | cold pass1 µs/addr | warm pass5 µs/addr | cold speedup | warm speedup | cold spread | warm spread |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| novel | baseline | 5 | 2.84 | 41.63 | 34.34 | 1.000x | 1.000x | 40.3% | 20.9% |
| novel | c003 | 5 | 2.91 | 38.14 | 30.21 | 1.092x | 1.137x | 115.7% | 9.3% |
| v1-all | baseline | 5 | 2.90 | 37.38 | 37.10 | 1.000x | 1.000x | 69.4% | 17.3% |
| v1-all | c003 | 5 | 2.88 | 44.75 | 30.55 | 0.835x | 1.214x | 34.6% | 16.9% |

Per-country speedup vs baseline (countries with >=100 addresses; medians over trials):
  novel   c003       cold: countries=17 min 0.985 (gb) median 1.080 max 1.098 (fr); #countries below 1.05x: 2
  novel   c003       warm: countries=17 min 1.101 (tw) median 1.138 max 1.158 (nl); #countries below 1.05x: 0
  v1-all  c003       cold: countries=17 min 0.746 (tw) median 0.852 max 0.888 (nl); #countries below 1.05x: 17
  v1-all  c003       warm: countries=17 min 1.160 (ru) median 1.206 max 1.299 (hu); #countries below 1.05x: 0
