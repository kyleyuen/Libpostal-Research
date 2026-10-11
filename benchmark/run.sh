#!/bin/bash
# One-command benchmark: original libpostal vs the optimized build, on the repository datasets or your own.
# macOS on Apple Silicon, bash 3.2. Nothing outside WORK_DIR (default ~/.cache/libpostal-speedup) and research/suite/results is written.
#
#   benchmark/run.sh                      fetch libpostal + model, build both, run the suite on every repository dataset, write the verdict
#   benchmark/run.sh --dataset FILE       test your own addresses instead (one per line, or country<TAB>address); add --name NAME (default: file name)
#   benchmark/run.sh --api                also compare every public libpostal API call (expand, classify, dedupe, normalize, ...)
#   benchmark/run.sh --api-only           only the public-API comparison (minutes; downloads the 50 MB language classifier once)
#   benchmark/run.sh --quick              one short smoke round with three passes and small fuzz (sanity check, not for acceptance)
#   benchmark/run.sh --setup-only         prepare source, model, builds and suite config; stop before running anything
#   benchmark/run.sh --model-dir DIR      use an existing libpostal v1.0.0 model directory instead of downloading (~760 MB)
#   benchmark/run.sh --rebuild            rebuild both versions even if an identical build already exists in WORK_DIR
#   benchmark/run.sh --cpi-source 203|119  select the optimized source snapshot (default: accepted memory-fixed 203)
#   benchmark/run.sh --deps               install missing Homebrew packages (autoconf automake libtool mimalloc) first
#   benchmark/run.sh --no-gate            do not require AC power / no thermal warning before each timed run
#   benchmark/run.sh --include-reserved   include any datasets marked reserved in suite.json
#   extra arguments after -- go to research/suite/run_suite.py (e.g. -- --rounds 5)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
W="${WORK_DIR:-$HOME/.cache/libpostal-speedup}"
COMMIT=25099c506612b34b23b1bfe286ca6321fcf06f35          # libpostal commit used for every measurement in this repository
MODEL_URL=https://github.com/openvenues/libpostal/releases/download/v1.0.0
MODEL_DIR=""; QUICK=0; DEPS=0; SETUP_ONLY=0; CPI_SOURCE=203; SUITE_ARGS=(); DATASET=""; NAME=""; API=0; API_ONLY=0; REBUILD=0
while [ $# -gt 0 ]; do case "$1" in
  --model-dir) MODEL_DIR=$2; shift 2;; --quick) QUICK=1; shift;; --deps) DEPS=1; shift;; --setup-only) SETUP_ONLY=1; shift;;
  --cpi-source) CPI_SOURCE=$2; shift 2;; --dataset) DATASET=$2; shift 2;; --name) NAME=$2; shift 2;;
  --api) API=1; shift;; --api-only) API=1; API_ONLY=1; shift;; --rebuild) REBUILD=1; shift;;
  --no-gate) SUITE_ARGS+=(--no-gate); shift;; --include-reserved) SUITE_ARGS+=(--include-reserved); shift;;
  --) shift; SUITE_ARGS+=("$@"); break;; -h|--help) sed -n 2,18p "$0"; exit 0;; *) echo "unknown option $1 (see --help)"; exit 2;; esac; done
case "$W" in *" "*) echo "WORK_DIR must not contain spaces (libpostal's build embeds the model path): $W"; exit 2;; esac
case "$CPI_SOURCE" in 203|119) ;; *) echo "--cpi-source must be 203 or 119"; exit 2;; esac
if [ -n "$DATASET" ]; then
  [ -f "$DATASET" ] || { echo "--dataset file not found: $DATASET"; exit 2; }
  DATASET=$(cd "$(dirname "$DATASET")" && pwd)/$(basename "$DATASET")
  [ -n "$NAME" ] || NAME=$(basename "$DATASET" | sed -e 's/\.[^.]*$//' -e 's/[^A-Za-z0-9._-]/-/g')
