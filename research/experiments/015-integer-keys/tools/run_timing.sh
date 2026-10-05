#!/bin/bash
# Controlled unattended timing session for 015 (same runner and settings as 012: 5 trials x 19 passes, both datasets, rotating order).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/015-integer-keys"; A="$E/artifacts"
"$W/research/tools/run_unattended.sh" "$E/raw/session1" 5 19 baseline="$A/bench_ext_baseline" h="$W/research/experiments/011-normalize-fastpaths/artifacts/h/bench_ext" \
  h0="$A/h0/bench_ext" m="$A/m/bench_ext" k1="$A/k1/bench_ext" k2="$A/k2/bench_ext"
python3 "$W/research/tools/analyze_ext.py" "$E/raw/session1" baseline > "$E/raw/session1-summary.md" 2>&1
