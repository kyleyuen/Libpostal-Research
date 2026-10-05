#!/usr/bin/env bash
# Resume after the documented script-read interruption, without recompiling scanner.c.
set -euo pipefail
set -x
dest=/out/artifacts/baseline
src=/out/source/src
test -s "$dest/obj/scanner.o"
test ! -e "$dest/build.complete"
clang -std=gnu2x -DHAVE_CONFIG_H -Wall -Wno-unused-function \
  -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self \
  -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS '-DLIBPOSTAL_DATA_DIR="/model"' \
  -O1 -iquote "$src" -I/out/source -c "$src/klib/drand48.c" -o "$dest/obj/drand48.o"
llvm-ar rcs "$dest/libpostal.a" "$dest"/obj/*.o
clang -O2 -g -iquote "$src" /workspace/research/experiments/003-batched-lookups/tools/dump_outputs.c \
  "$dest/libpostal.a" -lm -o "$dest/dump_outputs"
clang -O2 -g -iquote "$src" /out/bench_ext_linux.c "$dest/libpostal.a" -lm -o "$dest/bench_ext"
sha256sum "$dest/libpostal.a" "$dest/dump_outputs" "$dest/bench_ext" > "$dest/binary_hashes.txt"
date -Is > "$dest/build.complete"
