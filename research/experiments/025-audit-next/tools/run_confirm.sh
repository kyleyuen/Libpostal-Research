#!/bin/bash
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/025-audit-next"
T="$W/research/tools"
B="$E/raw/confirm_timing"
mkdir -p "$B"
date -Iseconds > "$B/start.txt"
"$T/env_gate.sh" > "$B/gate_start.txt"
"$T/env_snapshot.sh" "$B/env.txt" start
caffeinate -i -w $$ &
for ds in v1-all novel; do
    if [ "$ds" = v1-all ]; then warm=novel; else warm=v1-all; fi
    for trial in 1 2 3; do
        if (( trial % 2 )); then order=(fixes borrow); else order=(borrow fixes); fi
        for variant in "${order[@]}"; do
            "$T/env_gate.sh" > "$B/${ds}_t${trial}_${variant}.gate" || { date -Iseconds > "$B/ABORT"; exit 1; }
            "$T/env_snapshot.sh" "$B/${ds}_t${trial}_${variant}.env" pre
            "$E/tools/capture.sh" "confirm_${ds}_t${trial}_${variant}" /usr/bin/time -l "$E/artifacts/$variant/bench_ext" "$W/profiling/golden-v2-novel/$ds.tsv" 120 "$W/profiling/golden-v2-novel/$warm.tsv"
            "$T/env_snapshot.sh" "$B/${ds}_t${trial}_${variant}.env" post
            sleep 3
        done
    done
done
"$T/env_snapshot.sh" "$B/env.txt" end
date -Iseconds > "$B/DONE"
