#!/bin/bash
# 038: (1) feature-section timing (fs stats build, 3 x 50 passes, development split); (2) instructions/cycles per address, l3 vs dup
# (component + postal searches run twice), difference of 2 vs 102 passes, 3 rotating reps. Sequential, AC-gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/038-split-diagnostics"; B="$E/raw"; DEV="$X/025-audit-next/data/development.tsv"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
for r in 1 2 3; do gate; cap fs_r$r "$E/artifacts/fs/stage_bench" "$DEV" 50; sleep 2; done
for r in 1 2 3; do for v in l3 dup; do [ $v = l3 ] && bin="$X/021-final-stack/artifacts/l3/bench_ext" || bin="$E/artifacts/dup/bench_ext"
  for p in 2 102; do gate; cap ic_${v}_p${p}_r$r /usr/bin/time -l "$bin" "$DEV" $p; sleep 2; done; done; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
