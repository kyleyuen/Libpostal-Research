# Commands used in the second-opinion audit

Run from `/Users/kyle/Desktop/HW/CSCI 201 Reserach` on 2026-10-05 PDT.
`D` below was `/private/tmp/libpostal-professor-audit-20261005` during testing.
The exact tested `input.txt` and `input.tsv` bytes are now frozen in
`profiling/independent-audit-v1/` as `audit-input.txt` and `audit-input.tsv`.

## Exact-output comparison

```sh
research/experiments/017-loadtime-precompute/artifacts/dump_outputs_baseline "$D/input.txt" > "$D/original.jsonl" 2> "$D/original.dump.stderr"
research/experiments/119-component-phrase-ids/artifacts/cpi/dump_outputs "$D/input.txt" > "$D/cpi.jsonl" 2> "$D/cpi.dump.stderr"
cmp "$D/original.jsonl" "$D/cpi.jsonl"
```

The same two dump executables and `cmp` were used on `dad-international.txt`.
All four dumps exited 0 and both comparisons exited 0. The dump stdout and
stderr files are preserved under `correctness/`.

## Fifteen differential-fuzz pairs

```sh
for i in $(seq -w 1 15); do
  seed=$((20261005000 + 10#$i))
  research/tools/fuzz_baseline "$seed" 100000 "$D/input.txt" > "$D/fuzz-$i-original.txt" 2> "$D/fuzz-$i-original.stderr"
  research/experiments/119-component-phrase-ids/artifacts/cpi/fuzz_pipeline "$seed" 100000 "$D/input.txt" > "$D/fuzz-$i-cpi.txt" 2> "$D/fuzz-$i-cpi.stderr"
  cmp "$D/fuzz-$i-original.txt" "$D/fuzz-$i-cpi.txt"
done
```

The loop ran with `set -euo pipefail` and completed. Each output has 100,000
lines. Compressed original/candidate outputs, stderr and per-seed hashes are in
`fuzz/`. The fuzzer emits a 64-bit hash of the ordered parser output for each
mutation, so this is a hash comparison rather than a byte-for-byte dump.

## Fifteen paired timing rounds

The actual timing loop ran under `caffeinate -i bash -c 'set -euo pipefail; …'`.
Within each round, odd rounds ran `original cpi`, even rounds `cpi original`.
For each variant the loop used:

```sh
research/tools/env_gate.sh > "$D/gate-$i-$v.txt"
research/tools/env_snapshot.sh "$D/environment.txt" "before-$i-$v"
date -Iseconds > "$D/round-$i-$v.start"
/usr/bin/time -l "$bin" "$D/input.tsv" 900 > "$D/round-$i-$v.stdout" 2> "$D/round-$i-$v.stderr"
date -Iseconds > "$D/round-$i-$v.end"
research/tools/env_snapshot.sh "$D/environment.txt" "after-$i-$v"
sleep 2
```

`bin` was `research/experiments/015-integer-keys/artifacts/bench_ext_baseline`
for original and
`research/experiments/119-component-phrase-ids/artifacts/cpi/bench_ext`
for cpi. `timing/` contains every original/candidate stdout, stderr,
gate output and start/end timestamp. All 30 invocations completed with
900 numbered passes. The initial 40-pass calibration ran before this loop;
its files were overwritten by round 01 and are not used in the results.

## Memory, threads and API probe

Steady footprint and thread counts were sampled in separate, untimed 5,000-pass
processes. For each variant, the probe waited 20 seconds after launch, ran
`footprint -p <pid>`, then sampled `ps -M -p <pid>` six times at two-second
intervals, and terminated the process. The raw output is in `memory/`.

The existing `research/experiments/025-audit-next/tools/api_probe.c` was linked
once against each static library outside the repository. Original linked with:

```sh
clang -O2 -Ilibpostal/src research/experiments/025-audit-next/tools/api_probe.c research/experiments/000-baseline/artifacts/libpostal.a -lm -o "$D/api_probe_original"
```

Candidate linked with:

```sh
clang -O2 -Ilibpostal/src research/experiments/025-audit-next/tools/api_probe.c research/experiments/119-component-phrase-ids/artifacts/cpi/libpostal.a /opt/homebrew/opt/mimalloc/lib/mimalloc.o -lm -o "$D/api_probe_cpi"
```

Each executable then ran with `default`, `features`, `lifecycle`, and
`lifecycle_disable` arguments under `ulimit -c 0`. Exit status and captured
stdout/stderr are described in the experiment README and stored under
`correctness/`. The candidate's `features` exit was 134; no core dump was kept.
