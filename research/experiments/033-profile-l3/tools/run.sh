#!/bin/bash
# 033: (1) sample profile of the shipped l3 (unchanged binary); (2) stats build l3cf = l3 sources + EXP_STAGE_STATS + EXP_CF_STATS (same PGO/LTO/mimalloc),
# 3 x 50-pass stage_bench runs on the golden-v1 development split. Diagnostic only.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; R="$W/research"; X="$R/experiments"; E="$X/033-profile-l3"; B="$E/raw"; DEV="$X/025-audit-next/data/development.tsv"
log() { echo "$(date '+%Y-%m-%dT%H:%M:%S%z') $*" >> "$B/run.log"; }
cap() { local cn=$1; shift; log "START $cn: $*"; "$@" > "$B/$cn.stdout" 2> "$B/$cn.stderr"; local rc=$?; echo $rc > "$B/$cn.exit"; log "END $cn exit=$rc"; return $rc; }
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/start.txt"
cap sample /bin/bash "$E/tools/run_sample.sh"
cap build env EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=3 -DEXP_STAGE_STATS -DEXP_CF_STATS" \
  /bin/bash "$E/tools/build_cc.sh" l3cf clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o || { date > "$B/FAILED"; exit 1; }
for r in 1 2 3; do until "$R/tools/env_gate.sh" >> "$B/gate.log" 2>&1; do sleep 30; done; cap stage_r$r "$E/artifacts/l3cf/stage_bench" "$DEV" 50; sleep 3; done
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
