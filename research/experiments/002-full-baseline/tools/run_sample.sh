#!/bin/bash
# Fresh sampling profile of the preserved baseline binary on golden-v1/all (25 s run, sample 15 s @1 ms after READY).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; B="$W/research/experiments/000-baseline/artifacts/parser_profile"
O="$W/research/experiments/002-full-baseline/raw"
"$B" "$W/profiling/golden-v1/all.txt" 25 > "$O/sample-run.stdout" 2> "$O/sample-run.stderr" &
PID=$!
until grep -q READY "$O/sample-run.stderr" 2>/dev/null; do sleep 0.2; done
date -Iseconds > "$O/sample.start"
sample $PID 15 1 -file "$O/parser.sample.txt" > "$O/sample.log" 2>&1
wait $PID; echo "exit=$?" >> "$O/sample-run.stderr"; date -Iseconds > "$O/sample.end"
