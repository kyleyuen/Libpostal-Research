#!/bin/bash
# 046 screen: sequential, alternating l3 / ep on golden-v1 (v1-all.tsv), 20 passes, 4 rounds, AC-gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/046-early-prefetch"; B="$E/raw/screen"; mkdir -p "$B"; D="$W/profiling/golden-v2-novel/v1-all.tsv"
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
for r in 1 2 3 4; do for v in l3 ep; do [ $v = l3 ] && bin="$X/021-final-stack/artifacts/l3/bench_ext" || bin="$E/artifacts/ep/bench_ext"
  [ $((r % 2)) = 0 ] && [ $v = l3 ] && { gate; "$E/artifacts/ep/bench_ext" "$D" 20 > "$B/ep_r$r.stdout" 2>&1; sleep 2; gate; "$X/021-final-stack/artifacts/l3/bench_ext" "$D" 20 > "$B/l3_r$r.stdout" 2>&1; sleep 2; break; }
  gate; "$bin" "$D" 20 > "$B/${v}_r$r.stdout" 2>&1; sleep 2; done; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
