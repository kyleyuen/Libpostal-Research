#!/bin/bash
# 160 (p2b): print_features(true) dumps (r0's feat_dump.c) and API lifecycle probes (r0's api_probe2.c), original vs 160b/160c/160ctl.
# Sequential; every model process through model_slot.sh. Guard pass: system-malloc links under MallocGuardEdges/MallocScribble.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/160-p2-records-in-cpi"; P="$W/profiling"; A="$E/artifacts/probes"; S="$W/research/tools/model_slot.sh"
D=/Users/kyle/.cache/libpostal-profile/libpostal; o="$E/raw/probes"; T=/tmp/libpostal-160/feat; mkdir -p "$o/feat" "$o/api" "$o/guard" "$T"
run() { out=$1; shift; echo "# $(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$o/commands.txt"; "$S" "$@" > "$out" 2> "${out%.*}.stderr"; rc=$?; echo "#   exit=$rc $(date '+%Y-%m-%dT%H:%M:%S%z')" >> "$o/commands.txt"; echo $rc > "${out%.*}.exit"; }
for x in dev:golden-v1/development.txt audit:independent-audit-v1/audit-input.txt dad:independent-audit-v1/dad-international.txt; do
  for v in original 160b 160c 160ctl; do run "$T/$v-${x%%:*}.txt" "$A/$v/feat_dump" "$P/${x#*:}"; done; done
for m in default features lifecycle lifecycle_disable datadir datadir_features datadir_lifecycle datadir_bad lifecycle_long; do
  for v in original 160b 160c 160ctl; do run "$o/api/$v-$m.stdout" "$A/$v/api_probe2" $m "$D"; done; done
for m in lifecycle lifecycle_long datadir_lifecycle; do
  for v in original 160b_sysmalloc 160c_sysmalloc 160ctl_sysmalloc; do run "$o/guard/$v-$m.stdout" env MallocGuardEdges=1 MallocScribble=1 "$A/$v/api_probe2" $m "$D"; done; done
{ echo "## print_features(true) vs original (this run) and vs r0's original (158/raw/feat, sha of decompressed)"
  for x in dev audit dad; do r0=$(gzip -dc "$W/research/experiments/158-r0-feature-print/raw/feat/original-$x.txt.gz" | shasum -a 256 | cut -c1-16)
    echo "original-$x sha $(shasum -a 256 < "$T/original-$x.txt" | cut -c1-16) (r0's original $r0) exit $(cat "$T/original-$x.exit") lines $(wc -l < "$T/original-$x.txt" | tr -d ' ')"
    for v in 160b 160c 160ctl; do cmp -s "$T/original-$x.txt" "$T/$v-$x.txt" && r=IDENTICAL || r=DIFFERENT; echo "  $v-$x $r exit $(cat "$T/$v-$x.exit") stderr $(wc -c < "$T/$v-$x.stderr" | tr -d ' ')B"; done; done
  echo "## API probes (stdout/stderr vs original)"
  for m in default features lifecycle lifecycle_disable datadir datadir_features datadir_lifecycle datadir_bad lifecycle_long; do for v in 160b 160c 160ctl; do
    cmp -s "$o/api/original-$m.stdout" "$o/api/$v-$m.stdout" && so=same || so=DIFF; cmp -s "$o/api/original-$m.stderr" "$o/api/$v-$m.stderr" && se=same || se=DIFF
    echo "$m $v exit $(cat "$o/api/$v-$m.exit") (orig $(cat "$o/api/original-$m.exit")) stdout=$so stderr=$se"; done; done
  echo "## guard pass (system malloc, MallocGuardEdges=1 MallocScribble=1) stdout vs original normal run"
  for m in lifecycle lifecycle_long datadir_lifecycle; do for v in original 160b_sysmalloc 160c_sysmalloc 160ctl_sysmalloc; do
    cmp -s "$o/api/original-$m.stdout" "$o/guard/$v-$m.stdout" && so=same || so=DIFF; echo "$m $v exit $(cat "$o/guard/$v-$m.exit") stdout=$so"; done; done; } > "$o/summary.txt"
cat "$o/summary.txt"
