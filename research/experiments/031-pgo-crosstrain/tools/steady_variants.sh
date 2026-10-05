#!/bin/bash
# 031 follow-up: same-session steady footprint (footprint -p at pass 3 of a 1000-pass golden-v1 run, no warm-up) for the original and the
# preserved 021 builds s3, m3, l3, l4 (differ in dictionary load factor / clearing; see 021 README).
P="/Users/kyle/Desktop/HW/CSCI 201 Reserach/profiling"; X="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments"; S="$X/031-pgo-crosstrain/raw/steady"
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" s3="$X/021-final-stack/artifacts/s3/bench_ext" m3="$X/021-final-stack/artifacts/m3/bench_ext" l3="$X/021-final-stack/artifacts/l3/bench_ext" l4="$X/021-final-stack/artifacts/l4/bench_ext" orig2="$X/015-integer-keys/artifacts/bench_ext_baseline")
for v in "${V[@]}"; do
  n=${v%%=*}; b=${v#*=}; "$b" "$P/golden-v2-novel/v1-all.tsv" 1000 > "$S/var_$n.stdout" 2>&1 & pid=$!; t=0
  until grep -q "^pass=3 " "$S/var_$n.stdout" 2>/dev/null; do sleep 1; t=$((t+1)); [ $t -gt 120 ] && break; done
  echo "$n $(date -Iseconds) $(footprint -p $pid 2>&1 | grep -o 'Footprint: [0-9]* MB') $(grep -o 'setup_seconds=[0-9.]*' "$S/var_$n.stdout")" >> "$S/variants.txt"
  kill $pid; wait $pid 2>/dev/null
done
