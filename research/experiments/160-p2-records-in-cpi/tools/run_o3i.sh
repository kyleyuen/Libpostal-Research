#!/bin/bash
# 160 (p2b): O3i dumps from an EXP_RECORDS_ORACLE build vs research/oracles/v1 (read-only), sequential, one model process via model_slot.sh.
# Full-line cmp INCLUDING '-' (miss) records, stronger than the binding condition (misses dropped). Dumps go to /tmp/libpostal-160 (large).
# usage: run_o3i.sh VARIANT [corpus ...]   corpora: golden-v1 novel osm-v3 fuzz-s1-200k
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/160-p2-records-in-cpi"; P="$W/profiling"; O="$W/research/oracles/v1"; v=$1; shift
b="$E/artifacts/$v"; o="$E/raw/o3i_$v"; T="/tmp/libpostal-160/o3i_$v"; mkdir -p "$o" "$T"; S="$W/research/tools/model_slot.sh"
for c in "${@:-golden-v1 novel osm-v3 fuzz-s1-200k}"; do for c in $c; do
  mkdir -p "$T/$c"; rm -f "$T/$c/O3i.txt"
  case $c in golden-v1) cmd=("$b/dump_outputs" "$P/golden-v1/all.txt");; novel) cmd=("$b/dump_outputs" "$P/golden-v2-novel/novel.txt");;
    osm-v3) cmd=("$b/dump_outputs" "$P/golden-v3-osm/osm.txt");; fuzz-s1-200k) cmd=("$b/fuzz_pipeline" 1 200000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt");; esac
  echo "# $(date '+%Y-%m-%dT%H:%M:%S%z') $c: REC_ORACLE_DIR=$T/$c ${cmd[*]}" >> "$o/commands.txt"
  env REC_ORACLE_DIR="$T/$c" REC_ORACLE_IDS="$E/artifacts/records/slot_ids.u32" "$S" "${cmd[@]}" > "$T/$c/out.txt" 2> "$o/$c.stderr"; rc=$?
  echo "#   exit=$rc $(date '+%Y-%m-%dT%H:%M:%S%z')" >> "$o/commands.txt"
  if gzip -dc "$O/$c/O3i.txt.gz" | cmp -s - "$T/$c/O3i.txt"; then r=IDENTICAL; else r=DIFFERENT; fi
  echo "$c O3i $r (candidate $(wc -l < "$T/$c/O3i.txt" | tr -d ' ') lines, sha $(shasum -a 256 "$T/$c/O3i.txt" | cut -c1-16); oracle $(grep "$c/O3i.txt" "$O/SHA256SUMS.uncompressed" | cut -c1-16)) exit=$rc" | tee -a "$o/summary.txt"
done; done
