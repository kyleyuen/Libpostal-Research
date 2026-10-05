# Experiment 052 — memory composition of the deliverable `va` (measurement only)

Date/time/timezone: 2026-10-02 (`raw/created.txt`). Status: complete.

Diagnostic switch `EXP_MEMSIZE` (experiment copy of `final/048-va/sources`; -O2 build `ms`) prints, at the first parse, the bytes held by each model structure (`raw/memsize.stderr`):

| structure | MB |
|---|---:|
| postcode-context graph (5.48M rows, 122.3M edges) | 20.9 indptr + **466.5 indices** |
| state feature dictionary | 225.0 slots + 173.2 arena + 56.4 integer slots (9.83M string keys, 1.64M integer keys) |
| state weights (33.8M entries) | 43.8 row index + 322.3 records |
| role-id table (7.39M vocabulary slots × 6 × 4 B) | 169.1 |
| trans weights (13.4M entries) | 8.0 + 127.5 |
| component-phrase trie | 62.7 nodes + 30.6 data + 22.5 tail |
| vocabulary dictionary (3.69M keys) | 56.4 slots + 58.9 arena |
| postal-code trie | 57.2 nodes + 41.8 data + 6.3 tail |
| trans dictionary | 15.2 + 10.8 + 48.9 |
| phrase types | 15.3 |

Sum ≈2.03 GB of the 2.14 GB steady footprint. The postcode-context graph (≈487 MB), used only for ≈4 membership tests per address, is the largest single structure and a candidate for exact compression to fund speed structures.