fi
mkdir -p "$W/logs" "$W/bin/original" "$W/bin/cpi" "$W/data"
say() { echo "[$(date '+%H:%M:%S')] $*"; }
run_logged() { local name=$1; shift; if ! "$@" > "$W/logs/$name.log" 2>&1; then echo "FAILED: $name (log: $W/logs/$name.log)"; tail -20 "$W/logs/$name.log"; exit 1; fi; }

say "1/6 checks"
[ "$(uname -s)" = Darwin ] && [ "$(uname -m)" = arm64 ] || { echo "needs macOS on Apple Silicon"; exit 1; }
xcode-select -p > /dev/null 2>&1 || { echo "install the Xcode command line tools: xcode-select --install"; exit 1; }
for c in clang git curl python3 shasum; do command -v $c > /dev/null || { echo "missing $c"; exit 1; }; done
command -v brew > /dev/null || { echo "Homebrew is required (https://brew.sh) for autoconf automake libtool mimalloc"; exit 1; }
MISSING=""; for p in autoconf automake libtool mimalloc; do brew list --versions $p > /dev/null 2>&1 || MISSING="$MISSING $p"; done
if [ -n "$MISSING" ]; then
  if [ $DEPS = 1 ]; then run_logged brew_install brew install $MISSING; else echo "missing Homebrew packages:$MISSING  -> run: brew install$MISSING  (or rerun with --deps)"; exit 1; fi
fi
MIMALLOC="$(brew --prefix mimalloc)/lib/mimalloc.o"; [ -f "$MIMALLOC" ] || { echo "mimalloc.o not found at $MIMALLOC"; exit 1; }
case " ${SUITE_ARGS[*]-} " in *" --no-gate "*) ;; *)
  [ $SETUP_ONLY = 1 ] || [ $API_ONLY = 1 ] || pmset -g batt | head -1 | grep -q "AC Power" \
    || say "WARNING: on battery. Timed runs wait for AC power (research/tools/env_gate.sh); plug in, or rerun with --no-gate (noisier numbers).";;
esac

say "2/6 libpostal $COMMIT -> $W/libpostal"
if [ ! -d "$W/libpostal/.git" ]; then run_logged clone git clone -q https://github.com/openvenues/libpostal.git "$W/libpostal"; fi
git -C "$W/libpostal" checkout -q $COMMIT
SRC="$W/libpostal/src"
if [ ! -f "$SRC/.libs/libpostal.a" ]; then
  # exactly how the original was configured for all measurements (research/experiments/000-baseline, profiling/README.md)
  ( cd "$W/libpostal" && run_logged bootstrap ./bootstrap.sh && run_logged configure ./configure --disable-data-download --disable-sse2 --datadir="$W/data" \
    && run_logged make_original make -C src -j"$(sysctl -n hw.ncpu)" libpostal.la )
fi

say "3/6 model (libpostal v1.0.0 data) -> $W/data/libpostal"
if [ -n "$MODEL_DIR" ]; then ln -sfn "$(cd "$MODEL_DIR" && pwd)" "$W/data/libpostal"
elif [ ! -f "$W/data/libpostal/address_parser/address_parser_crf.dat" ]; then
  mkdir -p "$W/data/libpostal" "$W/download"
  for f in libpostal_data.tar.gz parser.tar.gz; do [ -f "$W/download/$f" ] || run_logged download_$f curl -fL --retry 3 -o "$W/download/$f" "$MODEL_URL/$f"; done
  for f in libpostal_data.tar.gz parser.tar.gz; do run_logged extract_$f tar -xzf "$W/download/$f" -C "$W/data/libpostal"; done
fi
( cd "$W/data/libpostal" && shasum -a 256 -c "$ROOT/benchmark/model_sha256.txt" ) > "$W/logs/model_check.log" 2>&1 \
  || { echo "model files differ from the ones every result here was measured with (see $W/logs/model_check.log)"; exit 1; }
