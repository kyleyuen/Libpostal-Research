#!/bin/bash
# 015 variants on top of 011 `h` (sources copied from 012's candidate, all 012 switches off):
#   h0 control (no new switch), m vocabulary memo only, k1 memo + integer keys for single-word features, k2 = k1 + word-pair features, kv = k2 + lookup verification.
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/015-integer-keys"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
flags() { CRF="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT"; AP="-DEXP_DIRECT_KEYS -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE"; NEW=""
  case $1 in h0) ;; m) NEW="-DEXP_VOCAB_MEMO";; k1) NEW="-DEXP_VOCAB_MEMO -DEXP_INT_KEYS";; k2) NEW="-DEXP_VOCAB_MEMO -DEXP_INT_KEYS -DEXP_INT_KEYS_PAIRS";;
    kv) NEW="-DEXP_VOCAB_MEMO -DEXP_INT_KEYS -DEXP_INT_KEYS_PAIRS -DEXP_INT_KEYS_VERIFY";; *) echo "unknown variant $1"; exit 2;; esac
  CRF="$CRF $NEW"; AP="$AP $NEW"; }
for v in ${@:-h0 m k1 k2 kv}; do
  flags $v; D="$E/artifacts/$v"; mkdir -p "$D/src"
  cp "$E/candidate/crf.c" "$E/candidate/crf.h" "$E/candidate/trie.c" "$E/candidate/trie.h" "$E/candidate/string_utils.c" "$E/candidate/feature_dict.h" "$E/candidate/address_parser.c" "$E/candidate/features.c" "$E/candidate/trie_search.c" "$D/src/"
  for f in crf trie string_utils address_parser features trie_search; do
    case $f in crf) FL="$CRF";; string_utils) FL="-DEXP_FAST_APPEND";; address_parser) FL="$AP";; features) FL="-DEXP_FAST_JOIN";; *) FL="";; esac
    ( cd "$SRC" && gcc $CF $FL "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o libpostal_la-trie.o libpostal_la-string_utils.o libpostal_la-address_parser.o libpostal_la-features.o libpostal_la-trie_search.o 2>/dev/null && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
  clang -O2 -g -I"$SRC" "$X/../tools/bench_ext.c" "$D/libpostal.a" -o "$D/bench_ext"
  clang -O2 -g -I"$SRC" "$X/../tools/fuzz_pipeline.c" "$D/libpostal.a" -lm -o "$D/fuzz_pipeline"
  ( cd "$D" && shasum -a 256 src/* libpostal_la-*.o libpostal.a parser_profile dump_outputs bench_ext fuzz_pipeline > hashes.txt; echo "flags: CRF=$CRF AP=$AP" >> hashes.txt )
  rm -rf "$D/libpostal.a" "$D"/*.dSYM
done
