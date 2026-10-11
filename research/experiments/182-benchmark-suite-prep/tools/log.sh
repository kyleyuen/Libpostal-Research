#!/bin/bash
# Capture argv, local timestamps, streams, and exit status for non-benchmark prep.
set -u
E="$(cd "$(dirname "$0")/.." && pwd)"
NAME=$1; shift
RAW="$E/raw"
date '+%Y-%m-%dT%H:%M:%S%z' > "$RAW/$NAME.start"
printf '%q ' "$@" > "$RAW/$NAME.command"; printf '\n' >> "$RAW/$NAME.command"
"$@" > "$RAW/$NAME.stdout" 2> "$RAW/$NAME.stderr"
rc=$?
printf '%s\n' "$rc" > "$RAW/$NAME.exit"
date '+%Y-%m-%dT%H:%M:%S%z' > "$RAW/$NAME.end"
exit "$rc"
