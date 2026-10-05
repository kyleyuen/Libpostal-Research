#!/bin/bash
# Builds 009 variants by swapping crf/trie/string_utils/address_parser objects into a temporary copy of the baseline libpostal.a; the copy is deleted after linking (disk is tight).
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E9="$X/009-direct-keys"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
BASE_CRF="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS"
flags() { case $1 in e) CRF="$BASE_CRF";; f) CRF="$BASE_CRF -DEXP_WEIGHT_PREFETCH";; chk) CRF="$BASE_CRF -DEXP_WEIGHT_PREFETCH -DEXP_CHECK_LEN";; esac; }
for v in ${@:-e f chk}; do
  flags $v; D="$E9/artifacts/$v"; mkdir -p "$D/src"
  cp "$E9/candidate/crf.c" "$E9/candidate/crf.h" "$E9/candidate/trie.c" "$E9/candidate/trie.h" "$E9/candidate/string_utils.c" "$E9/candidate/feature_dict.h" "$E9/candidate/address_parser.c" "$D/src/"
  for f in crf trie string_utils address_parser; do
    case $f in crf) FL="$CRF";; string_utils) FL="-DEXP_FAST_APPEND";; address_parser) FL="-DEXP_DIRECT_KEYS";; *) FL="";; esac
    ( cd "$SRC" && gcc $CF $FL "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o libpostal_la-address_parser.o && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
  clang -O2 -g -I"$SRC" "$X/../tools/bench_ext.c" "$D/libpostal.a" -o "$D/bench_ext"
  ( cd "$D" && shasum -a 256 src/crf.c src/address_parser.c src/feature_dict.h libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o libpostal_la-address_parser.o libpostal.a parser_profile dump_outputs bench_ext > hashes.txt; echo "flags: CRF=$CRF AP=-DEXP_DIRECT_KEYS" >> hashes.txt )
  rm -rf "$D/libpostal.a" "$D"/*.dSYM
done
