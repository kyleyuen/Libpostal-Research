#!/bin/bash
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/025-audit-next"
T="$W/research/tools"
B="$E/raw/dev_timing"
mkdir -p "$B"
date -Iseconds > "$B/start.txt"
"$T/env_gate.sh" > "$B/gate_start.txt"
"$T/env_snapshot.sh" "$B/env.txt" start
caffeinate -i -w $$ &
for trial in 1 2 3 4 5; do
    if (( trial % 2 )); then order=(fixes borrow); else order=(borrow fixes); fi
    for variant in "${order[@]}"; do
        "$T/env_gate.sh" > "$B/t${trial}_${variant}.gate" || { date -Iseconds > "$B/ABORT"; exit 1; }
        "$T/env_snapshot.sh" "$B/t${trial}_${variant}.env" pre
        top -l 30 -s 1 -n 0 2>/dev/null | grep 'CPU usage' > "$B/t${trial}_${variant}.cpu" &
        sampler=$!
        "$E/tools/capture.sh" "dev_t${trial}_${variant}" /usr/bin/time -l "$E/artifacts/$variant/bench_ext" "$E/data/development.tsv" 120 "$W/profiling/golden-v2-novel/novel.tsv"
        kill "$sampler" 2>/dev/null || true
        wait "$sampler" 2>/dev/null || true
        "$T/env_snapshot.sh" "$B/t${trial}_${variant}.env" post
        sleep 3
    done
done
"$T/env_snapshot.sh" "$B/env.txt" end
date -Iseconds > "$B/DONE"
