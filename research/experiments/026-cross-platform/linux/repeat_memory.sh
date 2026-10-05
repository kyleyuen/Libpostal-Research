#!/usr/bin/env bash
# Two additional paired untimed memory checks; first pair is already captured.
set -euo pipefail
W="$(cd "$(dirname "$0")/../../../.." && pwd)"
E="$W/research/experiments/026-cross-platform"
L="$E/linux"
T="$W/research/tools"
run() {
  tag=$1
  variant=$2
  "$T/env_gate.sh" > "$L/runtime/$tag.host-gate" 2>&1
  "$T/env_snapshot.sh" "$L/runtime/$tag.host-env" pre
  bash "$E/tools/capture.sh" "linux_memory_$tag" \
    docker run --rm --platform linux/arm64 \
    -v "$W:/workspace:ro" -v "$L:/out" \
    -v /Users/kyle/.cache/libpostal-profile/libpostal:/model:ro \
    libpostal-audit-linux-arm64:026 bash /out/check_runtime.sh \
    "$variant" /workspace/profiling/golden-v2-novel/novel.tsv 120 glibc "$tag"
  "$T/env_snapshot.sh" "$L/runtime/$tag.host-env" post
}
run t2_dense candidate_dense
run t2_upstream baseline_upstream
run t3_upstream baseline_upstream
run t3_dense candidate_dense
