#!/bin/bash
# 044: l3 time per address when cycling working sets of N addresses (first N of the golden-v1 development split), N = 30..10308; passes chosen so
# each run parses ~100k addresses (min 20 passes). Sequential, AC-gated, 2 runs per N (ascending then descending).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/044-working-set-sweep"; B="$E/raw"; L3="$X/021-final-stack/artifacts/l3/bench_ext"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for r in 1 2; do [ $r = 1 ] && L="30 100 300 1000 3000 10308" || L="10308 3000 1000 300 100 30"
  for n in $L; do p=$(( 100000 / n )); [ $p -lt 20 ] && p=20; gate; log "START n=$n r=$r passes=$p"
    "$L3" "$E/data/dev_first_$n.tsv" $p > "$B/ws_${n}_r$r.stdout" 2> "$B/ws_${n}_r$r.stderr"; log "END n=$n r=$r exit=$?"; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
