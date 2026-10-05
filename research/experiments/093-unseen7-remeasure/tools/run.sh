#!/bin/bash
# 093 (user rule: re-measure each faster candidate on data the code has not seen). golden-v7-osm-unseen = OSM addresses from 52 cities absent from v3-v6,
# strings in v1/novel/v3-v6 removed; never used for tuning, screening or selection before this run.
# Steps: build the set; original-library dump vs pcp, vmo, vmx; sequential timing (5 rotating rounds, 20 passes) of
# original / pcp / vmo / vmx; AC-gated. Set built before this script (build.stdout in the dataset dir).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/093-unseen7-remeasure"; B="$E/raw"; G="$W/profiling/golden-v7-osm-unseen"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
shasum -a 256 "$G/unseen7.tsv" "$G/unseen7.txt" > "$B/input_hashes.txt"
cap dump_orig "$X/017-loadtime-precompute/artifacts/dump_outputs_baseline" "$G/unseen7.txt"
: > "$B/output_check.txt"
for v in pcp:"$X/077-pc-prefetch/artifacts/pcp" vmo:"$X/091-viterbi-maxonly/artifacts/vmo" vmx:"$X/091-viterbi-maxonly/artifacts/vmx"; do n=${v%%:*}; d=${v#*:}
  cap dump_$n "$d/dump_outputs" "$G/unseen7.txt"
  cmp -s "$B/dump_$n.stdout" "$B/dump_orig.stdout" && echo "$n IDENTICAL to original ($(wc -l < "$B/dump_orig.stdout") addresses)" >> "$B/output_check.txt" || echo "$n DIFFERENT" >> "$B/output_check.txt"; done
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" pcp="$X/077-pc-prefetch/artifacts/pcp/bench_ext" vmo="$X/091-viterbi-maxonly/artifacts/vmo/bench_ext" vmx="$X/091-viterbi-maxonly/artifacts/vmx/bench_ext"); n=${#V[@]}
for r in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/u_r${r}_$name /usr/bin/time -l "$bin" "$G/unseen7.tsv" 20; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
