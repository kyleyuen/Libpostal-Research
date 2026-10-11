#!/bin/bash
# 151: instructions and cycles retired per address (029's difference method: (P=12 passes) - (P=2 passes), /usr/bin/time -l),
# original bench_ext_baseline vs cpi bench_ext, golden-v1 development and OSM v3; 3 rotated reps; gate before each run.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/151-optimality-bound"; O="$E/raw/ic"; mkdir -p "$O"
ORIG="$W/research/experiments/015-integer-keys/artifacts/bench_ext_baseline"; CPI="$W/research/experiments/119-component-phrase-ids/artifacts/cpi/bench_ext"
shasum -a 256 "$ORIG" "$CPI" "$W/research/experiments/025-audit-next/data/development.tsv" "$W/profiling/golden-v3-osm/osm.tsv" > "$O/hashes.txt"
for r in 1 2 3; do for ds in dev osm; do
  case $ds in dev) IN="$W/research/experiments/025-audit-next/data/development.tsv";; osm) IN="$W/profiling/golden-v3-osm/osm.tsv";; esac
  if [ $r = 2 ]; then ORDER="cpi orig"; else ORDER="orig cpi"; fi
  for b in $ORDER; do for P in 2 12; do
    case $b in orig) BIN="$ORIG";; cpi) BIN="$CPI";; esac
    tag="${b}_${ds}_p${P}_r${r}"
    until "$W/research/tools/env_gate.sh" >> "$O/gate.log" 2>&1; do sleep 30; done
    date "+%Y-%m-%dT%H:%M:%S%z start $tag" >> "$O/run.log"
    /usr/bin/time -l "$BIN" "$IN" $P > "$O/$tag.stdout" 2> "$O/$tag.stderr"; echo "exit $? $tag" >> "$O/run.log"
    sleep 2
  done; done
done; done
date "+%Y-%m-%dT%H:%M:%S%z DONE" >> "$O/run.log"
