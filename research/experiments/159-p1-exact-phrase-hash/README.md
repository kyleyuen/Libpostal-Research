# Experiment 159 — P1: exact hash phrase search (model-derived trie-quirk flags)

Date/time/timezone: 2026-10-05T17:16:01-0700 (created by the lead); work 17:2x–19:2x −0700 (America/Los_Angeles), with one user pause/stop in between (nothing was running at the stop; no partial outputs).
Status: **exactness passed; memory fails against the corrected in-memory baseline (simple table); timing pending (TIMING_REQUEST.md).**
Kill rule: `KILL_RULE.md` (pre-registered, unedited). Command log: `raw/commands.txt` (every command with start/end timestamps and exit codes; stdout/stderr in `raw/<name>.*`).

## Hypothesis and change

**Measured bottleneck and why this should help.** 156 measured an optimistic hash bound of the three context-fill phrase searches: 0.81 / 0.42 µs against cpi's whole context fill of 1.66 / 1.17 µs. That bound was not exact (0.25–0.97% of addresses wrong) because the trie search has structure-dependent quirks. P1 had to reproduce `trie_search_tokens_from_index` exactly, using flags derived from the model only.

**Exact change.** This is a new standalone component; cpi is not modified, and wiring it in is P3.
- `src/ps_exact.{h,c}`: the table builder, the mmap loader, the one-probe-per-token search and a reference port.
- `tools/trie_struct.c`: model structure dump.
- `tools/theory_check.c`: verification of the structure theory.
- `tools/ps_test.c`: correctness harness.
- `tools/ps_bench.c`: timing harness, for the lead.
- `tools/run_*.sh`: scripts for the runs.

Base: git `181737d3` (worktree state in the brief). All 159 files are new and untracked. Hashes (attempt 2, current) are in `artifacts/hashes.txt`.

## Root cause: the quirks, written from `libpostal/src/trie_search.c` and `trie.c`

The search walks the double-array trie character by character. Its per-token behaviour depends on whether a prefix is an **explicit node** (base ≥ 0) or lies inside a **tail-compressed leaf** (base < 0, the rest of the key stored as a string). The six quirks follow; line numbers refer to `trie_search.c`.

1. **No retry after a dead end (156 Q1).** A multi-token span can end on an explicit node that has neither a `'\0'` (terminal) child nor a `' '` child. The search then reports "No continuation … yielding tokens": it resets with `state = NO_MATCH` and does **not** set `i = phrase_start`, so start+1 is never retried. Falling off the trie, or a tail mismatch, after a partial match **does** retry from start+1 (`last_state == PARTIAL_MATCH → i = phrase_start`).
2. **Token-length `strncmp` in tails (156 Q2).** Once the walk enters a leaf, `trie_node_search_tail_tokens` compares each later token with `strncmp(tail_ptr, token, token_len)` and skips a `' '` only if the tail has one there. Tokens can therefore match a key with **no space between them** ("a", "bc" matches key "abc"). Success requires the tail to end exactly at a token end.
3. **Stop at the last token (found here).** Suppose the last token ends on an explicit node with no terminal and the phrase has no earlier match. Then the search hits `break` ("At last token") and **stops entirely**: no later start position is ever tried.
4. **Ideographic tokens join without a space (found here).** After an `IDEOGRAPHIC_CHAR` token with no `' '` child, the node is kept (`last_node = node`). The next token then walks on with no space, even if a match was just recorded; matches are not pushed there.
5. **Space-child leaf rollback (found here).** If the `' '` child is itself a leaf, the next token compares the whole token against that tail ("rolling back one character"). The outcome equals the general leaf rule below.
6. **Garbage reads (found here; checked on the model).** After an empty or WHITESPACE token, the code can read the terminal and `' '` transitions of a **leaf** node, using a negative base, which is out-of-contract memory. `trie_struct` checked every leaf whose data index is ≤ 7, the only ones where those reads land on in-range nodes, in all three tries. No read matches (`raw/trie_struct_a4.stdout`, `match=0` ×21). The `' '` read always lands on node index ≤ 0, which is the null node. So on this model, a leaf has "no terminal, no continuation".

