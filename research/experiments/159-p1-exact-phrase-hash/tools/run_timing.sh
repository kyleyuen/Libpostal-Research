#!/bin/bash
# 159 TIMING (lead only; serial, on AC, gated, under caffeinate -i). See TIMING_REQUEST.md.
# T1: ps_bench exact vs orig (same token streams; 60 passes; 3 rounds, order alternating), golden-v1 + OSM v3.
# T2: 119 css bench_split (cpi with stage timers), same session: cpi's whole context fill (CF) on golden-v1 all + OSM, 3 passes, 3 rounds.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/159-p1-exact-phrase-hash"; O="$E/raw/timing"; mkdir -p "$O"
PB="$E/artifacts/ps_bench"; TB="$E/artifacts/tables"; CSS="$X/119-component-phrase-ids/artifacts/css/bench_split"
shasum -a 256 "$PB" "$TB"/*.ps "$CSS" > "$O/hashes.txt"
gate() { until "$W/research/tools/env_gate.sh" >> "$O/gate.log" 2>&1; do sleep 30; done; }
run() { local n=$1; shift; gate; date "+%Y-%m-%dT%H:%M:%S%z start $n" >> "$O/run.log"; /usr/bin/time -l "$@" > "$O/$n.stdout" 2> "$O/$n.stderr"; echo "exit $? $n" >> "$O/run.log"; sleep 2; }
for r in 1 2 3; do for c in golden-v1 osm-v3; do
  OD="$W/research/oracles/v1/$c"
  if [ $((r % 2)) = 1 ]; then run exact_${c}_r$r "$PB" exact "$TB" "$OD" 60; run orig_${c}_r$r "$PB" orig "$OD" 60
  else run orig_${c}_r$r "$PB" orig "$OD" 60; run exact_${c}_r$r "$PB" exact "$TB" "$OD" 60; fi
done; done
for r in 1 2 3; do run css_v1_r$r "$CSS" "$W/profiling/golden-v2-novel/v1-all.tsv" 3; run css_osm_r$r "$CSS" "$W/profiling/golden-v3-osm/osm.tsv" 3; done
date "+%Y-%m-%dT%H:%M:%S%z DONE" >> "$O/run.log"
