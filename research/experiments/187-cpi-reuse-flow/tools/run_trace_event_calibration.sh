#!/bin/bash
set -eo pipefail
ROOT="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
EXP="$ROOT/research/experiments/187-cpi-reuse-flow"
RUN_ID="${1:?run id required}"
ART="$EXP/artifacts/calibration-$RUN_ID"
RAW="$EXP/raw/calibration-$RUN_ID"
[[ ! -e "$ART" && ! -e "$RAW" ]] || { echo "Run ID already exists" >&2; exit 2; }
mkdir -p "$ART" "$RAW/commands"
printf 'run_id=%s\nstart=%s\n' "$RUN_ID" "$(date '+%Y-%m-%dT%H:%M:%S%z')" > "$RAW/session.txt"
finish() { local status=$?; trap - EXIT; printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$RAW/session.txt"; exit "$status"; }
trap finish EXIT
{
  printf 'start=%s\ncwd=%q\nargv=' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$ROOT"
  printf '%q ' "$(command -v clang)" -std=gnu2x -Wall -Wextra -DEXP185_DIAG "-I$EXP/source" "$EXP/source/diag.c" "$EXP/tools/calibrate_trace_events.c" -o "$ART/calibrate_trace_events"
  printf '\n'
} > "$RAW/compile.command"
compile_status=0
"$(command -v clang)" -std=gnu2x -Wall -Wextra -DEXP185_DIAG -I"$EXP/source" \
  "$EXP/source/diag.c" "$EXP/tools/calibrate_trace_events.c" -o "$ART/calibrate_trace_events" \
  > "$RAW/compile.stdout" 2> "$RAW/compile.stderr" || compile_status=$?
printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$compile_status" >> "$RAW/compile.command"
[[ "$compile_status" -eq 0 ]] || exit "$compile_status"
{
  printf 'start=%s\ncwd=%q\nargv=' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$ROOT"
  printf '%q ' "$ART/calibrate_trace_events" "$RAW/events.jsonl"
  printf '\n'
} > "$RAW/run.command"
run_status=0
"$ART/calibrate_trace_events" "$RAW/events.jsonl" > "$RAW/run.stdout" 2> "$RAW/run.stderr" || run_status=$?
printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$run_status" >> "$RAW/run.command"
printf 'exit=%s\n' "$run_status" > "$RAW/run.exit"
[[ "$run_status" -eq 0 ]] || exit "$run_status"
{
  printf 'start=%s\ncwd=%q\nargv=' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$ROOT"
  printf '%q ' "$(command -v python3)" "$EXP/tools/validate_trace_event_calibration.py" "$RAW/events.jsonl"
  printf '\n'
} > "$RAW/validate.command"
validate_status=0
"$(command -v python3)" "$EXP/tools/validate_trace_event_calibration.py" "$RAW/events.jsonl" \
  >> "$RAW/run.stdout" 2>> "$RAW/run.stderr" || validate_status=$?
printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$validate_status" >> "$RAW/validate.command"
printf 'exit=%s\n' "$validate_status" >> "$RAW/run.exit"
[[ "$validate_status" -eq 0 ]] || exit "$validate_status"
shasum -a 256 "$EXP/source/diag.c" "$EXP/source/cpi/diag.h" \
  "$EXP/tools/calibrate_trace_events.c" "$EXP/tools/validate_trace_event_calibration.py" \
  "$EXP/tools/run_trace_event_calibration.sh" \
  "$ART/calibrate_trace_events" "$RAW/events.jsonl" > "$RAW/hashes.sha256"
printf 'artifacts=%s\nrecords=%s\n' "$ART" "$RAW" >> "$RAW/session.txt"
printf 'Trace calibration artifacts: %s\nTrace calibration records: %s\n' "$ART" "$RAW"
