#!/bin/bash
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/025-audit-next"
"$W/research/tools/env_gate.sh"
"$E/artifacts/fixes/bench_ext" "$E/data/development.tsv" 120 "$W/profiling/golden-v2-novel/novel.tsv" >/dev/null &
pid=$!
for sample in 1 2 3 4 5 6; do
    sleep 3
    if ! kill -0 "$pid" 2>/dev/null; then break; fi
    printf 'sample=%s time=%s pid=%s\n' "$sample" "$(date -Iseconds)" "$pid"
    ps -M -p "$pid"
done
wait "$pid"
