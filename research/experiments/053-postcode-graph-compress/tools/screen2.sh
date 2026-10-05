#!/bin/bash
# 053 screen 2: sequential, rotating va / pc / pc4 / pc6 on golden-v1, 20 passes, 4 rounds; then steady footprint of pc4 and pc6.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/053-postcode-graph-compress"; B="$E/raw/screen2"; mkdir -p "$B"; D="$W/profiling/golden-v2-novel/v1-all.tsv"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(va="$X/048-viterbi20-alias/artifacts/va/bench_ext" pc="$E/artifacts/pc/bench_ext" pc4="$E/artifacts/pc4/bench_ext" pc6="$E/artifacts/pc6/bench_ext"); n=${#V[@]}
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for r in 1 2 3 4; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; "$bin" "$D" 20 > "$B/${name}_r$r.stdout" 2>&1; sleep 2; done; done
for v in "${V[@]}"; do sn=${v%%=*}; b=${v#*=}; "$b" "$D" 1000 > "$B/steady_run.stdout" 2>&1 & pid=$!; t=0
  until grep -q "^pass=3 " "$B/steady_run.stdout" 2>/dev/null; do sleep 1; t=$((t+1)); [ $t -gt 120 ] && break; done
  echo "$sn $(footprint -p $pid 2>&1 | grep -o 'Footprint: [0-9]* MB')" >> "$B/steady.txt"; kill $pid; wait $pid 2>/dev/null; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
