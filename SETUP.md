# Setup on Apple Silicon macOS

## Access and prerequisites

The repository is private; GitHub access is required. Clone or pull the repository using the commands in [README.md](README.md). The setup script fetches the pinned external libpostal source and model required for a local build.

The benchmark helper targets Apple Silicon macOS and Bash 3.2. Install Xcode Command Line Tools and Homebrew:

```sh
xcode-select --install
```

Install Homebrew from [brew.sh](https://brew.sh). The helper checks for `clang`, `git`, `curl`, `python3`, and `shasum`, then uses Homebrew packages `autoconf`, `automake`, `libtool`, and `mimalloc`. The `--deps` option installs missing packages. The Python tools use only the standard library; there is no pip requirements file. Network access is needed for the private clone and pinned source/model downloads. Setup fetches the official libpostal v1.0.0 model when no local model directory is supplied.

## Prepare the build

From the repository root, run:

```sh
./benchmark/run.sh --setup-only
```

If you want it to install missing Homebrew packages, append `--deps`:

```sh
./benchmark/run.sh --deps --setup-only
```

Setup checks the platform, fetches pinned libpostal source and model, verifies model hashes, builds fresh original and CPI harnesses, and prepares the suite configuration. The CPI candidate defaults to the promoted streamed source in `research/final/203-cpi-memory`; use `--cpi-source 119` to rebuild the preserved pre-streaming CPI source for historical comparison. CPI profile-guided optimization uses golden-v1 development data. Setup does not run the full correctness, timing, memory, and thread campaign, though it runs parser work to train the profile. The runner was checked with fresh setup-only and an eight-row exact-output smoke; that smoke is not a new full acceptance run. These fresh binaries are not the preserved archive artifacts; compiler, PGO, and allocator versions can affect their hashes. Setup alone is not a new acceptance result.

By default, work files go under `~/.cache/libpostal-speedup/`; the repository retains its source records and validation documentation. The script prints the generated suite configuration path and the command to start a full validation campaign. A full campaign can take a long time and records results under `research/suite/results/`. Review the [measurement rules](research/METHOD.md) before starting it; timing is gated on AC power and thermal status by default.

## Recorded environment

The 202 captures report 28,160 MB total memory but do not identify exact hardware, OS, or Python versions. A separate audit dated 2026-10-05 records Mac16,8, macOS 26.5.2, Apple Clang 17.0.0, and Python 3.14.4. Older golden-v1 results identify an Apple M4 Pro. These are separate records, not a complete specification captured for each benchmark run. The promoted candidate’s compiler options and hashes are in [research/final/203-cpi-memory/BUILD.md](research/final/203-cpi-memory/BUILD.md) and [SHA256SUMS](research/final/203-cpi-memory/SHA256SUMS); the earlier [119 CPI build record](research/final/119-cpi/BUILD.md) remains available for comparison.
