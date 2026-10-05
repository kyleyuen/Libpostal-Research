#!/bin/bash
# 058 screen sk: sequential, rotating va / ftr / sk on golden-v1 and OSM, 20 passes, 4 rounds.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/058-numeric-postal-table"; B="$E/raw/screen_sk"; mkdir -p "$B"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(va="$X/048-viterbi20-alias/artifacts/va/bench_ext" ftr="$X/056-first-token-table/artifacts/ftr/bench_ext" sk="$E/artifacts/sk/bench_ext"); n=${#V[@]}
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for d in v1:"$W/profiling/golden-v2-novel/v1-all.tsv" osm:"$W/profiling/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for r in 1 2 3 4; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; "$bin" "$f" 20 > "$B/${ds}_${name}_r$r.stdout" 2>&1; sleep 2; done; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
