#!/usr/bin/env bash
# Paired original/dense memory checks on golden-v1/all and OSM, untimed.
set -euo pipefail
W="$(cd "$(dirname "$0")/../../../.." && pwd)"
E="$W/research/experiments/026-cross-platform"
L="$E/linux"
T="$W/research/tools"
run() {
  tag=$1 variant=$2 input=$3
  "$T/env_gate.sh" > "$L/runtime/$tag.host-gate" 2>&1
  "$T/env_snapshot.sh" "$L/runtime/$tag.host-env" pre
  bash "$E/tools/capture.sh" "linux_memory_$tag" \
    docker run --rm --platform linux/arm64 \
    -v "$W:/workspace:ro" -v "$L:/out" \
    -v /Users/kyle/.cache/libpostal-profile/libpostal:/model:ro \
    libpostal-audit-linux-arm64:026 bash /out/check_runtime.sh \
    "$variant" "$input" 120 glibc "$tag"
  "$T/env_snapshot.sh" "$L/runtime/$tag.host-env" post
}
run v1_upstream baseline_upstream /workspace/profiling/golden-v2-novel/v1-all.tsv
run v1_dense candidate_dense /workspace/profiling/golden-v2-novel/v1-all.tsv
run osm_dense candidate_dense /workspace/profiling/golden-v3-osm/osm.tsv
run osm_upstream baseline_upstream /workspace/profiling/golden-v3-osm/osm.tsv
