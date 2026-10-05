#!/bin/bash
# Builds control (unchanged crf.c) and candidate (candidate/crf.c) by swapping ONE object into a copy of the preserved baseline libpostal.a.
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E3="$X/003-batched-lookups"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
for v in control candidate; do
  D="$E3/artifacts/$v"; mkdir -p "$D"
  if [ $v = control ]; then C="$E3/candidate/crf.c.orig"; else C="$E3/candidate/crf.c"; fi
  cp "$C" "$D/crf_src.c"
  ( cd "$SRC" && gcc $CF "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I.. -I. -I/usr/local/include -include /dev/null -c "$D/crf_src.c" -o "$D/libpostal_la-crf.o" )
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"
  ( cd "$D" && ar -r libpostal.a libpostal_la-crf.o && ranlib libpostal.a )
  clang -O2 -g -I"$SRC" "$X/000-baseline/artifacts/parser_profile.c" "$D/libpostal.a" -o "$D/parser_profile"
  ( cd "$D" && shasum -a 256 crf_src.c libpostal_la-crf.o libpostal.a parser_profile > hashes.txt )
done
