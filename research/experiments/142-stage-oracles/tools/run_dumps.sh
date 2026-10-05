#!/bin/bash
# 142 part 2: oracle dumps (orc, ORACLE_DIR set) for novel, OSM, fuzz seed 1 first 200,000 inputs; second golden-v1 dump (determinism) into the session scratchpad.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/142-stage-oracles"; B="$E/raw/id"; P="$W/profiling"; A="$E/artifacts"; O="$W/research/oracles/v1"; S="/private/tmp/claude-501/-Users-kyle-Desktop-HW-CSCI-201-Reserach/1fd9b691-bcfa-41f8-b753-ee02bbe9bc7e/scratchpad/142_det_v1"
mkdir -p "$O/novel" "$O/osm-v3" "$O/fuzz-s1-200k" "$S"
run() { local cn=$1; shift; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') START $cn" >> "$B/run.log"; "$@" > "$B/$cn.out" 2> "$B/$cn.err"; echo $? > "$B/$cn.exit"; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') END $cn exit=$(cat "$B/$cn.exit")" >> "$B/run.log"; }
run orc_on_nv env ORACLE_DIR="$O/novel" "$A/orc/dump_outputs" "$P/golden-v2-novel/novel.txt" &
run orc_on_osm env ORACLE_DIR="$O/osm-v3" "$A/orc/dump_outputs" "$P/golden-v3-osm/osm.txt" &
run orc_on_fz1 env ORACLE_DIR="$O/fuzz-s1-200k" "$A/orc/fuzz_pipeline" 1 200000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" &
run orc_on_v1_again env ORACLE_DIR="$S" "$A/orc/dump_outputs" "$P/golden-v1/all.txt" & wait
