#!/bin/bash
# Build matching release/diagnostic pairs serially from the frozen 185 source copies.
set -eo pipefail

ROOT="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
EXP="$ROOT/research/experiments/185-diagnostic-instrumentation"
LIBPOSTAL_SRC="$ROOT/libpostal/src"
MODEL_DIR="${LIBPOSTAL_MODEL_DIR:-/Users/kyle/.cache/libpostal-profile/libpostal}"
BASE_FLAGS=(-std=gnu2x -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function
  -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self
  -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS)
CPI_FLAGS=(-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS
  -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT -DEXP_VOCAB_DICT -DEXP_ASCII_NORM
  -DEXP_ADMIN_SHARE -DEXP_FAST_APPEND -DEXP_FAST_JOIN -DEXP_VOCAB_MEMO
  -DEXP_INT_KEYS -DEXP_TRANS_SKIP -DEXP_NORM_TABLE -DEXP_CANON_CONST
  -DEXP_RESPONSE_BLOCK -DEXP_REUSE -DEXP_VITERBI_NEON -DEXP_TOKEN_ASCII
  -DEXP_FD_INLINE)
# These are the recorded production-deliverable EXTRA_SW flags in
# research/final/119-cpi/switches.txt. Keep the base SW above as well.
CPI_DELIVERY_FLAGS=(-DEXP_NAME_LEN -DEXP_NORM_HANGUL -DEXP_ROW_PACK
  -DEXP_ROLE_IDS -DEXP_WORD_MEMO -DEXP_WM_CANON -DEXP_CONST_IDS -DEXP_ST_ROW0
  -DEXP_LOADF=3 -DEXP_VITERBI20 -DEXP_TRANS_ALIAS -DEXP_PC_COMPRESS -DEXP_FTT
  -DEXP_FTT_ROLE_PREFETCH -DEXP_PC_PREFETCH -DEXP_VITERBI_MAXONLY
  -DEXP_VITERBI_FMAX -DEXP_NORM_PAIR -DEXP_PS_FLAGS -DEXP_ROW_LINES=8
  -DEXP_ROW_STEP=128 -DEXP_ROW_TRANS_ONLY -DEXP_CP_IDS -DEXP_NAME_BYTE
  -DEXP_LETTERS_FAST -DEXP_FA_LEN -DEXP_DIGITS_FAST -DEXP_FTT_PF1 -DEXP_FTT_PF2)
PROTOTYPE181_FLAGS=(-DEXP_FUSED_FRONT -DEXP_EVENT_FLOW)
DATA_FLAG="-DLIBPOSTAL_DATA_DIR=\"$MODEL_DIR\""
LIBSRC=(strndup libpostal expand address_dictionary transliterate tokens trie
  trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize
  numex features unicode_scripts address_parser address_parser_io
  averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger
  graph graph_builder language_classifier language_features logistic_regression
  logistic minibatch float_utils ngrams place near_dupe double_metaphone
  geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard)
VARIANTS=(original cpi prototype181)

usage() {
  cat <<'EOF'
Usage:
  bash research/experiments/185-diagnostic-instrumentation/tools/build_variants.sh --preflight
  bash research/experiments/185-diagnostic-instrumentation/tools/build_variants.sh --build

--preflight checks files and tools only. --build creates a new timestamped,
serial six-build record. It never removes existing artifacts or runs a model.
EOF
}

preflight() {
  local required f dir
  for required in "$MODEL_DIR" \
      "$ROOT/research/experiments/000-baseline/artifacts/libpostal.a" \
      "$ROOT/research/experiments/182-benchmark-suite-prep/artifacts/original-000/dump_outputs" \
      "$EXP/tools/dump_outputs.c" "$EXP/source/diag.c" "$EXP/tools/build_variants.sh"; do
    [[ -e "$required" ]] || { printf 'Missing: %s\n' "$required" >&2; return 1; }
  done
  [[ -d "$MODEL_DIR" ]] || { printf 'Model path is not a directory: %s\n' "$MODEL_DIR" >&2; return 1; }
  command -v clang >/dev/null || { echo 'clang not found' >&2; return 1; }
  command -v ar >/dev/null || { echo 'ar not found' >&2; return 1; }
  command -v ranlib >/dev/null || { echo 'ranlib not found' >&2; return 1; }
  for dir in "${VARIANTS[@]}"; do
    [[ -d "$EXP/source/$dir" ]] || { printf 'Missing source root: %s\n' "$EXP/source/$dir" >&2; return 1; }
    for f in "${LIBSRC[@]}" scanner klib/drand48; do
      [[ -f "$EXP/source/$dir/$f.c" ]] || {
        printf 'Missing source file: %s\n' "$EXP/source/$dir/$f.c" >&2; return 1;
      }
    done
    [[ -f "$EXP/source/$dir/diag.h" ]] || { printf 'Missing diag.h: %s\n' "$dir" >&2; return 1; }
  done
  printf 'Preflight passed. No compiler or model process was started.\n'
}

