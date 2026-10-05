#!/usr/bin/env bash
# Host-side controlled Linux ARM64 Docker session. Run only after output/memory checks.
set -euo pipefail
W="$(cd "$(dirname "$0")/../../../.." && pwd)"
L="$W/research/experiments/026-cross-platform/linux"
T="$W/research/tools"
trials=${1:-3}
passes=${2:-150}
out="$L/timing-dense-${trials}x${passes}"
if [ -e "$out" ]; then echo "timing directory exists: $out" >&2; exit 2; fi
mkdir -p "$out"
printf '%s\n' "start=$(date -Iseconds)" "trials=$trials" "passes=$passes" \
  'variants=upstream(glibc),candidate(exact l4 glibc),dense(default 0.75 load factor glibc)' \
  'image=libpostal-audit-linux-arm64:026' \
  'interpretation=noisy diagnostic only; not accepted Linux speed evidence' \
  > "$out/session.txt"
"$T/env_gate.sh" > "$out/gate-start.txt" 2>&1 || { touch "$out/ABORT"; exit 1; }
"$T/env_snapshot.sh" "$out/env-session.txt" start
caffeinate -i -w $$ &
caf=$!
trap 'kill "$caf" 2>/dev/null || true' EXIT

variants=(upstream candidate dense)
for corpus in v1-all novel; do
  if [ "$corpus" = v1-all ]; then warmup=novel; else warmup=v1-all; fi
  for ((trial=1; trial<=trials; trial++)); do
    for ((k=0; k<3; k++)); do
      variant=${variants[$(((k+trial)%3))]}
      tag="${corpus}-t${trial}-${variant}"
      "$T/env_gate.sh" > "$out/$tag.gate" 2>&1 || { touch "$out/ABORT"; exit 1; }
      "$T/env_snapshot.sh" "$out/$tag.env" pre
      bin=/out/artifacts/candidate/bench_ext
      if [ "$variant" = upstream ]; then bin=/out/artifacts/baseline_upstream/bench_ext; fi
      if [ "$variant" = dense ]; then bin=/out/artifacts/candidate_dense/bench_ext; fi
      cmd=(/usr/bin/time -v "$bin"
           "/workspace/profiling/golden-v2-novel/$corpus.tsv" "$passes"
           "/workspace/profiling/golden-v2-novel/$warmup.tsv")
      {
        printf 'start=%s\n' "$(date -Iseconds)"
        printf 'argv:'; printf ' <%q>' docker run --rm --platform linux/arm64 \
          -v "$W:/workspace:ro" -v "$L:/out:ro" \
          -v /Users/kyle/.cache/libpostal-profile/libpostal:/model:ro \
          libpostal-audit-linux-arm64:026 "${cmd[@]}"; printf '\n'
      } > "$out/$tag.meta"
      top -l 120 -s 1 -n 0 2>/dev/null | grep 'CPU usage' > "$out/$tag.host-cpu" &
      top_pid=$!
      set +e
      docker run --rm --platform linux/arm64 \
        -v "$W:/workspace:ro" -v "$L:/out:ro" \
        -v /Users/kyle/.cache/libpostal-profile/libpostal:/model:ro \
        libpostal-audit-linux-arm64:026 "${cmd[@]}" \
        > "$out/$tag.stdout" 2> "$out/$tag.stderr"
      code=$?
      set -e
      kill "$top_pid" 2>/dev/null || true
      wait "$top_pid" 2>/dev/null || true
      printf 'exit=%s\nend=%s\n' "$code" "$(date -Iseconds)" >> "$out/$tag.meta"
      "$T/env_snapshot.sh" "$out/$tag.env" post
      if [ "$code" -ne 0 ]; then touch "$out/ABORT"; exit "$code"; fi
      sleep 8
    done
  done
done
"$T/env_snapshot.sh" "$out/env-session.txt" end
printf 'end=%s\n' "$(date -Iseconds)" >> "$out/session.txt"
touch "$out/DONE"
