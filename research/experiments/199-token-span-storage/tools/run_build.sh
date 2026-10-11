#!/bin/bash
set +e
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/199-token-span-storage"
date '+%Y-%m-%d %H:%M:%S %Z %z' > "$E/raw/build.start"
EXTRA_SW="$(cat /Users/kyle/Desktop/HW/CSCI\ 201\ Reserach/research/experiments/119-component-phrase-ids/raw/switches_cpi.txt) -DEXP_TOKEN_SLICES"
export EXTRA_SW
/bin/bash "$E/tools/build.sh" release clang none '-O2 -flto' '-O0' '-flto' > "$E/raw/build.stdout" 2> "$E/raw/build.stderr"
rc=$?
printf '%s\n' "$rc" > "$E/raw/build.exit"
date '+%Y-%m-%d %H:%M:%S %Z %z' > "$E/raw/build.end"
exit "$rc"
