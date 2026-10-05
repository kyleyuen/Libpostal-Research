#!/usr/bin/env bash
# Functional checks only. This script intentionally does not time parsing.
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/027-root-transition"
C="$E/tools/capture.sh"
MODEL="$HOME/.cache/libpostal-profile/libpostal/address_parser"
DEV="$W/research/experiments/025-audit-next/data/development.tsv"
"$C" cache_root_check "$E/artifacts/cache_r2/check_root_cache" "$MODEL/address_parser_phrases.dat" "$MODEL/address_parser_postal_codes.dat" "$DEV"
set +e
"$C" cache_root_negative "$E/artifacts/cache_r2/check_root_cache" "$MODEL/address_parser_phrases.dat" "$MODEL/address_parser_postal_codes.dat" "$DEV" --negative
negative_rc=$?
set -e
if [ "$negative_rc" -ne 1 ]; then echo "negative control returned $negative_rc; expected 1" >&2; exit 1; fi
for v in control_r2 cache_r2; do
    bin="$E/artifacts/$v/dump_outputs"
    "$C" "${v}_v1" "$bin" "$W/profiling/golden-v1/all.txt"
    "$C" "${v}_novel" "$bin" "$W/profiling/golden-v2-novel/novel.txt"
    "$C" "${v}_osm" "$bin" "$W/profiling/golden-v3-osm/osm.txt"
    "$C" "${v}_frozen_v1" python3 "$W/research/experiments/015-integer-keys/tools/compare_frozen.py" "$E/raw/commands/${v}_v1.stdout" "$W/profiling/golden-v1/baseline-outputs.jsonl"
    "$C" "${v}_frozen_novel" python3 "$W/research/experiments/015-integer-keys/tools/compare_frozen.py" "$E/raw/commands/${v}_novel.stdout" "$W/profiling/golden-v2-novel/baseline-outputs.jsonl"
    "$C" "${v}_cmp_osm" cmp "$E/raw/commands/${v}_osm.stdout" "$W/research/experiments/017-loadtime-precompute/raw/v3check/outputs_baseline.jsonl"
done
for d in v1 novel osm; do
    "$C" "cache_cmp_control_${d}" cmp "$E/raw/commands/cache_r2_${d}.stdout" "$E/raw/commands/control_r2_${d}.stdout"
done
"$C" cache_fuzz_s1 "$E/artifacts/cache_r2/fuzz_pipeline" 1 2000000 "$W/profiling/golden-v1/all.txt" "$W/profiling/golden-v2-novel/novel.txt"
"$C" cache_fuzz_cmp_s1 cmp "$E/raw/commands/cache_fuzz_s1.stdout" "$W/research/experiments/011-normalize-fastpaths/raw/fuzz/base_s1.txt"
date -Iseconds > "$E/raw/VERIFY_DONE"
