#!/bin/bash
# 035 level 2 (okn: replay keys, lookups still paid): sequential timing (034 rule) of original, l3n (l3 sources, no PGO) and ocn (l3n + EXP_ORACLE replay; CEILING PROBE ONLY) on golden-v1, novel
# and OSM; 5 rotating rounds, 20 passes (pass 1 records, passes 2-20 replay for ocn), AC gate before each run.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/035-ceiling-oracle"; B="$E/raw"; T=timing2; P="$W/profiling"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/timing2_start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" l3n="$E/artifacts/l3n/bench_ext" okn="$E/artifacts/okn/bench_ext"); n=${#V[@]}
for r in 1 2 3 4 5; do for d in v1:"$P/golden-v2-novel/v1-all.tsv" nv:"$P/golden-v2-novel/novel.tsv" osm:"$P/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap $T/${ds}_r${r}_$name /usr/bin/time -l "$bin" "$f" 20; sleep 2; done; done; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE2"
