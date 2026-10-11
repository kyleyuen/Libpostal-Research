# API memory comparison plan (prepared, not run)

**Status:** metadata/source review only. No AC gate, build, model setup, parser,
benchmark, or test was run for this plan. The commands below are future commands;
they must use new output directories and preserve failed attempts.

## What the current API result establishes

The existing 78-safe-case capture is at
`research/analysis/parallel-exploration-2026-10-07/raw/api-edge-78-finalists-v1/manifest.json`.
It used the same `edge_api_dump.py`, model directory, case registry, and sequential
one-process-per-library method. All three output streams are byte-identical
(SHA-256 `bfcd44cb…`), each has 78 rows, and thread samples are all one. Four
unsafe registry records are excluded; one selected case is a NULL pointer, so
there are 77 ordinary non-NULL address calls. The recorded whole-process peak
footprints are 2,301,265,960 B (000), 2,622,114,072 B (197), and 2,530,331,120 B
(199-mimalloc-v2). This is a real difference in this API harness, not an
attributed parser-only memory delta.

The runner loads each library with `ctypes.CDLL`, calls
`libpostal_setup_datadir` and `libpostal_setup_parser_datadir` once with the same
explicit model path, then calls `libpostal_parse_address` in registry order with
the recorded nullable language/country options. It tears down both subsystems
after the capture. `run_api_edge_compare.py` gates every role, holds the shared
workload lock, records `/usr/bin/time -l`, samples thread counts, hashes the
library and outputs, requires 78 rows/one thread, and byte-compares each output
to 000. Its peak is the Python process plus loaded library and model footprint.
It is not the suite’s parser-executable steady-state measurement.

## Concrete linkage differences and unresolved attribution

The 000 API harness loaded the preserved
`research/experiments/000-baseline/artifacts/libpostal.dylib` (SHA-256
`734ab40b…`). The existing API candidate libraries were instead linked from
static archives with `clang -dynamiclib -Wl,-all_load`: 197 without mimalloc,
and 199 with `/opt/homebrew/opt/mimalloc/lib/mimalloc.o` plus `-flto`. The 199
link inputs are archive SHA-256 `447e00b4…` and mimalloc object SHA-256
`03460b36…`; its API dylib SHA-256 is `af526b34…`. The API link manifest is
`research/analysis/parallel-exploration-2026-10-07/raw/api-edge-link-v1/manifest.json`.
The `otool -L` inspection shows different dylib install-name strings; it does
not by itself show an extra dependent library. `nm -m` shows mimalloc internals
in the 199 dylib, but this inspection did not prove which allocator handles
every call from the LTO-linked parser or whether process-level symbol binding
affects Python allocations. Do not state allocator interposition as fact.

The 119 package’s preserved static archive is
`research/experiments/119-component-phrase-ids/artifacts/cpi/libpostal.a`
(SHA-256 `e56d3ae4…`, recorded in its `hashes.txt`). The 119 build record says
O2/full-LTO, development-trained LLVM PGO, O1 scanner and mimalloc at executable
link. Its static archive does not include the separately supplied mimalloc
link object. There is no packaged CPI dylib in that artifact directory. A
shared-library adaptation from this exact archive is therefore needed for the
same `ctypes` API harness; label it as such, not as the packaged executable.
The allocator ablation uses that same archive and linker settings, changing
only presence of the mimalloc object. To attribute 199’s source change, add a
separate CPI source control built from the preserved 119 sources with the 199
profile: `-O2 -flto`, scanner `-O0`, no PGO, and mimalloc at link. This matched
control is necessary because the accepted packaged CPI uses PGO and scanner
`-O1`, while 199 mimalloc-v2 has no PGO and scanner `-O0`. The exact existing
`119-component-phrase-ids/tools/build_cc.sh` interface supports `PROFMODE=none`
and these flags; it writes a uniquely named artifact and appends its build
record. Do not invent a different build recipe or compare package CPI directly
to 199 as if only source differed.

## Smallest rerun

After the environment gate is satisfied, first create two new API dylibs from
the immutable CPI archive, then build/link the source-matched control. Preserve
command JSON, stdout, stderr, exit status, timestamps, source/archive/object/
compiler hashes, and output hashes under new versioned paths. Use a new artifact
subdirectory; never overwrite an existing file. Gate before each build/link and
run every compiler command through `research/tools/run_locked.py`.

