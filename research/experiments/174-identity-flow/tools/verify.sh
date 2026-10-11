#!/bin/bash
# 174: paired output checks only; each model load is serialized by model_slot.sh.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/174-identity-flow"; O="$E/raw/verify-paired"
C="$E/artifacts/identity/dump_outputs"; R="$E/artifacts/original_dump_outputs"; SLOT="$W/research/tools/model_slot.sh"
mkdir -p "$O"; : > "$O/commands.txt"; : > "$O/summary.txt"
pair() { label=$1; input=$2
  printf '# %s original %q\n' "$(date -Iseconds)" "$input" >> "$O/commands.txt"
  "$SLOT" "$R" "$input" > "$O/$label.original.jsonl" 2> "$O/$label.original.stderr"; a=$?
  printf '# %s candidate %q\n' "$(date -Iseconds)" "$input" >> "$O/commands.txt"
  AP_IDENTITY_VERIFY=1 "$SLOT" "$C" "$input" > "$O/$label.candidate.jsonl" 2> "$O/$label.candidate.stderr"; b=$?
  cmp "$O/$label.original.jsonl" "$O/$label.candidate.jsonl"; c=$?
  printf '%s original_exit=%s candidate_exit=%s cmp_exit=%s lines=%s\n' "$label" "$a" "$b" "$c" "$(wc -l < "$O/$label.candidate.jsonl")" >> "$O/summary.txt"
  return $((a || b || c))
}
rc=0
pair development "$W/profiling/golden-v1/development.txt" || rc=1
pair golden "$W/profiling/golden-v1/all.txt" || rc=1
pair novel "$W/profiling/golden-v2-novel/novel.txt" || rc=1
pair osm "$W/profiling/golden-v3-osm/osm.txt" || rc=1
printf '# %s original fuzz\n' "$(date -Iseconds)" >> "$O/commands.txt"
"$SLOT" "$E/artifacts/original_fuzz_pipeline" 1 200000 "$W/profiling/golden-v1/development.txt" > "$O/fuzz.original.txt" 2> "$O/fuzz.original.stderr"; a=$?
printf '# %s candidate fuzz\n' "$(date -Iseconds)" >> "$O/commands.txt"
AP_IDENTITY_VERIFY=1 "$SLOT" "$E/artifacts/identity/fuzz_pipeline" 1 200000 "$W/profiling/golden-v1/development.txt" > "$O/fuzz.candidate.txt" 2> "$O/fuzz.candidate.stderr"; b=$?
cmp "$O/fuzz.original.txt" "$O/fuzz.candidate.txt"; c=$?
printf 'fuzz-200k original_exit=%s candidate_exit=%s cmp_exit=%s lines=%s\n' "$a" "$b" "$c" "$(wc -l < "$O/fuzz.candidate.txt")" >> "$O/summary.txt"
exit $((rc || a || b || c))
