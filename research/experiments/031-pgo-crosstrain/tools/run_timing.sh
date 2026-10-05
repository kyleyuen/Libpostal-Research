#!/bin/bash
# 031 timing rerun (attempt 2): same matrix as run.sh after its builds and output checks; cap() variable made local (attempt 1 bug).
#   tv1 = golden-v1 all, tnv = golden-v2-novel, tosm = golden-v3-osm, nopgo = same build without PGO. Plus the shipped 021 l4 (trained on golden-v1
#   development) and the preserved original. Output check of every build vs the original on all three datasets, then timing of every binary on
#   every dataset (5 rotating rounds, 20 passes, no warm-up file; median of warm passes 2..20), AC-gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/031-pgo-crosstrain"; B="$E/raw"; P="$W/profiling"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
date "+%Y-%m-%dT%H:%M:%S%z" > "$B/timing_start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" timing_before
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" l4="$X/021-final-stack/artifacts/l4/bench_ext" tv1="$E/artifacts/tv1/bench_ext" tnv="$E/artifacts/tnv/bench_ext" tosm="$E/artifacts/tosm/bench_ext" nopgo="$E/artifacts/nopgo/bench_ext")
n=${#V[@]}; mkdir -p "$B/timing"
for r in 1 2 3 4 5; do for d in v1:"$P/golden-v2-novel/v1-all.tsv" nv:"$P/golden-v2-novel/novel.tsv" osm:"$P/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}
    gate; cap timing/${ds}_r${r}_$name /usr/bin/time -l "$bin" "$f" 20; sleep 2; done; done; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
