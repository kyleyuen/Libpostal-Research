#!/bin/bash
# 161: run api_probe2 MODEs of artifacts/VARIANT (or an absolute dir) through model_slot.sh, one at a time, and compare stdout with the original's
# (158 raw/api/original-MODE.stdout). Extra environment (e.g. MallocGuardEdges=1 MallocScribble=1) is passed via env on the command line.
# usage: probe_run.sh VARIANT_DIR OUTNAME MODE...
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/161-resetup-defect"; vd=$1; o="$E/raw/$2"; shift 2; mkdir -p "$o"
case "$vd" in /*) b="$vd/api_probe2";; *) b="$E/artifacts/$vd/api_probe2";; esac
D=/Users/kyle/.cache/libpostal-profile/libpostal
for m in "$@"; do
  echo "# $(date "+%Y-%m-%dT%H:%M:%S%z") MallocGuardEdges=${MallocGuardEdges-} MallocScribble=${MallocScribble-} CP_IDS_DISABLE=${CP_IDS_DISABLE-} $b $m $D" >> "$o/commands.txt"
  "$W/research/tools/model_slot.sh" "$b" "$m" "$D" > "$o/$m.stdout" 2> "$o/$m.stderr"; rc=$?
  s=$(cmp -s "$o/$m.stdout" "$W/research/experiments/158-r0-feature-print/raw/api/original-$m.stdout" && echo equal || echo DIFFERENT)
  echo "#   exit=$rc stdout_vs_original=$s $(date "+%Y-%m-%dT%H:%M:%S%z")" >> "$o/commands.txt"
done
echo END >> "$o/commands.txt"
