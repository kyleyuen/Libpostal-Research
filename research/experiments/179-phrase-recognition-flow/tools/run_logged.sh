#!/bin/bash
# Capture one RTK-prefixed command with timestamps, exact argv, stdout/stderr and exit.
set -u
ROOT="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
RAW="$ROOT/research/experiments/179-phrase-recognition-flow/raw"
RTK="/Users/kyle/Library/Application Support/Headroom/headroom/bin/rtk"
name=$1
shift
date '+%Y-%m-%dT%H:%M:%S%z' > "$RAW/$name.start"
{
    printf '%q ' "$RTK" "$@"
    printf '\n'
} > "$RAW/$name.command"
"$RTK" "$@" > "$RAW/$name.stdout" 2> "$RAW/$name.stderr"
status=$?
printf '%s\n' "$status" > "$RAW/$name.exit"
date '+%Y-%m-%dT%H:%M:%S%z' > "$RAW/$name.end"
exit "$status"
