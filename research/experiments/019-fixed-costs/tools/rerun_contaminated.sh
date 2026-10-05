#!/bin/bash
# Rerun 019 session-A runs that overlapped the 020 verification CPU load (golden-v1 trial-1 runs started between 02:28:34 and 02:33:11).
# Originals are kept as *.excluded; reruns use the same binary, arguments and tag as run_unattended.sh, AC-gated, then the A summary is recomputed.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; E="$R/experiments/019-fixed-costs"; A="$E/raw/timing/A"
bin_of() { case $1 in baseline) echo "$R/experiments/015-integer-keys/artifacts/bench_ext_baseline";; u2) echo "$R/experiments/017-loadtime-precompute/artifacts/u2/bench_ext";; *) echo "$E/artifacts/$1/bench_ext";; esac; }
: > "$E/raw/timing/RERUN_LIST.txt"
for st in "$A"/v1-all-t1-*.start; do
  t=$(cat "$st"); tag=$(basename "$st" .start)
  if [[ "$t" > "2026-10-01T02:28:33" && "$t" < "2026-10-01T02:33:12" ]]; then
    for ext in stdout stderr env cpu start end gate; do [ -f "$A/$tag.$ext" ] && mv "$A/$tag.$ext" "$A/$tag.$ext.excluded"; done
    echo "$tag" >> "$E/raw/timing/RERUN_LIST.txt"
  fi
done
while read -r tag; do
  name=${tag#v1-all-t1-}; bin=$(bin_of "$name")
  until "$R/tools/env_gate.sh" > "$A/$tag.gate" 2>&1; do sleep 30; done
  "$R/tools/env_snapshot.sh" "$A/$tag.env" pre; date -Iseconds > "$A/$tag.start"
  /usr/bin/time -l "$bin" "$W/profiling/golden-v2-novel/v1-all.tsv" 19 "$W/profiling/golden-v2-novel/novel.tsv" > "$A/$tag.stdout" 2> "$A/$tag.stderr"; echo "exit=$?" >> "$A/$tag.stderr"
  date -Iseconds > "$A/$tag.end"; "$R/tools/env_snapshot.sh" "$A/$tag.env" post; sleep 8
done < "$E/raw/timing/RERUN_LIST.txt"
echo "$(date -Iseconds) reran $(wc -l < "$E/raw/timing/RERUN_LIST.txt" | tr -d ' ') runs (overlapped 020 verification 02:28:34-02:33:11)" >> "$E/raw/timing/RERUNS.txt"
python3 "$R/tools/analyze_quiet.py" "$A" baseline > "$E/raw/timing/A-summary.md" 2>&1
