#!/bin/bash
# 141: link 041's bench_warm against cpi's libpostal.a (same -flto/mimalloc/PGO flags as 119 build_cc.sh) and against the original 000-baseline libpostal.a.
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/141-warm-bound-cpi"; X="$E/.."; SRC="/Users/kyle/Desktop/HW/CSCI 201 Reserach/libpostal/src"; PD=/Users/kyle/.cache/libpostal-020-pgo/cpi/dev.profdata
cd "$E"
echo "# $(date '+%Y-%m-%dT%H:%M:%S%z') tools/link.sh" >> raw/commands.txt
clang -O2 -g -flto /opt/homebrew/opt/mimalloc/lib/mimalloc.o -fprofile-instr-use=$PD -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date -I"$SRC" tools/bench_warm.c "$X/119-component-phrase-ids/artifacts/cpi/libpostal.a" -lm -o tools/bench_warm_cpi > raw/link_cpi.stdout 2> raw/link_cpi.stderr; echo $? > raw/link_cpi.exit
clang -O2 -g -I"$SRC" tools/bench_warm.c "$X/000-baseline/artifacts/libpostal.a" -lm -o tools/bench_warm_orig > raw/link_orig.stdout 2> raw/link_orig.stderr; echo $? > raw/link_orig.exit
clang --version | head -1 > raw/clang_version.txt
shasum -a 256 tools/bench_warm.c tools/bench_split_vt.c tools/bench_warm_cpi tools/bench_warm_orig "$X/000-baseline/artifacts/libpostal.a" "$X/119-component-phrase-ids/artifacts/cpi/libpostal.a" $PD > raw/hashes.txt
