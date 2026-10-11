#!/bin/bash
# 160 TIMING REQUEST (for the lead; teammates do not time). Modelled on 121's run_timing.sh. Run alone, on AC, under env_gate.sh.
# Variants: original, cpi, 160ctl (unchanged-source rebuild), 160b (records, stage B), 160c (records, direct emission).
# Datasets: golden-v1 (v1-all.tsv) and OSM v3. 5 rotating rounds x 20 passes. bench_ext reports setup s, cold pass 1 and warm passes.
# Memory: /usr/bin/time -l on every run (peak footprint AND max RSS) + steady footprint and vmmap at pass 3 (mapped file counted separately).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/160-p2-records-in-cpi"; B="$E/raw/timing"; P="$W/profiling"
mkdir -p "$B/timing" "$B/steady"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/timing_start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
shasum -a 256 "$E/artifacts/records/address_parser_records.dat" > "$B/record_file.sha256"   # expect 3792d23d...0328
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" cpi="$X/119-component-phrase-ids/artifacts/cpi/bench_ext" ctl="$E/artifacts/160ctl/bench_ext"
   b="$E/artifacts/160b/bench_ext" c="$E/artifacts/160c/bench_ext"); n=${#V[@]}
# Page cache: the record file (1.1 GB) and the model are left WARM (one untimed 1-pass run of every variant first), as the model was in 121.
for v in "${V[@]}"; do cap warm_${v%%=*} "${v#*=}" "$P/golden-v3-osm/osm.tsv" 1; done
for r in 1 2 3 4 5; do for d in v1:"$P/golden-v2-novel/v1-all.tsv" osm:"$P/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/${ds}_r${r}_$name /usr/bin/time -l "$bin" "$f" 20; sleep 2; done; done; done
# Steady memory: footprint (excludes clean file-backed pages) AND vmmap --summary (mapped file resident) at pass 3, twice per variant.
for v in "${V[@]}" "${V[@]}"; do sn=${v%%=*}; bin=${v#*=}
  "$bin" "$P/golden-v2-novel/v1-all.tsv" 1000 > "$B/steady/run.stdout" 2>&1 & pid=$!; t=0
  until grep -q "^pass=3 " "$B/steady/run.stdout" 2>/dev/null; do sleep 1; t=$((t+1)); [ $t -gt 120 ] && break; done
  { echo "== $sn $(date -Iseconds) pid=$pid"; footprint -p $pid 2>&1 | grep -E 'Footprint|phys_footprint'; vmmap --summary $pid 2>&1 | grep -E '^mapped file|^TOTAL|Physical footprint'; } >> "$B/steady/steady.txt"
  kill $pid; wait $pid 2>/dev/null; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
