#!/bin/bash
set +e
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/198-certified-viterbi"
date '+%Y-%m-%d %H:%M:%S %Z %z' > "$E/raw/build-v3.start"
EXTRA_SW="$(cat /Users/kyle/Desktop/HW/CSCI\ 201\ Reserach/research/experiments/119-component-phrase-ids/raw/switches_cpi.txt)"
export EXTRA_SW
SRCDIR=sources-v3 /bin/bash "$E/tools/build.sh" release-v3 clang none '-O2 -ffp-contract=off -fno-fast-math' '-O0' '' > "$E/raw/build-v3.stdout" 2> "$E/raw/build-v3.stderr"
rc=$?
printf '%s\n' "$rc" > "$E/raw/build-v3.exit"
date '+%Y-%m-%d %H:%M:%S %Z %z' > "$E/raw/build-v3.end"
exit "$rc"
