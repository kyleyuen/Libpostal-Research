#!/bin/bash
# Builds variants n (control), a, b, c, all by swapping crf/trie/string_utils objects into copies of the preserved baseline libpostal.a.
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E5="$X/005-exact-work-bundle"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
flags() { case $1 in n) STR=""; CRF="";; a) STR="-DEXP_FAST_APPEND"; CRF="";; b) STR=""; CRF="-DEXP_NO_RESET";; c) STR=""; CRF="-DEXP_DEDUP_BATCH";; all) STR="-DEXP_FAST_APPEND"; CRF="-DEXP_NO_RESET -DEXP_DEDUP_BATCH";; esac; }
for v in ${@:-n a b c all}; do
  flags $v; D="$E5/artifacts/$v"; mkdir -p "$D/src"
  cp "$E5/candidate/crf.c" "$E5/candidate/trie.c" "$E5/candidate/trie.h" "$E5/candidate/string_utils.c" "$D/src/"
  for f in crf trie string_utils; do
    case $f in crf) FL="$CRF";; string_utils) FL="$STR";; *) FL="";; esac
    ( cd "$SRC" && gcc $CF $FL "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
  clang -O2 -g -I"$SRC" "$X/../tools/bench_ext.c" "$D/libpostal.a" -o "$D/bench_ext"
  ( cd "$D" && shasum -a 256 src/crf.c src/trie.c src/trie.h src/string_utils.c libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o libpostal.a parser_profile dump_outputs bench_ext > hashes.txt; echo "flags: STR=$STR CRF=$CRF" >> hashes.txt )
done
