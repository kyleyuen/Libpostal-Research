#!/bin/bash
# 147: ifs (cpi + EXP_INT_KEYS_PAIRS + stage + feature-section timers) alternating with 144 fss; dev/OSM/u11; 4 rounds; 3 passes. Serial, gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/147-ipr-section-split"; B="$E/raw"; P="$W/profiling"; mkdir -p "$B/split"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $cn | $(uptime | sed 's/.*load averages*: //')" >> "$B/load.log"; log "START $cn"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/timing_start.txt"
D=(dev:"$X/025-audit-next/data/development.tsv" osm:"$P/golden-v3-osm/osm.tsv" u11:"$P/golden-v11-osm-unseen/unseen11.tsv")
for r in 1 2 3 4; do for d in "${D[@]}"; do ds=${d%%:*}; f=${d#*:}
  if [ $((r % 2)) -eq 1 ]; then o="ifs fss"; else o="fss ifs"; fi
  for v in $o; do if [ $v = ifs ]; then b="$E/artifacts/ifs/bench_split"; else b="$X/144-feature-sections-cpi/artifacts/fss/bench_split"; fi
    gate; cap split/${ds}_r${r}_$v "$b" "$f" 3; sleep 2; done; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
