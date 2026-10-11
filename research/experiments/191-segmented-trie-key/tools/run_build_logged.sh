#!/bin/bash
set -u
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/191-segmented-trie-key"
name=build_release
printf '%s\n' '/bin/bash research/experiments/191-segmented-trie-key/tools/build_release.sh' > "$E/raw/$name.command"
date '+%Y-%m-%dT%H:%M:%S%z %Z' > "$E/raw/$name.start"
/bin/bash "$E/tools/build_release.sh" > "$E/raw/$name.stdout" 2> "$E/raw/$name.stderr"
rc=$?
printf '%s\n' "$rc" > "$E/raw/$name.exit"
date '+%Y-%m-%dT%H:%M:%S%z %Z' > "$E/raw/$name.end"
exit "$rc"
