#!/bin/bash
# 140 screen2 (diagnostic): cpi / i4m4 (4 in flight, yield 2 only) / i2m7 (2 in flight, yields 0-2).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/140-interleave-runtime"; B="$E/raw/screen2"; mkdir -p "$B"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(cpi="$X/119-component-phrase-ids/artifacts/cpi/bench_ext" i4m4="$E/artifacts/run_i4m4.sh" i2m7="$E/artifacts/run_i2m7.sh"); n=${#V[@]}
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for d in v1:"$W/profiling/golden-v2-novel/v1-all.tsv" osm:"$W/profiling/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for r in 1 2 3 4 5 6; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate
    { echo "== ${ds}_${name}_r$r $(date '+%H:%M:%S') $(uptime | sed 's/.*load averages: //')"; ps -Ao pcpu,comm -r | sed -n 2,4p | cut -c1-70; } >> "$B/load.log"
    "$bin" "$f" 20 > "$B/${ds}_${name}_r$r.stdout" 2>&1; sleep 2; done; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
