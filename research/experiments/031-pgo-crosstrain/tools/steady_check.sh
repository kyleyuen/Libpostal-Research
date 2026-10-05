#!/bin/bash
# 031 steady-footprint method check: sample footprint -p (text + JSON phys_footprint) at pass 3 of a 1000-pass run, original vs l4, with and
# without a warm-up file (021's timing harness warmed golden-v1 runs with novel).
P="/Users/kyle/Desktop/HW/CSCI 201 Reserach/profiling"; X="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments"; S="$X/031-pgo-crosstrain/raw/steady"
O="$X/015-integer-keys/artifacts/bench_ext_baseline"; L="$X/021-final-stack/artifacts/l4/bench_ext"
one() {  # name binary [warmup]
  local n=$1 b=$2 w=$3
  if [ -n "$w" ]; then "$b" "$P/golden-v2-novel/v1-all.tsv" 1000 "$w" > "$S/$n.stdout" 2>&1 & else "$b" "$P/golden-v2-novel/v1-all.tsv" 1000 > "$S/$n.stdout" 2>&1 & fi
  local pid=$! t=0
  until grep -q "^pass=3 " "$S/$n.stdout" 2>/dev/null; do sleep 1; t=$((t+1)); [ $t -gt 120 ] && { echo "$n TIMEOUT" >> "$S/method_check.txt"; kill $pid; return; }; done
  footprint -p $pid -j "$S/$n.json" > /dev/null 2>&1
  local txt=$(footprint -p $pid 2>&1 | grep -o 'Footprint: [0-9]* MB')
  local js=$(python3 -c "import json;d=json.load(open('$S/$n.json'));print([round(p['phys_footprint']/2**20) for p in d['processes']])" 2>&1)
  echo "$n $(date -Iseconds) $txt json_phys_footprint_MiB=$js" >> "$S/method_check.txt"
  kill $pid; wait $pid 2>/dev/null
}
one orig_warmup "$O" "$P/golden-v2-novel/novel.tsv"; one orig_nowarm "$O"; one l4_warmup "$L" "$P/golden-v2-novel/novel.tsv"; one l4_nowarm "$L"
