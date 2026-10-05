# Plan — full cross-address interleaving ("software hyperthreading") for the libpostal parser

Status: **executed through Phase 3 (experiments 139–140) and stopped at the Phase 3 gate**: exact in every mode, but slower (−3…−7% golden-v1, −7…−10% OSM). Requested by the user 2026-10-04. Base: `cpi` (`research/final/119-cpi`).
Every speed figure below is an UNVERIFIED estimate unless it cites an experiment.

## 1. Goal and constraints
Hide main-memory waits by keeping several addresses in flight on **one thread** and switching to another address whenever the current one is about to wait on memory.
- **Byte-identical output:** each address runs exactly the code it runs today, with the same floating-point operations in the same order; interleaving changes only timing.
- **Memory ≤ original:** peak today is 2.543 GB vs the original's 2.613 GB (≈70 MB headroom). N in-flight addresses add N copies of per-address scratch state, a few MB at most (measured in Phase 1).
- **Single thread:** user-space coroutines on the calling thread; no OS threads.
- **No cross-address reuse:** per-address state is private to each in-flight address and reset as today; nothing computed for one address is used by another.
- **No dataset tuning:** the batch width N and the yield points are fixed from hardware facts and code structure, never searched on benchmark data. The yield-placement profile uses golden-v1 *development* only (as PGO does) and is flagged as such.
- **Interface change, disclosed in the report:** the speedup needs a batch call (`libpostal_parse_address_batch`). A single isolated address gets no benefit.

## 2. Why this is the one lever left, and its ceiling
- **Cold vs warm (108/110, cpi on the cleaned machine):** about 57% of golden-v1 development time and about 40% of OSM time is cold-memory waiting.
- **What's already been tried:** within-address prefetching is exhausted, and 137/138 showed the data predictable from the next address's words is already cached.
- **What remains cold:** data only real execution reveals (deeper trie nodes, keys built from phrases, postcode context). Only running the actual code of several addresses side by side overlaps those waits.
- **Ceiling = warm-cache time:**

| Data | Warm-cache time (stage-timer harness) | Warm bound | Older 088 bound (pcp) |
|---|---:|---:|---:|
| golden-v1 dev | ≈2.5 µs | ≈12× | 11.1× |
| OSM | ≈2.2 µs | ≈8.6× | 8.1× |

The stage-timer harness reads about 8% high cold, so these bounds are rough. The 7.8× target lies below both bounds, so this is the only remaining path that could reach it.
- **Rough model:** time per address ≈ compute + (1 − f) × waiting + switch overhead, where f is the share of waiting hidden. At f = 50% and 30 switches × 8 ns: golden-v1 5.17 → about 4.2 µs (≈7.5×); OSM 3.33 → about 2.9 µs (≈6.7×). At f = 25%, about 6.6× and 6.2×. **UNVERIFIED.**

## 3. Design
### 3.1 Coroutines (stackful, hand-written arm64 switch)
- Each in-flight address runs on its own small stack (64–256 KB, sized in Phase 2 from measured stack depth). A switch saves and restores callee-saved registers (x19–x30, sp, d8–d15): about 20–30 instructions, expected 3–10 ns on M-series (measured in Phase 0).
- Why stackful: the yield points sit deep inside existing functions (trie walk loops, dictionary probes, accumulation). A stackful coroutine lets `PF_YIELD(addr)` sit inline with no rewrite of control flow.
- Why hand-written: `ucontext` is deprecated and slow on macOS; C has no native coroutines. A vendored minimal switch (as in minicoro / libco) is about 40 lines of assembly. Writing our own keeps the dependency list empty and the code auditable.

### 3.2 Per-address state ("address context")
Every mutable piece of parse state becomes a field of one `ap_inflight_t` struct; a single global `ap_cur` pointer is swapped on every switch. Inventory from the cpi sources (Phase 1 completes and audits it mechanically: `nm -m` on every object file lists every writable data symbol, including function-local statics, and each must be classified):
- **address_parser.c:**
  - `parser->context`: normalized and admin arrays, phrase arrays and memberships, feature arrays, separators;
  - `rs_*` reuse buffers (tokens, tokenized string, labels, components);
  - memos `ap_vm`, `ap_wm`, `ap_pm` plus `ap_pm_gen`;
  - block-static `ftt_slots`;
  - response-block buffers;
  - 101/119 scratch (`cp_tmp` is load-time only);
  - 077 prefetch locals.
