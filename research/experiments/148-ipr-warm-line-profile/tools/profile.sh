#!/bin/bash
# 148: for each variant, start bench_ext cycling dev300 (many passes), wait past setup, attach xctrace CPU Profiler 10 s, export cpu-profile table.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/148-ipr-warm-line-profile"; B="$E/raw"
for v in cpi:"$X/119-component-phrase-ids/artifacts/cpi/bench_ext" ipr:"$X/122-int-pairs-retest/artifacts/ipr/bench_ext" cpi2:"$X/119-component-phrase-ids/artifacts/cpi/bench_ext"; do
  n=${v%%:*}; b=${v#*:}
  until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done
  echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $n | $(uptime | sed 's/.*load averages*: //') | $(sysctl -n vm.swapusage)" >> "$B/load.log"
  "$b" "$B/dev300.tsv" 60000 > "$B/xc_${n}_bench.stdout" 2>&1 & pid=$!
  until grep -q "^pass=5 " "$B/xc_${n}_bench.stdout" 2>/dev/null; do sleep 1; done
  rm -rf "$B/xc_$n.trace"; xcrun xctrace record --template 'CPU Profiler' --attach $pid --time-limit 10s --output "$B/xc_$n.trace" > "$B/xc_${n}_record.log" 2>&1
  kill $pid; wait $pid 2>/dev/null
  xcrun xctrace export --input "$B/xc_$n.trace" --xpath '/trace-toc/run[@number="1"]/data/table[@schema="cpu-profile"]' > "$B/xc_${n}_cpu.xml" 2> "$B/xc_${n}_export.err"
  echo "$(date '+%Y-%m-%dT%H:%M:%S%z') done $n" >> "$B/load.log"; sleep 3
done
