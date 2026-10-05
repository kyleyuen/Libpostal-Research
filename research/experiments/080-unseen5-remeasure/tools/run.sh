#!/bin/bash
# 080 (user rule: re-measure each faster candidate on data the code has not seen). golden-v5-osm-unseen = OSM addresses from 49 cities absent from v3 and v4,
# strings in v1/novel/v3/v4 removed; never used for tuning, screening or selection before this run.
# Steps: build the set; original-library dump vs lf, pf2, pcp, ptp; sequential timing (5 rotating rounds, 20 passes) of
# original / lf / pf2 / pcp / ptp; AC-gated. Set built before this script (build.stdout in the dataset dir).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/080-unseen5-remeasure"; B="$E/raw"; G="$W/profiling/golden-v5-osm-unseen"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
shasum -a 256 "$G/unseen5.tsv" "$G/unseen5.txt" > "$B/input_hashes.txt"
cap dump_orig "$X/017-loadtime-precompute/artifacts/dump_outputs_baseline" "$G/unseen5.txt"
: > "$B/output_check.txt"
for v in lf:"$X/062-letters-fastpath/artifacts/lf" pf2:"$X/074-ftt-prefetch/artifacts/pf2" pcp:"$X/077-pc-prefetch/artifacts/pcp" ptp:"$X/079-types-prefetch/artifacts/ptp"; do n=${v%%:*}; d=${v#*:}
  cap dump_$n "$d/dump_outputs" "$G/unseen5.txt"
  cmp -s "$B/dump_$n.stdout" "$B/dump_orig.stdout" && echo "$n IDENTICAL to original ($(wc -l < "$B/dump_orig.stdout") addresses)" >> "$B/output_check.txt" || echo "$n DIFFERENT" >> "$B/output_check.txt"; done
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" lf="$X/062-letters-fastpath/artifacts/lf/bench_ext" pf2="$X/074-ftt-prefetch/artifacts/pf2/bench_ext" pcp="$X/077-pc-prefetch/artifacts/pcp/bench_ext" ptp="$X/079-types-prefetch/artifacts/ptp/bench_ext"); n=${#V[@]}
for r in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/u_r${r}_$name /usr/bin/time -l "$bin" "$G/unseen5.tsv" 20; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
