#!/bin/bash
# Floor measurement for 015: baseline, h0, k1 on 013's floor_non_ascii_only subset (5,404 real non-ASCII / & / control addresses).
# Same per-run protocol as 013 (setup, one untimed warm-up pass on novel.tsv, 15 timed passes); variant order rotates per trial; AC gate before every run.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/015-integer-keys"; OUT="$E/raw/floor"; mkdir -p "$OUT"
F="$R/experiments/013-random-and-floor/data/floor_non_ascii_only.tsv"; WU="$W/profiling/golden-v2-novel/novel.tsv"
V=("baseline|$E/artifacts/bench_ext_baseline" "h0|$E/artifacts/h0/bench_ext" "k1|$E/artifacts/k1/bench_ext")
date -Iseconds > "$OUT/start.txt"; "$R/tools/env_snapshot.sh" "$OUT/env-start.txt" start
caffeinate -i -w $$ &
for t in 1 2 3 4 5; do
  for k in 0 1 2; do
    IFS='|' read -r name bin <<< "${V[$(( (k + t) % 3 ))]}"
    until "$R/tools/env_gate.sh" > "$OUT/floor-t$t-$name.gate" 2>&1; do echo "$(date -Iseconds) waiting for AC/gate" >> "$OUT/waits.log"; sleep 30; done
    /usr/bin/time -l "$bin" "$F" 15 "$WU" > "$OUT/floor-t$t-$name.stdout" 2> "$OUT/floor-t$t-$name.stderr"; echo "exit=$?" >> "$OUT/floor-t$t-$name.stderr"; sleep 4
  done
done
"$R/tools/env_snapshot.sh" "$OUT/env-end.txt" end; date -Iseconds > "$OUT/DONE"
