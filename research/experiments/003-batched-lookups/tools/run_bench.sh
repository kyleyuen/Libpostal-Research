#!/bin/bash
# usage: run_bench.sh split rounds   -- round 0 is a burn-in (recorded, excluded from statistics); order rotated per round; 5 s gaps.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E3="$X/003-batched-lookups"
split=$1; rounds=$2; OUT="$E3/raw/bench-$split"; mkdir -p "$OUT"
BIN0="$X/000-baseline/artifacts/parser_profile"; BIN1="$E3/artifacts/control/parser_profile"; BIN2="$E3/artifacts/candidate/parser_profile"
N0=baseline; N1=control; N2=candidate
pmset -g batt > "$OUT/pmset-batt.txt"; sysctl vm.swapusage > "$OUT/swap-before.txt"; df -k /System/Volumes/Data | tail -1 > "$OUT/df-before.txt"
r=0
while [ $r -le $rounds ]; do
  for k in 0 1 2; do
    idx=$(( (k + r) % 3 )); eval "bin=\$BIN$idx; name=\$N$idx"; tag="round${r}-${name}"
    date -Iseconds > "$OUT/$tag.start"
    /usr/bin/time -l "$bin" "$W/profiling/golden-v1/$split.txt" 10 > "$OUT/$tag.stdout" 2> "$OUT/$tag.stderr"
    echo "exit=$?" >> "$OUT/$tag.stderr"; date -Iseconds > "$OUT/$tag.end"; sleep 5
  done
  r=$((r+1))
done
sysctl vm.swapusage > "$OUT/swap-after.txt"; echo done > "$OUT/DONE"
