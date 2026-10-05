#!/bin/bash
# 146 verification (correctness only, never timed). Wave 1: hrw dumps (ORACLE_DIR) on 4 oracle corpora + c146 v1. Wave 2: hrw full fuzz s1, s2 (2M each).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/146-rw-h-bigram"; B="$E/raw/v"; P="$W/profiling"; A="$E/artifacts"; S="/private/tmp/claude-501/-Users-kyle-Desktop-HW-CSCI-201-Reserach/1fd9b691-bcfa-41f8-b753-ee02bbe9bc7e/scratchpad/146"
mkdir -p "$B" "$S/golden-v1" "$S/novel" "$S/osm-v3" "$S/fuzz-s1-200k"
run() { local cn=$1; shift; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') START $cn" >> "$B/run.log"; "$@" > "$B/$cn.out" 2> "$B/$cn.err"; echo $? > "$B/$cn.exit"; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') END $cn exit=$(cat "$B/$cn.exit")" >> "$B/run.log"; }
run hrw_v1 env RW_VERBOSE=1 ORACLE_DIR="$S/golden-v1" "$A/hrw/dump_outputs" "$P/golden-v1/all.txt" &
run hrw_nv env RW_VERBOSE=1 ORACLE_DIR="$S/novel" "$A/hrw/dump_outputs" "$P/golden-v2-novel/novel.txt" &
run hrw_osm env RW_VERBOSE=1 ORACLE_DIR="$S/osm-v3" "$A/hrw/dump_outputs" "$P/golden-v3-osm/osm.txt" &
run hrw_fz200k env RW_VERBOSE=1 ORACLE_DIR="$S/fuzz-s1-200k" "$A/hrw/fuzz_pipeline" 1 200000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" & wait
run c146_v1 "$A/c146/dump_outputs" "$P/golden-v1/all.txt" &
run hrw_fz1 env RW_VERBOSE=1 "$A/hrw/fuzz_pipeline" 1 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" &
run hrw_fz2 env RW_VERBOSE=1 "$A/hrw/fuzz_pipeline" 2 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" &
run hrw_u11 env RW_VERBOSE=1 "$A/hrw/dump_outputs" "$P/golden-v11-osm-unseen/unseen11.txt" & wait
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
