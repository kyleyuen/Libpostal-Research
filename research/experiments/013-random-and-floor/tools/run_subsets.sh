#!/bin/bash
# Baseline vs candidate on subsets of the REAL golden-v1 set. Each process: setup, one untimed warm-up pass on novel.tsv (a different file), then 15 timed passes.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/013-random-and-floor"; OUT="$E/raw/subsets"; mkdir -p "$OUT"
BASE="$R/experiments/007-controlled-rebaseline/artifacts/bench_ext_baseline"; CAND="$R/experiments/011-normalize-fastpaths/artifacts/h/bench_ext"; WU="$W/profiling/golden-v2-novel/novel.tsv"
run_one() { name=$1; file=$2; tag=$3; bin=$4
  until "$R/tools/env_gate.sh" > /dev/null 2>&1; do echo "$(date -Iseconds) waiting for AC/gate" >> "$OUT/waits.log"; sleep 30; done
  "$bin" "$file" 15 "$WU" > "$OUT/$tag.stdout" 2> "$OUT/$tag.stderr"; sleep 4; }
# name|file|trials entries in an array (paths contain spaces)
SETS=("full_v1|$W/profiling/golden-v2-novel/v1-all.tsv|3" "floor_non_ascii_only|$E/data/floor_non_ascii_only.tsv|3" "ascii_only|$E/data/ascii_only.tsv|3")
for s in 1 2 3 4 5 6 7 8; do SETS+=("random_$s|$E/data/random_$s.tsv|2"); done
for spec in "${SETS[@]}"; do
  IFS='|' read -r name file trials <<< "$spec"
  t=1
  while [ $t -le $trials ]; do
    if [ $((t % 2)) -eq 1 ]; then run_one "$name" "$file" "$name-t$t-baseline" "$BASE"; run_one "$name" "$file" "$name-t$t-h" "$CAND"; else run_one "$name" "$file" "$name-t$t-h" "$CAND"; run_one "$name" "$file" "$name-t$t-baseline" "$BASE"; fi
    t=$((t+1))
  done
done
echo done > "$OUT/DONE"
