#!/bin/bash
# 149 development screen + stage split (serial, gated). Waits for 1-min load < 2.5 first (after builds/verification).
# Screen: cpi / r1a / r1 bench_ext rotating, golden-v1 all and OSM, 20 passes, 6 rounds. Split: css / r1as / r1s bench_split, dev and OSM, 3 passes, 4 rounds.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/149-rw-r1-bigram"; B="$E/raw/screen"; S="$E/raw/split"; mkdir -p "$B" "$S"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
snap() { echo "== $1 $(date '+%Y-%m-%dT%H:%M:%S%z') load $(sysctl -n vm.loadavg) swap $(sysctl -n vm.swapusage | awk '{print $6}') free% $(memory_pressure | awk '/free percentage/ {print $5}')" >> "$B/load.log"; ps -Ao pcpu,comm -r | sed -n 2,4p | cut -c1-70 >> "$B/load.log"; }
until [ "$(sysctl -n vm.loadavg | awk '{print ($2 < 2.5)}')" = 1 ]; do echo "$(date '+%H:%M:%S') waiting, load $(sysctl -n vm.loadavg)" >> "$B/settle.log"; sleep 30; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
V=(cpi="$X/119-component-phrase-ids/artifacts/cpi/bench_ext" r1a="$E/artifacts/r1a/bench_ext" r1="$E/artifacts/r1/bench_ext"); n=${#V[@]}
for d in v1:"$W/profiling/golden-v2-novel/v1-all.tsv" osm:"$W/profiling/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for r in 1 2 3 4 5 6; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; snap "${ds}_${name}_r$r"
    "$bin" "$f" 20 > "$B/${ds}_${name}_r$r.stdout" 2>&1; echo $? > "$B/${ds}_${name}_r$r.exit"; sleep 2; done; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
SV=(css="$X/119-component-phrase-ids/artifacts/css/bench_split" r1as="$E/artifacts/r1as/bench_split" r1s="$E/artifacts/r1s/bench_split"); n=${#SV[@]}
for r in 1 2 3 4; do for d in dev:"$X/025-audit-next/data/development.tsv" osm:"$W/profiling/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for k in $(seq 0 $((n-1))); do e=${SV[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; snap "split_${ds}_${name}_r$r"
    "$bin" "$f" 3 > "$S/${ds}_${name}_r$r.stdout" 2>&1; echo $? > "$S/${ds}_${name}_r$r.exit"; sleep 2; done; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$S/DONE"
