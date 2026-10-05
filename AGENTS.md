# Libpostal research workspace

Read `README.md`, `research/experiments/README.md` and `research/METHOD.md` before
performance work. Continue to apply the user's global engineering memory policy.

The user will present the final improvement to a professor. Preserve an auditable
experiment history, including unsuccessful attempts, instead of keeping only the
best result.

- Create a numbered experiment record using `research/experiments/TEMPLATE.md`.
- Capture the hypothesis, source commit/patch (including new source files), exact
  commands, timestamps, stdout/stderr/exit codes, build settings and binary/model/
  input hashes. Save raw trials, output checks, tests, memory results and decisions.
- Update the experiment ledger and append to `research/ACTIVITY_LOG.md` after work.
  Update the professor report when verified results change; never show predictions
  as measured improvement.
- Do not overwrite `profiling/golden-v1` inputs, source annotations, reference
  outputs, historical runs, the original checksum manifest, or the copied baseline
  artifacts. New datasets and experimental results get separate versioned paths.
- Use the preserved original executable/library in `research/experiments/000-baseline/artifacts`
  when comparing candidates. Follow the measurement protocol and include memory
  and output preservation, not speed alone. Do not mix different corpus timings.
- Use development inputs for iterative tuning and keep milestone/final validation
  results explicit. Record all regressions and do not silently edit reference data.
- Log limitations honestly. Historical command capture is incomplete; do not
  fabricate past timestamps, outputs, approvals or successful checks.

Hard constraints for every experiment (see `research/METHOD.md`): outputs must be byte-identical to the
original library on all golden-v1 inputs, and parsing must stay single-threaded. Violating either rejects the experiment.
