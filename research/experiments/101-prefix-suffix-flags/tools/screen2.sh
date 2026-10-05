#!/bin/bash
# 101 screen2 (repeat; screen 1 golden-v1 rounds 1-4 disturbed for both binaries): sequential, alternating npr / psf on golden-v1 and OSM, 20 passes, 6 rounds.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/101-prefix-suffix-flags"; B="$E/raw/screen2"; mkdir -p "$B"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(npr="$X/098-norm-pair/artifacts/npr/bench_ext" psf="$E/artifacts/psf/bench_ext"); n=${#V[@]}
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for d in v1:"$W/profiling/golden-v2-novel/v1-all.tsv" osm:"$W/profiling/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for r in 1 2 3 4 5 6; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; "$bin" "$f" 20 > "$B/${ds}_${name}_r$r.stdout" 2>&1; sleep 2; done; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
