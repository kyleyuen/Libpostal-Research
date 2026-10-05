#!/bin/bash
# usage: run_bench.sh split rounds variant... ; variants: baseline c003 L4 L8 L16. Round 0 = burn-in (recorded, excluded). Order rotated per round. 5 s gaps. bash 3.2 safe.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E6="$X/011-normalize-fastpaths"; E5="$X/005-exact-work-bundle"
split=$1; rounds=$2; shift 2; VARS=("$@"); nv=${#VARS[@]}
OUT="$E6/raw/bench-${split}${TAG:+-$TAG}"; mkdir -p "$OUT"
binof() { case $1 in baseline) echo "$X/000-baseline/artifacts/parser_profile";; c003) echo "$X/003-batched-lookups/artifacts/candidate/parser_profile";; a5) echo "$E5/artifacts/all/parser_profile";; *) echo "$E6/artifacts/$1/parser_profile";; esac; }
pmset -g batt > "$OUT/pmset-batt.txt"; sysctl vm.swapusage > "$OUT/swap-before.txt"; df -k /System/Volumes/Data | tail -1 > "$OUT/df-before.txt"
r=0
while [ $r -le $rounds ]; do
  k=0
  while [ $k -lt $nv ]; do
    name=${VARS[$(( (k + r) % nv ))]}; tag="round${r}-${name}"
    free=$(df -k /System/Volumes/Data | tail -1 | awk '{print $4}'); [ "$free" -lt 524288 ] && { echo "abort low disk" > "$OUT/ABORT"; exit 1; }
    date -Iseconds > "$OUT/$tag.start"
    /usr/bin/time -l "$(binof $name)" "$W/profiling/golden-v1/$split.txt" 10 > "$OUT/$tag.stdout" 2> "$OUT/$tag.stderr"
    echo "exit=$?" >> "$OUT/$tag.stderr"; date -Iseconds > "$OUT/$tag.end"; sleep 5
    k=$((k+1))
  done
  r=$((r+1))
done
sysctl vm.swapusage > "$OUT/swap-after.txt"; echo "${VARS[*]}" > "$OUT/VARIANTS"; echo done > "$OUT/DONE"
