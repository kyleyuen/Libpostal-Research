#!/bin/bash
# Release rebuild of 180: direct events on; oracle and statistics switches absent.
set -euo pipefail
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
E="$W/research/experiments/181-release-180"
SRC="$W/libpostal/src"
D="$E/artifacts/release"
O="$D/obj"
BASE='-DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS'
SW='-DEXP_FUSED_FRONT -DEXP_EVENT_FLOW'
DD='-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"'
LIBSRC='strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard'
rm -rf "$D"; mkdir -p "$O"
for f in $LIBSRC; do
  b=${f##*/}; s="$SRC/$f.c"
  case "$b" in address_parser|crf) s="$E/sources/$b.c";; esac
  (cd "$SRC" && clang $BASE -O2 $SW "$DD" -I"$E/sources" -I"$SRC" -I"$SRC/.." -c "$s" -o "$O/${f//\//_}.o")
done
(cd "$SRC" && clang $BASE -O0 "$DD" -I"$SRC" -I"$SRC/.." -c "$SRC/scanner.c" -o "$O/scanner.o")
(cd "$SRC" && clang $BASE -O0 -I"$SRC" -I"$SRC/.." -c "$SRC/klib/drand48.c" -o "$O/drand48.o")
(cd "$O" && ar -rc ../libpostal.a *.o && ranlib ../libpostal.a)
for t in dump_outputs:"$W/research/experiments/003-batched-lookups/tools/dump_outputs.c" bench_ext:"$W/research/tools/bench_ext.c" fuzz_pipeline:"$W/research/tools/fuzz_pipeline.c"; do
  clang -O2 -g -I"$SRC" "${t#*:}" "$D/libpostal.a" -lm -o "$D/${t%%:*}"
done
clang -O2 -g -I"$SRC" "$W/research/experiments/003-batched-lookups/tools/dump_outputs.c" "$W/research/experiments/000-baseline/artifacts/libpostal.a" -lm -o "$E/artifacts/original_dump_outputs"
(cd "$E" && shasum -a 256 sources/address_parser.c sources/crf.c tools/build_release.sh artifacts/release/libpostal.a artifacts/release/dump_outputs artifacts/release/bench_ext artifacts/release/fuzz_pipeline artifacts/original_dump_outputs > raw/final_hashes.sha256)
printf 'BASE=%s\nSW=%s\nDD=%s\n' "$BASE" "$SW" "$DD" > "$E/raw/build_flags.txt"
