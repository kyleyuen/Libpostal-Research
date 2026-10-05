#!/usr/bin/env bash
set -euo pipefail
pid=${1:?runner PID required}
out="$(cd "$(dirname "$0")" && pwd)/timing-dense-3x150"
while :; do
  complete=1
  for variant in upstream candidate dense; do
    grep -q '^exit=0$' "$out/v1-all-t3-$variant.meta" 2>/dev/null || complete=0
  done
  if [ "$complete" -eq 1 ]; then
    printf 'stop=%s\nreason=user must close laptop; stopped at completed golden-v1 trial boundary before novel timing\n' "$(date -Iseconds)" > "$out/STOPPED"
    kill -TERM "$pid"
    exit 0
  fi
  sleep 1
done
