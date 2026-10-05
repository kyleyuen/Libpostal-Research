#!/bin/bash
# 142 identity controls, part 1 (correctness only, never timed): ctl on v1/novel/OSM/fuzz-s1 (2M), orc dumps OFF on v1, orc dumps ON on v1 (size check).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/142-stage-oracles"; B="$E/raw/id"; P="$W/profiling"; A="$E/artifacts"; O="$W/research/oracles/v1"
mkdir -p "$B" "$O/golden-v1"
run() { local cn=$1; shift; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') START $cn" >> "$B/run.log"; "$@" > "$B/$cn.out" 2> "$B/$cn.err"; echo $? > "$B/$cn.exit"; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') END $cn exit=$(cat "$B/$cn.exit")" >> "$B/run.log"; }
run ctl_v1 "$A/ctl/dump_outputs" "$P/golden-v1/all.txt" & run ctl_nv "$A/ctl/dump_outputs" "$P/golden-v2-novel/novel.txt" &
run ctl_osm "$A/ctl/dump_outputs" "$P/golden-v3-osm/osm.txt" & run ctl_fz1 "$A/ctl/fuzz_pipeline" 1 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" & wait
run orc_off_v1 env -u ORACLE_DIR "$A/orc/dump_outputs" "$P/golden-v1/all.txt" & run orc_on_v1 env ORACLE_DIR="$O/golden-v1" "$A/orc/dump_outputs" "$P/golden-v1/all.txt" & wait
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
