#!/bin/bash
# 094: validation of vmx (091 = pcp + max-only Viterbi): sequential timing of original, pcp (previous deliverable) and vmx on golden-v1, novel, OSM; 5 rotating rounds, 20 passes; thread check of vmx; steady footprint.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/094-validate-vmx"; B="$E/raw"; P="$W/profiling"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/timing_start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" pcp="$X/077-pc-prefetch/artifacts/pcp/bench_ext" vmx="$X/091-viterbi-maxonly/artifacts/vmx/bench_ext"); n=${#V[@]}
for r in 1 2 3 4 5; do for d in v1:"$P/golden-v2-novel/v1-all.tsv" nv:"$P/golden-v2-novel/novel.tsv" osm:"$P/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/${ds}_r${r}_$name /usr/bin/time -l "$bin" "$f" 20; sleep 2; done; done; done
"$X/091-viterbi-maxonly/artifacts/vmx/bench_ext" "$P/golden-v2-novel/v1-all.tsv" 200 > "$B/threads_bench.stdout" 2>&1 & pid=$!
for t in 1 2 3 4 5 6; do sleep 3; echo "t=$((t*3))s threads=$(ps -M -p $pid | tail -n +2 | wc -l | tr -d ' ')" >> "$B/threads.txt"; done; kill $pid; wait $pid 2>/dev/null
for v in "${V[@]}" "${V[@]}"; do sn=${v%%=*}; b=${v#*=}
  "$b" "$P/golden-v2-novel/v1-all.tsv" 1000 > "$B/steady/run.stdout" 2>&1 & pid=$!; t=0
  until grep -q "^pass=3 " "$B/steady/run.stdout" 2>/dev/null; do sleep 1; t=$((t+1)); [ $t -gt 120 ] && break; done
  echo "$sn $(date -Iseconds) $(footprint -p $pid 2>&1 | grep -o 'Footprint: [0-9]* MB')" >> "$B/steady/steady.txt"; kill $pid; wait $pid 2>/dev/null; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
