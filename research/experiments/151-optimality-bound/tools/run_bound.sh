#!/bin/bash
# 151: build oracle_bound against the preserved ORIGINAL libpostal.a and run it on every 142 oracle corpus (no timing).
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/151-optimality-bound"
LIB="$W/research/experiments/000-baseline/artifacts/libpostal.a"; MODEL="/Users/kyle/.cache/libpostal-profile/libpostal/address_parser/address_parser_crf.dat"
mkdir -p "$E/raw/bound" "$E/artifacts"
clang -O2 -g -DHAVE_CONFIG_H -DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\" -I"$W/libpostal/src" -I"$W/libpostal" "$E/tools/oracle_bound.c" "$LIB" -lm -o "$E/artifacts/oracle_bound" \
  > "$E/raw/bound/build.stdout" 2> "$E/raw/bound/build.stderr"; echo $? > "$E/raw/bound/build.exit"
shasum -a 256 "$E/tools/oracle_bound.c" "$LIB" "$E/artifacts/oracle_bound" "$MODEL" > "$E/raw/bound/hashes.txt"
for c in golden-v1 novel osm-v3 fuzz-s1-200k; do
  date "+%Y-%m-%dT%H:%M:%S%z start $c" >> "$E/raw/bound/run.log"
  "$E/artifacts/oracle_bound" "$MODEL" "$W/research/oracles/v1/$c" "$E/raw/bound/$c.tsv" > "$E/raw/bound/$c.stdout" 2> "$E/raw/bound/$c.stderr"
  echo "exit $? $c" >> "$E/raw/bound/run.log"; date "+%Y-%m-%dT%H:%M:%S%z end $c" >> "$E/raw/bound/run.log"
done
