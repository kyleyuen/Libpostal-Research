# Experiment 093 — re-measure of 091 (max-only Viterbi) on a fourth unseen set (golden-v7-osm-unseen)

Date/time/timezone: created 2026-10-03T11:14:51-0700; run 2026-10-03T11:14:51-0700 → 2026-10-03T11:19:12-0700, America/Los_Angeles. Status: complete — **vmx accepted**

Outputs (`raw/output_check.txt`): pcp, vmo, vmx identical to the original on all 8,973 addresses. Speed (sequential, 5 rotating rounds, 20 passes; 20 runs exit 0; `raw/analyze.stdout`):
    | binary | unseen-v7 µs/address | speedup vs original (range) | vs pcp (per-round range) | max peak footprint |
    |---|---:|---:|---:|---:|
    | orig | 19.659 | 1.000× (1.00–1.00) | 0.1978 (0.198–0.199) | 2.612 GB |
    | pcp | 3.892 | 5.057× (5.01–5.06) | 1.0000 (1.000–1.000) | 2.538 GB |
    | vmo | 3.844 | 5.118× (5.07–5.15) | 1.0135 (1.002–1.019) | 2.538 GB |
    | vmx | 3.751 | 5.224× (5.18–5.25) | 1.0329 (1.024–1.045) | 2.538 GB |
vmx is 3.3% faster than pcp on unseen data (all rounds, 2.4–4.5%): 5.22×.
