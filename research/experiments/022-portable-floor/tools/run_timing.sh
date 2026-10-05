#!/bin/bash
# 022: original, 021 l4 and the portable build pt (only if verified); same protocol as 021.
# golden-v1 + novel via run_unattended (5 x 19), then floor, golden-v3-osm and the ASCII-only subset (5 processes each, rotating), AC-gated throughout.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/022-portable-floor"; X="$R/experiments"; B="$E/raw/timing"; mkdir -p "$B"
until [ -f "$E/raw/VERIFY_DONE" ]; do sleep 30; done
ok() { f="$1/summary.txt"; [ -f "$f" ] && [ "$(grep -c IDENTICAL "$f")" -eq 4 ] && [ "$(grep -c "sha=45574ee2b1972f1acf2d\|sha=806b8a81603ea9adf3af" "$f")" -eq 2 ]; }
V=(baseline="$X/015-integer-keys/artifacts/bench_ext_baseline" l4="$X/021-final-stack/artifacts/l4/bench_ext"); : > "$B/skipped.txt"
true
for v in pt; do if ok "$E/raw/verify_$v"; then V+=("$v=$E/artifacts/$v/bench_ext"); else echo "$v not verified" >> "$B/skipped.txt"; fi; done
date -Iseconds > "$B/start.txt"; printf '%s\n' "${V[@]}" > "$B/variant_list.txt"
MAX_WAITS=1000000 "$R/tools/run_unattended.sh" "$B/A" 5 19 "${V[@]}"
python3 "$R/tools/analyze_quiet.py" "$B/A" baseline > "$B/A-summary.md" 2>&1
n=${#V[@]}
for set in floor:"$X/013-random-and-floor/data/floor_non_ascii_only.tsv" v3:"$W/profiling/golden-v3-osm/osm.tsv" ascii:"$X/013-random-and-floor/data/ascii_only.tsv"; do d=${set%%:*}; f=${set#*:}; mkdir -p "$B/$d"
  for t in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do e=${V[$(( (k + t) % n ))]}; name=${e%%=*}; bin=${e#*=}
    until "$R/tools/env_gate.sh" > /dev/null 2>&1; do echo "$(date -Iseconds) waiting" >> "$B/waits.log"; sleep 30; done
    /usr/bin/time -l "$bin" "$f" 15 "$W/profiling/golden-v2-novel/novel.tsv" > "$B/$d/$d-t$t-$name.stdout" 2> "$B/$d/$d-t$t-$name.stderr"; sleep 3; done; done
  python3 "$X/016-batch/tools/analyze_runs.py" "$B/$d" $d baseline l4 > "$B/$d-summary.md" 2>&1
done
date -Iseconds > "$B/DONE"
