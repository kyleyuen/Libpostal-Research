#!/bin/bash
# Run one command and retain its exact argv, timestamps, stdout, stderr, and exit code.
set -u
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/025-audit-next"
name=$1; shift
mkdir -p "$E/raw/commands"
meta="$E/raw/commands/$name.meta"
{
  printf 'start=%s\n' "$(date -Iseconds)"
  printf 'cwd=%s\n' "$PWD"
  printf 'argv:'; printf ' <%q>' "$@"; printf '\n'
} > "$meta"
"$@" > "$E/raw/commands/$name.stdout" 2> "$E/raw/commands/$name.stderr"
code=$?
printf 'exit=%s\nend=%s\n' "$code" "$(date -Iseconds)" >> "$meta"
exit "$code"
