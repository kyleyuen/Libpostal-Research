# Team brief — parallel phases R0 / P1 / P2 (2026-10-05, lead session)

The user asked the lead session to pick up from `HANDOFF_2026-10-05.md` and to run the next phases of
`REDESIGN_PLAN_2026-10-05.md` with parallel agent teammates. Each phase was isolated into its own experiment. This
brief applies to every teammate.

Workspace: `W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"` (the path contains spaces, so always quote it).

| Exp | Phase | Teammate |
|---|---|---|
| `research/experiments/158-r0-feature-print` | R0 | r0 |
| `research/experiments/159-p1-exact-phrase-hash` | P1 | p1 |
| `research/experiments/160-p2-records-in-cpi` | P2 | p2 |

Each directory already contains `raw/created.txt`, `KILL_RULE.md` (pre-registered before any build; do not edit it)
and a `README.md` from TEMPLATE.

## Required reading

Read these before building:
- `AGENTS.md`;
- `research/METHOD.md`;
- `research/HANDOFF_2026-10-05.md`;
- `research/plans/REDESIGN_PLAN_2026-10-05.md`, including your phase and its kill rule;
- the READMEs of the experiments your phase builds on (listed in your prompt).

## Isolation (why: parallel teammates share one tree)

Git worktrees are not used. Experiments 150–157, oracles, model data and artifacts are untracked or gitignored, so they
would be missing from a worktree. Isolation is by directory instead.

**You may write only to:**
- your own experiment directory;
- `research/oracles/v2/` — P1 is the only writer;
- a `/tmp/libpostal-<exp>/` scratch directory of your own;
- a `~/.cache/libpostal-020-pgo/<unique name>` PGO directory, named by build. Prefix every build name with your
  experiment number, for example `158fp`, because `build_cc.sh` keys this cache by name.

**Everything else is read-only, including:**
- the ledger `research/experiments/README.md`, `research/ACTIVITY_LOG.md`, the root `README.md` and the handoffs (the
  lead updates these after integration);
- `research/final/*`;
- other experiments, `profiling/*` and `research/oracles/v1`;
- `libpostal/` and the model.

Do not commit, push or run any git command that changes state.

## Build recipe

1. Copy `research/final/119-cpi/build_cc.sh` into your `tools/`.
2. Retarget `E=` to your experiment directory, and keep `SRCDIR` and the rest unchanged.
3. Create `artifacts/scanner_cache` first. It already exists in your directory; the build failed twice without it.
4. Base sources are `research/final/119-cpi/sources`. Copy them into your `sources/` and check them against
   `research/final/119-cpi/SHA256SUMS` before editing. Do not use `157/sources`, which carries `EXP_NO_EMIT`.
5. Build cpi with the same command as `research/final/119-cpi/BUILD.md`, which uses the extra switches from
   `research/final/119-cpi/switches.txt`:
   `EXTRA_SW="..." tools/build_cc.sh <name> clang llvm "-O2 -flto" "-O1" "-flto" /opt/homebrew/opt/mimalloc/lib/mimalloc.o`
6. PGO is trained on `profiling/golden-v1/development.txt` only (the script already does this).
7. Also build an unchanged-source control under your own name (for example `158ctl`), so that a rebuild effect can be
   separated from your change. Record whether its hashes or outputs equal 119's recorded `build_hashes.txt` or outputs.

Record every command, its timestamp, stdout, stderr and exit code under `raw/`. Keep failed attempts and label them
"attempt N".

## Model key dumps (shared, read-only)

The 143 `keydump` was regenerated once by the lead into
`/private/tmp/claude-501/-Users-kyle-Desktop-HW-CSCI-201-Reserach/ebb68cd9-b05a-4683-bc8f-c08c24b1c66a/scratchpad/keys143/`.
All 6 `.tsv` files there have SHA-256 values equal to `143-rw-m1-probe/raw/keys.sha256`. Do not regenerate them.

## Memory cap (24 GB machine shared by 3 teammates)

Run every process that loads the libpostal model (dump, fuzz, oracle, probe or bench) through
`research/tools/model_slot.sh <cmd...>`. This allows at most 3 at once across all teammates.

Do not run more than one model-loading process of your own at a time.

## Hard rules (any violation rejects the experiment)

**1. Byte-identical output.** Compare the candidate against the preserved original, using the binaries in
`research/experiments/000-baseline/artifacts` and the 003 `dump_outputs` method. Run it:
- on every non-reserved set in `research/suite/suite.json`: golden-v1, novel, osm-v3 and unseen-v4 … v11;
- on the 150 holdout `profiling/independent-audit-v1/audit-input.txt`;
- against the frozen `profiling/golden-v1/baseline-outputs.jsonl` and `profiling/golden-v2-novel/baseline-outputs.jsonl`;
- on fuzz seeds 1 and 2 × 2M with `fuzz_pipeline`.

Also show a negative control that proves your check can fail. For the exact verification commands of a recent full
check, see the READMEs of `research/experiments/149-rw-r1-bigram` and `121`.

**2. Single thread.** Inspect your diff for thread or parallel primitives. Count the threads of the running candidate
with `ps -M` and record the result.

**3. Derived from the model only.** Tables, flags, orderings and layouts come from the model files. Nothing may be
derived from corpus statistics, and nothing is cached across addresses.

**4. Reserved data stays untouched.** Do not open `unseen-v12`, `unseen-v13` or any other reserved set. Do not build a
new unseen set.

**5. No timing.** Teammates do not time anything. Parallel processes contaminate timing (experiment 034), and so do
swap and footprint. Do not make peak-memory kill decisions either.
- Instead, write `TIMING_REQUEST.md`. It must list the exact build artifacts with their SHA-256 values, the exact
  bench and footprint commands (`research/tools/bench_ext.c` harness and `/usr/bin/time -l`), and which kill-rule
  numbers each measurement decides.
- The lead runs these serially, on AC power, under `research/tools/env_gate.sh`.
- Load-only memory numbers may be recorded as "indicative, under parallel load".

**6. Honesty.** Do not fabricate any output, timestamp or check. A projection is labelled UNVERIFIED. A step that was
skipped is stated as skipped.

## Tooling notes

- This environment's GateGuard hook blocks your first Bash call, and your first Write of a new file, until you state
  the facts it asks for. State them, then retry the same call.
- The shell is bash 3.2 or zsh. Do not use associative arrays or `${var,,}`. Watch for zsh word-splitting (149 hit a
  false "DIFFERENT" from it).

## Finish

1. Fill in your `README.md`: hypothesis, base hashes, commands, all checks, limitations and decision so far ("pending
   timing" where applicable).
2. Write `raw/DONE` with a timestamp.
3. End with a short summary for the lead covering:
   - correctness status per set;
   - the artifact hashes;
   - the timing request;
   - which kill-rule items are decided and which remain open;
   - anything another phase must know.
