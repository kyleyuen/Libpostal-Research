#!/bin/bash
# 154: build replay_k3 (ORIGINAL libpostal.a, -O3 -mcpu=native) and time it on golden-v1 and OSM oracles, serial, gated, 3 rounds, 60 passes.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/154-backhalf-engineering"; O="$E/raw/replay"; mkdir -p "$O"
LIB="$W/research/experiments/000-baseline/artifacts/libpostal.a"; MODEL="/Users/kyle/.cache/libpostal-profile/libpostal/address_parser/address_parser_crf.dat"
clang -O3 -mcpu=native -g -DHAVE_CONFIG_H -DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\" -I"$W/libpostal/src" -I"$W/libpostal" \
  "$E/tools/replay_k3.c" "$LIB" -lm -o "$E/artifacts/replay_k3" > "$O/build.stdout" 2> "$O/build.stderr"; echo $? > "$O/build.exit"
shasum -a 256 "$E/tools/replay_k3.c" "$E/artifacts/replay_k3" "$LIB" > "$O/hashes.txt"
for r in 1 2 3; do for c in golden-v1 osm-v3; do
  until "$W/research/tools/env_gate.sh" >> "$O/gate.log" 2>&1; do sleep 30; done
  date "+%Y-%m-%dT%H:%M:%S%z start $c r$r" >> "$O/run.log"
  /usr/bin/time -l "$E/artifacts/replay_k3" "$MODEL" "$W/research/oracles/v1/$c" 60 > "$O/${c}_r$r.stdout" 2> "$O/${c}_r$r.stderr"
  echo "exit $? $c r$r" >> "$O/run.log"; sleep 2
done; done
date "+%Y-%m-%dT%H:%M:%S%z DONE" >> "$O/run.log"
