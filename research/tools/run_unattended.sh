#!/bin/bash
# Controlled, interference-aware benchmark session. usage: run_controlled.sh OUTDIR trials passes name=bin ...
# Hard gate: AC power, no Low Power Mode, no thermal warning (env_gate.sh). Per run: environment snapshot before and after, `top` CPU sampler during, /usr/bin/time -l.
# Variant order rotates per trial and dataset. The whole session runs under caffeinate so the machine cannot idle-sleep.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; T="$W/research/tools"; OUT=$1; trials=$2; passes=$3; shift 3; ARGS=("$@"); nv=${#ARGS[@]}; mkdir -p "$OUT"
wait_ok() {   # unattended: wait up to 2 h for AC/thermal gate and >= 512 MB free disk; log every wait
  n=0
  while true; do
    g=$("$T/env_gate.sh" 2>&1); free=$(df -k /System/Volumes/Data | tail -1 | awk '{print $4}')
    if echo "$g" | grep -q "GATE OK" && [ "$free" -ge 524288 ]; then echo "$g" > "$1"; return 0; fi
    echo "$(date -Iseconds) waiting: $g | free_kb=$free" >> "$OUT/waits.log"; n=$((n+1)); [ $n -ge ${MAX_WAITS:-240} ] && { echo "gave up: $g" > "$OUT/ABORT"; exit 1; }; sleep 30
  done
}
wait_ok "$OUT/gate-start.txt"
system_profiler SPPowerDataType 2>/dev/null | grep -E "Cycle Count|Condition|Maximum Capacity|State of Charge|Charging|Connected" > "$OUT/battery-health.txt"
"$T/env_snapshot.sh" "$OUT/env-session-start.txt" session-start
caffeinate -i -w $$ &
for ds in v1-all novel; do
  t=1
  while [ $t -le $trials ]; do
    k=0
    while [ $k -lt $nv ]; do
      e=${ARGS[$(( (k + t) % nv ))]}; name=${e%%=*}; bin=${e#*=}; tag="$ds-t$t-$name"
      wait_ok "$OUT/$tag.gate"
      if [ "$ds" = "novel" ]; then wu=v1-all; else wu=novel; fi
      "$T/env_snapshot.sh" "$OUT/$tag.env" pre
      date -Iseconds > "$OUT/$tag.start"
      top -l 60 -s 1 -n 0 2>/dev/null | grep "CPU usage" > "$OUT/$tag.cpu" &
      TOP=$!
      /usr/bin/time -l "$bin" "$W/profiling/golden-v2-novel/$ds.tsv" $passes "$W/profiling/golden-v2-novel/$wu.tsv" > "$OUT/$tag.stdout" 2> "$OUT/$tag.stderr"; echo "exit=$?" >> "$OUT/$tag.stderr"
      kill $TOP 2>/dev/null; wait $TOP 2>/dev/null
      date -Iseconds > "$OUT/$tag.end"
      "$T/env_snapshot.sh" "$OUT/$tag.env" post
      sleep 8; k=$((k+1))
    done
    t=$((t+1))
  done
done
"$T/env_snapshot.sh" "$OUT/env-session-end.txt" session-end
echo "${ARGS[*]}" > "$OUT/VARIANTS"; echo done > "$OUT/DONE"
