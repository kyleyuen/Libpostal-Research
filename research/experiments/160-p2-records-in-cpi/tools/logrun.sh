#!/bin/bash
# usage: tools/logrun.sh LOGNAME cmd...   — records command, start/end timestamps, stdout, stderr, exit code under raw/LOGNAME/
E="$(cd "$(dirname "$0")/.." && pwd)"; L="$1"; D="$E/raw/$1"; shift; mkdir -p "$D"
printf '%q ' "$@" > "$D/cmd.txt"; echo >> "$D/cmd.txt"; pwd > "$D/cwd.txt"
date '+%Y-%m-%dT%H:%M:%S%z' > "$D/start.txt"
"$@" > "$D/stdout.txt" 2> "$D/stderr.txt"; rc=$?
date '+%Y-%m-%dT%H:%M:%S%z' > "$D/end.txt"; echo $rc > "$D/exit.txt"; echo "$L exit $rc ($D)"; exit $rc
