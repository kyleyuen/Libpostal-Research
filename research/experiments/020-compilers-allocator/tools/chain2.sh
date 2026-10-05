#!/bin/bash
# Session-2 orchestration: wait for z1/z3 builds; build z2 (GCC, no -g) with a 30-minute watchdog (kills only its own process tree); verify z1, z3, z2.
cd "$(dirname "$0")/.."
until [ -f raw/ZBUILDS_DONE ]; do sleep 10; done
killtree() { for c in $(pgrep -P "$1"); do killtree "$c"; done; kill "$1" 2>/dev/null; }
if ! grep -q "^z2 exit=0" raw/compile_times.txt; then
  s=$(date +%s); EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK" DEBUG_FLAG="" tools/build_cc.sh z2 /opt/homebrew/bin/gcc-16 gcc "-O2 -flto=auto" "-O1" "-flto=auto" > raw/build_z2.log 2>&1 &
  pid=$!; while kill -0 $pid 2>/dev/null; do sleep 10; if [ $(( $(date +%s)-s )) -ge 1800 ]; then killtree $pid; echo "z2 killed by watchdog after 30 min (unreasonable build time)" >> raw/compile_times.txt; break; fi; done
  wait $pid 2>/dev/null; echo "z2 (rerun, watchdog) exit=$? total $(( $(date +%s)-s ))s" >> raw/compile_times.txt
fi
for v in z1 z3 z2; do [ -x artifacts/$v/bench_ext ] && tools/verify.sh $v > raw/verify_$v.log 2>&1; done
date -Iseconds > raw/VERIFY2_DONE
