#!/bin/bash
# 160 (copied from 158, E retargeted) functional checks (no timing). 149's verify.sh + verify_unseen.sh, run SEQUENTIALLY, each model process through model_slot.sh (team brief).
# Original-library references: the same files 149/121 use (outputs of the preserved original), plus 150's original dump of the holdout.
# usage: verify.sh VARIANT
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/160-p2-records-in-cpi"; P="$W/profiling"; v="$1"; b="$E/artifacts/$v"; o="$E/raw/verify_$v"
S="$W/research/tools/model_slot.sh"; mkdir -p "$o"
run() { out=$1; shift; echo "# $(date "+%Y-%m-%dT%H:%M:%S%z") $*" >> "$o/commands.txt"; "$S" "$@" > "$o/$out" 2> "$o/${out%.*}.err"; echo "#   exit=$? $(date "+%Y-%m-%dT%H:%M:%S%z")" >> "$o/commands.txt"; }
run v1.jsonl "$b/dump_outputs" "$P/golden-v1/all.txt"
run nv.jsonl "$b/dump_outputs" "$P/golden-v2-novel/novel.txt"
run osm.jsonl "$b/dump_outputs" "$P/golden-v3-osm/osm.txt"
run u4.jsonl "$b/dump_outputs" "$P/golden-v4-osm-unseen/unseen.txt"
for n in 5 6 7 8 9 10 11; do run "u$n.jsonl" "$b/dump_outputs" "$P/golden-v$n-osm-unseen/unseen$n.txt"; done
run audit.jsonl "$b/dump_outputs" "$P/independent-audit-v1/audit-input.txt"
run dad.jsonl "$b/dump_outputs" "$P/independent-audit-v1/dad-international.txt"
run fz1.txt "$b/fuzz_pipeline" 1 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt"
run fz2.txt "$b/fuzz_pipeline" 2 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt"
c() { if cmp -s "$1" "$2"; then echo "$3 IDENTICAL ($(wc -l < "$1" | tr -d ' ') lines)"; else echo "$3 DIFFERENT"; fi; }
{ echo "v1 $(shasum -a 256 "$o/v1.jsonl" | cut -c1-20) (expect 45574ee2b1972f1acf2d)"; echo "nv $(shasum -a 256 "$o/nv.jsonl" | cut -c1-20) (expect 806b8a81603ea9adf3af)"
  c "$o/osm.jsonl" "$X/017-loadtime-precompute/raw/v3check/outputs_baseline.jsonl" osm-v3
  c "$o/u4.jsonl" "$X/069-unseen-remeasure/raw/dump_orig.stdout" unseen-v4
  for x in 5:080-unseen5-remeasure 6:086-unseen6-remeasure 7:093-unseen7-remeasure 8:102-unseen8-remeasure 9:115-unseen9-remeasure 10:120-unseen10-remeasure 11:134-unseen11-confirm-cpi; do
    n=${x%%:*}; c "$o/u$n.jsonl" "$X/${x#*:}/raw/dump_orig.stdout" "unseen-v$n"; done
  c "$o/audit.jsonl" "$X/150-independent-professor-audit/raw/correctness/original.txt" holdout-150
  c "$o/dad.jsonl" "$X/150-independent-professor-audit/raw/correctness/dad-original.txt" dad-150
  echo "frozen golden-v1: $(python3 "$X/015-integer-keys/tools/compare_frozen.py" "$o/v1.jsonl" "$P/golden-v1/baseline-outputs.jsonl" | tail -1)"
  echo "frozen novel: $(python3 "$X/015-integer-keys/tools/compare_frozen.py" "$o/nv.jsonl" "$P/golden-v2-novel/baseline-outputs.jsonl" | tail -1)"
  c "$o/fz1.txt" "$X/011-normalize-fastpaths/raw/fuzz/base_s1.txt" "fuzz s1"
  c "$o/fz2.txt" "$X/011-normalize-fastpaths/raw/fuzz/base_s2.txt" "fuzz s2"
  echo "stderr bytes: $(cat "$o"/*.err | wc -c | tr -d ' ')"; } > "$o/summary.txt"
cat "$o/summary.txt"
