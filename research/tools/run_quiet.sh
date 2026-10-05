#!/bin/bash
# Interference-aware protocol v3. usage: run_quiet.sh OUTDIR trials passes name=bin ...
# Each process: setup, one untimed pass on the OTHER dataset, then `passes` timed passes on the target. A background `top` sampler records system CPU idle
# during each run. Variant order rotates per trial. Analysis uses per-pass robust statistics (see analyze_quiet.py).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; OUT=$1; trials=$2; passes=$3; shift 3; ARGS=("$@"); nv=${#ARGS[@]}; mkdir -p "$OUT"
sysctl vm.swapusage > "$OUT/swap-before.txt"
for ds in v1-all novel; do
  t=1
  while [ $t -le $trials ]; do
    k=0
    while [ $k -lt $nv ]; do
      e=${ARGS[$(( (k + t) % nv ))]}; name=${e%%=*}; bin=${e#*=}; tag="$ds-t$t-$name"
      free=$(df -k /System/Volumes/Data | tail -1 | awk '{print $4}'); [ "$free" -lt 524288 ] && { echo low disk > "$OUT/ABORT"; exit 1; }
      if [ "$ds" = "novel" ]; then wu=v1-all; else wu=novel; fi
      date -Iseconds > "$OUT/$tag.start"
      top -l 40 -s 1 -n 0 2>/dev/null | grep "CPU usage" > "$OUT/$tag.cpu" &
      TOP=$!
      "$bin" "$W/profiling/golden-v2-novel/$ds.tsv" $passes "$W/profiling/golden-v2-novel/$wu.tsv" > "$OUT/$tag.stdout" 2> "$OUT/$tag.stderr"; echo "exit=$?" >> "$OUT/$tag.stderr"
      kill $TOP 2>/dev/null; wait $TOP 2>/dev/null
      date -Iseconds > "$OUT/$tag.end"; sleep 3; k=$((k+1))
    done
    t=$((t+1))
  done
done
sysctl vm.swapusage > "$OUT/swap-after.txt"; echo "${ARGS[*]}" > "$OUT/VARIANTS"; echo done > "$OUT/DONE"
