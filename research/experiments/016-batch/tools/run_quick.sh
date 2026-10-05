#!/bin/bash
# 016 quick AC session (~15 min): headline variants only. A: run_unattended.sh 3 trials x 19 passes on golden-v1 + novel; B: floor subset, 3 processes each.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/016-batch"; A="$E/artifacts"; Q="$E/raw/quick"; mkdir -p "$Q/floor"
BASE="$E/../015-integer-keys/artifacts/bench_ext_baseline"; SUB="$R/experiments/013-random-and-floor/data"; WU="$W/profiling/golden-v2-novel/novel.tsv"
date -Iseconds > "$Q/start.txt"
"$R/tools/run_unattended.sh" "$Q/A" 3 19 baseline="$BASE" k0="$A/k0/bench_ext" all="$A/all/bench_ext" all7="$A/all7/bench_ext" pgolto="$A/pgolto/bench_ext"
python3 "$R/tools/analyze_quiet.py" "$Q/A" baseline > "$Q/A-summary.md" 2>&1
FL=("baseline|$BASE" "k0|$A/k0/bench_ext" "all|$A/all/bench_ext" "all7|$A/all7/bench_ext" "pgolto|$A/pgolto/bench_ext"); n=${#FL[@]}
for t in 1 2 3; do for k in $(seq 0 $((n-1))); do IFS='|' read -r name bin <<< "${FL[$(( (k + t) % n ))]}"
  until "$R/tools/env_gate.sh" > /dev/null 2>&1; do echo "$(date -Iseconds) waiting for AC/gate" >> "$Q/waits.log"; sleep 30; done
  /usr/bin/time -l "$bin" "$SUB/floor_non_ascii_only.tsv" 15 "$WU" > "$Q/floor/floor-t$t-$name.stdout" 2> "$Q/floor/floor-t$t-$name.stderr"; sleep 3; done; done
python3 "$E/tools/analyze_runs.py" "$Q/floor" floor baseline k0 > "$Q/B-floor-summary.md" 2>&1
date -Iseconds > "$Q/DONE"
