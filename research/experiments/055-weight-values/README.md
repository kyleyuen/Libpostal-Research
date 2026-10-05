# Experiment 055 — distinct weight values (model-only measurement)

Date: 2026-10-02 (`raw/created.txt`). Status: complete; idea not pursued.

Diagnostic `EXP_WVAL_COUNT` (-O2 build `wc`, on 053 sources) counts distinct 64-bit weight values in the packed rows (`raw/wval.stderr`): state 33,799,350 entries, **22,123,311 distinct**; transition 13,369,573 entries, **12,450,028 distinct**. A value-table encoding (small index per entry) cannot shrink weight rows; not pursued.
