#!/bin/bash
# 010 variants (each a switch on top of 009 f): f0 control, fJ fast feature join, fC lean Viterbi context, fE arena prefetch, fV vocab dictionary, g = all four.
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E10="$X/010-vocab-dict"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
BASE="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH"
flags() { CRF="$BASE"; AP="-DEXP_DIRECT_KEYS"; FE=""
  case $1 in f0) ;; fJ) FE="-DEXP_FAST_JOIN";; fC) CRF="$BASE -DEXP_LEAN_CONTEXT";; fE) CRF="$BASE -DEXP_ARENA_PREFETCH";; fV) AP="-DEXP_DIRECT_KEYS -DEXP_VOCAB_DICT";;
    g) FE="-DEXP_FAST_JOIN"; CRF="$BASE -DEXP_LEAN_CONTEXT -DEXP_ARENA_PREFETCH"; AP="-DEXP_DIRECT_KEYS -DEXP_VOCAB_DICT";; esac; }
for v in ${@:-f0 fJ fC fE fV g}; do
  flags $v; D="$E10/artifacts/$v"; mkdir -p "$D/src"
  cp "$E10/candidate/crf.c" "$E10/candidate/crf.h" "$E10/candidate/trie.c" "$E10/candidate/trie.h" "$E10/candidate/string_utils.c" "$E10/candidate/feature_dict.h" "$E10/candidate/address_parser.c" "$E10/candidate/features.c" "$D/src/"
  for f in crf trie string_utils address_parser features; do
    case $f in crf) FL="$CRF";; string_utils) FL="-DEXP_FAST_APPEND";; address_parser) FL="$AP";; features) FL="$FE";; *) FL="";; esac
    ( cd "$SRC" && gcc $CF $FL "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o libpostal_la-address_parser.o libpostal_la-features.o && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
  clang -O2 -g -I"$SRC" "$X/../tools/bench_ext.c" "$D/libpostal.a" -o "$D/bench_ext"
  ( cd "$D" && shasum -a 256 src/crf.c src/address_parser.c src/features.c src/feature_dict.h libpostal_la-*.o libpostal.a parser_profile dump_outputs bench_ext > hashes.txt; echo "flags: CRF=$CRF AP=$AP FE=$FE" >> hashes.txt )
  rm -rf "$D/libpostal.a" "$D"/*.dSYM
done
