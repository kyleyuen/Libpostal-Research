#!/usr/bin/env bash
set -euo pipefail
W="$(cd "$(dirname "$0")/../../../.." && pwd)"
L="$W/research/experiments/026-cross-platform/linux"
if [ -e "$L/source" ]; then
  echo "source copy already exists" >&2
  exit 2
fi
mkdir -p "$L/source"
tar -C "$W/libpostal" --exclude=.git --exclude='*.o' --exclude='*.lo' \
  --exclude='.libs' --exclude='*.dylib' -cf - . | tar -C "$L/source" -xf -
git -C "$W/libpostal" rev-parse HEAD > "$L/source_commit.txt"
