#!/bin/bash
# 018 AC timing: original, 016 control k0, 016 best pgolto, and every verified 018 build; golden-v1 + novel (run_unattended, 5 x 19), then the floor subset.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/018-compiler-build"; A="$E/artifacts"; B="$E/raw/timing"; mkdir -p "$B/floor"
until grep -q "^c2s2: " "$E/raw/compile_times.txt" && [ -f "$E/raw/verify_c2s2/end.txt" ]; do sleep 30; done
ok() { f="$E/raw/verify_$1/summary.txt"; [ -f "$f" ] && [ "$(grep -c IDENTICAL "$f")" -eq 4 ] && [ "$(grep -c "sha=45574ee2b1972f1acf2d\|sha=806b8a81603ea9adf3af" "$f")" -eq 2 ]; }
BASE="$E/../015-integer-keys/artifacts/bench_ext_baseline"; X16="$E/../016-batch/artifacts"
V=(baseline="$BASE" k0="$X16/k0/bench_ext" pgolto="$X16/pgolto/bench_ext"); : > "$B/skipped.txt"
for v in c2 c2s1 c2s2 c3 c4; do if ok $v; then V+=("$v=$A/$v/bench_ext"); else echo "$v not verified" >> "$B/skipped.txt"; fi; done
date -Iseconds > "$B/start.txt"
MAX_WAITS=1000000 "$R/tools/run_unattended.sh" "$B/A" 5 19 "${V[@]}"
python3 "$R/tools/analyze_quiet.py" "$B/A" baseline > "$B/A-summary.md" 2>&1
n=${#V[@]}
for t in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + t) % n ))]}; name=${e%%=*}; bin=${e#*=}
  until "$R/tools/env_gate.sh" > /dev/null 2>&1; do echo "$(date -Iseconds) waiting" >> "$B/waits.log"; sleep 30; done
  /usr/bin/time -l "$bin" "$R/experiments/013-random-and-floor/data/floor_non_ascii_only.tsv" 15 "$W/profiling/golden-v2-novel/novel.tsv" > "$B/floor/floor-t$t-$name.stdout" 2> "$B/floor/floor-t$t-$name.stderr"; sleep 3; done; done
python3 "$E/../016-batch/tools/analyze_runs.py" "$B/floor" floor baseline k0 > "$B/B-floor-summary.md" 2>&1
date -Iseconds > "$B/DONE"
