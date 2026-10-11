# Build identity

The promoted binaries are the tested 203 streaming postcode-graph build. The immutable copies and their build-time checksum list are in `artifacts/cpi-stream-203-v2/`; source files are in `sources/`.

## Build used for the tested binaries

- Compiler: Apple clang 17 (`clang-1700.6.3.2`)
- Base libpostal commit: `25099c506612b34b23b1bfe286ca6321fcf06f35`
- Library: `-O2 -flto`, using the existing CPI PGO profile at `/Users/kyle/.cache/libpostal-020-pgo/cpi/dev.profdata` (SHA-256 `3f82219e1979a7936eff4046c4b410fa09ab2e0d8965ba9dc16c2a3272a0aedd`)
- Scanner: `-O1`
- Link: `-flto` and `/opt/homebrew/opt/mimalloc/lib/mimalloc.o` (SHA-256 `03460b36edb0f104270109d33d96f8e7fc03c08d326cc5c2182c34b0a4900b82`)
- Build-time elapsed entry: 6 seconds, recorded in experiment 203 `raw/compile_times.txt`.
- `SOURCE_MANIFEST.sha256` pins the 16 promoted source/header files; these hashes match the source entries in `artifacts/cpi-stream-203-v2/hashes.txt`. That artifact manifest also pins `libpostal.a`, `dump_outputs`, `bench_ext`, and `fuzz_pipeline`.
- `MODEL_FILES.sha256` pins the 7 model files by path relative to the model root. From the model root, verify with `shasum -a 256 -c /path/to/203-cpi-memory/MODEL_FILES.sha256`. Suite integrity manifests also pin the model, both executables, and each measured TSV.
- `SHA256SUMS` pins this promoted source tree, copied binaries, and build script.

`build_cc.sh` is a rebuild recipe and writes new named build artifacts under `artifacts/`. Use a new artifact name; do not overwrite `cpi-stream-203-v2`. It uses the workspace's libpostal checkout, model data, PGO profile and mimalloc object. It does not download these external inputs. The recorded binary is the validation artifact; a rebuild is not implied by this document.

Exact compiler switches are preserved in `BUILD_SWITCHES.txt`; full command and warning logs remain under `research/experiments/203-cpi-memory-regression/raw/stream-build-v3/`.
