#!/bin/bash
# One-command benchmark: original libpostal vs the optimized build "cpi", on every dataset in this repository.
# macOS on Apple Silicon, bash 3.2. Nothing outside WORK_DIR (default ~/.cache/libpostal-speedup) and research/suite/results is written.
#
#   benchmark/run.sh                      fetch libpostal + model, build both, verify, time (about 1 hour), write the report
#   benchmark/run.sh --quick              same, 1 round x 3 passes, small fuzz: a ~15-minute smoke test (numbers are rough)
#   benchmark/run.sh --model-dir DIR      use an existing libpostal v1.0.0 model directory instead of downloading (~1 GB)
#   benchmark/run.sh --deps               install missing Homebrew packages (autoconf automake libtool mimalloc) first
#   benchmark/run.sh --no-gate            do not require AC power / no thermal warning before each timed run
#   benchmark/run.sh --include-reserved   also run the never-used unseen sets v12, v13 (spends them)
#   extra arguments after -- go to research/suite/run_suite.py (e.g. -- --datasets osm-v3 --rounds 3)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
W="${WORK_DIR:-$HOME/.cache/libpostal-speedup}"
COMMIT=25099c506612b34b23b1bfe286ca6321fcf06f35          # libpostal commit used for every measurement in this repository
MODEL_URL=https://github.com/openvenues/libpostal/releases/download/v1.0.0
MODEL_DIR=""; QUICK=0; DEPS=0; SUITE_ARGS=()
while [ $# -gt 0 ]; do case "$1" in
  --model-dir) MODEL_DIR=$2; shift 2;; --quick) QUICK=1; shift;; --deps) DEPS=1; shift;;
  --no-gate) SUITE_ARGS+=(--no-gate); shift;; --include-reserved) SUITE_ARGS+=(--include-reserved); shift;;
  --) shift; SUITE_ARGS+=("$@"); break;; -h|--help) sed -n 2,13p "$0"; exit 0;; *) echo "unknown option $1"; exit 2;; esac; done
case "$W" in *" "*) echo "WORK_DIR must not contain spaces (libpostal's build embeds the model path): $W"; exit 2;; esac
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
  pmset -g batt | head -1 | grep -q "AC Power" || say "WARNING: on battery. Timed runs wait for AC power (research/tools/env_gate.sh); plug in, or rerun with --no-gate (noisier numbers).";;
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

say "4/6 build the original (reference) harness"
HARNESS=("$ROOT/research/tools/bench_ext.c" "$ROOT/research/experiments/003-batched-lookups/tools/dump_outputs.c" "$ROOT/research/tools/fuzz_pipeline.c")
for h in "${HARNESS[@]}"; do n=$(basename "$h" .c); run_logged link_original_$n clang -O2 -g -I"$SRC" "$h" "$SRC/.libs/libpostal.a" -lm -o "$W/bin/original/$n"; done

