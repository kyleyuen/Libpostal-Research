#!/bin/bash
# 048 screen: sequential, rotating order of l3 / v20 / al / va on golden-v1 (v1-all.tsv), 20 passes, 4 rounds, AC-gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/048-viterbi20-alias"; B="$E/raw/screen"; mkdir -p "$B"; D="$W/profiling/golden-v2-novel/v1-all.tsv"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(l3="$X/021-final-stack/artifacts/l3/bench_ext" v20="$E/artifacts/v20/bench_ext" al="$E/artifacts/al/bench_ext" va="$E/artifacts/va/bench_ext"); n=${#V[@]}
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for r in 1 2 3 4; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; "$bin" "$D" 20 > "$B/${name}_r$r.stdout" 2>&1; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
