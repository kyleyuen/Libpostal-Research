#!/bin/bash
# 151 (review fix): same-session, same-corpus denominators. Per round and corpus: original bench_ext (20 passes), cpi bench_ext (20),
# replay_kernel (12); order rotated per round; gate before each; serial.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/151-optimality-bound"; O="$E/raw/same"; mkdir -p "$O"
ORIG="$W/research/experiments/015-integer-keys/artifacts/bench_ext_baseline"; CPI="$W/research/experiments/119-component-phrase-ids/artifacts/cpi/bench_ext"
RK="$E/artifacts/replay_kernel"; MODEL="/Users/kyle/.cache/libpostal-profile/libpostal/address_parser/address_parser_crf.dat"
shasum -a 256 "$ORIG" "$CPI" "$RK" "$W/profiling/golden-v2-novel/v1-all.tsv" "$W/profiling/golden-v3-osm/osm.tsv" > "$O/hashes.txt"
for r in 1 2 3; do for ds in v1 osm; do
  case $ds in v1) IN="$W/profiling/golden-v2-novel/v1-all.tsv"; OR="$W/research/oracles/v1/golden-v1";; osm) IN="$W/profiling/golden-v3-osm/osm.tsv"; OR="$W/research/oracles/v1/osm-v3";; esac
  case $r in 1) ORDER="orig cpi rk";; 2) ORDER="rk cpi orig";; 3) ORDER="cpi orig rk";; esac
  for b in $ORDER; do
    until "$W/research/tools/env_gate.sh" >> "$O/gate.log" 2>&1; do sleep 30; done
    tag="${b}_${ds}_r$r"; date "+%Y-%m-%dT%H:%M:%S%z start $tag" >> "$O/run.log"
    case $b in orig) /usr/bin/time -l "$ORIG" "$IN" 20 > "$O/$tag.stdout" 2> "$O/$tag.stderr";;
               cpi)  /usr/bin/time -l "$CPI" "$IN" 20 > "$O/$tag.stdout" 2> "$O/$tag.stderr";;
               rk)   /usr/bin/time -l "$RK" "$MODEL" "$OR" 12 > "$O/$tag.stdout" 2> "$O/$tag.stderr";; esac
    echo "exit $? $tag" >> "$O/run.log"; sleep 2
  done
done; done
date "+%Y-%m-%dT%H:%M:%S%z DONE" >> "$O/run.log"
