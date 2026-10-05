#!/bin/bash
# 114 screen: sequential, rotating psf / rt8 / rt4 on golden-v1 and OSM, 20 passes, 6 rounds. Logs uptime and the top 3 CPU processes before each run (raw/screen/load.log).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/114-trans-rows-prefetch"; B="$E/raw/screen"; mkdir -p "$B"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(psf="$X/101-prefix-suffix-flags/artifacts/psf/bench_ext" rt8="$E/artifacts/rt8/bench_ext" rt4="$E/artifacts/rt4/bench_ext"); n=${#V[@]}
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for d in v1:"$W/profiling/golden-v2-novel/v1-all.tsv" osm:"$W/profiling/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for r in 1 2 3 4 5 6; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate
    { echo "== ${ds}_${name}_r$r $(date '+%H:%M:%S') $(uptime | sed 's/.*load averages: //')"; ps -Ao pcpu,comm -r | sed -n 2,4p | cut -c1-70; } >> "$B/load.log"
    "$bin" "$f" 20 > "$B/${ds}_${name}_r$r.stdout" 2>&1; sleep 2; done; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
