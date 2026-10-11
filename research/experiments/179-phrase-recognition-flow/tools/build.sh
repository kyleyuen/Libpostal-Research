#!/bin/bash
# Build original-source oracle control and root-mask candidate serially, no PGO/LTO.
set -eu
ROOT="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$ROOT/research/experiments/179-phrase-recognition-flow"
MODEL="$HOME/.cache/libpostal-profile/libpostal"
BASE="-DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS"
DD="-DLIBPOSTAL_DATA_DIR=\"$MODEL\""
LIBSRC="strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard"
HARNESS_DUMP="$ROOT/research/experiments/003-batched-lookups/tools/dump_outputs.c"
HARNESS_FUZZ="$ROOT/research/tools/fuzz_pipeline.c"

build_one() {
    local name=$1
    local source=$2
    local out="$E/artifacts/$name"
    local extra="-DEXP_ORACLE"
    if [ "$name" = "root-gate" ]; then extra="$extra -DEXP_ROOT_GATE_STATS"; fi
    mkdir -p "$out/obj"
    for f in $LIBSRC; do
        ( cd "$source/src" && clang $BASE -O2 $extra "$DD" -I"$source/src" -I"$source" -c "$source/src/$f.c" -o "$out/obj/$(echo "$f" | tr / _).o" )
    done
    ( cd "$source/src" && clang $BASE -O0 "$DD" -I"$source/src" -I"$source" -c "$source/src/scanner.c" -o "$out/obj/scanner.o" )
    ( cd "$source/src" && clang $BASE -O0 -I"$source/src" -I"$source" -c "$source/src/klib/drand48.c" -o "$out/obj/drand48.o" )
    ( cd "$out/obj" && ar -rc ../libpostal.a *.o && ranlib ../libpostal.a )
    clang -O2 -g -I"$source/src" "$HARNESS_DUMP" "$out/libpostal.a" -lm -o "$out/dump_outputs"
    clang -O2 -g -I"$source/src" "$HARNESS_FUZZ" "$out/libpostal.a" -lm -o "$out/fuzz_pipeline"
    ( cd "$out" && shasum -a 256 libpostal.a dump_outputs fuzz_pipeline > hashes.sha256 )
}

build_one control "$E/scratch/control"
build_one root-gate "$E/scratch/candidate"
