#!/bin/bash
# 029: (1) stats build l4s = 021 l4 flags + EXP_STAGE_STATS, same PGO/LTO/mimalloc procedure; (2) stage_bench on development split;
# (3) instructions/cycles per address for preserved original harness vs unchanged 021 l4, by the two-pass-count difference method (time -l).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/029-stage-split"; B="$E/raw"
DEV="$X/025-audit-next/data/development.tsv"; ORIG="$X/015-integer-keys/artifacts/bench_ext_baseline"; L4="$X/021-final-stack/artifacts/l4/bench_ext"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { n=$1; shift; log "START $n: $*"; "$@" > "$B/$n.stdout" 2> "$B/$n.stderr"; rc=$?; echo $rc > "$B/$n.exit"; log "END $n exit=$rc"; return $rc; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
"$R/tools/env_snapshot.sh" > "$B/env_before.txt" 2>&1
shasum -a 256 "$DEV" "$ORIG" "$L4" "$X/021-final-stack/artifacts/l4/libpostal.a" > "$B/input_binary_hashes.txt"
cap build env EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=4 -DEXP_STAGE_STATS" \
  /bin/bash "$E/tools/build_cc.sh" l4s clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o || { date > "$B/FAILED"; exit 1; }
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
for r in 1 2 3; do gate; cap stage_r$r /usr/bin/time -l "$E/artifacts/l4s/stage_bench" "$DEV" 50; sleep 3; done
# instruction counts: P=2 and P=22 passes; per-address = (I22 - I2) / (20 * 10308). Rotating order.
for r in 1 2 3; do for v in orig l4; do [ $v = orig ] && bin="$ORIG" || bin="$L4"; for p in 2 22; do gate; cap ic_${v}_p${p}_r$r /usr/bin/time -l "$bin" "$DEV" $p; sleep 3; done; done; done
"$R/tools/env_snapshot.sh" > "$B/env_after.txt" 2>&1
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
