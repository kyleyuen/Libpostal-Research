#!/bin/bash
# 069 (user request 2026-10-03): re-measure on a dataset the code has never seen. golden-v4-osm-unseen = OpenStreetMap addresses from 58 cities absent from
# every earlier set, duplicates of golden-v1 / novel / golden-v3 removed; never used for tuning, screening or selection before this run.
# Steps: build the set; original-library dump (017's dump_outputs_baseline) vs va, lf, pl dumps; sequential timing (5 rotating rounds, 20 passes) of
# original / va / lf / pl; AC-gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/069-unseen-remeasure"; B="$E/raw"; G="$W/profiling/golden-v4-osm-unseen"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
cap build_set python3 "$G/build_unseen.py" build
( cd "$G" && shasum -a 256 unseen.tsv unseen.txt build_unseen.py > SHA256SUMS )
shasum -a 256 "$G/unseen.tsv" "$G/unseen.txt" > "$B/input_hashes.txt"
cap dump_orig "$X/017-loadtime-precompute/artifacts/dump_outputs_baseline" "$G/unseen.txt"
: > "$B/output_check.txt"
for v in va:"$X/048-viterbi20-alias/artifacts/va" lf:"$X/062-letters-fastpath/artifacts/lf" pl:"$X/067-phrase-memo-len/artifacts/pl"; do n=${v%%:*}; d=${v#*:}
  cap dump_$n "$d/dump_outputs" "$G/unseen.txt"
  cmp -s "$B/dump_$n.stdout" "$B/dump_orig.stdout" && echo "$n IDENTICAL to original ($(wc -l < "$B/dump_orig.stdout") addresses)" >> "$B/output_check.txt" || echo "$n DIFFERENT" >> "$B/output_check.txt"; done
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" va="$X/048-viterbi20-alias/artifacts/va/bench_ext" lf="$X/062-letters-fastpath/artifacts/lf/bench_ext" pl="$X/067-phrase-memo-len/artifacts/pl/bench_ext"); n=${#V[@]}
for r in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/u_r${r}_$name /usr/bin/time -l "$bin" "$G/unseen.tsv" 20; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
