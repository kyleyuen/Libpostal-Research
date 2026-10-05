#!/bin/bash
# 020: build 019 `b1`'s exact sources (019-fixed-costs/candidate, same switches) with another compiler / allocator. No source change.
# usage: build_cc.sh NAME CC PROFMODE(llvm|gcc|none) "LIB_FLAGS" "SCANNER_FLAGS" "LINK_FLAGS" [EXTRA_LINK_OBJECT]
set -e
NAME=$1; CC=$2; PM=$3; LOPT=$4; SOPT=$5; LINK=$6; XOBJ=$7
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/110-clean-cf-split"; C="$E/${SRCDIR:-sources}"; SRC="$W/libpostal/src"
BASE="-std=gnu2x -DHAVE_CONFIG_H -Wall -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare ${DEBUG_FLAG--g} -D LIBPOSTAL_EXPORTS"
SW="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE -DEXP_FAST_APPEND -DEXP_FAST_JOIN -DEXP_VOCAB_MEMO -DEXP_INT_KEYS -DEXP_TRANS_SKIP -DEXP_NORM_TABLE -DEXP_CANON_CONST -DEXP_RESPONSE_BLOCK -DEXP_REUSE -DEXP_VITERBI_NEON -DEXP_TOKEN_ASCII -DEXP_FD_INLINE $EXTRA_SW"
CHANGED="crf trie string_utils address_parser features trie_search transliterate normalize crf_context libpostal tokens"
LIBSRC="strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard"
DD='-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"'
build() {  # $1 dir, $2 extra flags for every compile and link
  D="$E/artifacts/$1"; mkdir -p "$D/src" "$D/obj"; for f in $CHANGED; do cp "$C/$f.c" "$D/src/"; done; cp "$C/"*.h "$D/src/"
  t0=$(date +%s)
  for f in $LIBSRC; do b=$(basename $f); case " $CHANGED " in *" $b "*) S="$D/src/$b.c";; *) S="$SRC/$f.c";; esac
    ( cd "$SRC" && $CC $BASE $SW $LOPT $2 "$DD" -I"$D/src" -I"$SRC" -I"$SRC/.." -I/usr/local/include -c "$S" -o "$D/obj/$(echo $f | tr / _).o" ); done
  SK="$E/artifacts/scanner_cache/scanner_$(echo "$CC" | tr -dc 'A-Za-z0-9')_$(echo "$SOPT" | tr -dc 'A-Za-z0-9').o"
  if [ ! -f "$SK" ]; then ( cd "$SRC" && $CC $BASE $SOPT "$DD" -I"$SRC" -I"$SRC/.." -c "$SRC/scanner.c" -o "$SK" ); fi
  cp "$SK" "$D/obj/scanner.o"; ( cd "$SRC" && $CC $BASE $SOPT -I"$SRC" -I"$SRC/.." -c "$SRC/klib/drand48.c" -o "$D/obj/drand48.o" )
  ( cd "$D/obj" && ${AR:-ar} -rc ../libpostal.a *.o && ${RANLIB:-ranlib} ../libpostal.a )   # AR/RANLIB overridable (LLVM 23 bitcode needs llvm-ar)
  for t in dump_outputs:"$X/003-batched-lookups/tools/dump_outputs.c" bench_ext:"$W/research/tools/bench_ext.c" fuzz_pipeline:"$W/research/tools/fuzz_pipeline.c"; do
    $CC -O2 ${DEBUG_FLAG--g} $LINK $2 -I"$SRC" $XOBJ "${t#*:}" "$D/libpostal.a" -lm -o "$D/${t%%:*}"; done
  echo "$1: $CC lib $LOPT scanner $SOPT link $LINK $XOBJ: $(( $(date +%s)-t0 ))s" >> "$E/raw/compile_times.txt"
  ( cd "$D" && shasum -a 256 src/* libpostal.a dump_outputs bench_ext fuzz_pipeline > hashes.txt; echo "cc: $CC | lib: $SW $LOPT $2 | scanner: $SOPT | link: $LINK $XOBJ $2" >> hashes.txt )
}
P="/Users/kyle/.cache/libpostal-020-pgo/$NAME"; mkdir -p "$P"
case $PM in
  llvm) PD=$(dirname $CC)/llvm-profdata; [ -x "$PD" ] || PD="xcrun llvm-profdata"
        build "${NAME}_gen" "-fprofile-instr-generate"; rm -f "$P"/*.profraw
        LLVM_PROFILE_FILE="$P/p-%p.profraw" "$E/artifacts/${NAME}_gen/dump_outputs" "$W/profiling/golden-v1/development.txt" > /dev/null
        $PD merge -output="$P/dev.profdata" "$P"/*.profraw
        build "$NAME" "-fprofile-instr-use=$P/dev.profdata -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date";;
  gcc)  build "${NAME}_gen" "-fprofile-generate -fprofile-dir=$P"
        "$E/artifacts/${NAME}_gen/dump_outputs" "$W/profiling/golden-v1/development.txt" > /dev/null
        build "$NAME" "-fprofile-use -fprofile-dir=$P -fprofile-partial-training -Wno-missing-profile";;
  *)    build "$NAME" "";;
esac
