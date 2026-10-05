#!/usr/bin/env bash
# Capture one command without overwriting a prior attempt.
set -u
E="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
name=$1; shift
mkdir -p "$E/raw/commands"
meta="$E/raw/commands/$name.meta"
if [ -e "$meta" ]; then echo "capture name already exists: $name" >&2; exit 2; fi
{
  printf 'start=%s\n' "$(date -Iseconds)"
  printf 'cwd=%s\n' "$PWD"
  printf 'argv:'; printf ' <%q>' "$@"; printf '\n'
} > "$meta"
"$@" > "$E/raw/commands/$name.stdout" 2> "$E/raw/commands/$name.stderr"
code=$?
printf 'exit=%s\nend=%s\n' "$code" "$(date -Iseconds)" >> "$meta"
exit "$code"
