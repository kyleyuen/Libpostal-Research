#!/bin/bash
set +e
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/198-certified-viterbi"
date '+%Y-%m-%d %H:%M:%S %Z %z' > "$E/raw/build-v2.start"
EXTRA_SW="$(cat /Users/kyle/Desktop/HW/CSCI\ 201\ Reserach/research/experiments/119-component-phrase-ids/raw/switches_cpi.txt)"
export EXTRA_SW
SRCDIR=sources-v2 /bin/bash "$E/tools/build.sh" release-v2 clang none '-O2 -ffp-contract=off -fno-fast-math' '-O0' '' > "$E/raw/build-v2.stdout" 2> "$E/raw/build-v2.stderr"
rc=$?
printf '%s\n' "$rc" > "$E/raw/build-v2.exit"
date '+%Y-%m-%d %H:%M:%S %Z %z' > "$E/raw/build-v2.end"
exit "$rc"