```sh
# PLANNED ONLY — same CPI archive and -flto; only mimalloc input differs.
research/tools/env_gate.sh
mkdir -p research/experiments/201-cpi-work-and-memory-prep/artifacts/api \
  research/experiments/201-cpi-work-and-memory-prep/raw/api-memory-matched-v1/link-cpi-no-mimalloc \
  research/experiments/201-cpi-work-and-memory-prep/raw/api-memory-matched-v1/link-cpi-mimalloc \
  research/experiments/201-cpi-work-and-memory-prep/raw/api-memory-matched-v1/build-cpi-profile-control
python3 research/tools/run_locked.py -- /usr/bin/clang -dynamiclib -Wl,-all_load -flto \
  research/experiments/119-component-phrase-ids/artifacts/cpi/libpostal.a -lm \
  -o research/experiments/201-cpi-work-and-memory-prep/artifacts/api/cpi-no-mimalloc.dylib
research/tools/env_gate.sh
python3 research/tools/run_locked.py -- /usr/bin/clang -dynamiclib -Wl,-all_load -flto \
  /opt/homebrew/opt/mimalloc/lib/mimalloc.o \
  research/experiments/119-component-phrase-ids/artifacts/cpi/libpostal.a -lm \
  -o research/experiments/201-cpi-work-and-memory-prep/artifacts/api/cpi-mimalloc.dylib
research/tools/env_gate.sh
EXTRA_SW="$(cat research/experiments/119-component-phrase-ids/raw/switches_cpi.txt)" \
  python3 research/tools/run_locked.py -- \
  research/experiments/119-component-phrase-ids/tools/build_cc.sh cpi-199-profile-control clang none \
  "-O2 -flto" "-O0" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
research/tools/env_gate.sh
python3 research/tools/run_locked.py -- /usr/bin/clang -dynamiclib -Wl,-all_load -flto \
  /opt/homebrew/opt/mimalloc/lib/mimalloc.o \
  research/experiments/119-component-phrase-ids/artifacts/cpi-199-profile-control/libpostal.a -lm \
  -o research/experiments/201-cpi-work-and-memory-prep/artifacts/api/cpi-profile-control.dylib
```

Then run the existing API comparator once, sequentially, with all roles. Reuse
the fixed safe registry and model identity, and give `--out` a fresh versioned
path. Keep the previous 78-case logs intact.

```sh
# PLANNED ONLY — run only after AC / Low Power / thermal gate passes.
/usr/bin/caffeinate -i python3 research/tools/run_api_edge_compare.py \
  --baseline research/experiments/000-baseline/artifacts/libpostal.dylib \
  --candidate cpi-packaged-link=research/experiments/201-cpi-work-and-memory-prep/artifacts/api/cpi-mimalloc.dylib \
  --candidate 199-mimalloc-v2=research/experiments/199-token-span-storage/artifacts/api-mimalloc-v2/libpostal.dylib \
  --candidate cpi-no-mimalloc-ablation=research/experiments/201-cpi-work-and-memory-prep/artifacts/api/cpi-no-mimalloc.dylib \
  --candidate cpi-profile-matched-control=research/experiments/201-cpi-work-and-memory-prep/artifacts/api/cpi-profile-control.dylib \
  --cases research/experiments/186-architecture-benchmark/edge-cases.json \
  --datadir /Users/kyle/.cache/libpostal-profile/libpostal \
  --model-manifest-sha256 1d4e0863c590577de70b39219b56195d324823d2c5033e829b93266829f4b2ad \
  --out research/experiments/201-cpi-work-and-memory-prep/raw/api-memory-matched-v1
```

Pass criteria for interpretation: 78 rows, exit 0, one observed thread per
role, identical ordered API output, matching case/model hashes, and complete
time/thread logs. Compare `peak_footprint_bytes`, not RSS. The CPI with/without
mimalloc pair isolates the allocator-link delta within this harness. Compare
the profile-matched CPI control to 199 for source attribution; compare packaged
CPI to the profile-matched control to show the profile/build effect. The
000-to-CPI comparison still differs in build provenance, so report concrete
artifact identities and do not claim a source-only cause from it.

This API rerun cannot close the formal peak/steady acceptance gate. If a new
acceptance claim is needed, use the existing 183 `run_suite.py` workflow with
the preserved original and actual uninstrumented `bench_ext` artifacts,
including its `/usr/bin/time -l` peak-footprint and `footprint -p` after-pass-3
steady-footprint stages; retain full raw suite logs and test all13 on one gated
session. Do not substitute the Python API peak or RSS for that gate.

## Unresolved API memory gate

Do not dismiss a higher API peak as irrelevant because the release corpus suite passes. If the matched API comparison still exceeds original or allocator binding remains uncertain, retain that failed/inconclusive capture and use a minimal native API runner with the same safe cases and library setup/options to separate Python-host footprint from parser footprint. Preparing that runner is later work, not a completed201 artifact. No blanket memory acceptance or promotion while the API excess is unresolved.
