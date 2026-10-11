#!/bin/bash
# 155: one gated session -- original bench_ext (20 passes), cpi bench_ext (20), cpi css bench_split (3, as 141), rec_replay (60);
# golden-v1 all + OSM v3; 3 rounds, order rotated; serial; gate before each run.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/155-front-half"; O="$E/raw/budget"; mkdir -p "$O"
ORIG="$X/015-integer-keys/artifacts/bench_ext_baseline"; CPI="$X/119-component-phrase-ids/artifacts/cpi/bench_ext"; CSS="$X/119-component-phrase-ids/artifacts/css/bench_split"
RR="$E/artifacts/rec_replay"; MODEL="/Users/kyle/.cache/libpostal-profile/libpostal/address_parser/address_parser_crf.dat"
KEYS="/private/tmp/claude-501/-Users-kyle-Desktop-HW-CSCI-201-Reserach/b427676b-2d52-43eb-8ebf-a09347826a17/scratchpad/keys"
shasum -a 256 "$ORIG" "$CPI" "$CSS" "$RR" "$E/tools/rec_replay.c" "$W/profiling/golden-v2-novel/v1-all.tsv" "$W/profiling/golden-v3-osm/osm.tsv" > "$O/hashes.txt"
for r in 1 2 3; do for ds in v1 osm; do
  case $ds in v1) IN="$W/profiling/golden-v2-novel/v1-all.tsv"; OR="$W/research/oracles/v1/golden-v1";; osm) IN="$W/profiling/golden-v3-osm/osm.tsv"; OR="$W/research/oracles/v1/osm-v3";; esac
  case $r in 1) ORDER="orig cpi css rr";; 2) ORDER="rr css cpi orig";; 3) ORDER="cpi rr orig css";; esac
  for b in $ORDER; do
    until "$W/research/tools/env_gate.sh" >> "$O/gate.log" 2>&1; do sleep 30; done
    tag="${b}_${ds}_r$r"; date "+%Y-%m-%dT%H:%M:%S%z start $tag" >> "$O/run.log"
    case $b in orig) /usr/bin/time -l "$ORIG" "$IN" 20 > "$O/$tag.stdout" 2> "$O/$tag.stderr";;
               cpi)  /usr/bin/time -l "$CPI" "$IN" 20 > "$O/$tag.stdout" 2> "$O/$tag.stderr";;
               css)  /usr/bin/time -l "$CSS" "$IN" 3 > "$O/$tag.stdout" 2> "$O/$tag.stderr";;
               rr)   /usr/bin/time -l "$RR" "$MODEL" "$KEYS" "$OR" 60 > "$O/$tag.stdout" 2> "$O/$tag.stderr";; esac
    echo "exit $? $tag" >> "$O/run.log"; sleep 2
  done
done; done
date "+%Y-%m-%dT%H:%M:%S%z DONE" >> "$O/run.log"
