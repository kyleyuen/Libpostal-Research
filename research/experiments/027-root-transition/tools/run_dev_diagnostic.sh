#!/usr/bin/env bash
# Noisy-host exploratory screen, not final validation.
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/027-root-transition"
T="$W/research/tools"
B="$E/raw/dev_diagnostic_r1"
C="$E/tools/capture.sh"
mkdir -p "$B"
date -Iseconds > "$B/start.txt"
"$T/env_gate.sh" > "$B/gate_start.txt" || { cp "$B/gate_start.txt" "$B/ABORT"; exit 1; }
"$T/env_snapshot.sh" "$B/session.env" start
caffeinate -i -w $$ &
caffeinate_pid=$!
sampler=""
cleanup() {
    if [ -n "$sampler" ]; then kill "$sampler" 2>/dev/null || true; wait "$sampler" 2>/dev/null || true; fi
    kill "$caffeinate_pid" 2>/dev/null || true
}
trap cleanup EXIT
for trial in 1 2 3 4 5; do
    if (( trial % 2 )); then order=(control_r2 cache_r2); else order=(cache_r2 control_r2); fi
    for variant in "${order[@]}"; do
        tag="t${trial}_${variant}"
        "$T/env_gate.sh" > "$B/$tag.gate" || { cp "$B/$tag.gate" "$B/ABORT"; exit 1; }
        "$T/env_snapshot.sh" "$B/$tag.env" pre
        top -l 30 -s 1 -n 0 2>/dev/null | grep 'CPU usage' > "$B/$tag.cpu" &
        sampler=$!
        "$C" "diag_$tag" /usr/bin/time -l "$E/artifacts/$variant/bench_ext" "$W/research/experiments/025-audit-next/data/development.tsv" 120 "$W/profiling/golden-v2-novel/novel.tsv"
        kill "$sampler" 2>/dev/null || true
        wait "$sampler" 2>/dev/null || true
        sampler=""
        "$T/env_snapshot.sh" "$B/$tag.env" post
        sleep 3
    done
done
"$T/env_snapshot.sh" "$B/session.env" end
date -Iseconds > "$B/DONE"
