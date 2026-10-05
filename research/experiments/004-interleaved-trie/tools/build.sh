#!/bin/bash
# Builds candidates L4/L8/L16 (+ dump tools) by swapping crf.o and trie.o into copies of the preserved baseline libpostal.a. usage: build.sh [lanes...]
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E4="$X/004-interleaved-trie"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
LANES="${@:-4 8 16}"
for L in $LANES; do
  D="$E4/artifacts/L$L"; mkdir -p "$D/src"
  cp "$E4/candidate/crf.c" "$E4/candidate/trie.c" "$E4/candidate/trie.h" "$D/src/"
  for f in crf trie; do
    ( cd "$SRC" && gcc $CF -DTRIE_BATCH_LANES=$L "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o libpostal_la-trie.o && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  clang -O2 -g -I"$SRC" "$X/003-batched-lookups/tools/dump_outputs.c" "$D/libpostal.a" -o "$D/dump_outputs"
  ( cd "$D" && shasum -a 256 src/crf.c src/trie.c src/trie.h libpostal_la-crf.o libpostal_la-trie.o libpostal.a parser_profile dump_outputs > hashes.txt )
done
