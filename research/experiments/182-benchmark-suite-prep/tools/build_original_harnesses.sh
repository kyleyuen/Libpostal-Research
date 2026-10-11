#!/bin/bash
# Link suite harnesses to the immutable experiment-000 original archive.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
E="$ROOT/research/experiments/182-benchmark-suite-prep"
SRC="$ROOT/libpostal/src"
LIB="$ROOT/research/experiments/000-baseline/artifacts/libpostal.a"
OUT="$E/artifacts/original-000"
mkdir -p "$OUT"
test "$(git -C "$ROOT/libpostal" rev-parse HEAD)" = 25099c506612b34b23b1bfe286ca6321fcf06f35
test -f "$LIB"
for h in "$ROOT/research/tools/bench_ext.c" "$ROOT/research/experiments/003-batched-lookups/tools/dump_outputs.c" "$ROOT/research/tools/fuzz_pipeline.c"; do
  n="$(basename "$h" .c)"
  clang -O2 -g -I"$SRC" "$h" "$LIB" -lm -o "$OUT/$n"
done
(cd "$OUT" && shasum -a 256 bench_ext dump_outputs fuzz_pipeline > hashes.sha256)
