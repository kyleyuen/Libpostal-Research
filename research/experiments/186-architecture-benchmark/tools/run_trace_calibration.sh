#!/bin/bash
set -eo pipefail
ROOT="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
EXP="$ROOT/research/experiments/186-architecture-benchmark"
RUN_ID="${1:-initial}"
ART="$EXP/artifacts/calibration-$RUN_ID"
RAW="$EXP/raw/calibration-$RUN_ID"
[[ ! -e "$ART" && ! -e "$RAW" ]] || { printf 'Calibration ID exists: %s\n' "$RUN_ID" >&2; exit 2; }
mkdir -p "$ART" "$RAW"
printf 'run_id=%s\nstart=%s\n' "$RUN_ID" "$(date '+%Y-%m-%dT%H:%M:%S%z')" > "$RAW/session.txt"
finish() { local rc=$?; trap - EXIT; printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$rc" >> "$RAW/session.txt"; exit "$rc"; }
trap finish EXIT
run() {
  local name="$1" status=0 arg
  shift
  local stem="$RAW/$name"
  { printf 'start=%s\ncwd=%q\nargv=' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$ROOT"; for arg in "$@"; do printf '%q ' "$arg"; done; printf '\n'; } > "$stem.command"
  (cd "$ROOT" && "$@" > "$stem.stdout" 2> "$stem.stderr") || status=$?
  printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$stem.command"
  printf '%s %s\n' "$stem" "$status" >> "$RAW/exit_codes.txt"
  [[ $status -eq 0 ]]
}
run_expected() {
  local name="$1" expected_status="$2" status=0 arg
  shift 2
  local stem="$RAW/$name"
  { printf 'start=%s\ncwd=%q\nargv=' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$ROOT"; for arg in "$@"; do printf '%q ' "$arg"; done; printf '\n'; } > "$stem.command"
  (cd "$ROOT" && "$@" > "$stem.stdout" 2> "$stem.stderr") || status=$?
  printf 'end=%s\nexit=%s\nexpected_exit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" "$expected_status" >> "$stem.command"
  printf '%s %s expected=%s\n' "$stem" "$status" "$expected_status" >> "$RAW/exit_codes.txt"
  [[ $status -eq $expected_status ]]
}
run compile "$(command -v clang)" -std=gnu2x -Wall -Wextra -Wformat -Werror=format-security -DEXP185_DIAG -I"$EXP/source/original" "$EXP/source/diag.c" "$EXP/tools/calibrate_trace.c" -o "$ART/calibrate_trace"
run emit env TRACE_JSONL="$RAW/fixture.jsonl" TRACE_CASE_ID=instrumented-known-fixture TRACE_EXPECTED_ADDRESSES=1 TRACE_EVENT_CAP=16 "$ART/calibrate_trace"
run validate "$(command -v python3)" "$EXP/tools/validate_trace_calibration.py" "$RAW/fixture.jsonl"
run_expected emit-overflow 1 env TRACE_JSONL="$RAW/overflow.jsonl" TRACE_CASE_ID=instrumented-overflow-fixture TRACE_EXPECTED_ADDRESSES=1 TRACE_EVENT_CAP=2 "$ART/calibrate_trace"
run validate-overflow "$(command -v python3)" "$EXP/tools/validate_trace_calibration.py" "$RAW/overflow.jsonl" overflow
shasum -a 256 "$EXP/source/diag.c" "$EXP/source/original/diag.h" "$EXP/tools/calibrate_trace.c" "$EXP/tools/validate_trace_calibration.py" "$EXP/tools/run_trace_calibration.sh" "$ART/calibrate_trace" "$RAW/fixture.jsonl" "$RAW/overflow.jsonl" > "$RAW/hashes.sha256"
printf 'artifact=%s/calibrate_trace\ntrace=%s/fixture.jsonl\n' "$ART" "$RAW" >> "$RAW/session.txt"
