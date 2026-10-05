#!/bin/bash
# Non-timing verification of the running CANDIDATE binary: thread count (ps -M) and steady-state footprint after READY.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E3="$W/research/experiments/003-batched-lookups"; O="$E3/raw/verify"; mkdir -p "$O"
"$E3/artifacts/candidate/parser_profile" "$W/profiling/golden-v1/development.txt" 25 > "$O/run.stdout" 2> "$O/run.stderr" &
PID=$!
until grep -q READY "$O/run.stderr" 2>/dev/null; do sleep 0.2; done; R0=$(date +%s)
for at in 1 3 6 10 15 20; do
  while [ $(( $(date +%s)-R0 )) -lt $at ]; do sleep 0.3; done
  echo "== after_READY=${at}s" >> "$O/threads.txt"; ps -M $PID >> "$O/threads.txt" 2>&1
  echo "thread_rows_after_header=$(( $(ps -M $PID | wc -l) - 1 ))" >> "$O/threads.txt"
  echo "== after_READY=${at}s" >> "$O/footprint.txt"; footprint -p $PID >> "$O/footprint.txt" 2>&1
done
wait $PID; echo "exit=$?" >> "$O/run.stderr"