**Structure theory, verified on the model.** Sort the keys bytewise, comparing each key with its terminating `'\0'`. Then a prefix P is an explicit node iff at least 2 keys have prefix P, and a key's leaf depth = max(LCP with its neighbours) + 1.
- `theory_check` compares this prediction with the leaf depth of every key dumped from the real tries (`trie_struct`, linked to the original library, 142 `ctl`).
- **0 mismatches** for all 153,670 dictionary keys, 4,012,750 component keys and 5,483,892 postal keys.
- The predicted explicit-node counts **equal** the real tries' counts: 98,558 / 4,207,676 / 744,087. Nodes = explicit + leaves, with no other nodes in use.

Logs: `raw/theory_*.stdout`, `raw/trie_struct_a4.stdout`. Attempt 1 of `trie_struct` crashed in its stats printout (signed `char *alphabet` index); it is kept in `raw/` and `raw/commands.txt`.

**Consequence.** For the original search, the outcome of one token depends only on the class of X = (current node path) + (`' '` or nothing) + token:
- **E** = X is an explicit node, with its terminal?, data and `' '` child?;
- **L** = X is a prefix of exactly one key K, the leaf was entered inside this token, and the result is the tail search on K;
- **F** = X is not a key prefix: it fell off, or there was a tail mismatch. The original handles both identically (checked line by line; also proven empirically below).

## Design: flags derived from the tries at build time

The table holds a 12-byte open-addressing slot per entry at load 0.75: `koff u32`, `data u32`, `len u8`, `flags u8 {E, TERM, CONT, L}`, `tag u16`. Every hit is verified against the key bytes, which are the model's keys sorted. Entries are derived from the sorted key set:

- **E entries.** Explicit nodes with terminal (data), with a `' '` child (CONT), or whose path contains `' '` or a byte ≥ 0x80. The last group are dead-end nodes, which matter only when the span has more than one token or uses an ideographic join (quirks 1, 3 and 4).
- **L entries.** For each key K whose leaf depth ℓ ≤ |K|, the prefixes K[0..e) for e in [ℓ, first `' '` at or after ℓ, or |K|]. This is the explicit-node / tail-boundary span that covers quirk 2. Each carries K and its data.
- **Why a miss is exactly F (completeness).** A "normal" query X = S + J + w, with w a non-empty token without spaces, enters a leaf only inside J+w. So in L it ends within the anchor word [ℓ, nsp]. A dead-end node that is not stored has a path with no space and no high byte; it can only be queried by a single non-ideographic token at a phrase start, where E-dead-end and F give the same result.
- **Routing to the reference port.** Inputs outside that argument go to `ps_search_tokens_ref`. These are any token that is empty, of type WHITESPACE, contains `' '`, or is ASCII-only with type IDEOGRAPHIC_CHAR. The reference is a line-by-line port of `trie_search_tokens_from_index` whose node reads (child, terminal, leaf tail) are answered by binary search on the sorted keys, which is exact by the theory above.
- **Search.** The fast path is the original state machine verbatim, with the character loop replaced by one probe per token (`src/ps_exact.c`).

