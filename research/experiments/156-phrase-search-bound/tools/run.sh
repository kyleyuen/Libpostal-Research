#!/bin/bash
# 156: time ps_replay (optimistic bound; see README amendment), golden-v1 + OSM, 3 rounds x 60 passes, gated, serial.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/156-phrase-search-bound"; O="$E/raw/timing"; mkdir -p "$O"
K="/private/tmp/claude-501/-Users-kyle-Desktop-HW-CSCI-201-Reserach/b427676b-2d52-43eb-8ebf-a09347826a17/scratchpad/keys"
shasum -a 256 "$E/tools/ps_replay.c" "$E/artifacts/ps_replay" > "$O/hashes.txt"
for r in 1 2 3; do for c in golden-v1 osm-v3; do
  until "$W/research/tools/env_gate.sh" >> "$O/gate.log" 2>&1; do sleep 30; done
  date "+%Y-%m-%dT%H:%M:%S%z start $c r$r" >> "$O/run.log"
  /usr/bin/time -l "$E/artifacts/ps_replay" "$K" "$W/research/oracles/v1/$c" 60 > "$O/${c}_r$r.stdout" 2> "$O/${c}_r$r.stderr"
  echo "exit $? $c r$r" >> "$O/run.log"; sleep 2
done; done
date "+%Y-%m-%dT%H:%M:%S%z DONE" >> "$O/run.log"
