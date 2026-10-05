#!/usr/bin/env bash
# Relink the exact 021 Linux candidate with mimalloc 3.5.3's override object.
set -euo pipefail
set -x
src=/out/source/src
dest=/out/artifacts/candidate_mi3
test ! -e "$dest/build.complete"
mkdir -p "$dest"
cp /out/artifacts/candidate/libpostal.a "$dest/libpostal.a"
obj=/out/mimalloc-build/mimalloc.o
test -s "$obj"
clang -O2 -g -iquote "$src" "$obj" \
  /workspace/research/experiments/003-batched-lookups/tools/dump_outputs.c \
  "$dest/libpostal.a" -lm -pthread -o "$dest/dump_outputs"
clang -O2 -g -iquote "$src" "$obj" /out/bench_ext_linux.c \
  "$dest/libpostal.a" -lm -pthread -o "$dest/bench_ext"
sha256sum "$obj" "$dest/libpostal.a" "$dest/dump_outputs" "$dest/bench_ext" \
  > "$dest/binary_hashes.txt"
printf '%s\n' 'source=021 l4 Linux candidate archive' \
  'allocator=mimalloc 3.5.3 static override object, clang Release build' \
  'link=-O2 -g -pthread -lm' > "$dest/build_settings.txt"
date -Is > "$dest/build.complete"