if [ $API = 1 ]; then   # the language classifier is used only by the public-API comparison (expand, classify, dedupe)
  if [ ! -f "$W/data/libpostal/language_classifier/language_classifier.dat" ]; then
    [ -z "$MODEL_DIR" ] || { echo "--api needs language_classifier/ in $MODEL_DIR: extract $MODEL_URL/language_classifier.tar.gz there"; exit 1; }
    mkdir -p "$W/download"; [ -f "$W/download/language_classifier.tar.gz" ] || run_logged download_language_classifier curl -fL --retry 3 -o "$W/download/language_classifier.tar.gz" "$MODEL_URL/language_classifier.tar.gz"
    run_logged extract_language_classifier tar -xzf "$W/download/language_classifier.tar.gz" -C "$W/data/libpostal"
  fi
  ( cd "$W/data/libpostal" && shasum -a 256 -c "$ROOT/benchmark/language_classifier_sha256.txt" ) >> "$W/logs/model_check.log" 2>&1 \
    || { echo "language classifier differs from the recorded hash (see $W/logs/model_check.log)"; exit 1; }
fi

HARNESS=("$ROOT/research/tools/bench_ext.c" "$ROOT/research/experiments/003-batched-lookups/tools/dump_outputs.c" "$ROOT/research/tools/fuzz_pipeline.c" "$ROOT/research/tools/api_suite.c")
TOOLS="bench_ext dump_outputs fuzz_pipeline api_suite"
if [ "$CPI_SOURCE" = 203 ]; then
  C="$ROOT/research/final/203-cpi-memory/sources"
  CPI_DESC="optimized: memory-fixed CPI from research/final/203-cpi-memory (streamed postcode graph loader)"
else
  C="$ROOT/research/final/119-cpi/sources"
  CPI_DESC="optimized: CPI from research/final/119-cpi (pre-streaming postcode graph loader)"
