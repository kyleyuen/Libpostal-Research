#!/bin/bash
# Non-timing verification: steady-state footprint (footprint -p), thread count (ps -M) and CPU-vs-wall of the RUNNING binary. usage: run_verify.sh name path/to/parser_profile
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; O="$W/research/experiments/006-feature-dict/raw/verify-$1"; mkdir -p "$O"
/usr/bin/time -l "$2" "$W/profiling/golden-v1/all.txt" 25 > "$O/run.stdout" 2> "$O/run.stderr" &
WRAP=$!
sleep 1; PID=$(pgrep -P $WRAP -n)   # the benchmark itself, not the /usr/bin/time wrapper
until grep -q READY "$O/run.stderr" 2>/dev/null; do
  sleep 1; echo "== load-phase $(date +%s)" >> "$O/footprint-load.txt"; footprint -p $PID 2>/dev/null | grep -E "Footprint:" >> "$O/footprint-load.txt"
done
R0=$(date +%s)
for at in 1 3 6 10 15 20; do
  while [ $(( $(date +%s)-R0 )) -lt $at ]; do sleep 0.3; done
  echo "== after_READY=${at}s" >> "$O/footprint.txt"; footprint -p $PID 2>/dev/null | grep -E "Footprint:|MALLOC_LARGE|MALLOC_SMALL|TOTAL" >> "$O/footprint.txt"
  echo "== after_READY=${at}s" >> "$O/threads.txt"; ps -M $PID >> "$O/threads.txt" 2>&1; echo "thread_rows_after_header=$(( $(ps -M $PID | wc -l) - 1 ))" >> "$O/threads.txt"
done
wait $WRAP; echo "exit=$?" >> "$O/run.stderr"
