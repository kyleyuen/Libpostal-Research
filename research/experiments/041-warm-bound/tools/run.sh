#!/bin/bash
# 041: warm bound of l3 (each address parsed twice, second call timed) vs normal bench_ext of l3; golden-v1, 3 runs each alternating, 10 passes.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/041-warm-bound"; B="$E/raw"; D="$W/profiling/golden-v2-novel/v1-all.tsv"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for r in 1 2 3; do gate; cap warm_r$r "$E/tools/bench_warm_l3" "$D" 10; sleep 2; gate; cap normal_r$r "$X/021-final-stack/artifacts/l3/bench_ext" "$D" 10; sleep 2; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
