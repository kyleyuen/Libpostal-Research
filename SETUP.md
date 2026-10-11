# Setup on Apple Silicon macOS

## Access and prerequisites

The repository is private; GitHub access is required. Clone or pull the repository using the commands in [README.md](README.md). The setup script fetches the pinned external libpostal source and model required for a local build.

The benchmark helper targets Apple Silicon macOS and Bash 3.2. Install Xcode Command Line Tools and Homebrew:

```sh
xcode-select --install
```

Install Homebrew from [brew.sh](https://brew.sh). The helper checks for `clang`, `git`, `curl`, `python3`, and `shasum`, then uses Homebrew packages `autoconf`, `automake`, `libtool`, and `mimalloc`. The `--deps` option installs missing packages. The Python tools use only the standard library; there is no pip requirements file. Network access is needed for the private clone and pinned source/model downloads. Setup fetches the official libpostal v1.0.0 model (about 760 MB compressed, 1.8 GB unpacked) when no local model directory is supplied; plan for about 2.8 GB of free disk in the work directory.

## Prepare the build

From the repository root, run:

```sh
./benchmark/run.sh --setup-only
```

If you want it to install missing Homebrew packages, append `--deps`:

```sh
./benchmark/run.sh --deps --setup-only
```

Setup checks the platform, fetches pinned libpostal source and model, verifies model hashes, builds fresh original and CPI harnesses, and prepares the suite configuration. The CPI candidate defaults to the promoted streamed source in `research/final/203-cpi-memory`; use `--cpi-source 119` to rebuild the preserved pre-streaming CPI source for historical comparison. CPI profile-guided optimization uses golden-v1 development data. Setup does not run the full correctness, timing, memory, and thread campaign, though it runs parser work to train the profile.

A fresh-clone audit on 2026-10-10 ran `--setup-only` to completion (exit 0) in 38 minutes, of which 27 minutes were the model download; building took about 11 minutes. These fresh binaries are not the preserved archive artifacts; compiler, PGO, and allocator versions can affect their hashes. Setup alone is not a new acceptance result.

By default, work files go under `~/.cache/libpostal-speedup/`; the repository retains its source records and validation documentation. The script prints the generated suite configuration path and the command to start a full validation campaign. A full campaign can take a long time and records results under `research/suite/results/`. Review the [measurement rules](research/METHOD.md) before starting it; timing is gated on AC power and thermal status by default.

## Options

| Option | Effect |
| --- | --- |
| `--model-dir DIR` | Use an existing libpostal v1.0.0 model directory instead of downloading it. Its files are still checked against `benchmark/model_sha256.txt`. |
| `WORK_DIR=/path ./benchmark/run.sh ...` | Put builds, model and logs somewhere other than `~/.cache/libpostal-speedup/`. The path must not contain spaces, because libpostal's build embeds the model path. |
| `--quick` | After setup, run one short smoke round (three passes, small fuzz). This is a sanity check, not an acceptance run. |
| `--cpi-source 119` | Build the preserved pre-streaming 119 CPI source instead of 203. |
| `--deps` | Install missing Homebrew packages first. |
| `--no-gate` | Do not require AC power and no thermal warning before each timed run. |

Run `./benchmark/run.sh --help` for the full list.

## Exact PGO profile

Setup trains a fresh PGO profile, so its binaries are equivalent to the measured ones but not bit-identical. The profile used for the measured 203 build is committed at `research/final/203-cpi-memory/pgo/dev.profdata`; its SHA-256 (`3f82219e…aedd`) matches the value recorded in the [203 build record](research/final/203-cpi-memory/BUILD.md). The recorded `build_cc.sh` there documents the exact original build command and its absolute paths on the author's machine; it is kept unedited as a build record, and `benchmark/run.sh` is the portable way to rebuild.

## Recorded environment

The 202 captures report 28,160 MB total memory but do not identify exact hardware, OS, or Python versions. A separate audit dated 2026-10-05 records Mac16,8, macOS 26.5.2, Apple Clang 17.0.0, and Python 3.14.4. Older golden-v1 results identify an Apple M4 Pro. These are separate records, not a complete specification captured for each benchmark run. The promoted candidate’s compiler options and hashes are in [research/final/203-cpi-memory/BUILD.md](research/final/203-cpi-memory/BUILD.md) and [SHA256SUMS](research/final/203-cpi-memory/SHA256SUMS); the earlier [119 CPI build record](research/final/119-cpi/BUILD.md) remains available for comparison.
