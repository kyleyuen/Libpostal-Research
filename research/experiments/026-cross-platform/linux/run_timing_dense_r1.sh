#!/usr/bin/env bash
# Host-side controlled Linux ARM64 Docker session. Run only after output/memory checks.
set -euo pipefail
W="$(cd "$(dirname "$0")/../../../.." && pwd)"
L="$W/research/experiments/026-cross-platform/linux"
T="$W/research/tools"
trials=${1:-3}
passes=${2:-150}
model=/Users/kyle/.cache/libpostal-profile/libpostal
out="$L/timing-dense-r1-${trials}x${passes}"
if [ -e "$out" ]; then echo "timing directory exists: $out" >&2; exit 2; fi
mkdir -p "$out"
identity() {
  { date -Iseconds
    docker image inspect libpostal-audit-linux-arm64:026 --format '{{.Id}} {{json .RepoDigests}} {{.Architecture}} {{.Os}}'
    shasum -a 256 "$L"/artifacts/{baseline_upstream,candidate,candidate_dense}/bench_ext \
      "$W"/profiling/golden-v2-novel/{v1-all,novel}.tsv
    docker run --rm --platform linux/arm64 -v "$model:/model:ro" \
      libpostal-audit-linux-arm64:026 bash -c 'find /model -type f -print0 | sort -z | xargs -0 sha256sum'
  } > "$out/identity-$1.txt"
}
printf '%s\n' "start=$(date -Iseconds)" "trials=$trials" "passes=$passes" \
  'variants=upstream(glibc),candidate(exact l4 glibc),dense(default 0.75 load factor glibc)' \
  'image=libpostal-audit-linux-arm64:026' \
  'warmup=other corpus before pass 1; first pass is not METHOD true cold' \
  > "$out/session.txt"
"$T/env_gate.sh" > "$out/gate-start.txt" 2>&1 || { touch "$out/ABORT"; exit 1; }
identity start
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
          -v "$model:/model:ro" \
          libpostal-audit-linux-arm64:026 "${cmd[@]}"; printf '\n'
      } > "$out/$tag.meta"
      top -l 120 -s 1 -n 8 -stats pid,command,cpu > "$out/$tag.host-cpu" 2> "$out/$tag.host-cpu.err" &
      top_pid=$!
      set +e
      docker run --rm --platform linux/arm64 \
        -v "$W:/workspace:ro" -v "$L:/out:ro" \
        -v "$model:/model:ro" \
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
identity end
"$T/env_snapshot.sh" "$out/env-session.txt" end
printf 'end=%s\n' "$(date -Iseconds)" >> "$out/session.txt"
touch "$out/DONE"
