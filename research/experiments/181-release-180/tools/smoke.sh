#!/bin/bash
# Correctness and live-thread smoke only; deliberately contains no timer.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/181-release-180"
R="${RAW_DIR:-$E/raw}"
mkdir -p "$R"
S="$W/research/tools/model_slot.sh"
ORIG="$E/artifacts/original_dump_outputs"
CAND="$E/artifacts/release/dump_outputs"
run() {
  local n=$1 x
  shift
  date -Iseconds > "$R/$n.start"
  printf '%q ' "$@" > "$R/$n.command"; printf '\n' >> "$R/$n.command"
  "$@" > "$R/$n.stdout" 2> "$R/$n.stderr"; x=$?
  printf '%s\n' "$x" > "$R/$n.exit"; date -Iseconds > "$R/$n.end"
  return 0
}
for d in golden-v1:profiling/golden-v1/all.txt novel:profiling/golden-v2-novel/novel.txt osm-v3:profiling/golden-v3-osm/osm.txt; do
  n=${d%%:*}; p="$W/${d#*:}"
  run "original_$n" "$S" "$ORIG" "$p"
  run "release_$n" "$S" "$CAND" "$p"
  cmp "$R/original_$n.stdout" "$R/release_$n.stdout" > "$R/cmp_$n.stdout" 2> "$R/cmp_$n.stderr"; printf '%s\n' "$?" > "$R/cmp_$n.exit"
done
date -Iseconds > "$R/thread.start"
printf '%q ' "$S" "$CAND" "$W/profiling/golden-v1/all.txt" > "$R/thread.command"; printf '\n' >> "$R/thread.command"
"$S" "$CAND" "$W/profiling/golden-v1/all.txt" > "$R/thread.stdout" 2> "$R/thread.stderr" & slotpid=$!
sleep 1
child=$(pgrep -P "$slotpid" | head -1)
if [ -n "$child" ]; then ps -M -o pid,tid,command -p "$child" > "$R/thread.psM.txt" 2> "$R/thread.psM.stderr"; else echo 'candidate child not observed' > "$R/thread.psM.txt"; fi
wait "$slotpid"; x=$?; printf '%s\n' "$x" > "$R/thread.exit"; date -Iseconds > "$R/thread.end"