if [[ $# -ne 1 ]]; then usage >&2; exit 2; fi
case "$1" in
  --preflight) preflight; exit $? ;;
  --build) ;;
  -h|--help) usage; exit 0 ;;
  *) usage >&2; exit 2 ;;
esac

preflight
BUILD_ID="${BUILD_ID:-$(date '+%Y%m%d-%H%M%S')-$$}"
RAW="$EXP/raw/build-$BUILD_ID"
ART="$EXP/artifacts/build-$BUILD_ID"
[[ ! -e "$RAW" && ! -e "$ART" ]] || { echo "Build ID already exists: $BUILD_ID" >&2; exit 2; }
mkdir -p "$RAW" "$ART" "$RAW/source-hashes" "$RAW/provenance"
printf 'build_id=%s\nstart=%s\ncommand=%q --build\n' \
  "$BUILD_ID" "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$0" > "$RAW/session.txt"
CLANG="$(command -v clang)"
AR_BIN="$(command -v ar)"
RANLIB_BIN="$(command -v ranlib)"
printf 'clang=%s\nar=%s\nranlib=%s\nPATH=%s\nSDKROOT=%s\nDEVELOPER_DIR=%s\n' \
  "$CLANG" "$AR_BIN" "$RANLIB_BIN" "$PATH" "${SDKROOT:-}" "${DEVELOPER_DIR:-}" >> "$RAW/session.txt"
finish() {
  local status=$?
  trap - EXIT
  printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$RAW/session.txt"
  exit "$status"
}
trap finish EXIT

hash_sources() {
  local variant
  for variant in "${VARIANTS[@]}"; do
    (cd "$EXP" && find "source/$variant" -type f -print0 | sort -z | xargs -0 shasum -a 256)
  done
  (cd "$EXP" && shasum -a 256 source/diag.c tools/dump_outputs.c tools/build_variants.sh)
}

hash_sources > "$RAW/source-before.sha256"

COMMAND_NUMBER=0
CURRENT_LOG=
run_cmd() {
  local label="$1" cwd="$2" n stem arg
  shift 2
  COMMAND_NUMBER=$((COMMAND_NUMBER + 1))
  n=$(printf '%04d' "$COMMAND_NUMBER")
  stem="$CURRENT_LOG/$n-$label"
  {
    printf 'start=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')"
    printf 'cwd=%q\nargv=' "$cwd"
    for arg in "$@"; do printf '%q ' "$arg"; done
    printf '\n'
  } > "$stem.command"
  local status=0
  if (cd "$cwd" && "$@") > "$stem.stdout" 2> "$stem.stderr"; then
    status=0
  else
    status=$?
  fi
  printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$stem.command"
  printf '%s %s\n' "$stem" "$status" >> "$CURRENT_LOG/exit_codes.txt"
  return "$status"
}

