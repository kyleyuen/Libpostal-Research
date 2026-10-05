#!/usr/bin/env bash
set -euo pipefail
W="$(cd "$(dirname "$0")/../../../../.." && pwd)"
X="$W/research/experiments/026-cross-platform/linux/x86"
if [ -e "$X/source" ]; then echo "x86 source copy exists" >&2; exit 2; fi
mkdir -p "$X/source"
tar -C "$W/libpostal" --exclude=.git --exclude='*.o' --exclude='*.lo' \
  --exclude='.libs' --exclude='*.dylib' -cf - . | tar -C "$X/source" -xf -
git -C "$W/libpostal" rev-parse HEAD > "$X/source_commit.txt"
