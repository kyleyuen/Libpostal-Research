#!/bin/bash
# 159: build the P1 harness against the ORIGINAL library (142 ctl build: original sources, -O2). ps_exact.c is the component under test.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/159-p1-exact-phrase-hash"; S="$W/libpostal/src"
clang -O2 -g -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -DHAVE_CONFIG_H '-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"' \
  -I"$S" -I"$S/.." "$E/tools/ps_test.c" "$E/src/ps_exact.c" "$W/research/experiments/142-stage-oracles/artifacts/ctl/libpostal.a" -lm -o "$E/artifacts/ps_test"
