#!/usr/bin/env bash
# Diagnostic profile only. Do not overlap with a timing session.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
OUT="$ROOT/research/experiments/026-cross-platform/profile"
BIN="$ROOT/research/experiments/021-final-stack/artifacts/l4/bench_ext"
INPUT="$ROOT/research/experiments/025-audit-next/data/development.tsv"

"$ROOT/research/tools/env_gate.sh" > "$OUT/gate.txt" 2>&1
gate_status=$?
if [ "$gate_status" -ne 0 ]; then
  printf 'gate_exit=%s\n' "$gate_status" >&2
  exit "$gate_status"
fi
"$ROOT/research/tools/env_snapshot.sh" "$OUT/environment.txt" before

printf 'binary=%s\ninput=%s\npasses=500\nsample_delay_seconds=20\nsample_duration_seconds=15\nsample_interval_ms=1\n' "$BIN" "$INPUT" > "$OUT/parameters.txt"
shasum -a 256 "$BIN" "$INPUT" > "$OUT/hashes.txt"
printf 'benchmark_start=%s\n' "$(date -Iseconds)" >> "$OUT/parameters.txt"
"$BIN" "$INPUT" 500 > "$OUT/bench.stdout" 2> "$OUT/bench.stderr" &
bench_pid=$!
caffeinate -i -w "$bench_pid" > "$OUT/caffeinate.stdout" 2> "$OUT/caffeinate.stderr" &
caffeinate_pid=$!
printf 'benchmark_pid=%s\ncaffeinate_pid=%s\n' "$bench_pid" "$caffeinate_pid" >> "$OUT/parameters.txt"

sleep 20
printf 'sample_start=%s\n' "$(date -Iseconds)" >> "$OUT/parameters.txt"
/usr/bin/sample "$bench_pid" 15 1 -file "$OUT/parser.sample.txt" > "$OUT/sample.stdout" 2> "$OUT/sample.stderr"
sample_status=$?
printf 'sample_exit=%s\nsample_end=%s\n' "$sample_status" "$(date -Iseconds)" >> "$OUT/parameters.txt"
wait "$bench_pid"
bench_status=$?
wait "$caffeinate_pid"
caffeinate_status=$?
printf 'benchmark_exit=%s\nbenchmark_end=%s\ncaffeinate_exit=%s\n' "$bench_status" "$(date -Iseconds)" "$caffeinate_status" >> "$OUT/parameters.txt"
"$ROOT/research/tools/env_snapshot.sh" "$OUT/environment.txt" after

if [ "$sample_status" -ne 0 ]; then exit "$sample_status"; fi
exit "$bench_status"
