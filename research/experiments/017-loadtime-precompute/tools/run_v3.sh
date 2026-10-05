#!/bin/bash
# Third, independent dataset (profiling/golden-v3-osm, OpenStreetMap via Overpass): original vs r0 vs u2, 5 processes each, rotating order, AC-gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/017-loadtime-precompute"; B="$E/raw/v3"; mkdir -p "$B"
D3="$W/profiling/golden-v3-osm/osm.tsv"; WU="$W/profiling/golden-v2-novel/novel.tsv"
V=("baseline|$E/../015-integer-keys/artifacts/bench_ext_baseline" "r0|$E/artifacts/r0/bench_ext" "u2|$E/artifacts/u2/bench_ext"); n=${#V[@]}
date -Iseconds > "$B/start.txt"; shasum -a 256 "$D3" > "$B/input_hash.txt"
for t in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do IFS='|' read -r name bin <<< "${V[$(( (k + t) % n ))]}"
  until "$R/tools/env_gate.sh" > /dev/null 2>&1; do echo "$(date -Iseconds) waiting" >> "$B/waits.log"; sleep 30; done
  "$R/tools/env_snapshot.sh" "$B/v3-t$t-$name.env" pre
  /usr/bin/time -l "$bin" "$D3" 19 "$WU" > "$B/v3-t$t-$name.stdout" 2> "$B/v3-t$t-$name.stderr"
  "$R/tools/env_snapshot.sh" "$B/v3-t$t-$name.env" post; sleep 4; done; done
python3 "$E/../016-batch/tools/analyze_runs.py" "$B" v3 baseline r0 > "$B/summary.md" 2>&1
date -Iseconds > "$B/DONE"
