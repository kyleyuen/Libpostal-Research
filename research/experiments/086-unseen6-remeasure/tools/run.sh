#!/bin/bash
# 086 (user rule: re-measure each faster candidate on data the code has not seen). golden-v6-osm-unseen = OSM addresses from 54 cities absent from v3, v4 and v5,
# strings in v1/novel/v3/v4/v5 removed; never used for tuning, screening or selection before this run.
# Steps: build the set; original-library dump vs pcp, rl8; sequential timing (5 rotating rounds, 20 passes) of
# original / pcp / rl8; AC-gated. Set built before this script (build.stdout in the dataset dir).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/086-unseen6-remeasure"; B="$E/raw"; G="$W/profiling/golden-v6-osm-unseen"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
shasum -a 256 "$G/unseen6.tsv" "$G/unseen6.txt" > "$B/input_hashes.txt"
cap dump_orig "$X/017-loadtime-precompute/artifacts/dump_outputs_baseline" "$G/unseen6.txt"
: > "$B/output_check.txt"
for v in pcp:"$X/077-pc-prefetch/artifacts/pcp" rl8:"$X/085-row-lines-prefetch/artifacts/rl8"; do n=${v%%:*}; d=${v#*:}
  cap dump_$n "$d/dump_outputs" "$G/unseen6.txt"
  cmp -s "$B/dump_$n.stdout" "$B/dump_orig.stdout" && echo "$n IDENTICAL to original ($(wc -l < "$B/dump_orig.stdout") addresses)" >> "$B/output_check.txt" || echo "$n DIFFERENT" >> "$B/output_check.txt"; done
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" pcp="$X/077-pc-prefetch/artifacts/pcp/bench_ext" rl8="$X/085-row-lines-prefetch/artifacts/rl8/bench_ext"); n=${#V[@]}
for r in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/u_r${r}_$name /usr/bin/time -l "$bin" "$G/unseen6.tsv" 20; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
