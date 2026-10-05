#!/bin/bash
# 016 T11: profile-guided (and link-time) optimization of the `all` variant. Training input: golden-v1 DEVELOPMENT split only (anti-overfit rule).
# Variants: pgo (instrumented profile -> -fprofile-instr-use), pgolto (same + -flto=thin on the recompiled files and the final link).
set -e
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/016-batch"; SRC="$W/libpostal/src"
CF='-std=gnu23 -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -O2 -D LIBPOSTAL_EXPORTS'
BASE_CRF="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT"
BASE_AP="-DEXP_DIRECT_KEYS -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE"
K1="-DEXP_VOCAB_MEMO -DEXP_INT_KEYS"; NEW=$(cat "$E/tools/variants/all")
FILES="crf trie string_utils address_parser features trie_search transliterate normalize crf_context libpostal tokens"
PROF="$E/artifacts/pgo_profile"; mkdir -p "$PROF"
build() {  # $1 = dir, $2 = extra compile flags, $3 = extra link flags
  D="$E/artifacts/$1"; mkdir -p "$D/src"; for f in $FILES; do cp "$E/candidate/$f.c" "$D/src/"; done; cp "$E/candidate/"*.h "$D/src/"; OBJS=""
  for f in $FILES; do
    case $f in crf) FL="$BASE_CRF $K1 $NEW";; address_parser) FL="$BASE_AP $K1 $NEW";; string_utils) FL="-DEXP_FAST_APPEND $NEW";; features) FL="-DEXP_FAST_JOIN $NEW";; *) FL="$NEW";; esac
    ( cd "$SRC" && clang $CF $FL $2 "-DLIBPOSTAL_DATA_DIR=\"/Users/kyle/.cache/libpostal-profile/libpostal\"" -I"$D/src" -I.. -I. -I/usr/local/include -c "$D/src/$f.c" -o "$D/libpostal_la-$f.o" )
    OBJS="$OBJS libpostal_la-$f.o"
  done
  cp "$X/000-baseline/artifacts/libpostal.a" "$D/libpostal.a"; ( cd "$D" && ar -r libpostal.a $OBJS 2>/dev/null && ranlib libpostal.a )
  for t in dump_outputs:"$X/003-batched-lookups/tools/dump_outputs.c" bench_ext:"$X/../tools/bench_ext.c" fuzz_pipeline:"$X/../tools/fuzz_pipeline.c"; do
    clang -O2 -g $3 -I"$SRC" "${t#*:}" "$D/libpostal.a" -lm -o "$D/${t%%:*}"; done
  ( cd "$D" && shasum -a 256 src/* libpostal_la-*.o libpostal.a dump_outputs bench_ext fuzz_pipeline > hashes.txt; echo "flags: $BASE_CRF $K1 $NEW | extra compile: $2 | extra link: $3" >> hashes.txt )
}
build pgogen "-fprofile-instr-generate" "-fprofile-instr-generate"
rm -f "$PROF"/*.profraw
LLVM_PROFILE_FILE="$PROF/dev-%p.profraw" "$E/artifacts/pgogen/dump_outputs" "$W/profiling/golden-v1/development.txt" > /dev/null
xcrun llvm-profdata merge -output="$PROF/dev.profdata" "$PROF"/dev-*.profraw
mkdir -p /Users/kyle/.cache/libpostal-016-pgo && cp "$PROF/dev.profdata" /Users/kyle/.cache/libpostal-016-pgo/dev.profdata   # compile flags cannot carry a path with spaces
PU=/Users/kyle/.cache/libpostal-016-pgo/dev.profdata
shasum -a 256 "$PROF/dev.profdata" "$PU" "$W/profiling/golden-v1/development.txt" > "$PROF/hashes.txt"
build pgo "-fprofile-instr-use=$PU -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date" ""
build pgolto "-fprofile-instr-use=$PU -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date -flto=thin" "-flto=thin"
for d in pgogen pgo pgolto; do rm -f "$E/artifacts/$d/libpostal.a"; done
