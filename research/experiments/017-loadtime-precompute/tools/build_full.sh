#!/bin/bash
# 018: build the whole libpostal library (41 sources + scanner) with t45's source switches and given compiler flags; optional PGO (trained on development only).
# usage: build_full.sh NAME "LIB_OPT_FLAGS" "SCANNER_FLAGS" "LINK_FLAGS" [pgo]
set -e
NAME=$1; LOPT=$2; SOPT=$3; LINK=$4; PGO=$5
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/017-loadtime-precompute"; C16="$E/candidate"; SRC="$W/libpostal/src"
BASE="-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS"
SW="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE -DEXP_FAST_APPEND -DEXP_FAST_JOIN -DEXP_VOCAB_MEMO -DEXP_INT_KEYS -DEXP_TRANS_SKIP -DEXP_NORM_TABLE $EXTRA_SW"
CHANGED="crf trie string_utils address_parser features trie_search transliterate normalize crf_context libpostal tokens"
LIBSRC="strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard"
DD='-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"'
build() {  # $1 dir, $2 extra flags for every compile and link (profile gen/use)
  D="$E/artifacts/$1"; mkdir -p "$D/src" "$D/obj"; for f in $CHANGED; do cp "$C16/$f.c" "$D/src/"; done; cp "$C16/"*.h "$D/src/"
  t0=$(date +%s)
  for f in $LIBSRC; do
    b=$(basename $f); case " $CHANGED " in *" $b "*) S="$D/src/$b.c";; *) S="$SRC/$f.c";; esac
    ( cd "$SRC" && clang $BASE $SW $LOPT $2 "$DD" -I"$D/src" -I"$SRC" -I"$SRC/.." -I/usr/local/include -c "$S" -o "$D/obj/$(echo $f | tr / _).o" )
  done
  t1=$(date +%s)
  # The scanner (310,256 generated lines) is compiled ONCE per optimization level, without profile flags, and reused by every build (cache keyed by flags).
  SK="$E/artifacts/scanner_cache/scanner_$(echo "$SOPT" | tr -dc 'A-Za-z0-9').o"; mkdir -p "$E/artifacts/scanner_cache"
  if [ ! -f "$SK" ]; then ( cd "$SRC" && clang $BASE $SOPT "$DD" -I"$SRC" -I"$SRC/.." -c "$SRC/scanner.c" -o "$SK" ); fi
  cp "$SK" "$D/obj/scanner.o"
  ( cd "$SRC" && clang $BASE $SOPT -I"$SRC" -I"$SRC/.." -c "$SRC/klib/drand48.c" -o "$D/obj/drand48.o" )
  t2=$(date +%s)
  ( cd "$D/obj" && ar -rc ../libpostal.a *.o && ranlib ../libpostal.a ) 2>/dev/null
  for t in dump_outputs:"$X/003-batched-lookups/tools/dump_outputs.c" bench_ext:"$W/research/tools/bench_ext.c" fuzz_pipeline:"$W/research/tools/fuzz_pipeline.c"; do
    clang -O2 -g $LINK $2 -I"$SRC" "${t#*:}" "$D/libpostal.a" -lm -o "$D/${t%%:*}"; done
  t3=$(date +%s)
  echo "$1: library ${LOPT} $((t1-t0))s, scanner ${SOPT} $((t2-t1))s, link $((t3-t2))s, total $((t3-t0))s" >> "$E/raw/compile_times.txt"
  ( cd "$D" && shasum -a 256 src/* libpostal.a dump_outputs bench_ext fuzz_pipeline > hashes.txt; echo "lib: $SW $LOPT $2 | scanner: $SOPT $2 | link: $LINK $2" >> hashes.txt )
}
if [ "$PGO" = pgo ]; then
  build "${NAME}_gen" "-fprofile-instr-generate"
  P="/Users/kyle/.cache/libpostal-018-pgo"; mkdir -p "$P"
  LLVM_PROFILE_FILE="$P/$NAME-%p.profraw" "$E/artifacts/${NAME}_gen/dump_outputs" "$W/profiling/golden-v1/development.txt" > /dev/null
  xcrun llvm-profdata merge -output="$P/$NAME.profdata" "$P/$NAME"-*.profraw
  shasum -a 256 "$P/$NAME.profdata" >> "$E/raw/pgo_profiles.txt"
  build "$NAME" "-fprofile-instr-use=$P/$NAME.profdata -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date"
else
  build "$NAME" ""
fi
