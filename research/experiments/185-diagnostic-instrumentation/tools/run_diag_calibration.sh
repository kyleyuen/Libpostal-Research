#!/bin/bash
# Compile and run the standalone diagnostic counter fixtures; never loads libpostal.
set -eo pipefail
ROOT="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
EXP="$ROOT/research/experiments/185-diagnostic-instrumentation"
RUN_ID="${1:?run id required}"
ART="$EXP/artifacts/calibration-$RUN_ID"
RAW="$EXP/raw/calibration-$RUN_ID"
[[ ! -e "$ART" && ! -e "$RAW" ]] || { echo 'Run ID already exists' >&2; exit 2; }
mkdir -p "$ART" "$RAW/commands"
printf 'run_id=%s\nstart=%s\n' "$RUN_ID" "$(date '+%Y-%m-%dT%H:%M:%S%z')" > "$RAW/session.txt"
finish() { local status=$?; trap - EXIT; printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$RAW/session.txt"; exit "$status"; }
trap finish EXIT
run() {
  local name="$1" status=0 arg stem="$RAW/commands/$1"
  shift
  { printf 'start=%s\ncwd=%q\nargv=' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$ROOT"; for arg in "$@"; do printf '%q ' "$arg"; done; printf '\n'; } > "$stem.command"
  (cd "$ROOT" && "$@" > "$stem.stdout" 2> "$stem.stderr") || status=$?
  printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$stem.command"
  printf '%s %s\n' "$stem" "$status" >> "$RAW/exit_codes.txt"
  [[ "$status" -eq 0 ]] || exit "$status"
}
run compile "$(command -v clang)" -std=gnu2x -Wall -Wextra -DEXP185_DIAG \
  -I"$EXP/source/original" "$EXP/source/diag.c" "$EXP/tools/calibrate_diag.c" -o "$ART/calibrate_diag"
for mode in basic query-overflow pattern-overflow nested-labels label-too-long label-pool-full; do
  run "$mode" "$ART/calibrate_diag" "$mode" "$RAW/$mode.json"
done
run validate "$(command -v python3)" "$EXP/tools/validate_diag_calibration.py" \
  "$RAW/basic.json" "$RAW/query-overflow.json" "$RAW/pattern-overflow.json" "$RAW/nested-labels.json" \
  "$RAW/commands/label-too-long.stderr" "$RAW/commands/label-pool-full.stderr"
shasum -a 256 "$EXP/source/diag.c" "$EXP/tools/calibrate_diag.c" \
  "$EXP/tools/validate_diag_calibration.py" "$EXP/tools/run_diag_calibration.sh" \
  "$ART/calibrate_diag" "$RAW/basic.json" "$RAW/query-overflow.json" "$RAW/pattern-overflow.json" \
  "$RAW/nested-labels.json" "$RAW/commands/label-too-long.stderr" \
  "$RAW/commands/label-pool-full.stderr" > "$RAW/hashes.sha256"
printf 'artifacts=%s\nrecords=%s\n' "$ART" "$RAW" >> "$RAW/session.txt"
printf 'Calibration artifacts: %s\nCalibration records: %s\n' "$ART" "$RAW"
