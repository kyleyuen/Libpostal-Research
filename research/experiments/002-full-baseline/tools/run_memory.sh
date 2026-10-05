#!/bin/bash
# Steady-state memory of the preserved binary, sampled from outside. NOT a timing run (footprint/vmmap pause the target).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; B="$W/research/experiments/000-baseline/artifacts/parser_profile"
O="$W/research/experiments/002-full-baseline/raw/memory"; mkdir -p "$O"
for split in all development; do
  "$B" "$W/profiling/golden-v1/$split.txt" 40 > "$O/$split.stdout" 2> "$O/$split.stderr" &
  PID=$!; T0=$(date +%s); i=0
  # phase 1: model load / warm-up (before READY), every 2 s
  while ! grep -q READY "$O/$split.stderr" 2>/dev/null; do
    echo "== t=$(( $(date +%s)-T0 ))s phase=load" >> "$O/$split.footprint.txt"; footprint -p $PID >> "$O/$split.footprint.txt" 2>&1; sleep 2; done
  R0=$(date +%s)
  # phase 2: steady state after READY
  for at in 1 3 6 10 15 20 25 30; do
    while [ $(( $(date +%s)-R0 )) -lt $at ]; do sleep 0.5; done
    echo "== after_READY=$(( $(date +%s)-R0 ))s phase=steady" >> "$O/$split.footprint.txt"; footprint -p $PID >> "$O/$split.footprint.txt" 2>&1
    if [ $at = 3 ] || [ $at = 30 ]; then echo "== after_READY=${at}s vmmap" >> "$O/$split.vmmap.txt"; vmmap --summary $PID >> "$O/$split.vmmap.txt" 2>&1; fi
  done
  wait $PID; echo "exit=$?" >> "$O/$split.stderr"
done
echo done > "$O/DONE"
