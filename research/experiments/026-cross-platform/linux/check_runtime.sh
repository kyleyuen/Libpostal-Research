#!/usr/bin/env bash
# Untimed functional/memory/thread check inside Linux container.
set -euo pipefail
variant=${1:?artifact name}
corpus=${2:?TSV path}
passes=${3:-120}
allocator=${4:-glibc}
tag=${5:?unique output tag}
bin=/out/artifacts/$variant/bench_ext
results=/out/runtime/$tag
if [ -e "$results" ]; then echo "runtime result exists: $results" >&2; exit 2; fi
mkdir -p "$results"
printf 'variant=%s\ncorpus=%s\npasses=%s\nallocator=%s\n' \
  "$variant" "$corpus" "$passes" "$allocator" > "$results/settings.txt"
if [ "$allocator" = mimalloc ]; then
  export LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libmimalloc.so
fi
"$bin" "$corpus" "$passes" > "$results/bench.stdout" 2> "$results/bench.stderr" &
pid=$!
printf 'pid=%s\n' "$pid" >> "$results/settings.txt"
max_threads=0
samples=0
while kill -0 "$pid" 2>/dev/null; do
  if [ -r "/proc/$pid/status" ]; then
    if grep -q '^State:[[:space:]]*Z' "/proc/$pid/status"; then break; fi
    threads=$(find "/proc/$pid/task" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l) || break
    if [ "$threads" -gt "$max_threads" ]; then max_threads=$threads; fi
    {
      date -Ins
      awk '/^Threads:|^VmRSS:|^VmHWM:|^VmSwap:/{print}' "/proc/$pid/status" 2>/dev/null || true
      printf 'cgroup_memory_current='; cat /sys/fs/cgroup/memory.current
      printf 'cgroup_swap_current='; cat /sys/fs/cgroup/memory.swap.current
    } >> "$results/samples.txt"
    samples=$((samples + 1))
  fi
  sleep 0.25
done
set +e
wait "$pid"
code=$?
set -e
printf 'exit=%s\nmax_threads=%s\nsamples=%s\n' "$code" "$max_threads" "$samples" \
  >> "$results/settings.txt"
cat "$results/settings.txt"
exit "$code"
