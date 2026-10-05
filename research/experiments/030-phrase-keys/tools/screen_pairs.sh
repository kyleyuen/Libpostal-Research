#!/bin/bash
# 030 screen: l4 + EXP_INT_KEYS_PAIRS (existing switch) vs unchanged 021 l4. Output check on golden-v1 development, then instructions/cycles per
# address by the difference method with 2 vs 102 passes (3 rotating reps). Same PGO/LTO/mimalloc procedure as l4.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/030-phrase-keys"; B="$E/raw/screen_pairs"; mkdir -p "$B"
DEV_TSV="$X/025-audit-next/data/development.tsv"; DEV_TXT="$W/profiling/golden-v1/development.txt"; L4="$X/021-final-stack/artifacts/l4"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { n=$1; shift; log "START $n: $*"; "$@" > "$B/$n.stdout" 2> "$B/$n.stderr"; rc=$?; echo $rc > "$B/$n.exit"; log "END $n exit=$rc"; return $rc; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"; "$R/tools/env_snapshot.sh" "$B/env.txt" before
cap build env EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=4 -DEXP_INT_KEYS_PAIRS" \
  /bin/bash "$E/tools/build_cc.sh" pr clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o || { date > "$B/FAILED"; exit 1; }
cap dump_pr "$E/artifacts/pr/dump_outputs" "$DEV_TXT"; cap dump_l4 "$L4/dump_outputs" "$DEV_TXT"
if cmp -s "$B/dump_pr.stdout" "$B/dump_l4.stdout"; then echo IDENTICAL > "$B/output_check.txt"; else echo DIFFERENT > "$B/output_check.txt"; date > "$B/FAILED"; exit 1; fi
gate() { until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; }
for r in 1 2 3; do for v in l4 pr; do [ $v = l4 ] && bin="$L4/bench_ext" || bin="$E/artifacts/pr/bench_ext"
  for p in 2 102; do gate; cap ic_${v}_p${p}_r$r /usr/bin/time -l "$bin" "$DEV_TSV" $p; sleep 2; done; done; done
"$R/tools/env_snapshot.sh" "$B/env.txt" after; date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
