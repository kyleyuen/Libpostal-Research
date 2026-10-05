#!/bin/bash
# 072 functional checks (no timing). Usage: verify.sh <variant>
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/099-phrase-len"; v="$1"; b="$E/artifacts/$v"; o="$E/raw/verify_$v"; mkdir -p "$o"
"$b/dump_outputs" "$W/profiling/golden-v1/all.txt" > "$o/v1.jsonl" 2> "$o/v1.err" & "$b/dump_outputs" "$W/profiling/golden-v2-novel/novel.txt" > "$o/nv.jsonl" 2> "$o/nv.err" &
"$b/dump_outputs" "$W/profiling/golden-v3-osm/osm.txt" > "$o/osm.jsonl" 2> "$o/osm.err" & "$b/dump_outputs" "$W/profiling/golden-v4-osm-unseen/unseen.txt" > "$o/u4.jsonl" 2> "$o/u4.err" &
"$b/fuzz_pipeline" 1 2000000 "$W/profiling/golden-v1/all.txt" "$W/profiling/golden-v2-novel/novel.txt" > "$o/fz1.txt" 2> "$o/fz1.err" &
"$b/fuzz_pipeline" 2 2000000 "$W/profiling/golden-v1/all.txt" "$W/profiling/golden-v2-novel/novel.txt" > "$o/fz2.txt" 2> "$o/fz2.err" & wait
{ echo "v1 $(shasum -a 256 "$o/v1.jsonl" | cut -c1-20) (expect 45574ee2b1972f1acf2d)"; echo "nv $(shasum -a 256 "$o/nv.jsonl" | cut -c1-20) (expect 806b8a81603ea9adf3af)"
  cmp -s "$o/osm.jsonl" "$X/017-loadtime-precompute/raw/v3check/outputs_baseline.jsonl" && echo "osm IDENTICAL" || echo "osm DIFFERENT"
  cmp -s "$o/u4.jsonl" "$X/069-unseen-remeasure/raw/dump_orig.stdout" && echo "unseen-v4 IDENTICAL" || echo "unseen-v4 DIFFERENT/CHECK"
  python3 "$X/015-integer-keys/tools/compare_frozen.py" "$o/v1.jsonl" "$W/profiling/golden-v1/baseline-outputs.jsonl" | tail -1
  python3 "$X/015-integer-keys/tools/compare_frozen.py" "$o/nv.jsonl" "$W/profiling/golden-v2-novel/baseline-outputs.jsonl" | tail -1
  cmp -s "$o/fz1.txt" "$X/011-normalize-fastpaths/raw/fuzz/base_s1.txt" && echo "fuzz s1 IDENTICAL" || echo "fuzz s1 DIFFERENT"
  cmp -s "$o/fz2.txt" "$X/011-normalize-fastpaths/raw/fuzz/base_s2.txt" && echo "fuzz s2 IDENTICAL" || echo "fuzz s2 DIFFERENT"; } > "$o/summary.txt"
