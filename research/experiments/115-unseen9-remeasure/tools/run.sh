#!/bin/bash
# 115 (user rule: re-measure each faster candidate on data the code has not seen). golden-v9-osm-unseen = OSM addresses from 55 cities absent from v3-v8,
# strings in v1/novel/v3-v8 removed; never used for tuning, screening or selection before this run.
# Steps: build the set; original-library dump vs psf, rt8; sequential timing (5 rotating rounds, 20 passes) of
# original / psf / rt8 (114); AC-gated. Set built before this script (build.stdout in the dataset dir).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/115-unseen9-remeasure"; B="$E/raw"; G="$W/profiling/golden-v9-osm-unseen"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
shasum -a 256 "$G/unseen9.tsv" "$G/unseen9.txt" > "$B/input_hashes.txt"
cap dump_orig "$X/017-loadtime-precompute/artifacts/dump_outputs_baseline" "$G/unseen9.txt"
: > "$B/output_check.txt"
for v in psf:"$X/101-prefix-suffix-flags/artifacts/psf" rt8:"$X/114-trans-rows-prefetch/artifacts/rt8"; do n=${v%%:*}; d=${v#*:}
  cap dump_$n "$d/dump_outputs" "$G/unseen9.txt"
  cmp -s "$B/dump_$n.stdout" "$B/dump_orig.stdout" && echo "$n IDENTICAL to original ($(wc -l < "$B/dump_orig.stdout") addresses)" >> "$B/output_check.txt" || echo "$n DIFFERENT" >> "$B/output_check.txt"; done
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" psf="$X/101-prefix-suffix-flags/artifacts/psf/bench_ext" rt8="$X/114-trans-rows-prefetch/artifacts/rt8/bench_ext"); n=${#V[@]}
for r in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/u_r${r}_$name /usr/bin/time -l "$bin" "$G/unseen9.tsv" 20; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
