#!/bin/bash
# 032: final same-session validation of the preserved 021 `l3` build (= l4 sources/switches with EXP_LOADF=3; PGO trained on golden-v1 development).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/032-final-l3"; B="$E/raw"; P="$W/profiling"
A="$X/021-final-stack/artifacts/l3"; L4="$X/021-final-stack/artifacts/l4"; ORIG="$X/015-integer-keys/artifacts/bench_ext_baseline"; F="$X/011-normalize-fastpaths/raw/fuzz"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
# 1 binary identity: current l3 files vs the hashes recorded when 021 built them
( cd "$A" && shasum -a 256 libpostal.a dump_outputs bench_ext fuzz_pipeline ) > "$B/l3_hashes_now.txt"
grep -E " (libpostal.a|dump_outputs|bench_ext|fuzz_pipeline)$" "$A/hashes.txt" > "$B/l3_hashes_021.txt"
cmp -s "$B/l3_hashes_now.txt" "$B/l3_hashes_021.txt" && echo "binary identity: SAME as 021 record" > "$B/summary.txt" || echo "binary identity: DIFFERENT" > "$B/summary.txt"
shasum -a 256 "$ORIG" "$L4/bench_ext" "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" "$P/golden-v3-osm/osm.txt" "$P/golden-v2-novel/v1-all.tsv" "$P/golden-v2-novel/novel.tsv" "$P/golden-v3-osm/osm.tsv" > "$B/input_hashes.txt"
# 2 output checks
cap dump_v1 "$A/dump_outputs" "$P/golden-v1/all.txt"; echo "dump v1 exit=$(cat $B/dump_v1.exit) sha=$(shasum -a 256 "$B/dump_v1.stdout" | cut -c1-20) (original 45574ee2b1972f1acf2d)" >> "$B/summary.txt"
cap dump_nv "$A/dump_outputs" "$P/golden-v2-novel/novel.txt"; echo "dump novel exit=$(cat $B/dump_nv.exit) sha=$(shasum -a 256 "$B/dump_nv.stdout" | cut -c1-20) (original 806b8a81603ea9adf3af)" >> "$B/summary.txt"
cap dump_osm "$A/dump_outputs" "$P/golden-v3-osm/osm.txt"; cmp -s "$B/dump_osm.stdout" "$X/017-loadtime-precompute/raw/v3check/outputs_baseline.jsonl" && r=IDENTICAL || r=DIFFERENT; echo "dump osm exit=$(cat $B/dump_osm.exit) vs original OSM dump (017 v3check): $r" >> "$B/summary.txt"
cap frozen_v1 python3 "$X/015-integer-keys/tools/compare_frozen.py" "$B/dump_v1.stdout" "$P/golden-v1/baseline-outputs.jsonl"; echo "frozen v1: $(tail -1 $B/frozen_v1.stdout)" >> "$B/summary.txt"
cap frozen_nv python3 "$X/015-integer-keys/tools/compare_frozen.py" "$B/dump_nv.stdout" "$P/golden-v2-novel/baseline-outputs.jsonl"; echo "frozen novel: $(tail -1 $B/frozen_nv.stdout)" >> "$B/summary.txt"
for s in 1 2; do log "START fuzz_s$s"; "$A/fuzz_pipeline" $s 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" > "$B/fuzz_s$s.txt" 2> "$B/fuzz_s$s.stderr"; e=$?
  cmp -s "$B/fuzz_s$s.txt" "$F/base_s$s.txt" && r=IDENTICAL || r=DIFFER; log "END fuzz_s$s exit=$e $r"; echo "fuzz seed$s exit=$e $r (vs original 011 base_s$s)" >> "$B/summary.txt"; rm -f "$B/fuzz_s$s.txt"; done
# 3 thread check
"$A/bench_ext" "$P/golden-v2-novel/v1-all.tsv" 200 > "$B/threads_bench.stdout" 2>&1 & pid=$!
for t in 1 2 3 4 5 6; do sleep 3; echo "t=$((t*3))s threads=$(ps -M -p $pid | tail -n +2 | wc -l | tr -d ' ')" >> "$B/threads.txt"; done; kill $pid; wait $pid 2>/dev/null
echo "threads: $(cut -d= -f3 $B/threads.txt | sort -u | tr '\n' ' ')" >> "$B/summary.txt"
# 4 timing: original, l3, l4 on golden-v1 / novel / OSM, 5 rotating rounds, 20 passes, AC gate
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(orig="$ORIG" l3="$A/bench_ext" l4="$L4/bench_ext"); n=${#V[@]}
for r in 1 2 3 4 5; do for d in v1:"$P/golden-v2-novel/v1-all.tsv" nv:"$P/golden-v2-novel/novel.tsv" osm:"$P/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}; gate; cap timing/${ds}_r${r}_$name /usr/bin/time -l "$bin" "$f" 20; sleep 2; done; done; done
# 5 steady footprint (footprint -p at pass 3 of a 1000-pass golden-v1 run), alternating original / l3 / l4 twice
for v in orig="$ORIG" l3="$A/bench_ext" l4="$L4/bench_ext" orig="$ORIG" l3="$A/bench_ext" l4="$L4/bench_ext"; do sn=${v%%=*}; b=${v#*=}
  "$b" "$P/golden-v2-novel/v1-all.tsv" 1000 > "$B/steady/run.stdout" 2>&1 & pid=$!; t=0
  until grep -q "^pass=3 " "$B/steady/run.stdout" 2>/dev/null; do sleep 1; t=$((t+1)); [ $t -gt 120 ] && break; done
  echo "$sn $(date -Iseconds) $(footprint -p $pid 2>&1 | grep -o 'Footprint: [0-9]* MB') $(grep -o 'setup_seconds=[0-9.]*' "$B/steady/run.stdout")" >> "$B/steady/steady.txt"; kill $pid; wait $pid 2>/dev/null; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
