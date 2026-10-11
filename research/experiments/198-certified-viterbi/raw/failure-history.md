# Retained build failures

1. Initial build command used only `EXTRA_SW=-DEXP_CERT_F32_VITERBI`, so required CPI switches were absent. The compile reached copied `address_parser.c` and reported missing selected macros such as `APF`/`phrase_string_len`; the process output was shown in the tool transcript but its complete stdout/stderr files and exit code were not captured. This is recorded as incomplete historical command capture, not a successful build.
2. A rebuild with the CPI switch set compiled source files but stopped because `artifacts/scanner_cache/` did not exist. The observed final compiler diagnostic was `unable to open output file .../artifacts/scanner_cache/scanner_clang_O0.o`; captured `build.exit` was 1. The attempt's `build.stderr` was subsequently replaced by the successful build log; this loss is disclosed rather than reconstructed as raw output.

The successful corrected build is the distinct `raw/build.*` set and `artifacts/release/` output.
