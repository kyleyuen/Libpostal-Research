#!/bin/bash
# 034: can several single-threaded benchmark processes run at once without distorting each other? Same l3 binary, golden-v1 (v1-all.tsv), 20 passes,
# run solo and as 2/3/4 simultaneous copies; also original+l3 simultaneously vs sequentially. Records vm_stat swap/pageout deltas and memory pressure.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/034-parallel-calibration"; B="$E/raw"; D="$W/profiling/golden-v2-novel/v1-all.tsv"
L3="$X/021-final-stack/artifacts/l3/bench_ext"; ORIG="$X/015-integer-keys/artifacts/bench_ext_baseline"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
vmline() { vm_stat | awk -v t="$1" '/Swapins|Swapouts|Pageouts/ {gsub("\\.","",$NF); printf "%s %s=%s ", t, $1, $NF} END {print ""}'; }
group() {  # tag k binary...   run the given binaries simultaneously
  local tag=$1; shift; gate; vmline "$tag before" >> "$B/vm.txt"; memory_pressure | grep -i "free percentage" | sed "s/^/$tag before /" >> "$B/vm.txt"
  local pids="" i=0
  for b in "$@"; do i=$((i+1)); /usr/bin/time -l "$b" "$D" 20 > "$B/${tag}_p$i.stdout" 2> "$B/${tag}_p$i.stderr" & pids="$pids $!"; done
  log "START $tag ($#): $*"; sleep 6; memory_pressure | grep -i "free percentage" | sed "s/^/$tag during /" >> "$B/vm.txt"
  for p in $pids; do wait $p; done; log "END $tag"; vmline "$tag after" >> "$B/vm.txt"; sleep 3
}
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
for r in 1 2 3; do
  group solo_r$r "$L3"
  group k2_r$r "$L3" "$L3"
  group k3_r$r "$L3" "$L3" "$L3"
  group k4_r$r "$L3" "$L3" "$L3" "$L3"
  group seq_orig_r$r "$ORIG"; group seq_l3_r$r "$L3"
  group par_pair_r$r "$ORIG" "$L3"
done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
