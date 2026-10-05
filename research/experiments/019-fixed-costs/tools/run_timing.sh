#!/bin/bash
# 019 AC timing: original, 017 u2, rebuilt control r1 and every verified 019 build; golden-v1 + novel (run_unattended 5 x 19), floor and golden-v3-osm (5 each).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/019-fixed-costs"; A="$E/artifacts"; B="$E/raw/timing"; mkdir -p "$B/floor" "$B/v3"
until [ -f "$E/raw/verify_b1/end.txt" ] && [ -f "$E/raw/verify_b2/end.txt" ] && [ -f "$E/raw/verify_h1/end.txt" ] && [ -f "$E/raw/verify_p1/end.txt" ]; do sleep 30; done
ok() { f="$E/raw/verify_$1/summary.txt"; [ -f "$f" ] && [ "$(grep -c IDENTICAL "$f")" -eq 4 ] && [ "$(grep -c "sha=45574ee2b1972f1acf2d\|sha=806b8a81603ea9adf3af" "$f")" -eq 2 ]; }
BASE="$R/experiments/015-integer-keys/artifacts/bench_ext_baseline"
V=(baseline="$BASE" u2="$R/experiments/017-loadtime-precompute/artifacts/u2/bench_ext"); : > "$B/skipped.txt"
for v in r1 v1 n1 f1 a1 u3 b1 b2 h1 p1; do if ok $v; then V+=("$v=$A/$v/bench_ext"); else echo "$v not verified" >> "$B/skipped.txt"; fi; done
date -Iseconds > "$B/start.txt"
MAX_WAITS=1000000 "$R/tools/run_unattended.sh" "$B/A" 5 19 "${V[@]}"
python3 "$R/tools/analyze_quiet.py" "$B/A" baseline > "$B/A-summary.md" 2>&1
n=${#V[@]}
for set in floor:"$R/experiments/013-random-and-floor/data/floor_non_ascii_only.tsv" v3:"$W/profiling/golden-v3-osm/osm.tsv"; do d=${set%%:*}; f=${set#*:}
  for t in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + t) % n ))]}; name=${e%%=*}; bin=${e#*=}
    until "$R/tools/env_gate.sh" > /dev/null 2>&1; do echo "$(date -Iseconds) waiting" >> "$B/waits.log"; sleep 30; done
    /usr/bin/time -l "$bin" "$f" 15 "$W/profiling/golden-v2-novel/novel.tsv" > "$B/$d/$d-t$t-$name.stdout" 2> "$B/$d/$d-t$t-$name.stderr"; sleep 3; done; done
  python3 "$R/experiments/016-batch/tools/analyze_runs.py" "$B/$d" $d baseline r1 > "$B/$d-summary.md" 2>&1
done
date -Iseconds > "$B/DONE"
