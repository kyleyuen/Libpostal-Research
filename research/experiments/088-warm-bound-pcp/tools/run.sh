#!/bin/bash
# 088: warm bound of pcp (041 harness: each address parsed twice, second call timed) vs normal pcp bench_ext; golden-v1, OSM, unseen v6; 3 alternating runs, 10 passes.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/088-warm-bound-pcp"; B="$E/raw"; P="$W/profiling"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for r in 1 2 3; do for d in v1:"$P/golden-v2-novel/v1-all.tsv" osm:"$P/golden-v3-osm/osm.tsv" u6:"$P/golden-v6-osm-unseen/unseen6.tsv"; do ds=${d%%:*}; f=${d#*:}
  gate; cap warm_${ds}_r$r "$E/tools/bench_warm_pcp" "$f" 10; sleep 2; gate; cap normal_${ds}_r$r "$X/077-pc-prefetch/artifacts/pcp/bench_ext" "$f" 10; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