- **trie_search.c:** `trie_ftt_slot`, `trie_ftt_n` (set per address by the component search).
- **crf.c:** `crf_state_ids`, `crf_trans_ids`, `crf_state_tok`, `crf_trans_tok`, `crf_ik_keys`, `crf_ik_n`/`m`, `crf_feature_ids`, `crf_lean_cap_items`, `crf_trans_saved`, and the CRF context (`state`, `state_trans`, `alpha`, `backward_edges`).
- **crf_context.c:** `vmo_max`.
- **Read-only after load (stay shared):** the model, the dictionaries, the 056/101/119 tables, `nt_table`, character tables, `crf_role_ids`.
- **Watch for:** lazy one-time initializers that run during the first parse (e.g. `nt_ready`, `ci0`/`cis`, name-byte caches in `fa_nb_cache_t` statics). They must be idempotent and finished before interleaving starts (Phase 1 forces them at load).

### 3.3 Scheduler and batch API
`libpostal_parse_address_batch(char **addrs, size_t n, options, libpostal_address_parser_response_t **out)`:
- round-robin over N slots; when an address finishes, its slot takes the next one;
- responses are written to `out[i]` in input order;
- **N = 4**, fixed in advance: about 4 addresses × ≈300 touched lines ≈ 150 KB fits in L2, and 4 overlapping miss chains stay well under the core's outstanding-miss capacity;
- N = 1 must reproduce today's behaviour exactly (used for overhead measurement).

### 3.4 Yield points (prefetch, then switch)
Placed where the line profiles (112, 124, 125) show dependent cold misses, in order of measured cost:
1. **Trie walks** (`trie_search_tokens_from_index`, ≈0.28 µs cold on dev): before reading the next node in a walk that has left the FTT-assisted first step, prefetch the node and yield.
2. **Feature-dictionary lookup batches** (`crf_dict_lookup_array`, `fd_next` probes): after the batch slot prefetch (already present), yield once before the compare loop; again after the arena prefetch.
3. **Weight-row accumulation:** after the weight-prefetch loop, yield once before accumulating.
4. **Vocabulary slot loop and postcode-context checks** (`fd_find_slot` ≈0.11, `pc_has_edge` ≈0.05): one yield after their batch prefetch.
Start coarse (one yield per stage boundary, items 2–4), then add the fine-grained trie yields (item 1) only if coarse yields pay.

## 4. Phases, gates, cost

| Phase | Work | Gate (stop if not met) | Estimate |
|---|---|---|---|
| 0 | Microbenchmarks: switch cost on this Mac; chase-latency with 1/2/4 interleaved chains | switch ≤ 10 ns; 4 chains ≥ 2.5× throughput of 1 | 1–2 h |
| 1 | Per-address state refactor (`ap_cur->` everywhere), no interleaving; `nm` audit of writable symbols | byte-identical on full suite + fuzz; slowdown ≤ 1% vs cpi; memory unchanged | 0.5–1 day |
| 2 | Coroutine runtime + batch API + harnesses (`bench_ext_batch`, `dump_outputs_batch`, `fuzz_pipeline_batch`), N = 1 | identical; overhead ≤ 1% | 0.5 day |
| 3 | Coarse yields (items 2–4), N = 4 | identical; ≥ 3% faster on golden-v1 **and** not slower on OSM (screen + stage timers) | 0.5 day |
| 4 | Fine trie yields (item 1) | each addition ≥ 1.5% with a mechanism, else reverted | 0.5–1 day |
| 5 | Stress verification: random-switch mode (yield at every point with pseudo-random choice of next slot), all sets, 2 × 2M fuzz, N = 1/2/4/8 (outputs only) | identical everywhere | 0.5 day |
| 6 | Full protocol: unseen golden-v12, validation (103 protocol), memory, threads, packaging | as for every deliverable | 0.5 day |

Total: about 3–5 working days if every gate passes. Phases 0 and 3 are the early kill points.

## 5. Risks
- **Little waiting left to hide on OSM-style data:** 137/138 showed much of the model is already cached; f may be small there.
- **Refactor cost:** turning globals into `ap_cur->` fields adds an indirection on hot paths (Phase 1 gate).
- **Cache pressure:** N working sets compete for L1 (128 KB): more misses, partly offsetting the overlap.
- **Toolchain:** PGO/LTO and coroutine stacks. A custom stack switch is invisible to the compiler; correctness relies on the ABI (callee-saved registers). Verified by Phase 5's random-switch stress.
- **Measurement fairness:** the baseline keeps per-address calls (the original API); the batch result is reported separately with the interface difference stated.
- **Professor's view:** batching is a legitimate single-thread technique, but it changes the API; confirm acceptability before Phase 6.

## 6. Exactness argument
Interleaving changes the order in which *different* addresses' instructions execute, never the order within one address. Each address reads only shared read-only model data and its own `ap_inflight_t`. Hence its floating-point operations, branch decisions and output bytes are those of the sequential run. Phase 1's `nm` audit and Phase 5's random-switch stress test are the evidence; all standard output checks run through the batch API.
