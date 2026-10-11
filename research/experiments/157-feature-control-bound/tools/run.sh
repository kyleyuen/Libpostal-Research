#!/bin/bash
# 157: nem (no string emission, diagnostic) vs 119 css, split harness (3 passes), golden-v1 all + OSM, 4 rounds alternating, gated, serial.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/157-feature-control-bound"; O="$E/raw/split"; mkdir -p "$O"
for r in 1 2 3 4; do for ds in v1 osm; do
  case $ds in v1) IN="$W/profiling/golden-v2-novel/v1-all.tsv";; osm) IN="$W/profiling/golden-v3-osm/osm.tsv";; esac
  if [ $((r % 2)) = 1 ]; then o="nem css"; else o="css nem"; fi
  for v in $o; do if [ $v = nem ]; then b="$E/artifacts/nem/bench_split"; else b="$X/119-component-phrase-ids/artifacts/css/bench_split"; fi
    until "$W/research/tools/env_gate.sh" >> "$O/gate.log" 2>&1; do sleep 30; done
    date "+%Y-%m-%dT%H:%M:%S%z start ${v}_${ds}_r$r" >> "$O/run.log"
    /usr/bin/time -l "$b" "$IN" 3 > "$O/${v}_${ds}_r$r.stdout" 2> "$O/${v}_${ds}_r$r.stderr"; echo "exit $? ${v}_${ds}_r$r" >> "$O/run.log"; sleep 2
  done; done; done
date "+%Y-%m-%dT%H:%M:%S%z DONE" >> "$O/run.log"
