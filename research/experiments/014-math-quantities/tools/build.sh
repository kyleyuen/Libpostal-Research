#!/bin/bash
# Builds the 014 measurement tools against the PRESERVED baseline library (no library source is compiled or changed).
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/014-math-quantities"; SRC="$W/libpostal/src"
clang -O2 -g -Wall -Wno-unused-function -Wno-#warnings -Wno-visibility "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I"$E/tools" -I"$SRC" -I"$W/libpostal" "$E/tools/trace.c" "$W/research/experiments/000-baseline/artifacts/libpostal.a" -o "$E/artifacts/trace"
clang -O2 -g -Wall "$E/tools/chase.c" -o "$E/artifacts/chase" 2>/dev/null || echo "chase.c not built (not written yet)"
( cd "$E" && shasum -a 256 tools/trace.c tools/feature_dict.h artifacts/trace "$W/research/experiments/000-baseline/artifacts/libpostal.a" > artifacts/hashes.txt; [ -f artifacts/chase ] && shasum -a 256 tools/chase.c artifacts/chase >> artifacts/hashes.txt; clang --version | head -1 >> artifacts/hashes.txt )
rm -rf "$E"/artifacts/*.dSYM
