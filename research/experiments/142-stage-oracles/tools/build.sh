#!/bin/bash
# 142: build the ORIGINAL libpostal sources (commit 25099c50) with the original Makefile flags (-O2 library, -O0 scanner, system malloc, no PGO/LTO).
# address_parser.c and crf.c come from 142/src (fprintf-only hooks under EXP_ORACLE). usage: build.sh NAME "EXTRA_FLAGS"
set -e
NAME=$1; XF=$2
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/142-stage-oracles"; SRC="$W/libpostal/src"; D="$E/artifacts/$NAME"
BASE="-DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS"
DD='-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"'
CHANGED="address_parser crf"
LIBSRC="strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard"
mkdir -p "$D/obj"
for f in $LIBSRC; do b=$(basename $f); case " $CHANGED " in *" $b "*) S="$E/src/$b.c";; *) S="$SRC/$f.c";; esac
  ( cd "$SRC" && clang $BASE -O2 $XF "$DD" -I"$SRC" -I"$SRC/.." -c "$S" -o "$D/obj/$(echo $f | tr / _).o" ) & done; wait
( cd "$SRC" && clang $BASE -O0 "$DD" -I"$SRC" -I"$SRC/.." -c "$SRC/scanner.c" -o "$D/obj/scanner.o" )
( cd "$SRC" && clang $BASE -O0 -I"$SRC" -I"$SRC/.." -c "$SRC/klib/drand48.c" -o "$D/obj/drand48.o" )
rm -f "$D/libpostal.a"; ( cd "$D/obj" && ar -rc ../libpostal.a *.o && ranlib ../libpostal.a )
for t in dump_outputs:"$W/research/experiments/003-batched-lookups/tools/dump_outputs.c" fuzz_pipeline:"$W/research/tools/fuzz_pipeline.c"; do
  clang -O2 -g -I"$SRC" "${t#*:}" "$D/libpostal.a" -lm -o "$D/${t%%:*}"; done
( cd "$D" && shasum -a 256 libpostal.a dump_outputs fuzz_pipeline > hashes.txt; echo "flags: $BASE -O2 $XF | scanner -O0 | $(clang --version | head -1)" >> hashes.txt )
