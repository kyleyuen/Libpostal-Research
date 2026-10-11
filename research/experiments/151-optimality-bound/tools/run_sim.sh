#!/bin/bash
# 151: build sim_layout against the ORIGINAL libpostal.a and run it on golden-v1, novel, OSM oracles (3 passes; no timing).
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/151-optimality-bound"
LIB="$W/research/experiments/000-baseline/artifacts/libpostal.a"; MODEL="/Users/kyle/.cache/libpostal-profile/libpostal/address_parser/address_parser_crf.dat"
mkdir -p "$E/raw/sim"
clang -O2 -g -DHAVE_CONFIG_H -DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\" -I"$W/libpostal/src" -I"$W/libpostal" \
  "$E/tools/sim_layout.c" "$LIB" -lm -o "$E/artifacts/sim_layout" > "$E/raw/sim/build.stdout" 2> "$E/raw/sim/build.stderr"; echo $? > "$E/raw/sim/build.exit"
shasum -a 256 "$E/tools/sim_layout.c" "$E/artifacts/sim_layout" > "$E/raw/sim/hashes.txt"
for c in golden-v1 novel osm-v3; do
  date "+%Y-%m-%dT%H:%M:%S%z start $c" >> "$E/raw/sim/run.log"
  "$E/artifacts/sim_layout" "$MODEL" "$W/research/oracles/v1/$c" 3 > "$E/raw/sim/$c.stdout" 2> "$E/raw/sim/$c.stderr"
  echo "exit $? $c" >> "$E/raw/sim/run.log"
done
