#!/bin/bash
set -eo pipefail
ROOT="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
EXP="$ROOT/research/experiments/186-architecture-benchmark"
LIBPOSTAL_SRC="$ROOT/libpostal/src"
MODEL_DIR="${LIBPOSTAL_MODEL_DIR:-/Users/kyle/.cache/libpostal-profile/libpostal}"
BASE_FLAGS=(-std=gnu2x -DHAVE_CONFIG_H -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS)
DATA_FLAG="-DLIBPOSTAL_DATA_DIR=\"$MODEL_DIR\""
LIBSRC=(strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard)
usage() { cat <<'EOF'
Usage:
  bash research/experiments/186-architecture-benchmark/tools/build_trace.sh --preflight
  BUILD_ID=calibration bash research/experiments/186-architecture-benchmark/tools/build_trace.sh --build

Builds original-control and original-trace sequentially. It never runs the model.
EOF
}
preflight() {
  local f
  for f in "$MODEL_DIR" "$ROOT/research/experiments/000-baseline/artifacts/libpostal.a" "$EXP/tools/dump_outputs.c" "$EXP/source/diag.c"; do
    [[ -e "$f" ]] || { printf 'Missing: %s\n' "$f" >&2; return 1; }
  done
  [[ -d "$MODEL_DIR" ]] || { printf 'Model path is not a directory: %s\n' "$MODEL_DIR" >&2; return 1; }
  for f in "${LIBSRC[@]}" scanner klib/drand48; do [[ -f "$EXP/source/original/$f.c" ]] || { printf 'Missing source: %s\n' "$EXP/source/original/$f.c" >&2; return 1; }; done
  command -v clang >/dev/null && command -v ar >/dev/null && command -v ranlib >/dev/null
  printf 'Preflight passed. No compiler or model process was started.\n'
}
[[ $# -eq 1 ]] || { usage >&2; exit 2; }
case "$1" in --preflight) preflight; exit $? ;; --build) ;; -h|--help) usage; exit 0 ;; *) usage >&2; exit 2 ;; esac
preflight
BUILD_ID="${BUILD_ID:-$(date '+%Y%m%d-%H%M%S')-$$}"
RAW="$EXP/raw/build-$BUILD_ID"; ART="$EXP/artifacts/build-$BUILD_ID"
[[ ! -e "$RAW" && ! -e "$ART" ]] || { printf 'Build ID already exists: %s\n' "$BUILD_ID" >&2; exit 2; }
mkdir -p "$RAW/commands" "$RAW/source-hashes" "$RAW/provenance" "$ART"
printf 'build_id=%s\nstart=%s\nargv=' "$BUILD_ID" "$(date '+%Y-%m-%dT%H:%M:%S%z')" > "$RAW/session.txt"
printf '%q ' "$0" "$@" >> "$RAW/session.txt"; printf '\nmodel_dir=%s\n' "$MODEL_DIR" >> "$RAW/session.txt"
finish() { local rc=$?; trap - EXIT; printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$rc" >> "$RAW/session.txt"; exit "$rc"; }
trap finish EXIT
(cd "$EXP" && find source/original -type f -print0 | sort -z | xargs -0 shasum -a 256 && shasum -a 256 source/diag.c tools/dump_outputs.c tools/build_trace.sh) > "$RAW/source-before.sha256"
cp "$ROOT/benchmark/model_sha256.txt" "$RAW/model_sha256.txt"
shasum -a 256 "$RAW/model_sha256.txt" > "$RAW/model-manifest.sha256"
(cd "$MODEL_DIR" && shasum -a 256 -c "$RAW/model_sha256.txt") > "$RAW/model-verification.stdout" 2> "$RAW/model-verification.stderr"
CLANG="$(command -v clang)"; AR_BIN="$(command -v ar)"; RANLIB_BIN="$(command -v ranlib)"
printf 'clang=%s\nar=%s\nranlib=%s\n' "$CLANG" "$AR_BIN" "$RANLIB_BIN" >> "$RAW/session.txt"
run_cmd() {
  local variant="$1" label="$2" cwd="$3" status=0 arg
  shift 3
  local stem="$RAW/commands/$variant-$label"
  { printf 'start=%s\ncwd=%q\nargv=' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$cwd"; for arg in "$@"; do printf '%q ' "$arg"; done; printf '\n'; } > "$stem.command"
  (cd "$cwd" && "$@" > "$stem.stdout" 2> "$stem.stderr") || status=$?
  printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$stem.command"
  printf '%s %s\n' "$stem" "$status" >> "$RAW/exit_codes.txt"
  [[ $status -eq 0 ]]
}
for variant in original-control original-trace; do
  out="$ART/$variant"; obj="$out/obj"; mkdir -p "$obj"
  if [[ "$variant" == original-trace ]]; then mode=(-DEXP185_DIAG -DEXP186_TRACE); else mode=(); fi
  { printf 'BASE_FLAGS\n'; printf '%s\n' "${BASE_FLAGS[@]}"; printf 'MODE_FLAGS\n'; printf '%s\n' "${mode[@]}"; printf 'DATA_FLAG\n%s\n-O2\nSCANNER_OPTIMIZATION\n-O0\nLINK_FLAGS\n-O2 -g\nMODEL_DIR\n%s\n' "$DATA_FLAG" "$MODEL_DIR"; } > "$RAW/flags-$variant.txt"
  for f in "${LIBSRC[@]}"; do
    objname=${f//\//_}.o
    run_cmd "$variant" "compile-${objname%.o}" "$LIBPOSTAL_SRC" "$CLANG" "${BASE_FLAGS[@]}" -O2 "${mode[@]}" "$DATA_FLAG" -I"$EXP/source/original" -I"$EXP/source" -I"$LIBPOSTAL_SRC" -I"$LIBPOSTAL_SRC/.." -c "$EXP/source/original/$f.c" -o "$obj/$objname"
  done
  if [[ "$variant" == original-trace ]]; then run_cmd "$variant" compile-diag "$LIBPOSTAL_SRC" "$CLANG" "${BASE_FLAGS[@]}" -O2 "${mode[@]}" "$DATA_FLAG" -I"$EXP/source/original" -I"$EXP/source" -I"$LIBPOSTAL_SRC" -I"$LIBPOSTAL_SRC/.." -c "$EXP/source/diag.c" -o "$obj/diag.o"; fi
  run_cmd "$variant" compile-scanner "$LIBPOSTAL_SRC" "$CLANG" "${BASE_FLAGS[@]}" -O0 "${mode[@]}" "$DATA_FLAG" -I"$EXP/source/original" -I"$EXP/source" -I"$LIBPOSTAL_SRC" -I"$LIBPOSTAL_SRC/.." -c "$EXP/source/original/scanner.c" -o "$obj/scanner.o"
  run_cmd "$variant" compile-drand48 "$LIBPOSTAL_SRC" "$CLANG" "${BASE_FLAGS[@]}" -O0 -I"$EXP/source/original" -I"$EXP/source" -I"$LIBPOSTAL_SRC" -I"$LIBPOSTAL_SRC/.." -c "$EXP/source/original/klib/drand48.c" -o "$obj/drand48.o"
  run_cmd "$variant" archive "$LIBPOSTAL_SRC" "$AR_BIN" -rc "$out/libpostal.a" "$obj"/*.o
  run_cmd "$variant" ranlib "$LIBPOSTAL_SRC" "$RANLIB_BIN" "$out/libpostal.a"
  run_cmd "$variant" link-dylib "$ROOT" "$CLANG" -dynamiclib "$obj"/*.o -lm -o "$out/libpostal.dylib"
  if [[ "$variant" == original-trace ]]; then dumpmode=(-DEXP185_DIAG); else dumpmode=(); fi
  run_cmd "$variant" link-dumper "$ROOT" "$CLANG" -O2 -g "${dumpmode[@]}" -I"$EXP/source/original" -I"$EXP/source" "$EXP/tools/dump_outputs.c" "$out/libpostal.a" -lm -o "$out/dump_outputs"
  shasum -a 256 "$out/libpostal.a" "$out/libpostal.dylib" "$out/dump_outputs" > "$RAW/$variant-artifacts.sha256"
  { printf 'variant=%s\nmodel_dir=%s\nmodel_manifest_sha256=%s\n' "$variant" "$MODEL_DIR" "$(cut -d ' ' -f 1 "$RAW/model-manifest.sha256")"; printf 'model_files_verified:\n'; cat "$RAW/model-verification.stdout"; "$CLANG" --version | head -n 1; printf 'flags:\n'; cat "$RAW/flags-$variant.txt"; printf 'source_sha256:\n'; (cd "$EXP" && find source/original -type f -print0 | sort -z | xargs -0 shasum -a 256); printf 'shared_sha256:\n'; (cd "$EXP" && shasum -a 256 source/diag.c tools/dump_outputs.c tools/build_trace.sh); printf 'instrumentation_deltas: delta-address_parser.diff delta-diag_h.diff delta-diag_c.diff delta-dump_outputs.diff\n'; printf 'artifacts_sha256:\n'; cat "$RAW/$variant-artifacts.sha256"; } > "$RAW/provenance/$variant.txt"
done
(cd "$EXP" && find source/original -type f -print0 | sort -z | xargs -0 shasum -a 256 && shasum -a 256 source/diag.c tools/dump_outputs.c tools/build_trace.sh) > "$RAW/source-after.sha256"
cmp "$RAW/source-before.sha256" "$RAW/source-after.sha256"
diff -u "$ROOT/research/experiments/185-diagnostic-instrumentation/source/original/address_parser.c" "$EXP/source/original/address_parser.c" > "$RAW/delta-address_parser.diff" || :
diff -u "$ROOT/research/experiments/185-diagnostic-instrumentation/source/original/diag.h" "$EXP/source/original/diag.h" > "$RAW/delta-diag_h.diff" || :
diff -u "$ROOT/research/experiments/185-diagnostic-instrumentation/source/diag.c" "$EXP/source/diag.c" > "$RAW/delta-diag_c.diff" || :
diff -u "$ROOT/research/experiments/185-diagnostic-instrumentation/tools/dump_outputs.c" "$EXP/tools/dump_outputs.c" > "$RAW/delta-dump_outputs.diff" || :
printf 'artifacts=%s\nrecords=%s\n' "$ART" "$RAW" >> "$RAW/session.txt"
printf 'Built: %s\nRecords: %s\n' "$ART" "$RAW"
