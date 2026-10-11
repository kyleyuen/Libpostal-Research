# Initial edge-address smoke attempt

Input: `edge-address.txt`, one line: `221B Baker St, Apt #4½` (24 bytes including newline).

All seven dumper invocations below exited 0 in serial order. Each `.stdout` file is 117 bytes and byte-identical to `original-000.stdout`; all `.stderr` files are empty. A first attempted `original-release` command used an incorrectly entered working directory and exited 1 before launching the dumper; the correctly rooted rerun is the recorded successful output.

- Preserved reference: `182-benchmark-suite-prep/artifacts/original-000/dump_outputs`
- Diagnostic and release executables: `artifacts/build-20261007-1505-final-registry/{original,cpi,prototype181}/{diagnostic,release}/dump_outputs`
- Each executable received only `raw/calibration-20261007-final-registry/edge-address.txt`.
- On each diagnostic invocation, `MEASUREMENTS_JSON` pointed to the corresponding `*-diagnostic.metrics.json`; no such sidecar was created despite exit 0. Inspection showed the build script omitted `-DEXP185_DIAG` when compiling the dumper translation unit, so its header compiled the metrics call to a no-op. This is a failed instrumentation smoke, not evidence that the metrics are zero.
- Root authorized the runner to correct the diagnostic dumper link flags and relink only diagnostic dumpers into a fresh artifact set. Release binaries and libraries are unchanged. A new gated smoke and real sidecar validation remain outstanding.

Captured outputs: `original-000.stdout/.stderr`, `original-diagnostic.stdout/.stderr`, `original-release.stdout/.stderr`, `cpi-diagnostic.stdout/.stderr`, `cpi-release.stdout/.stderr`, `prototype181-diagnostic.stdout/.stderr`, and `prototype181-release.stdout/.stderr`.
