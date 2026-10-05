#!/bin/bash
# 042: instructions/cycles per address, l3 vs dupl (lookups + accumulation run twice), 2 vs 102 passes, 3 rotating reps; sequential, AC-gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/042-cold-attribution"; B="$E/raw"; DEV="$X/025-audit-next/data/development.tsv"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for r in 1 2 3; do for v in l3 dupl; do [ $v = l3 ] && bin="$X/021-final-stack/artifacts/l3/bench_ext" || bin="$E/artifacts/dupl/bench_ext"
  for p in 2 102; do gate; cap ic_${v}_p${p}_r$r /usr/bin/time -l "$bin" "$DEV" $p; sleep 2; done; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
