#!/bin/bash
# 5 trials x 3 splits, rotated order, each under /usr/bin/time -l. bash 3.2 compatible.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
B="$W/research/experiments/000-baseline/artifacts/parser_profile"
OUT="$W/research/experiments/002-full-baseline/raw/throughput"
mkdir -p "$OUT"
S0=development; S1=validation; S2=all
pmset -g > "$OUT/pmset-g.txt"; pmset -g batt > "$OUT/pmset-batt.txt"; pmset -g therm > "$OUT/pmset-therm.txt" 2>&1
for trial in 1 2 3 4 5; do
  for k in 0 1 2; do
    idx=$(( (k + trial - 1) % 3 ))
    eval "split=\$S$idx"
    tag="trial${trial}-${split}"
    date -Iseconds > "$OUT/$tag.start"
    /usr/bin/time -l "$B" "$W/profiling/golden-v1/$split.txt" 10 > "$OUT/$tag.stdout" 2> "$OUT/$tag.stderr"
    echo "exit=$?" >> "$OUT/$tag.stderr"
    date -Iseconds > "$OUT/$tag.end"
    sleep 5
  done
done
echo done > "$OUT/DONE"