say "5/6 build the optimized version (cpi: sources in research/final/119-cpi, PGO trained on golden-v1 development, LTO, mimalloc)"
C="$ROOT/research/final/119-cpi/sources"
BASE="-std=gnu2x -DHAVE_CONFIG_H -Wall -Wno-unused-function -Wno-unused-parameter -Wformat -Werror=format-security -Winit-self -Wno-sign-compare -g -D LIBPOSTAL_EXPORTS"
SW="-DEXP_NO_RESET -DEXP_DEDUP_BATCH -DEXP_FEATURE_DICT -DEXP_DIRECT_KEYS -DEXP_WEIGHT_PREFETCH -DEXP_LEAN_CONTEXT -DEXP_VOCAB_DICT -DEXP_ASCII_NORM -DEXP_ADMIN_SHARE -DEXP_FAST_APPEND -DEXP_FAST_JOIN -DEXP_VOCAB_MEMO -DEXP_INT_KEYS -DEXP_TRANS_SKIP -DEXP_NORM_TABLE -DEXP_CANON_CONST -DEXP_RESPONSE_BLOCK -DEXP_REUSE -DEXP_VITERBI_NEON -DEXP_TOKEN_ASCII -DEXP_FD_INLINE $(cat "$ROOT/research/final/119-cpi/switches.txt")"
CHANGED="crf trie string_utils address_parser features trie_search transliterate normalize crf_context libpostal tokens"
LIBSRC="strndup libpostal expand address_dictionary transliterate tokens trie trie_search trie_utils string_utils file_utils utf8proc/utf8proc normalize numex features unicode_scripts address_parser address_parser_io averaged_perceptron crf crf_context sparse_matrix averaged_perceptron_tagger graph graph_builder language_classifier language_features logistic_regression logistic minibatch float_utils ngrams place near_dupe double_metaphone geohash/geohash dedupe string_similarity acronyms soft_tfidf jaccard"
DD="-DLIBPOSTAL_DATA_DIR=\"$W/data/libpostal\""
build_cpi() {   # $1 output dir, $2 extra flags for every compile and link (PGO)
  local D=$1 X=$2; rm -rf "$D"; mkdir -p "$D/src" "$D/obj"
  for f in $CHANGED; do cp "$C/$f.c" "$D/src/"; done; cp "$C/"*.h "$D/src/"
  for f in $LIBSRC; do b=$(basename $f); case " $CHANGED " in *" $b "*) S="$D/src/$b.c";; *) S="$SRC/$f.c";; esac
    ( cd "$SRC" && clang $BASE $SW -O2 -flto $X "$DD" -I"$D/src" -I"$SRC" -I"$SRC/.." -c "$S" -o "$D/obj/$(echo $f | tr / _).o" ) || return 1; done
  ( cd "$SRC" && clang $BASE -O1 "$DD" -I"$SRC" -I"$SRC/.." -c "$SRC/scanner.c" -o "$D/obj/scanner.o" && clang $BASE -O1 -I"$SRC" -I"$SRC/.." -c "$SRC/klib/drand48.c" -o "$D/obj/drand48.o" ) || return 1
  ( cd "$D/obj" && ar -rc ../libpostal.a *.o && ranlib ../libpostal.a ) || return 1
  for h in "${HARNESS[@]}"; do clang -O2 -g -flto $X -I"$SRC" "$MIMALLOC" "$h" "$D/libpostal.a" -lm -o "$D/$(basename "$h" .c)" || return 1; done
}
P="$W/pgo"; rm -rf "$P"; mkdir -p "$P"
run_logged build_cpi_gen build_cpi "$W/build/cpi_gen" "-fprofile-instr-generate"
LLVM_PROFILE_FILE="$P/p-%p.profraw" "$W/build/cpi_gen/dump_outputs" "$ROOT/profiling/golden-v1/development.txt" > /dev/null 2> "$W/logs/pgo_train.log"
run_logged pgo_merge xcrun llvm-profdata merge -output="$P/dev.profdata" "$P"/*.profraw
run_logged build_cpi build_cpi "$W/build/cpi" "-fprofile-instr-use=$P/dev.profdata -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date"
for n in bench_ext dump_outputs fuzz_pipeline; do cp "$W/build/cpi/$n" "$W/bin/cpi/$n"; done
( cd "$W/bin" && find original cpi -maxdepth 1 -type f -exec shasum -a 256 {} + > "$W/logs/binaries.sha256" )

say "6/6 benchmark suite (correctness, timing, memory, threads, report)"
python3 - "$ROOT" "$W" <<'PY'
import json, sys
root, w = sys.argv[1], sys.argv[2]
c = json.load(open(f"{root}/research/suite/suite.json"))
c["workspace"] = root; c["model_dir"] = f"{w}/data/libpostal"
c["variants"] = [
  {"name": "original", "role": "baseline", "enabled": True, "desc": "libpostal 25099c50 built with its own autotools (-O2)",
   "bench_ext": f"{w}/bin/original/bench_ext", "dump_outputs": f"{w}/bin/original/dump_outputs", "fuzz_pipeline": f"{w}/bin/original/fuzz_pipeline"},
  {"name": "cpi", "role": "candidate", "enabled": True, "desc": "optimized build from research/final/119-cpi",
   "bench_ext": f"{w}/bin/cpi/bench_ext", "dump_outputs": f"{w}/bin/cpi/dump_outputs", "fuzz_pipeline": f"{w}/bin/cpi/fuzz_pipeline"}]
json.dump(c, open(f"{w}/suite.json", "w"), indent=1)
PY
if [ $QUICK = 1 ]; then SUITE_ARGS=(--rounds 1 --passes 3 --fuzz-count 20000 "${SUITE_ARGS[@]+"${SUITE_ARGS[@]}"}"); fi
python3 "$ROOT/research/suite/run_suite.py" --config "$W/suite.json" "${SUITE_ARGS[@]+"${SUITE_ARGS[@]}"}"
say "done: the report is research/suite/results/<newest>/report.md"
