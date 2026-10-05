#!/bin/bash
# usage: run_sample_bin.sh NAME path/to/parser_profile   -- sampling profile (1 ms, 15 s) of a 25 s run on golden-v1/all after READY. Profiling run, never used for timing.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; O="$W/research/experiments/008-profile-after-006/raw/sample-$1"; mkdir -p "$O"
"$2" "$W/profiling/golden-v1/all.txt" 25 > "$O/run.stdout" 2> "$O/run.stderr" &
PID=$!
until grep -q READY "$O/run.stderr" 2>/dev/null; do sleep 0.2; done
date -Iseconds > "$O/sample.start"
sample $PID 15 1 -file "$O/parser.sample.txt" > "$O/sample.log" 2>&1
wait $PID; echo "exit=$?" >> "$O/run.stderr"; date -Iseconds > "$O/sample.end"
