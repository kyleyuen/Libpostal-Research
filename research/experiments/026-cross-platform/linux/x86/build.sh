#!/usr/bin/env bash
# Functional x86-64 Linux build under Docker emulation; no speed claim.
set -euo pipefail
set -x
variant=${1:?upstream or candidate}
case "$variant" in upstream|candidate) ;; *) exit 2 ;; esac
src=/out/x86/source/src
overlay=/workspace/research/final/021-l4/sources
dest=/out/x86/artifacts/$variant
test ! -e "$dest/build.complete"
mkdir -p "$dest/obj"
base=(-std=gnu2x -DHAVE_CONFIG_H -DUSE_SSE -msse2 -mfpmath=sse
      -Wall -Wno-unused-function -Wno-unused-parameter -Wformat
      -Werror=format-security -Winit-self -Wno-sign-compare -g
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
printf '%s\n' "variant=$variant" "compiler=$(clang --version | head -1)" \
  "base=${base[*]}" "switches=${switches[*]}" "library=-O2" \
  "scanner=-O0" "link=-O2" > "$dest/build_settings.txt"
for file in $libsrc; do
  stem=${file##*/}
  path="$src/$file.c"
  includes=(-iquote "$src" -I/out/x86/source)
  flags=()
  if [ "$variant" = candidate ]; then
    if [[ " $changed " == *" $stem "* ]]; then path="$overlay/$stem.c"; fi
    includes=(-iquote "$overlay" "${includes[@]}")
    flags=("${switches[@]}")
  fi
  clang "${base[@]}" "${flags[@]}" -O2 "${includes[@]}" -c "$path" \
    -o "$dest/obj/${file//\//_}.o"
done
if [ "$variant" = candidate ]; then
  cp /out/x86/artifacts/upstream/obj/scanner.o "$dest/obj/scanner.o"
  cp /out/x86/artifacts/upstream/obj/drand48.o "$dest/obj/drand48.o"
else
  clang "${base[@]}" -O0 -iquote "$src" -I/out/x86/source -c "$src/scanner.c" \
    -o "$dest/obj/scanner.o"
  clang "${base[@]}" -O0 -iquote "$src" -I/out/x86/source -c "$src/klib/drand48.c" \
    -o "$dest/obj/drand48.o"
fi
llvm-ar rcs "$dest/libpostal.a" "$dest"/obj/*.o
clang -O2 -g -iquote "$src" /workspace/research/experiments/003-batched-lookups/tools/dump_outputs.c \
  "$dest/libpostal.a" -lm -o "$dest/dump_outputs"
sha256sum "$dest/libpostal.a" "$dest/dump_outputs" > "$dest/binary_hashes.txt"
date -Is > "$dest/build.complete"
