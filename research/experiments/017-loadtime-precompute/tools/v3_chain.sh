#!/bin/bash
# Wait for the OSM fetch (PID given), build golden-v3-osm, check u2 vs the original on it, then time it (run_v3.sh waits for AC by itself).
PID=$1; W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; G="$W/profiling/golden-v3-osm"; E="$W/research/experiments/017-loadtime-precompute"; O="$E/raw/v3check"; mkdir -p "$O"
while kill -0 "$PID" 2>/dev/null; do sleep 10; done
cd "$G" && python3 build_osm.py build > build.log 2>&1 && shasum -a 256 build_osm.py osm.tsv osm.txt raw/*.json > SHA256SUMS
"$E/artifacts/dump_outputs_baseline" "$G/osm.txt" > "$O/outputs_baseline.jsonl" 2> "$O/baseline.stderr"; echo "baseline exit=$?" > "$O/summary.txt"
"$E/artifacts/u2/dump_outputs" "$G/osm.txt" > "$O/outputs_u2.jsonl" 2> "$O/u2.stderr"; echo "u2 exit=$?" >> "$O/summary.txt"
cmp -s "$O/outputs_baseline.jsonl" "$O/outputs_u2.jsonl" && echo "u2 vs original: IDENTICAL ($(wc -l < "$O/outputs_u2.jsonl") addresses)" >> "$O/summary.txt" || echo "u2 vs original: DIFFERENT" >> "$O/summary.txt"
grep -q IDENTICAL "$O/summary.txt" && "$E/tools/run_v3.sh"
