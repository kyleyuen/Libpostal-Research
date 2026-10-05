#!/bin/bash
# Builds variants dn (dictionary only) and d (dictionary + 005 pieces) by swapping crf/trie/string_utils objects into copies of the preserved baseline libpostal.a.
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E6="$X/006-feature-dict"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
flags() { case $1 in dn) STR=""; CRF="-DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT";; d) STR="-DEXP_FAST_APPEND"; CRF="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT";; esac; }
for v in ${@:-dn d}; do
  flags $v; D="$E6/artifacts/$v"; mkdir -p "$D/src"
  cp "$E6/candidate/crf.c" "$E6/candidate/crf.h" "$E6/candidate/trie.c" "$E6/candidate/trie.h" "$E6/candidate/string_utils.c" "$E6/candidate/feature_dict.h" "$D/src/"
  for f in crf trie string_utils; do
    case $f in crf) FL="$CRF";; string_utils) FL="$STR";; *) FL="";; esac
    ( cd "$SRC" && gcc $CF $FL "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
  clang -O2 -g -I"$SRC" "$X/../tools/bench_ext.c" "$D/libpostal.a" -o "$D/bench_ext"
  ( cd "$D" && shasum -a 256 src/crf.c src/crf.h src/trie.c src/trie.h src/string_utils.c src/feature_dict.h libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o libpostal.a parser_profile dump_outputs bench_ext > hashes.txt; echo "flags: STR=$STR CRF=$CRF" >> hashes.txt )
done
