#!/bin/bash
# 141: warm bound + stage split of cpi. Serial only. Per round and dataset, rotate: original bench_ext, cpi bench_ext, cpi bench_warm, original bench_warm (20 passes);
# then cpi stage-timer build (119 css bench_split, 3 passes) 4 runs per dataset. Gate + load line before every run. bash 3.2.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/141-warm-bound-cpi"; B="$E/raw"; P="$W/profiling"
mkdir -p "$B/timing" "$B/split"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
load() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $1 | $(uptime | sed 's/.*load averages*: //') | $(top -l 1 -o cpu -n 3 -stats command,cpu | tail -3 | tr -s ' ' | tr '\n' ';')" >> "$B/load.log"; }
cap() { local cn=$1; shift; load "$cn"; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/timing_start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
V=(orig_ext="$X/015-integer-keys/artifacts/bench_ext_baseline" cpi_ext="$X/119-component-phrase-ids/artifacts/cpi/bench_ext" cpi_warm="$E/tools/bench_warm_cpi" orig_warm="$E/tools/bench_warm_orig"); n=${#V[@]}
D=(dev:"$X/025-audit-next/data/development.tsv" osm:"$P/golden-v3-osm/osm.tsv" u11:"$P/golden-v11-osm-unseen/unseen11.tsv")
for r in 1 2 3; do for d in "${D[@]}"; do ds=${d%%:*}; f=${d#*:}
  for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/${ds}_r${r}_$name /usr/bin/time -l "$bin" "$f" 20; sleep 2; done; done; done
for r in 1 2 3 4; do for d in "${D[@]}"; do ds=${d%%:*}; f=${d#*:}
  gate; cap split/${ds}_r$r "$X/119-component-phrase-ids/artifacts/css/bench_split" "$f" 3; sleep 2; done; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
