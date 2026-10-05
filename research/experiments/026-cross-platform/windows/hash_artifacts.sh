#!/usr/bin/env bash
set -euo pipefail
R="$(cd "$(dirname "$0")/../../../.." && pwd)"
W=research/experiments/026-cross-platform/windows
cd "$R"
printf 'mimalloc_source_commit=%s\n' "$(git -C "$W/mimalloc" rev-parse HEAD)" > "$W/SOURCE_COMMITS.txt"
printf 'libpostal_source_commit=25099c506612b34b23b1bfe286ca6321fcf06f35\n' >> "$W/SOURCE_COMMITS.txt"
shasum -a 256 \
  research/final/021-l4/sources/* \
  "$W"/sources/* "$W"/r2_sources/* "$W"/src.Makefile.am "$W"/configure_candidate.sh \
  "$W"/upstream/src/Makefile.am "$W"/upstream/windows/configure.ac "$W"/upstream/configure.ac "$W"/candidate/configure.ac \
  "$W"/candidate/src/feature_dict.h "$W"/candidate/src/win_vm.c "$W"/candidate/src/win_vm.h "$W"/candidate/src/Makefile.am \
  "$W"/bench_ext_win.c "$W"/dump_outputs_win.c "$W"/inspect_binaries.sh \
  "$W"/make_port_patch.py "$W"/hash_artifacts.sh "$W"/SOURCE_COMMITS.txt "$W"/port.patch "$W"/README.md "$W"/review_r3.md \
  "$W"/port.patch.r2 "$W"/README.r2.md "$W"/SHA256SUMS.r5 \
  "$W"/artifacts/original/* "$W"/artifacts/l4/* "$W"/artifacts/l4_r2/* "$W"/artifacts/l4_r3/* \
  "$W"/mimalloc-build/mimalloc.obj \
  profiling/golden-v1/all.txt profiling/golden-v1/baseline-outputs.jsonl \
  profiling/golden-v2-novel/novel.txt profiling/golden-v2-novel/novel.tsv profiling/golden-v2-novel/v1-all.tsv \
  profiling/golden-v2-novel/baseline-outputs.jsonl profiling/manifest.json \
  > "$W/SHA256SUMS"
