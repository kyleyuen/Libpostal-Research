#!/bin/bash
# 012 variants on top of 011 `h`: h0 control, iA inline dictionary hash/compare, iB fast Viterbi, iC phrase-search prefetch, i = all three.
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E12="$X/012-inline-viterbi-search"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
flags() { CRF="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT"; AP="-DEXP_DIRECT_KEYS -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE"; TS=""
  case $1 in h0) ;; iA) CRF="$CRF -DEXP_FD_INLINE"; AP="$AP -DEXP_FD_INLINE";; iB) CRF="$CRF -DEXP_FAST_VITERBI";; iC) TS="-DEXP_SEARCH_PREFETCH";;
    i) CRF="$CRF -DEXP_FD_INLINE -DEXP_FAST_VITERBI"; AP="$AP -DEXP_FD_INLINE"; TS="-DEXP_SEARCH_PREFETCH";; esac; }
for v in ${@:-h0 iA iB iC i}; do
  flags $v; D="$E12/artifacts/$v"; mkdir -p "$D/src"
  cp "$E12/candidate/crf.c" "$E12/candidate/crf.h" "$E12/candidate/trie.c" "$E12/candidate/trie.h" "$E12/candidate/string_utils.c" "$E12/candidate/feature_dict.h" "$E12/candidate/address_parser.c" "$E12/candidate/features.c" "$E12/candidate/trie_search.c" "$D/src/"
  for f in crf trie string_utils address_parser features trie_search; do
    case $f in crf) FL="$CRF";; string_utils) FL="-DEXP_FAST_APPEND";; address_parser) FL="$AP";; features) FL="-DEXP_FAST_JOIN";; trie_search) FL="$TS";; *) FL="";; esac
    ( cd "$SRC" && gcc $CF $FL "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o libpostal_la-address_parser.o libpostal_la-features.o libpostal_la-trie_search.o && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
  clang -O2 -g -I"$SRC" "$X/../tools/bench_ext.c" "$D/libpostal.a" -o "$D/bench_ext"
  clang -O2 -g -I"$SRC" "$X/../tools/fuzz_pipeline.c" "$D/libpostal.a" -lm -o "$D/fuzz_pipeline"
  ( cd "$D" && shasum -a 256 src/crf.c src/address_parser.c src/feature_dict.h src/trie_search.c libpostal_la-*.o libpostal.a parser_profile dump_outputs bench_ext fuzz_pipeline > hashes.txt; echo "flags: CRF=$CRF AP=$AP TS=$TS" >> hashes.txt )
  rm -rf "$D/libpostal.a" "$D"/*.dSYM
done
