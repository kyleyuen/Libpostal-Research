#!/bin/bash
# End-to-end exactness checks for one 016 variant: dumps on golden-v1 + novel (hash vs baseline dump), frozen-reference comparison, 2 x 2M fuzz vs 011's baseline fuzz output.
V=$1; E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/021-final-stack"; P="/Users/kyle/Desktop/HW/CSCI 201 Reserach/profiling"; F="$E/../011-normalize-fastpaths/raw/fuzz"; A="$E/artifacts/$V"; R="$E/raw/verify_$V"; mkdir -p "$R"
date -Iseconds > "$R/start.txt"
"$A/dump_outputs" "$P/golden-v1/all.txt" > "$R/outputs_v1.jsonl" 2> "$R/dump_v1.stderr"; echo "dump v1 exit=$? sha=$(shasum -a 256 "$R/outputs_v1.jsonl" | cut -c1-20) (baseline 45574ee2b1972f1acf2d)" > "$R/summary.txt"
"$A/dump_outputs" "$P/golden-v2-novel/novel.txt" > "$R/outputs_novel.jsonl" 2> "$R/dump_novel.stderr"; echo "dump novel exit=$? sha=$(shasum -a 256 "$R/outputs_novel.jsonl" | cut -c1-20) (baseline 806b8a81603ea9adf3af)" >> "$R/summary.txt"
python3 "$E/../015-integer-keys/tools/compare_frozen.py" "$R/outputs_v1.jsonl" "$P/golden-v1/baseline-outputs.jsonl" | tail -1 | sed 's/^/frozen v1: /' >> "$R/summary.txt"
python3 "$E/../015-integer-keys/tools/compare_frozen.py" "$R/outputs_novel.jsonl" "$P/golden-v2-novel/baseline-outputs.jsonl" | tail -1 | sed 's/^/frozen novel: /' >> "$R/summary.txt"
for s in 1 2; do "$A/fuzz_pipeline" $s 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" > "$R/fuzz_s$s.txt" 2> /dev/null; e=$?; cmp -s "$R/fuzz_s$s.txt" "$F/base_s$s.txt" && r=IDENTICAL || r=DIFFER; echo "fuzz seed$s exit=$e $r" >> "$R/summary.txt"; done
rm -f "$R/fuzz_s1.txt" "$R/fuzz_s2.txt"
date -Iseconds > "$R/end.txt"; cat "$R/summary.txt"
