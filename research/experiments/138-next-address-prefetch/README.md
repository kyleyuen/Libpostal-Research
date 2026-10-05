# Experiment 138 — cross-address lookahead: prefetch the next address's model data while parsing the current one (EXP_XLA)

Date/time/timezone: created 2026-10-04T16:33:06-0700 (`raw/created.txt`); screens 2026-10-04T16:36:19-0700 → 2026-10-04T16:42:22-0700 and 2026-10-04T16:47:53-0700 → 2026-10-04T16:53:53-0700; America/Los_Angeles; commands in `raw/commands.txt`. Status: **rejected (slower)**

Idea (user, 2026-10-04): use the time the thread spends waiting on memory. This is the "light" pipelined version: one thread, byte-identical, a few KB of extra state, nothing reused between addresses.
- **Interface.** A batch-aware caller passes the next address with `libpostal_parse_hint_next()`. Harnesses: `tools/bench_ext_la.c`, `tools/dump_outputs_la.c`, and `tools/fuzz_pipeline_la.c` (hint = the previous fuzzed input, i.e. arbitrary bytes).
- **Stages.** While address N is parsed, the next address's plain ASCII words are resolved one memory hop per stage: vocabulary slot (at entry) → key bytes (after tokenizing) → first-token entry and role row (after context fill) → row-index entries and component-trie node (after scoring) → weight rows (at the end).
- **Hints only.** Every index is bounds-checked (`sources/address_parser.c`, `sources/crf.c`).

## Correctness
- `xla` (PGO trained without hints) and `xlp` (PGO trained with hints, `tools/build_cc_lapgo.sh`) are identical on golden-v1, novel, OSM, unseen v4, both frozen references and fuzz seeds 1 and 2, with hints active on every input (`raw/verify_la_*`). `xla` is also identical without hints (`raw/verify_xla`).
- `xlp` attempts 1–2 failed:
  - attempt 1: a check in the command chain stopped it before building;
  - attempt 2: the switch file had not been created, so it built with no experiment switches.
  Their meaningless outputs (verify/screen run without a binary; sha e3b0… = empty file) are kept in `raw/attempt1_xlp_not_built/`. Attempt 3 is the result.

## Speed
The machine was in interactive use (load average 3–5).
Screen 1, cpi vs xla (`raw/screen_analyze2.stdout`):
        v1 median cpi 5.571 xla 5.668; xla speedup per round [0.9772, 0.9702, 0.9615, 0.9948, 0.9695, 1.0045] median 0.9737; rounds faster 1/6
        v1 min    cpi 5.371 xla 5.531; xla speedup per round [0.9669, 0.9751, 0.964, 0.9817, 0.9828, 0.9941] median 0.9784; rounds faster 0/6
        osm median cpi 3.503 xla 3.768; xla speedup per round [0.9658, 0.9106, 0.8886, 0.9201, 0.9351, 0.9538] median 0.9276; rounds faster 0/6
        osm min    cpi 3.373 xla 3.611; xla speedup per round [0.9683, 0.935, 0.9483, 0.9176, 0.9229, 0.9384] median 0.9367; rounds faster 0/6
Screen 2, cpi vs xlp (`raw/screen2_analyze2.stdout`):
        v1 median cpi 5.428 xlp 5.639; xlp speedup per round [1.0428, 0.8559, 0.967, 0.8861, 0.9633, 0.9618] median 0.9626; rounds faster 1/6
        v1 min    cpi 5.321 xlp 5.51; xlp speedup per round [0.9928, 0.8734, 0.97, 0.9487, 0.952, 0.974] median 0.961; rounds faster 0/6
        osm median cpi 3.51 xlp 3.727; xlp speedup per round [0.9483, 0.9302, 0.9652, 0.9514, 0.9354, 0.8961] median 0.9419; rounds faster 0/6
        osm min    cpi 3.383 xlp 3.618; xlp speedup per round [0.9097, 0.9394, 0.9557, 0.9266, 0.9398, 0.8969] median 0.933; rounds faster 0/6

## Decision
Rejected: −2.6% / −7.2% (xla) and −3.7% / −5.8% (xlp) on golden-v1 / OSM, slower in almost every round.
Interpretation, consistent with 137: the model data that can be predicted from the next address's words (vocabulary entries, word-feature rows, first trie nodes) is mostly cached already. Predicting it costs about 0.1–0.25 µs per address (splitting and hashing words, key compares) and saves little.
What remains cold is data that only real execution reveals: deeper trie nodes, dictionary keys built from phrases, postcode context. Only full interleaving, running several addresses' actual code in lockstep, could overlap those waits. That is a multi-day restructuring with uncertain payoff, not attempted.
