#!/usr/bin/env bash
# Untimed runs for thread count and late footprint; kept separate from paired timing.
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/027-root-transition"
B="$E/raw/thread_memory_r1"
C="$E/tools/capture.sh"
mkdir -p "$B"
"$W/research/tools/env_gate.sh" > "$B/gate.txt" || exit 1
for variant in control_r2 cache_r2; do
    bin="$E/artifacts/$variant/bench_ext"
    "$C" "memory_${variant}" /usr/bin/time -l "$bin" "$W/research/experiments/025-audit-next/data/development.tsv" 180 "$W/profiling/golden-v2-novel/novel.tsv" &
    capture_pid=$!
    bench_pid=""
    for attempt in $(seq 1 15); do
        time_pid=$(pgrep -P "$capture_pid" | head -n 1 || true)
        if [ -n "$time_pid" ]; then bench_pid=$(pgrep -P "$time_pid" | head -n 1 || true); fi
        if [ -n "$bench_pid" ]; then break; fi
        sleep 1
    done
    if [ -z "$bench_pid" ]; then echo "could not identify child for $variant" >&2; wait "$capture_pid"; exit 1; fi
    printf '%s variant=%s capture_pid=%s benchmark_pid=%s\n' "$(date -Iseconds)" "$variant" "$capture_pid" "$bench_pid" >> "$B/pids.txt"
    sleep 13
    "$C" "threads_${variant}" ps -M -p "$bench_pid"
    "$C" "footprint_${variant}" footprint -p "$bench_pid"
    wait "$capture_pid"
done
date -Iseconds > "$B/DONE"
