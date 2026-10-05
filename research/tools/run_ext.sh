#!/bin/bash
# usage: run_ext.sh OUTDIR trials name=bin ... ; runs each variant on v1-all.tsv and novel.tsv, 5 passes/process, rotating variant order; 5 s gaps.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; OUT=$1; trials=$2; shift 2; ARGS=("$@"); nv=${#ARGS[@]}; mkdir -p "$OUT"
sysctl vm.swapusage > "$OUT/swap-before.txt"
for ds in v1-all novel; do
  t=1
  while [ $t -le $trials ]; do
    k=0
    while [ $k -lt $nv ]; do
      e=${ARGS[$(( (k + t) % nv ))]}; name=${e%%=*}; bin=${e#*=}; tag="$ds-t$t-$name"
      free=$(df -k /System/Volumes/Data | tail -1 | awk '{print $4}'); [ "$free" -lt 524288 ] && { echo low disk > "$OUT/ABORT"; exit 1; }
      date -Iseconds > "$OUT/$tag.start"
      if [ "$ds" = "novel" ]; then wu=v1-all; else wu=novel; fi
      "$bin" "$W/profiling/golden-v2-novel/$ds.tsv" 5 "$W/profiling/golden-v2-novel/$wu.tsv" > "$OUT/$tag.stdout" 2> "$OUT/$tag.stderr"; echo "exit=$?" >> "$OUT/$tag.stderr"
      date -Iseconds > "$OUT/$tag.end"; sleep 5; k=$((k+1))
    done
    t=$((t+1))
  done
done
sysctl vm.swapusage > "$OUT/swap-after.txt"; echo "${ARGS[*]}" > "$OUT/VARIANTS"; echo done > "$OUT/DONE"
