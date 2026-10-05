#!/usr/bin/env bash
# Rebuild 021 control or 027 cache with the accepted 021 flags and a fresh PGO profile.
# usage: build_cc.sh control|cache[_attempt] clang llvm '-O2 -flto' '-O1' '-flto' /path/to/mimalloc.o
set -euo pipefail
NAME=$1; CC=$2; PM=$3; LOPT=$4; SOPT=$5; LINK=$6; XOBJ=${7:-}
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; E="$X/027-root-transition"; SRC="$W/libpostal/src"
case "$NAME" in
  control*) C="$W/research/final/021-l4/sources"; VARIANT=control;;
  cache*) C="$E/sources"; VARIANT=cache;;
  *) echo "unknown variant: $NAME" >&2; exit 2;;
esac
BASE="-std=gnu2x -DHAVE_CONFIG_H -Wall -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare ${DEBUG_FLAG--g} -D LIBPOSTAL_EXPORTS"
SW="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE -DEXP_FAST_APPEND -DEXP_FAST_JOIN -DEXP_VOCAB_MEMO -DEXP_INT_KEYS -DEXP_TRANS_SKIP -DEXP_NORM_TABLE -DEXP_CANON_CONST -DEXP_RESPONSE_BLOCK -DEXP_REUSE -DEXP_VITERBI_NEON -DEXP_TOKEN_ASCII -DEXP_FD_INLINE ${EXTRA_SW:-}"
CHANGED="crf trie string_utils address_parser features trie_search transliterate normalize crf_context libpostal tokens"
LIBSRC="strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard"
DD='-DLIBPOSTAL_DATA_DIR="/Users/kyle/.cache/libpostal-profile/libpostal"'
build() {
  BN=$1
  D="$E/artifacts/$BN"
  shift
  PF=("$@")
  if [ -e "$D" ]; then echo "artifact directory already exists: $D" >&2; exit 2; fi
  mkdir -p "$D/src" "$D/obj"
  for f in $CHANGED; do cp "$C/$f.c" "$D/src/"; done
  cp "$C/"*.h "$D/src/"
  t0=$(date +%s)
  for f in $LIBSRC; do
    b=$(basename "$f")
    case " $CHANGED " in *" $b "*) S="$D/src/$b.c";; *) S="$SRC/$f.c";; esac
    ( cd "$SRC" && $CC $BASE $SW $LOPT "${PF[@]}" "$DD" -I"$D/src" -I"$SRC" -I"$SRC/.." -I/usr/local/include -c "$S" -o "$D/obj/$(echo "$f" | tr / _).o" )
  done
  SK="$X/021-final-stack/artifacts/scanner_cache/scanner_$(echo "$CC" | tr -dc 'A-Za-z0-9')_$(echo "$SOPT" | tr -dc 'A-Za-z0-9').o"
  if [ -f "$SK" ]; then cp "$SK" "$D/obj/scanner.o"; else
    ( cd "$SRC" && $CC $BASE $SOPT "$DD" -I"$SRC" -I"$SRC/.." -c "$SRC/scanner.c" -o "$D/obj/scanner.o" )
  fi
  ( cd "$SRC" && $CC $BASE $SOPT -I"$SRC" -I"$SRC/.." -c "$SRC/klib/drand48.c" -o "$D/obj/drand48.o" )
  ( cd "$D/obj" && ${AR:-ar} -rc ../libpostal.a *.o && ${RANLIB:-ranlib} ../libpostal.a )
  for t in dump_outputs:"$X/003-batched-lookups/tools/dump_outputs.c" bench_ext:"$W/research/tools/bench_ext.c" fuzz_pipeline:"$W/research/tools/fuzz_pipeline.c"; do
    $CC -O2 ${DEBUG_FLAG--g} $LINK "${PF[@]}" -I"$SRC" "$XOBJ" "${t#*:}" "$D/libpostal.a" -lm -o "$D/${t%%:*}"
  done
  if [ "$BN" = "$NAME" ] && [ "$VARIANT" = cache ]; then
    $CC -O2 ${DEBUG_FLAG--g} $LINK "${PF[@]}" "$DD" -I"$D/src" -I"$SRC" "$XOBJ" "$E/tools/check_root_cache.c" "$D/libpostal.a" -lm -o "$D/check_root_cache"
  fi
  printf '%s seconds=%s\n' "$BN" "$(( $(date +%s)-t0 ))" >> "$E/raw/compile_times.txt"
  ( cd "$D" && shasum -a 256 src/* libpostal.a dump_outputs bench_ext fuzz_pipeline > hashes.txt )
  if [ -e "$D/check_root_cache" ]; then shasum -a 256 "$D/check_root_cache" >> "$D/hashes.txt"; fi
  printf 'cc=%s\nbase=%s\nswitches=%s\nlib=%s %s\nscanner=%s\nlink=%s %s\nextra_link_object=%s\n' "$CC" "$BASE" "$SW" "$LOPT" "${PF[*]}" "$SOPT" "$LINK" "${PF[*]}" "$XOBJ" >> "$D/build_settings.txt"
}
P="$E/pgo/$NAME"
if [ -e "$P" ]; then echo "profile directory already exists: $P" >&2; exit 2; fi
mkdir -p "$P"
case "$PM" in
  llvm)
    PD=$(dirname "$CC")/llvm-profdata; [ -x "$PD" ] || PD="xcrun llvm-profdata"
    build "${NAME}_gen" "-fprofile-instr-generate"
    train="$E/artifacts/${NAME}_gen/dump_outputs"
    printf 'start=%s\nargv: <%s> <%s>\n' "$(date -Iseconds)" "$train" "$W/profiling/golden-v1/development.txt" > "$E/raw/pgo_${NAME}_train.meta"
    set +e
    LLVM_PROFILE_FILE="$P/p-%p.profraw" "$train" "$W/profiling/golden-v1/development.txt" > "$E/raw/pgo_${NAME}_train.stdout" 2> "$E/raw/pgo_${NAME}_train.stderr"
    rc=$?
    set -e
    printf 'exit=%s\nend=%s\n' "$rc" "$(date -Iseconds)" >> "$E/raw/pgo_${NAME}_train.meta"
    [ "$rc" -eq 0 ] || exit "$rc"
    $PD merge -output="$P/dev.profdata" "$P"/*.profraw
    build "$NAME" "-fprofile-instr-use=$P/dev.profdata" -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date;;
  none) build "$NAME" "";;
  *) echo "unsupported profile mode: $PM" >&2; exit 2;;
esac
