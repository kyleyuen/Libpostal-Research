#!/bin/bash
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/175-direct-event-flow"; B="$E/artifacts/event"; P="$W/profiling"; O="$W/research/oracles/v1"; R="$E/raw"; S="$W/research/tools/model_slot.sh"
run() { local n="$1"; shift; local start end rc; start=$(date '+%Y-%m-%dT%H:%M:%S%z'); "$@" >"$R/$n.stdout" 2>"$R/$n.stderr"; rc=$?; end=$(date '+%Y-%m-%dT%H:%M:%S%z'); printf '%s\t%s\t%s\t%s\n' "$n" "$start" "$end" "$rc" >>"$R/commands.tsv"; printf '%s' "$rc" >"$R/$n.exit"; return "$rc"; }
mkdir -p /tmp/libpostal-175/o3/{golden-v1,novel,osm-v3}
run o3_golden env EVENT_ORACLE_DIR=/tmp/libpostal-175/o3/golden-v1 "$S" "$B/dump_outputs" "$P/golden-v1/all.txt"
run o3_novel env EVENT_ORACLE_DIR=/tmp/libpostal-175/o3/novel "$S" "$B/dump_outputs" "$P/golden-v2-novel/novel.txt"
run o3_osm env EVENT_ORACLE_DIR=/tmp/libpostal-175/o3/osm-v3 "$S" "$B/dump_outputs" "$P/golden-v3-osm/osm.txt"
for c in golden-v1 novel osm-v3; do for k in O3i O4; do gzip -dc "$O/$c/$k.txt.gz" | cmp -s - "/tmp/libpostal-175/o3/$c/$k.txt"; printf '%s %s cmp=%s\n' "$c" "$k" "$?" >>"$R/oracle_summary.txt"; done; done
run final_golden "$S" "$B/dump_outputs" "$P/golden-v1/all.txt"
run final_novel "$S" "$B/dump_outputs" "$P/golden-v2-novel/novel.txt"
run final_osm "$S" "$B/dump_outputs" "$P/golden-v3-osm/osm.txt"
cmp -s "$R/final_golden.stdout" "$P/golden-v1/baseline-outputs.jsonl"; echo "golden frozen cmp=$?" >>"$R/final_summary.txt"
cmp -s "$R/final_novel.stdout" "$P/golden-v2-novel/baseline-outputs.jsonl"; echo "novel frozen cmp=$?" >>"$R/final_summary.txt"
cmp -s "$R/final_osm.stdout" "$W/research/experiments/017-loadtime-precompute/raw/v3check/outputs_baseline.jsonl"; echo "osm original cmp=$?" >>"$R/final_summary.txt"
