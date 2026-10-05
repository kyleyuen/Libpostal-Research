#!/bin/bash
# 030 step 1: count string-path vs integer-path lookups per feature name on the development split (EXP_DUP_STATS build, no PGO; measurement only).
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/030-phrase-keys"; B="$E/raw"
echo "$(date '+%Y-%m-%dT%H:%M:%S%z') START count build" >> "$B/run.log"
EXTRA_SW="-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0 -DEXP_LOADF=4 -DEXP_DUP_STATS" \
  /bin/bash "$E/tools/build_cc.sh" cnt clang none "-O2" "-O1" "" > "$B/count_build.stdout" 2> "$B/count_build.stderr"; echo $? > "$B/count_build.exit"
cp "$W/profiling/golden-v1/development.txt" "$B/dev_addresses.txt"
DUP_NAMES=1 "$E/artifacts/cnt/dump_outputs" "$B/dev_addresses.txt" > "$B/count_dump.stdout" 2> "$B/count_names.stderr"; echo $? > "$B/count_dump.exit"
echo "$(date '+%Y-%m-%dT%H:%M:%S%z') END count exit build=$(cat $B/count_build.exit) dump=$(cat $B/count_dump.exit)" >> "$B/run.log"
