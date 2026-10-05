#!/bin/bash
# 016 variants. Base switches = 015 k1. Usage: build.sh [variant ...]; variants: k0 (control = k1 switches), s (stats build), plus one per optimization as added.
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/016-batch"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
BASE_CRF="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT"
BASE_AP="-DEXP_DIRECT_KEYS -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE"
K1="-DEXP_VOCAB_MEMO -DEXP_INT_KEYS"
FILES="crf trie string_utils address_parser features trie_search transliterate normalize crf_context libpostal tokens"
new_flags() { case $1 in
  k0) echo "";; s) echo "-DEXP_STAGE_STATS";;
  *) [ -f "$E/tools/variants/$1" ] && cat "$E/tools/variants/$1" || { echo "unknown variant $1" >&2; exit 2; };; esac; }
for v in ${@:-k0 s}; do
  NEW=$(new_flags $v); D="$E/artifacts/$v"; mkdir -p "$D/src"
  for f in $FILES; do cp "$E/candidate/$f.c" "$D/src/"; done; cp "$E/candidate/"*.h "$D/src/"
  OBJS=""
  for f in $FILES; do
    case $f in crf) FL="$BASE_CRF $K1 $NEW";; address_parser) FL="$BASE_AP $K1 $NEW";; string_utils) FL="-DEXP_FAST_APPEND $NEW";; features) FL="-DEXP_FAST_JOIN $NEW";; *) FL="$NEW";; esac
    ( cd "$SRC" && gcc $CF $FL "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I"$D/src" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
    OBJS="$OBJS libpostal_la-$f.o"
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a $OBJS 2>/dev/null && ranlib libpostal.a )
  if [ "${v:0:1}" = s ]; then clang -O2 -g -I"$SRC" "$E/tools/stage_bench.c" "$D/libpostal.a" -o "$D/stage_bench"
  else
    clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
    clang -O2 -g -I"$SRC" "$X/../tools/bench_ext.c" "$D/libpostal.a" -o "$D/bench_ext"
    clang -O2 -g -I"$SRC" "$X/../tools/fuzz_pipeline.c" "$D/libpostal.a" -lm -o "$D/fuzz_pipeline"
  fi
  ( cd "$D" && shasum -a 256 src/* libpostal_la-*.o libpostal.a $(ls dump_outputs bench_ext fuzz_pipeline stage_bench 2>/dev/null) > hashes.txt; echo "flags: CRF=$BASE_CRF $K1 $NEW AP=$BASE_AP $K1 $NEW ALL=$NEW" >> hashes.txt )
  rm -f "$D/libpostal.a"
done
