#!/bin/bash
# 011 variants on top of 010's best (fast join + lean context + vocab dictionary; no arena prefetch): g2 control, h1 ASCII fast normalization, h2 shared admin normalization, h both.
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E11="$X/011-normalize-fastpaths"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
CRF="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT"
flags() { AP="-DEXP_DIRECT_KEYS -DEXP_VOCAB_DICT"; case $1 in g2) ;; h1) AP="$AP -DEXP_ASCII_NORM";; h2) AP="$AP -DEXP_ADMIN_SHARE";; h) AP="$AP -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE";; esac; }
for v in ${@:-g2 h1 h2 h}; do
  flags $v; D="$E11/artifacts/$v"; mkdir -p "$D/src"
  cp "$E11/candidate/crf.c" "$E11/candidate/crf.h" "$E11/candidate/trie.c" "$E11/candidate/trie.h" "$E11/candidate/string_utils.c" "$E11/candidate/feature_dict.h" "$E11/candidate/address_parser.c" "$E11/candidate/features.c" "$D/src/"
  for f in crf trie string_utils address_parser features; do
    case $f in crf) FL="$CRF";; string_utils) FL="-DEXP_FAST_APPEND";; address_parser) FL="$AP";; features) FL="-DEXP_FAST_JOIN";; *) FL="";; esac
    ( cd "$SRC" && gcc $CF $FL "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o libpostal_la-address_parser.o libpostal_la-features.o && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
  clang -O2 -g -I"$SRC" "$X/../tools/bench_ext.c" "$D/libpostal.a" -o "$D/bench_ext"
  ( cd "$D" && shasum -a 256 src/crf.c src/address_parser.c src/features.c src/feature_dict.h libpostal_la-*.o libpostal.a parser_profile dump_outputs bench_ext > hashes.txt; echo "flags: CRF=$CRF AP=$AP FE=-DEXP_FAST_JOIN" >> hashes.txt )
  rm -rf "$D/libpostal.a" "$D"/*.dSYM
done