fi
SWITCH_FILE="$ROOT/research/final/119-cpi/switches.txt"
[ -d "$C" ] || { echo "candidate source directory not found: $C"; exit 1; }
BASE="-std=gnu2x -DHAVE_CONFIG_H -Wall -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS"
SW="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE -DEXP_FAST_APPEND -DEXP_FAST_JOIN -DEXP_VOCAB_MEMO -DEXP_INT_KEYS -DEXP_TRANS_SKIP -DEXP_NORM_TABLE -DEXP_CANON_CONST -DEXP_RESPONSE_BLOCK -DEXP_REUSE -DEXP_VITERBI_NEON -DEXP_TOKEN_ASCII -DEXP_FD_INLINE $(cat "$SWITCH_FILE")"
CHANGED="crf trie string_utils address_parser features trie_search transliterate normalize crf_context libpostal tokens"
LIBSRC="strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard"
DD="-DLIBPOSTAL_DATA_DIR=\"$W/data/libpostal\""
P="$W/pgo"; PGO_USE="-fprofile-instr-use=$P/dev.profdata -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date"
link_harnesses() {   # $1 output dir of an optimized build, $2 extra flags (PGO); the original harnesses link against the autotools library
  local D=$1 X=$2 h
  for h in "${HARNESS[@]}"; do clang -O2 -g -I"$SRC" "$h" "$SRC/.libs/libpostal.a" -lm -o "$W/bin/original/$(basename "$h" .c)" || return 1; done
  for h in "${HARNESS[@]}"; do clang -O2 -g -flto $X -I"$SRC" "$MIMALLOC" "$h" "$D/libpostal.a" -lm -o "$D/$(basename "$h" .c)" || return 1; done
}
build_cpi() {   # $1 output dir, $2 extra flags for every compile and link (PGO)
  local D=$1 X=$2; rm -rf "$D"; mkdir -p "$D/src" "$D/obj"
  for f in $CHANGED; do cp "$C/$f.c" "$D/src/"; done; cp "$C/"*.h "$D/src/"
  for f in $LIBSRC; do b=$(basename $f); case " $CHANGED " in *" $b "*) S="$D/src/$b.c";; *) S="$SRC/$f.c";; esac
    ( cd "$SRC" && clang $BASE $SW -O2 -flto $X "$DD" -I"$D/src" -I"$SRC" -I"$SRC/.." -c "$S" -o "$D/obj/$(echo $f | tr / _).o" ) || return 1; done
  ( cd "$SRC" && clang $BASE -O1 "$DD" -I"$SRC" -I"$SRC/.." -c "$SRC/scanner.c" -o "$D/obj/scanner.o" && clang $BASE -O1 -I"$SRC" -I"$SRC/.." -c "$SRC/klib/drand48.c" -o "$D/obj/drand48.o" ) || return 1
  ( cd "$D/obj" && ar -rc ../libpostal.a *.o && ranlib ../libpostal.a ) || return 1
  link_harnesses "$D" "$X"
}
# Reuse earlier work in WORK_DIR only when its inputs are unchanged: the optimized library (sources, switches, flags, compiler,
# allocator, model path) is rebuilt only when those change; the test harnesses are relinked when only a harness changed.
LIBKEY=$( { cat "$SWITCH_FILE" "$C"/*; echo "$BASE $SW $CHANGED $LIBSRC $DD $CPI_SOURCE"; clang --version; shasum -a 256 "$MIMALLOC" "$SRC/.libs/libpostal.a"; } | shasum -a 256 | cut -c1-64)
HKEY=$( { echo "$LIBKEY"; cat "${HARNESS[@]}"; } | shasum -a 256 | cut -c1-64)
HAVE=1; for n in $TOOLS; do [ -x "$W/bin/original/$n" ] && [ -x "$W/bin/cpi/$n" ] || HAVE=0; done
if [ $REBUILD = 0 ] && [ -f "$W/build/cpi/libpostal.a" ] && [ -f "$P/dev.profdata" ] && [ "$(cat "$W/bin/lib.key" 2>/dev/null)" = "$LIBKEY" ]; then
  if [ $HAVE = 1 ] && [ "$(cat "$W/bin/harness.key" 2>/dev/null)" = "$HKEY" ]; then
    say "4-5/6 reusing the identical original and optimized builds in $W/bin (use --rebuild to force)"
  else
    say "4-5/6 optimized library unchanged; relinking the test harnesses of both versions"
    rm -f "$W/bin/harness.key"; run_logged relink_harnesses link_harnesses "$W/build/cpi" "$PGO_USE"
    for n in $TOOLS; do cp "$W/build/cpi/$n" "$W/bin/cpi/$n"; done; echo "$HKEY" > "$W/bin/harness.key"
  fi
else
  rm -f "$W/bin/lib.key" "$W/bin/harness.key"
  say "4/6 build the original (reference) harness"
  for h in "${HARNESS[@]}"; do n=$(basename "$h" .c); run_logged link_original_$n clang -O2 -g -I"$SRC" "$h" "$SRC/.libs/libpostal.a" -lm -o "$W/bin/original/$n"; done
  say "5/6 build the optimized version (source $CPI_SOURCE: $C, PGO trained on golden-v1 development, LTO, mimalloc; about 10 minutes)"
  rm -rf "$P"; mkdir -p "$P"
  run_logged build_cpi_gen build_cpi "$W/build/cpi_gen" "-fprofile-instr-generate"
  LLVM_PROFILE_FILE="$P/p-%p.profraw" "$W/build/cpi_gen/dump_outputs" "$ROOT/profiling/golden-v1/development.txt" > /dev/null 2> "$W/logs/pgo_train.log"
  run_logged pgo_merge xcrun llvm-profdata merge -output="$P/dev.profdata" "$P"/*.profraw
  run_logged build_cpi build_cpi "$W/build/cpi" "$PGO_USE"
  for n in $TOOLS; do cp "$W/build/cpi/$n" "$W/bin/cpi/$n"; done
  echo "$LIBKEY" > "$W/bin/lib.key"; echo "$HKEY" > "$W/bin/harness.key"
fi
( cd "$W/bin" && find original cpi -maxdepth 1 -type f -exec shasum -a 256 {} + > "$W/logs/binaries.sha256" )

say "6/6 suite configuration"
python3 - "$ROOT" "$W" "$CPI_DESC" <<'PY'
import json, sys
root, w = sys.argv[1], sys.argv[2]
c = json.load(open(f"{root}/research/suite/suite.json"))
c["workspace"] = root; c["model_dir"] = f"{w}/data/libpostal"
c["variants"] = [
  {"name": "original", "role": "baseline", "enabled": True, "desc": "libpostal 25099c50 built with its own autotools (-O2)",
   "bench_ext": f"{w}/bin/original/bench_ext", "dump_outputs": f"{w}/bin/original/dump_outputs", "fuzz_pipeline": f"{w}/bin/original/fuzz_pipeline"},
  {"name": "optimized", "role": "candidate", "enabled": True, "desc": sys.argv[3],
   "bench_ext": f"{w}/bin/cpi/bench_ext", "dump_outputs": f"{w}/bin/cpi/dump_outputs", "fuzz_pipeline": f"{w}/bin/cpi/fuzz_pipeline"}]
json.dump(c, open(f"{w}/suite.json", "w"), indent=1)
PY
if [ -n "$DATASET" ]; then
  python3 "$ROOT/benchmark/add_dataset.py" "$W/suite.json" "$DATASET" "$NAME" "$W"
  SUITE_ARGS=(--datasets "$NAME" "${SUITE_ARGS[@]+"${SUITE_ARGS[@]}"}")
fi
if [ "$SETUP_ONLY" = 1 ]; then
  say "setup complete; benchmark suite was not run"
  say "generated config: $W/suite.json"
  echo "To run the benchmark later: python3 \"$ROOT/research/suite/run_suite.py\" --config \"$W/suite.json\""
  exit 0
fi
RUN_ID="$(date +%Y%m%d-%H%M%S)-${NAME:-repository-datasets}"; RUN="$ROOT/research/suite/results/$RUN_ID"
if [ $API_ONLY = 1 ]; then
  mkdir -p "$RUN"; printf '{"argv": ["benchmark/run.sh", "--api-only"], "variants": ["original", "optimized"], "datasets": []}\n' > "$RUN/invocation.json"
else
  if [ $QUICK = 1 ]; then SUITE_ARGS=(--rounds 1 --passes 3 --fuzz-count 20000 "${SUITE_ARGS[@]+"${SUITE_ARGS[@]}"}"); fi
  say "run suite (correctness, timing, memory, threads, report) -> $RUN"
  python3 "$ROOT/research/suite/run_suite.py" --config "$W/suite.json" --run-id "$RUN_ID" "${SUITE_ARGS[@]+"${SUITE_ARGS[@]}"}"
fi
if [ $API = 1 ]; then
  API_IN="$RUN/api/input.txt"; mkdir -p "$RUN/api"
  if [ -n "$DATASET" ]; then cp "$W/datasets/$NAME/$NAME.txt" "$API_IN"
  else cat "$ROOT/profiling/golden-v1/validation.txt" "$ROOT/profiling/golden-v14-osm-unseen/unseen14.txt" > "$API_IN"; fi
  say "public-API comparison on $(grep -c . "$API_IN") addresses -> $RUN/api"
  python3 "$ROOT/research/tools/api_parity.py" --original "$W/bin/original/api_suite" --optimized "$W/bin/cpi/api_suite" --input "$API_IN" --out "$RUN/api" || true
fi
say "verdict"
python3 "$ROOT/benchmark/verdict.py" "$RUN" && STATUS=0 || STATUS=$?
say "done: $RUN/VERDICT.md (full report: $RUN/report.md)"
exit $STATUS
