# Experiment 135 — can the weight rows be compressed losslessly? (diagnostic, read-only)

Date/time/timezone: created 2026-10-04T15:47:44-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete — **idea closed**

Question (user, 2026-10-04): is there a mathematical way to touch less model data without changing any value? Candidate: replace each 8-byte weight with an index into a table of the model's distinct weight values (exact by construction).
Build `wd` = cpi + `EXP_WD_PROBE` (`sources/crf.c`), loading the model only (`raw/probe.stderr`):
- **State weights:** 11,470,623 rows, 33,799,350 records (mean 2.9 per row). **22,123,311 distinct values (65.5%)**; the most common value occurs 132 times; the top 65,536 values cover 2.1% of records; float32-exact 0.0%. An index needs 25 bits; with the 5-bit class, rows shrink from 322 MB to 121 MB, but the value table is 169 MB. Expected 128-byte lines per row: 1.22 → 1.08.
- **Transition weights:** 2,086,474 rows, 13,369,573 records (mean 6.4). **12,450,028 distinct (93.1%)**; float32-exact 0.0%. Rows 128 MB → 53 MB plus a 95 MB table; lines per row 1.49 → 1.20.
Conclusion: the trained weights are essentially all distinct full-precision doubles, so there is no lossless value-sharing or narrower format to exploit. A value table would add one random access into a 95–169 MB table per weight, which is more memory traffic, not less. Rows already span only about 1.2–1.5 cache lines, so even ideal packing saves little. Closed.
