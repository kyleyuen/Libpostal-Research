#!/bin/bash
# Cross-agent cap on concurrent model-loading processes (each ~2.5 GB; 24 GB machine). bash 3.2, mkdir locks, PID-owned.
# usage: research/tools/model_slot.sh <command...>   — waits for one of $SLOTS slots, runs the command, frees the slot.
# ponytail: polling every 5 s; a queue is not needed for 3 agents.
SLOTS=${MODEL_SLOTS:-3}; L=/tmp/libpostal-model-slots; mkdir -p "$L"
while :; do
  for i in $(seq 1 $SLOTS); do
    s="$L/slot$i"
    if [ -d "$s" ] && ! kill -0 "$(cat "$s/pid" 2>/dev/null)" 2>/dev/null; then rm -rf "$s"; fi   # stale: owner gone
    if mkdir "$s" 2>/dev/null; then echo $$ > "$s/pid"; trap 'rm -rf "$s"' EXIT; "$@"; exit $?; fi
  done; sleep 5
done
