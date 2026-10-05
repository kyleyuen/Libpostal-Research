#!/bin/bash
# 031: PGO cross-training matrix. Exact 021 l4 sources/switches/toolchain; only the PGO training input differs:
#   tv1 = golden-v1 all, tnv = golden-v2-novel, tosm = golden-v3-osm, nopgo = same build without PGO. Plus the shipped 021 l4 (trained on golden-v1
#   development) and the preserved original. Output check of every build vs the original on all three datasets, then timing of every binary on
#   every dataset (5 rotating rounds, 20 passes, no warm-up file; median of warm passes 2..20), AC-gated.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/031-pgo-crosstrain"; B="$E/raw"; P="$W/profiling"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { n=$1; shift; log "START $n: $*"; "$@" > "$B/$n.stdout" 2> "$B/$n.stderr"; rc=$?; echo $rc > "$B/$n.exit"; log "END $n exit=$rc"; return $rc; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=4"
MI=/opt/homebrew/opt/mimalloc/lib/mimalloc.o
shasum -a 256 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" "$P/golden-v3-osm/osm.txt" "$P/golden-v2-novel/v1-all.tsv" "$P/golden-v2-novel/novel.tsv" "$P/golden-v3-osm/osm.tsv" \
  "$X/015-integer-keys/artifacts/bench_ext_baseline" "$X/021-final-stack/artifacts/l4/bench_ext" "$X/000-baseline/artifacts/libpostal.a" > "$B/input_hashes.txt"
for t in tv1:"$P/golden-v1/all.txt" tnv:"$P/golden-v2-novel/novel.txt" tosm:"$P/golden-v3-osm/osm.txt"; do
  cap build_${t%%:*} env EXTRA_SW="$SW" PGO_TRAIN="${t#*:}" /bin/bash "$E/tools/build_cc.sh" ${t%%:*} clang llvm "-O2 -flto" "-O1" "-flto" $MI || { date > "$B/FAILED"; exit 1; }
done
cap build_nopgo env EXTRA_SW="$SW" /bin/bash "$E/tools/build_cc.sh" nopgo clang none "-O2 -flto" "-O1" "-flto" $MI || { date > "$B/FAILED"; exit 1; }
# output check: each build vs the shipped l4 dumps (l4 was verified byte-identical to the original on all three datasets in 021/025)
: > "$B/output_check.txt"
for d in v1:"$P/golden-v1/all.txt" nv:"$P/golden-v2-novel/novel.txt" osm:"$P/golden-v3-osm/osm.txt"; do ds=${d%%:*}; f=${d#*:}
  cap dump_l4_$ds "$X/021-final-stack/artifacts/l4/dump_outputs" "$f"
  for v in tv1 tnv tosm nopgo; do cap dump_${v}_$ds "$E/artifacts/$v/dump_outputs" "$f"
    if cmp -s "$B/dump_${v}_$ds.stdout" "$B/dump_l4_$ds.stdout"; then echo "$v $ds IDENTICAL_TO_L4" >> "$B/output_check.txt"; else echo "$v $ds DIFFERENT" >> "$B/output_check.txt"; fi; done; done
grep -q DIFFERENT "$B/output_check.txt" && { date > "$B/FAILED"; exit 1; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
V=(orig="$X/015-integer-keys/artifacts/bench_ext_baseline" l4="$X/021-final-stack/artifacts/l4/bench_ext" tv1="$E/artifacts/tv1/bench_ext" tnv="$E/artifacts/tnv/bench_ext" tosm="$E/artifacts/tosm/bench_ext" nopgo="$E/artifacts/nopgo/bench_ext")
n=${#V[@]}; mkdir -p "$B/timing"
for r in 1 2 3 4 5; do for d in v1:"$P/golden-v2-novel/v1-all.tsv" nv:"$P/golden-v2-novel/novel.tsv" osm:"$P/golden-v3-osm/osm.tsv"; do ds=${d%%:*}; f=${d#*:}
  for k in $(seq 0 $((n-1))); do e=${V[$(( (k + r) % n ))]}; name=${e%%=*}; bin=${e#*=}
    gate; cap timing/${ds}_r${r}_$name /usr/bin/time -l "$bin" "$f" 20; sleep 2; done; done; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
