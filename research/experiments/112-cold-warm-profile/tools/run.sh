#!/bin/bash
# 112: psf bench_ext, warm (300 development addresses cycled) and cold (all 12,863 golden-v1 addresses cycled); `sample <pid> 20 1` after 20 s; two runs each, alternating.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/112-cold-warm-profile"; B="$X/101-prefix-suffix-flags/artifacts/psf/bench_ext"
for r in 1 2; do for m in warm:"$E/raw/dev300.tsv":100000 cold:"$W/profiling/golden-v2-novel/v1-all.tsv":2000; do n=${m%%:*}; rest=${m#*:}; f=${rest%:*}; p=${rest##*:}
  until "$W/research/tools/env_gate.sh" >> "$E/raw/gate.log" 2>&1; do sleep 30; done
  "$B" "$f" $p > "$E/raw/bench_${n}_r$r.stdout" 2> "$E/raw/bench_${n}_r$r.stderr" & pid=$!
  sleep 20; date '+%Y-%m-%dT%H:%M:%S%z' > "$E/raw/sample_${n}_r$r.start"; sample $pid 20 1 -file "$E/raw/sample_${n}_r$r.txt" > /dev/null 2>&1; date '+%Y-%m-%dT%H:%M:%S%z' > "$E/raw/sample_${n}_r$r.end"
  sleep 1; kill $pid; wait $pid 2>/dev/null; done; done
