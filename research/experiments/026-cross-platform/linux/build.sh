#!/usr/bin/env bash
# Run inside the pinned Ubuntu ARM64 image with /out, /workspace and /model mounts.
set -euo pipefail
set -x

variant=${1:?baseline, baseline_upstream, or candidate}
case "$variant" in baseline|baseline_upstream|candidate) ;; *) exit 2 ;; esac
src=/out/source/src
overlay=/workspace/research/final/021-l4/sources
dest=/out/artifacts/$variant
if [ -e "$dest/build.complete" ]; then echo "build already complete: $dest" >&2; exit 2; fi
mkdir -p "$dest/obj"

base=(-std=gnu2x -DHAVE_CONFIG_H -Wall -Wno-unused-function -Wno-unused-parameter
      -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g
      -D LIBPOSTAL_EXPORTS '-DLIBPOSTAL_DATA_DIR="/model"')
switches=(-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS
          -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT -DEXP_VOCAB_DICT -DEXP_ASCII_NORM
          -DEXP_ADMIN_SHARE -DEXP_FAST_APPEND -DEXP_FAST_JOIN -DEXP_VOCAB_MEMO
          -DEXP_INT_KEYS -DEXP_TRANS_SKIP -DEXP_NORM_TABLE -DEXP_CANON_CONST
          -DEXP_RESPONSE_BLOCK -DEXP_REUSE -DEXP_VITERBI_NEON -DEXP_TOKEN_ASCII
          -DEXP_FD_INLINE -DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK
          -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS
          -DEXP_ST_ROW0 -DEXP_LOADF=4)
changed='crf trie string_utils address_parser features trie_search transliterate normalize crf_context libpostal tokens'
libsrc='strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard'

scanner_opt=-O1
if [ "$variant" = baseline_upstream ]; then scanner_opt=-O0; fi
printf '%s\n' "variant=$variant" "compiler=$(clang --version | head -1)" \
  "base=${base[*]}" "switches=${switches[*]}" "library=-O2" \
  "scanner=$scanner_opt" "link=-O2" > "$dest/build_settings.txt"

for file in $libsrc; do
  stem=${file##*/}
  if [ "$variant" = baseline_upstream ]; then
    cp "/out/artifacts/baseline/obj/${file//\//_}.o" "$dest/obj/"
    continue
  fi
  path="$src/$file.c"
  if [ "$variant" = candidate ] && [[ " $changed " == *" $stem "* ]]; then
    path="$overlay/$stem.c"
  fi
  # libpostal's features.h must not shadow glibc's <features.h>.
  includes=(-iquote "$src" -I/out/source)
  flags=()
  if [ "$variant" = candidate ]; then
    includes=(-iquote "$overlay" "${includes[@]}")
    flags=("${switches[@]}")
  fi
  clang "${base[@]}" "${flags[@]}" -O2 "${includes[@]}" -c "$path" \
    -o "$dest/obj/${file//\//_}.o"
done
if [ "$variant" = candidate ]; then
  for _ in $(seq 1 900); do
    if [ -f /out/artifacts/baseline/obj/scanner.o ] && [ -f /out/artifacts/baseline/obj/drand48.o ]; then break; fi
    sleep 1
  done
  test -f /out/artifacts/baseline/obj/scanner.o
  test -f /out/artifacts/baseline/obj/drand48.o
  cp /out/artifacts/baseline/obj/scanner.o "$dest/obj/scanner.o"
  cp /out/artifacts/baseline/obj/drand48.o "$dest/obj/drand48.o"
  printf '%s\n' 'scanner_object=reused_baseline_identical_flags' >> "$dest/build_settings.txt"
else
  clang "${base[@]}" "$scanner_opt" -iquote "$src" -I/out/source -c "$src/scanner.c" -o "$dest/obj/scanner.o"
  clang "${base[@]}" "$scanner_opt" -iquote "$src" -I/out/source -c "$src/klib/drand48.c" -o "$dest/obj/drand48.o"
fi
llvm-ar rcs "$dest/libpostal.a" "$dest"/obj/*.o
clang -O2 -g -iquote "$src" /workspace/research/experiments/003-batched-lookups/tools/dump_outputs.c \
  "$dest/libpostal.a" -lm -o "$dest/dump_outputs"
clang -O2 -g -iquote "$src" /out/bench_ext_linux.c "$dest/libpostal.a" -lm -o "$dest/bench_ext"
sha256sum "$dest/libpostal.a" "$dest/dump_outputs" "$dest/bench_ext" > "$dest/binary_hashes.txt"
date -Is > "$dest/build.complete"