Table sizes (`raw/build_tables.stdout`; entry counts equal `theory_check`'s, no duplicates):

| Table | Keys | Entries (E + L) | Slots @0.75 | File/mmap bytes |
|---|---:|---:|---:|---:|
| dict | 153,670 | 454,225 | 605,635 | 10,104,988 |
| comp | 4,012,750 | 19,027,064 (12,469,788 midword) | 25,369,420 | 399,470,808 |
| postal | 5,483,892 | 6,176,053 | 8,234,739 | 189,887,380 |
| **total** | | | | **599,463,176** |

The build from the struct TSVs took 4.07 s and peaked at 854 MB RSS. This is indicative only: it ran under parallel load, with `/usr/bin/time` on the build command, and is not a benchmark.

## Exposed API (for P3)

```c
#include "ps_exact.h"
ps_table_t *ps_table_load(const char *path);          // one open + fstat + read-only mmap + header check; nothing parsed or rebuilt
bool ps_search_tokens_with_phrases(const ps_table_t *t, char *str, token_array *tokens, phrase_array **phrases);
//   drop-in for trie_search_tokens_with_phrases(trie, str, tokens, phrases): same return value, same phrases (start, len, data, order)
ps_table_t *ps_table_build_tsv(const char *tsv); bool ps_table_save(const ps_table_t *, const char *); void ps_table_destroy(ps_table_t *);
```

Wiring for P3 in `address_parser_context_fill`. The validity filter and `token_phrase_memberships` stay as they are.
- `search_address_dictionaries_tokens_with_phrases(normalized_str, normalized_tokens, NULL, &p)` becomes `ps_search_tokens_with_phrases(dict, …)` (the lang NULL search starts at ROOT, `address_dictionary.c:241`).
- `trie_search_tokens_with_phrases(parser->phrases, …)` becomes `ps_search_tokens_with_phrases(comp, …)`.
- `trie_search_tokens_with_phrases(parser->postal_codes, …)` becomes `ps_search_tokens_with_phrases(postal, …)`.

Load cost:
- 3 mmaps, O(1) work.
- Pages fault in on first touch, which is part of the cold pass in the timing request. I did not time it (rule 5).
- The tables are built offline from the model: `trie_struct` (model load) dumps the TSVs, then `ps_test build` turns them into the table files.
- Caveat: cpi's own search uses `EXP_FTT` and keeps the dictionary trie for prefix/suffix search, so that trie stays loaded.

## Correctness (all exit codes and logs in `raw/`)

**Attempts.**
- **Attempt 1** (`raw/o2_*`, `gen_*`, `negctl_*`, `smoke_bench_*`; hashes `raw/attempt1_hashes.txt`) used `tag = h >> 48`, taken from the same high bits that pick the slot. Correctness was unaffected, but the tag filter was weak, which would bias timing.
- **Attempt 2** uses `tag = low 16 bits`. Tables were rebuilt with identical entry counts. Its first run (`raw/a2_*`) failed on an unquoted path containing spaces (exit 2) and is kept. The rerun is **`raw/a2b_*`**.
- **Same counts.** The table below holds for both attempt 1 and attempt 2 (a2b); every count is identical.


| Set | Addresses | Tokens | O2 per-token mismatches (dict / comp / postal) | Phrase arrays: hash vs reference / vs ORIGINAL library | Searches routed to reference |
|---|---:|---:|---|---|---:|
| golden-v1 (v1) | 12,863 | 75,659 | 0 / 0 / 0 | 0 / 0 | 3 |
| novel (v1) | 12,863 | 73,763 | 0 / 0 / 0 | 0 / 0 | 3 |
| osm-v3 (v1) | 11,405 | 61,157 | 0 / 0 / 0 | 0 / 0 | 0 |
| fuzz-s1-200k (v1) | 178,955 | 1,363,734 | 0 / 0 / 0 | 0 / 0 | 21 |
| fuzz-s2-2m (v2, new) | 1,788,077 | 13,641,474 | 0 / 0 / 0 | 0 / 0 | 468 |
| fuzz-s3-2m (v2, new) | 1,788,831 | 13,629,005 | 0 / 0 / 0 | 0 / 0 | 426 |
| generator seed 1 / 2 (model keys) | 2,000,000 cases each | — | n/a | 0 / 0 each | 559,621 / 558,394 |

- **What the comparison covers.** The phrase arrays are compared in full: start, len, data, order and return value. The ORIGINAL side is the library function on the real tries (142 `ctl`, `-O2`, original sources). The 156 quirk addresses (61 golden-v1, 28 OSM, 48 novel, 1,738 fuzz-s1) are all exact now.
- **The generator.** It is derived from the model's keys only. It produces keys split at spaces and at random byte positions, truncated keys, a neighbour-key prefix followed by a key, one IDEOGRAPHIC_CHAR token per UTF-8 character, junk tokens, and (in 10% of cases) empty, WHITESPACE, space-containing and ASCII-ideographic tokens. Limitation: per-quirk hit counts were not instrumented. The per-character ideographic kind also gives ASCII keys IDEOGRAPHIC_CHAR tokens, which are routed; that is why about 28% of generator searches go to the reference. Fast-path coverage of the no-space ideographic join comes mainly from the corpora's type-3 tokens: 3,360 in golden-v1, 41,687 in fuzz-s1, and more in v2.
- **Negative controls.**
  - **Attempt 1** (`raw/negctl_flip_golden-v1.*`) swapped E↔L on comp entry "sk" and **did not fail**. For a key that is the first key of its own range, the L path's tail is empty and yields the same match and data, so that swap is semantically equivalent. Kept.
  - **Attempt 2** (`raw/negctl_a2_flipterm_golden-v1.*`) flipped TERM on "sk" and gave **43 mismatches** (reproduced in `raw/a2b_negctl_flipterm_golden-v1.*`), flagged both against O2 and against the reference (exit 1).
  - **Routing control** (`raw/negctl_forcefast_gen_s1.*`, `PS_FORCE_FAST=1`): 217,758 generator cases mismatch the original. So the routing is necessary, and the comparator detects failures.
- **Timing harness.** A functional smoke run (`raw/smoke_bench_*` and `raw/a2b_smoke_bench_*`, OSM, 2 passes) gives the same phrase checksum for the exact and orig variants (`2de04b861a01c7a4`). Its printed µs values are not results.
- **Threads.** `ps -M` on a running `ps_test` shows 1 thread (`raw/threads_ps_test.txt`). There are no thread or parallel primitives in the 159 sources (`raw/thread_primitive_grep.txt`).
- **Hard rule 1 (byte-identical parser output on all sets).** Not applicable yet: no parser binary changed in P1, so no cpi build or unchanged-source control was made. It applies at P3.

## Memory (measured sizes; P3 verdict is a projection)

| | Bytes | Source |
|---|---:|---|
| Tries replaceable: components 121,433,298 + postal 110,452,043 | **231,885,341** | nodes×8 + tail + data×8 from `trie_struct` (`sizeof` both 8, `trie.h`) |
| Dictionary trie (stays: cpi's prefix/suffix search reads it) | 3,801,276 | same |
| Plan's figure (on-disk, includes non-trie data such as the postal graph) | ≈768 MB | 156 |
| 159 tables: comp + postal / + dict | 589,358,188 / 599,463,176 | `raw/build_tables.stdout` |

**Reference fallback memory (from `src/ps_exact.c` and `raw/build_tables.stdout`).** `ps_search_tokens_ref` reads the sorted keys through `off[]`, `kdata[]` and the key blob. All three are already inside the 599.5 MB table files, so nothing is extra:
- key data and offsets 77.2 MB (dict 1.2 + comp 32.1 + postal 43.9);
- blob 111.7 MB (1.6 + 62.9 + 47.2).

The hash path verifies hits against the blob too. Only `off[]` and `kdata[]` are touched solely by the fallback, so under mmap those pages become resident only when the fallback runs (UNVERIFIED that they stay cold; not measured).

Readings of kill item 2 ("tables exceeding the tries they replace"):

| Baseline | Tables | Verdict |
|---|---|---|
| Literal ≈768 MB on-disk figure | 599 MB | not exceeded |
| Corrected in-memory 232 MB (the lead's decision) | 589 MB, 2.54× | **exceeded, kill triggered for this simple variant** |

**Projected P3 memory verdict (UNVERIFIED).**
- **Assumptions.** Tables fully resident, replacing the two tries (−231.9 MB) and adding all three tables (+599.5 MB).
- **Delta.** About **+367.6 MB** peak/steady resident against cpi's ≈51–54 MB headroom (original 2.614 GB − 20 MB, cpi peak 2.54 GB). P3 would **fail** memory with this table.
- **Offset from cpi's FTT table (measured).** cpi keeps a first-token table `trie_ftt_tab` for the component search: 7,389,122 slots × 8 B = 56.4 MiB ≈ 59.1 MB (`raw/cpi_ftt_verbose.stderr`, `FTT_VERBOSE=1`, 119 cpi, load only). Its walk fields become dead once P3 replaces the component search. Its bits 61/62 also carry `EXP_PS_FLAGS` (prefix/suffix), which would have to be repacked (2 bits/slot ≈ 1.8 MB). Net offset ≈ −57 MB, which makes the simple table ≈ **+310 MB**, still failing.
- **mmap caveat.** Clean file-backed mmap pages count in RSS but not in phys_footprint, so a footprint-only reading would understate this. The honest comparison is resident bytes.

**159b compaction bound (not built; derived from the measured counts, UNVERIFIED).**
- **Components of a compact variant.**
  - drop the midword L entries (comp 12.47M, postal 0.42M, dict 0.28M) in favour of an exact filter plus a reference fallback (≈16.5 MB at ≈1.25 B/entry);
  - store the keys front-coded in blocks of 16 (comp 34.6 + postal 20.8 + dict 0.9 MB, measured by `tools/frontcode_size.py`, `raw/frontcode_size.stdout`);
  - key data 4 B/key (38.6 MB);
  - sampled offsets (≈2.4 MB).
- **Totals.**
  - With 12-byte slots at 0.75 (comp 104.9 + postal 92.0 + dict 2.9 MB), the total is ≈314 MB. That is ≈+70 MB vs the tries replaced, and still over the headroom.
  - With a minimal perfect hash (load ≈1.0), the total is ≈265 MB, about **+20–35 MB**, which would fit within the ≈51–54 MB headroom.
- **With the FTT offset** (≈ −57 MB, above), the perfect-hash bound is about −25 MB and the 0.75-load bound about +13 MB. Both are within the ≈51–54 MB headroom.
- **Reading.** The bound does **not** kill P3 on memory by itself. Any compact variant pays front-coded verification (a block decode per hit) and a filter probe per miss, and its speed is unknown.
- **Not admissible.** Key-free fingerprints would break exactness for arbitrary inputs, because a collision changes the output and cannot be ruled out.

## Timing

Not run by me (rule 5). `TIMING_REQUEST.md` lists the artifacts, hashes, commands and the kill items each run decides.

## Decision so far

| Kill item | Status |
|---|---|
| Unexplained mismatch | **Not triggered**: 0 mismatches on 6 corpora (2 of them new v2 oracles) and 4M generator cases. Every quirk is explained above. |
| Tables vs tries | Passes the literal 768 MB line. **Fails the corrected 232 MB in-memory baseline** (simple table, 2.54×). |
| Context-fill gain | **Open**: pending the lead's timing (TIMING_REQUEST.md). Even a pass there cannot rescue P3 on memory unless 159b is built and also passes. |

**Next action.** The lead decides from the timing request. The options are to build 159b and size its memory exactly before P3, or stop P1/P3.

**Known limitations.**
- The O2 checks cover the three searches, not the full parse.
- Per-quirk coverage of the generator is not instrumented.
- The table build peak (854 MB) was not part of any parser memory.
- The ledger, activity log and professor report are not updated by me (the brief makes them read-only); the lead integrates.
