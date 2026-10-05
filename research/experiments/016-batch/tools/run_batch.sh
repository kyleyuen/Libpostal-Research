#!/bin/bash
# 016 AC batch (run plugged in; every run waits for the AC/thermal gate). About 60-75 minutes. Only variants whose raw/verify_<v>/summary.txt shows
# identical dumps, frozen references and both fuzz seeds are timed; skipped variants are listed in raw/batch/skipped.txt.
#   A  controlled session (research/tools/run_unattended.sh): baseline + control k0 + every verified optimization, golden-v1 all + novel, 5 trials x 19 passes
#   B  floor subset (5,404 non-ASCII addresses): baseline, k0 and the combined variants, 5 processes each, rotating order
#   C  T2 stage timing: stats builds s (= k1) and sall7 (= all7) on golden-v1 all, the floor subset and the ASCII subset, 3 runs each
#   D  T3 paired confirmation of 015: h0 vs k1, 20 alternating rounds on golden-v1 all
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/016-batch"; A="$E/artifacts"; B="$E/raw/batch"; mkdir -p "$B"
SUB="$R/experiments/013-random-and-floor/data"; WU="$W/profiling/golden-v2-novel/novel.tsv"; V1="$W/profiling/golden-v2-novel/v1-all.tsv"
gate() { until "$R/tools/env_gate.sh" > /dev/null 2>&1; do echo "$(date -Iseconds) waiting for AC/gate" >> "$B/waits.log"; sleep 30; done; }
ok() { f="$E/raw/verify_$1/summary.txt"; [ -f "$f" ] && [ "$(grep -c 'IDENTICAL\|(baseline 45574ee2b1972f1acf2d)\|(baseline 806b8a81603ea9adf3af)' "$f")" -ge 6 ] && [ "$(grep -c "sha=45574ee2b1972f1acf2d\|sha=806b8a81603ea9adf3af" "$f")" -eq 2 ] && ! grep -q DIFFER "$f"; }
n=0; while [ ! -f "$E/raw/verify_pgolto/end.txt" ] && [ $n -lt 240 ]; do sleep 30; n=$((n+1)); done   # wait for the verification chain (max 2 h)
date -Iseconds > "$B/start.txt"; : > "$B/skipped.txt"
CAND=(); for v in t4 t45 t6 t7 t8 all all7 pgo pgolto; do if ok $v; then CAND+=("$v=$A/$v/bench_ext"); else echo "$v not verified" >> "$B/skipped.txt"; fi; done
echo "timed: ${CAND[*]}" > "$B/variants.txt"
# A
"$R/tools/run_unattended.sh" "$B/A" 5 19 baseline="$E/../015-integer-keys/artifacts/bench_ext_baseline" k0="$A/k0/bench_ext" "${CAND[@]}"
python3 "$R/tools/analyze_quiet.py" "$B/A" baseline > "$B/A-summary.md" 2>&1
# B
FL=("baseline|$E/../015-integer-keys/artifacts/bench_ext_baseline" "k0|$A/k0/bench_ext"); for v in t45 all all7 pgolto; do ok $v && FL+=("$v|$A/$v/bench_ext"); done
n=${#FL[@]}; mkdir -p "$B/floor"
for t in 1 2 3 4 5; do for k in $(seq 0 $((n-1))); do IFS='|' read -r name bin <<< "${FL[$(( (k + t) % n ))]}"; gate
  /usr/bin/time -l "$bin" "$SUB/floor_non_ascii_only.tsv" 15 "$WU" > "$B/floor/floor-t$t-$name.stdout" 2> "$B/floor/floor-t$t-$name.stderr"; sleep 4; done; done
python3 "$E/tools/analyze_runs.py" "$B/floor" floor baseline k0 > "$B/B-floor-summary.md" 2>&1
# C
mkdir -p "$B/stages"
for r in 1 2 3; do for v in s sall7; do for set in v1:"$V1" floor:"$SUB/floor_non_ascii_only.tsv" ascii:"$SUB/ascii_only.tsv"; do gate
  "$A/$v/stage_bench" "${set#*:}" 5 > "$B/stages/$v-${set%%:*}-r$r.txt" 2>&1; sleep 3; done; done; done
python3 "$E/tools/analyze_stages.py" "$B/stages" > "$B/C-stages-summary.md" 2>&1
# D
mkdir -p "$B/paired"; H0="$E/../015-integer-keys/artifacts/h0/bench_ext"; K1="$E/../015-integer-keys/artifacts/k1/bench_ext"
for r in $(seq 1 20); do if [ $((r % 2)) -eq 1 ]; then o="h0 k1"; else o="k1 h0"; fi
  for v in $o; do [ $v = h0 ] && bin=$H0 || bin=$K1; gate; "$bin" "$V1" 10 "$WU" > "$B/paired/r$r-$v.stdout" 2>&1; sleep 3; done; done
python3 "$E/tools/analyze_runs.py" "$B/paired" r h0 h0 --paired k1 > "$B/D-paired-summary.md" 2>&1
date -Iseconds > "$B/DONE"
