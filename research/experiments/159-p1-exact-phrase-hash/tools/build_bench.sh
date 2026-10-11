#!/bin/bash
# 159: ps_bench (timing harness for the lead), -O2 like the component would get; linked to the ORIGINAL library (142 ctl) for the "orig" variant.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/159-p1-exact-phrase-hash"; S="$W/libpostal/src"
clang -O2 -g -DHAVE_CONFIG_H '-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"' -I"$S" -I"$S/.." "$E/tools/ps_bench.c" "$E/src/ps_exact.c" "$W/research/experiments/142-stage-oracles/artifacts/ctl/libpostal.a" -lm -o "$E/artifacts/ps_bench"
