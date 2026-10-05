#!/usr/bin/env bash
# Link a read-only model-name inspector against the preserved 021 l4 library.
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/028-name-dispatch"
D="$E/artifacts"
if [ -e "$D/model_names" ]; then echo "model_names already exists" >&2; exit 2; fi
mkdir -p "$D"
clang -O2 -flto -DHAVE_CONFIG_H -DEXP_INT_KEYS -DEXP_FD_INLINE \
    '-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"' \
    -I"$E/sources" -I"$W/libpostal/src" -I"$W/libpostal" \
    "$E/tools/model_names.c" /opt/homebrew/opt/mimalloc/lib/mimalloc.o \
    "$W/research/experiments/021-final-stack/artifacts/l4/libpostal.a" -lm \
    -o "$D/model_names"
shasum -a 256 "$D/model_names" "$W/research/experiments/021-final-stack/artifacts/l4/libpostal.a" "$E/tools/model_names.c"
