#!/bin/bash
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/025-audit-next"
C="$E/tools/capture.sh"
"$W/research/tools/env_gate.sh" > "$E/raw/verify_gate.txt"
for v in control fixes borrow; do
    bin="$E/artifacts/$v/dump_outputs"
    "$C" "${v}_v1_r1" "$bin" "$W/profiling/golden-v1/all.txt"
    "$C" "${v}_novel_r1" "$bin" "$W/profiling/golden-v2-novel/novel.txt"
    "$C" "${v}_osm_r1" "$bin" "$W/profiling/golden-v3-osm/osm.txt"
    "$C" "${v}_frozen_v1_r1" python3 "$W/research/experiments/015-integer-keys/tools/compare_frozen.py" "$E/raw/commands/${v}_v1_r1.stdout" "$W/profiling/golden-v1/baseline-outputs.jsonl"
    "$C" "${v}_frozen_novel_r1" python3 "$W/research/experiments/015-integer-keys/tools/compare_frozen.py" "$E/raw/commands/${v}_novel_r1.stdout" "$W/profiling/golden-v2-novel/baseline-outputs.jsonl"
    "$C" "${v}_cmp_v1_r1" cmp "$E/raw/commands/${v}_v1_r1.stdout" "$E/raw/commands/l4_dump_v1.stdout"
    "$C" "${v}_cmp_novel_r1" cmp "$E/raw/commands/${v}_novel_r1.stdout" "$E/raw/commands/l4_dump_novel.stdout"
    "$C" "${v}_cmp_osm_r1" cmp "$E/raw/commands/${v}_osm_r1.stdout" "$W/research/experiments/017-loadtime-precompute/raw/v3check/outputs_baseline.jsonl"
done
for seed in 1 2; do
    "$C" "borrow_fuzz_${seed}_r1" "$E/artifacts/borrow/fuzz_pipeline" "$seed" 2000000 "$W/profiling/golden-v1/all.txt" "$W/profiling/golden-v2-novel/novel.txt"
    "$C" "borrow_fuzz_cmp_${seed}_r1" cmp "$E/raw/commands/borrow_fuzz_${seed}_r1.stdout" "$W/research/experiments/011-normalize-fastpaths/raw/fuzz/base_s$seed.txt"
done
date -Iseconds > "$E/raw/VERIFY_DONE"