for variant in "${VARIANTS[@]}"; do
  source_root="$EXP/source/$variant"
  case "$variant" in
    original) variant_flags=() ;;
    cpi) variant_flags=("${CPI_FLAGS[@]}" "${CPI_DELIVERY_FLAGS[@]}") ;;
    prototype181) variant_flags=("${PROTOTYPE181_FLAGS[@]}") ;;
  esac
  for mode in release diagnostic; do
    output="$ART/$variant/$mode"
    obj="$output/obj"
    CURRENT_LOG="$RAW/commands/$variant-$mode"
    mkdir -p "$obj" "$CURRENT_LOG"
    COMMAND_NUMBER=0
    if [[ "$mode" == diagnostic ]]; then mode_flags=(-DEXP185_DIAG); else mode_flags=(); fi
    {
      printf 'BASE_FLAGS\n'; printf '%s\n' "${BASE_FLAGS[@]}"
      printf 'VARIANT_FLAGS\n'; printf '%s\n' "${variant_flags[@]}"
      if [[ "$variant" == cpi ]]; then
        printf '119_DELIVERY_EXTRA_FLAGS\n'; printf '%s\n' "${CPI_DELIVERY_FLAGS[@]}"
      fi
      printf 'MODE_FLAGS\n'; printf '%s\n' "${mode_flags[@]}"
      printf 'DATA_FLAG\n%s\n' "$DATA_FLAG"
      printf 'LIBRARY_OPTIMIZATION\n-O2\nSCANNER_OPTIMIZATION\n-O0\n'
      printf 'LINK_FLAGS\n-O2 -g\nMODEL_DIR\n%s\n' "$MODEL_DIR"
    } > "$RAW/flags-$variant-$mode.txt"
    printf 'start=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" > "$RAW/session-$variant-$mode.txt"
    for f in "${LIBSRC[@]}"; do
      object=${f//\//_}.o
      run_cmd "compile-${object%.o}" "$LIBPOSTAL_SRC" \
        "$CLANG" "${BASE_FLAGS[@]}" -O2 "${variant_flags[@]}" "${mode_flags[@]}" "$DATA_FLAG" \
        -I"$source_root" -I"$source_root/.." -I"$LIBPOSTAL_SRC" -I"$LIBPOSTAL_SRC/.." \
        -c "$source_root/$f.c" -o "$obj/$object"
    done
    if [[ "$mode" == diagnostic ]]; then
      run_cmd compile-diag "$LIBPOSTAL_SRC" \
        "$CLANG" "${BASE_FLAGS[@]}" -O2 "${variant_flags[@]}" "${mode_flags[@]}" "$DATA_FLAG" \
        -I"$source_root" -I"$source_root/.." -I"$LIBPOSTAL_SRC" -I"$LIBPOSTAL_SRC/.." \
        -c "$EXP/source/diag.c" -o "$obj/diag.o"
    fi
    run_cmd compile-scanner "$LIBPOSTAL_SRC" \
      "$CLANG" "${BASE_FLAGS[@]}" -O0 "${variant_flags[@]}" "$DATA_FLAG" \
      -I"$source_root" -I"$source_root/.." -I"$LIBPOSTAL_SRC" -I"$LIBPOSTAL_SRC/.." \
      -c "$source_root/scanner.c" -o "$obj/scanner.o"
    run_cmd compile-drand48 "$LIBPOSTAL_SRC" \
      "$CLANG" "${BASE_FLAGS[@]}" -O0 -I"$source_root" -I"$source_root/.." \
      -I"$LIBPOSTAL_SRC" -I"$LIBPOSTAL_SRC/.." -c "$source_root/klib/drand48.c" \
      -o "$obj/drand48.o"
    run_cmd archive "$LIBPOSTAL_SRC" "$AR_BIN" -rc "$output/libpostal.a" "$obj"/*.o
    run_cmd ranlib "$LIBPOSTAL_SRC" "$RANLIB_BIN" "$output/libpostal.a"
    if [[ "$mode" == diagnostic ]]; then dump_mode_flags=(-DEXP185_DIAG); else dump_mode_flags=(); fi
    run_cmd link-dumper "$ROOT" "$CLANG" -O2 -g "${dump_mode_flags[@]}" -I"$source_root" -I"$source_root/.." \
      "$EXP/tools/dump_outputs.c" "$output/libpostal.a" -lm -o "$output/dump_outputs"
    run_cmd hash-artifacts "$ROOT" shasum -a 256 "$output/libpostal.a" "$output/dump_outputs"
    printf 'end=%s\nexit=0\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" >> "$RAW/session-$variant-$mode.txt"
  done
done

# Record each source tree and the shared runtime/build harness in its provenance file.
for variant in "${VARIANTS[@]}"; do
  {
    printf 'build_id=%s\nvariant=%s\nmodel_dir=%s\n' "$BUILD_ID" "$variant" "$MODEL_DIR"
    printf 'compiler='; "$CLANG" --version | head -n 1
    printf '\nflags_release:\n'; cat "$RAW/flags-$variant-release.txt"
    printf '\nflags_diagnostic:\n'; cat "$RAW/flags-$variant-diagnostic.txt"
    printf '\nsource_sha256:\n'
    (cd "$EXP" && find "source/$variant" -type f -print0 | sort -z | xargs -0 shasum -a 256)
    printf '\nshared_source_sha256:\n'
    (cd "$EXP" && shasum -a 256 source/diag.c tools/dump_outputs.c tools/build_variants.sh)
    printf '\nrelease_artifacts_sha256:\n'
    shasum -a 256 "$ART/$variant/release/libpostal.a" "$ART/$variant/release/dump_outputs"
    printf '\ndiagnostic_artifacts_sha256:\n'
    shasum -a 256 "$ART/$variant/diagnostic/libpostal.a" "$ART/$variant/diagnostic/dump_outputs"
  } > "$RAW/provenance/$variant.txt"
done

hash_sources > "$RAW/source-after.sha256"
cmp "$RAW/source-before.sha256" "$RAW/source-after.sha256"
printf 'build_id\tvariant\tmode\tlibrary\tdump\tprovenance\n' > "$RAW/outputs.tsv"
for variant in "${VARIANTS[@]}"; do
  for mode in release diagnostic; do
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$BUILD_ID" "$variant" "$mode" \
      "$ART/$variant/$mode/libpostal.a" "$ART/$variant/$mode/dump_outputs" \
      "$RAW/provenance/$variant.txt" >> "$RAW/outputs.tsv"
  done
done
printf 'artifacts=%s\noutputs=%s\n' "$ART" "$RAW/outputs.tsv" >> "$RAW/session.txt"
printf 'Build artifacts: %s\nBuild records: %s\n' "$ART" "$RAW"
