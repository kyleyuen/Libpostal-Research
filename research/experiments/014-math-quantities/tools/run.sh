#!/bin/bash
# usage: run.sh NAME CMD...  -> raw/NAME.{cmd,stdout,stderr,meta}; exit code preserved. Captures exact command, start/end time and exit code.
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/014-math-quantities"; n=$1; shift
printf '%q ' "$@" > "$E/raw/$n.cmd"; echo >> "$E/raw/$n.cmd"
s=$(date '+%Y-%m-%dT%H:%M:%S%z'); "$@" > "$E/raw/$n.stdout" 2> "$E/raw/$n.stderr"; rc=$?
printf 'start=%s\nend=%s\nexit=%s\n' "$s" "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$rc" > "$E/raw/$n.meta"; exit $rc
