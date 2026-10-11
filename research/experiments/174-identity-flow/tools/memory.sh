#!/bin/bash
# 174: bounded memory observation only; /usr/bin/time -l supplies process peak.
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/174-identity-flow"; O="$E/raw/memory"; SLOT="$W/research/tools/model_slot.sh"
mkdir -p "$O"; : > "$O/commands.txt"; : > "$O/summary.txt"
run() { label=$1; bin=$2; out="$O/$label"
  printf '# %s /usr/bin/time -l model_slot %q memory_hold\n' "$(date -Iseconds)" "$bin" >> "$O/commands.txt"
  /usr/bin/time -l "$SLOT" "$bin" "123 Main St, Seattle WA 98101" 20 > "$out.stdout" 2> "$out.stderr" & wrap=$!
  pid=""; for i in $(seq 1 35); do pid=$(sed -n 's/.*READY pid=\([0-9][0-9]*\).*/\1/p' "$out.stderr" | tail -1); [ -n "$pid" ] && break; sleep 1; done
  if [ -n "$pid" ]; then
    ps -M -p "$pid" > "$out.psM.txt" 2>&1
    { echo "== $label $(date -Iseconds) pid=$pid"; footprint -p "$pid" 2>&1 | grep -E 'Footprint|phys_footprint'; vmmap --summary "$pid" 2>&1 | grep -E '^mapped file|^TOTAL|Physical footprint'; } > "$out.steady.txt"
  fi
  wait "$wrap"; rc=$?
  printf '%s exit=%s pid=%s\n' "$label" "$rc" "${pid:-missing}" >> "$O/summary.txt"
  return "$rc"
}
rc=0
run original "$E/artifacts/original_memory_hold" || rc=1
run cpi "$E/artifacts/cpi/memory_hold" || rc=1
run identity "$E/artifacts/identity/memory_hold" || rc=1
exit "$rc"
