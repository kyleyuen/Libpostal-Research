#!/usr/bin/env bash
# Functional output only under x86-64 Docker emulation; no timing or memory claim.
set -euo pipefail
W="$(cd "$(dirname "$0")/../../../../.." && pwd)"
E="$W/research/experiments/026-cross-platform"
L="$E/linux"
capture="$E/tools/capture.sh"
for variant in upstream candidate; do
  for dataset in v1 novel osm; do
    case "$dataset" in
      v1) input=profiling/golden-v1/all.txt ;;
      novel) input=profiling/golden-v2-novel/novel.txt ;;
      osm) input=profiling/golden-v3-osm/osm.txt ;;
    esac
    bash "$capture" "linux_x86_dump_${variant}_${dataset}" \
      docker run --rm --platform linux/amd64 \
      -v "$W:/workspace:ro" -v "$L:/out:ro" \
      -v /Users/kyle/.cache/libpostal-profile/libpostal:/model:ro \
      libpostal-audit-linux-amd64:026 \
      "/out/x86/artifacts/$variant/dump_outputs" "/workspace/$input"
  done
done
for dataset in v1 novel osm; do
  bash "$capture" "linux_x86_cmp_${dataset}" cmp \
    "$E/raw/commands/linux_x86_dump_upstream_${dataset}.stdout" \
    "$E/raw/commands/linux_x86_dump_candidate_${dataset}.stdout"
done
bash "$capture" linux_x86_frozen_v1 python3 \
  "$W/research/experiments/015-integer-keys/tools/compare_frozen.py" \
  "$E/raw/commands/linux_x86_dump_candidate_v1.stdout" \
  "$W/profiling/golden-v1/baseline-outputs.jsonl"
bash "$capture" linux_x86_frozen_novel python3 \
  "$W/research/experiments/015-integer-keys/tools/compare_frozen.py" \
  "$E/raw/commands/linux_x86_dump_candidate_novel.stdout" \
  "$W/profiling/golden-v2-novel/baseline-outputs.jsonl"
