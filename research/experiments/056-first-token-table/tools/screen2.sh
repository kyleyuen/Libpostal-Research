#!/bin/bash
# 056 screen 2: sequential, alternating ftt / ftr on golden-v1, 20 passes, 4 rounds.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/056-first-token-table"; B="$E/raw/screen2"; mkdir -p "$B"; D="$W/profiling/golden-v2-novel/v1-all.tsv"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(ftt="$E/artifacts/ftt/bench_ext" ftr="$E/artifacts/ftr/bench_ext"); n=${#V[@]}
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for r in 1 2 3 4; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; "$bin" "$D" 20 > "$B/${name}_r$r.stdout" 2>&1; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
