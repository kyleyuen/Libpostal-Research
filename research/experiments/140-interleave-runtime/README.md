# Experiment 140 — interleaving Phases 1–3: per-address state, coroutine runtime, batch API, yield points (EXP_BATCH)

Date/time/timezone: created 2026-10-04T18:10:39-0700 (`raw/created.txt`); screens 2026-10-04T18:20:21-0700 → 2026-10-04T18:29:25-0700 and 2026-10-04T18:29:42-0700 → 2026-10-04T18:38:43-0700; America/Los_Angeles; commands in `raw/commands.txt`. Status: **stopped at the Phase 3 gate (slower); exact**

Plan: `research/plans/INTERLEAVING_PLAN_2026-10-04.md` (user: "lets do it"); Phase 0 in 139.

## What was built
`tools/patch_140.py` applies the change to a copy of cpi's sources; the runtime itself is `tools/runtime_140.c`.
- **Per-address state.** Every writable symbol in the compiled objects was inventoried (`xcrun nm`). The per-address ones are saved and restored at every switch, with slot 0 holding the parser's original contexts:
  - the parser context and CRF context;
  - the slot-loop scratch (block-static before), memos `ap_vm` / `ap_wm` / `ap_pm` (+ generation counter), reuse buffers `rs_*`;
  - crf.c's key / id / token arrays, int-key buffers, lean-context capacity, transition alias, accumulate flag;
  - the Viterbi buffer; `trie_ftt_slot` / `_n`; `fa_nb_arr`.
  Model data and load-time tables stay shared and read-only.
- **Runtime.** Stackful coroutines with 139's arm64 switch (8 MB stacks); round-robin scheduler; `libpostal_parse_address_batch(addrs, n, options, out)`.
- **Yield points:**
  - 0: context fill, after the 074 prefetch passes;
  - 1: each dictionary-lookup window, after its slot prefetch;
  - 2: scoring, after the weight-row prefetch.
  `AP_YIELD_MASK`, `AP_BATCH_N` and `AP_STRESS` (random switching, verification only) are environment controls.
- **Harnesses:** `tools/bench_ext_batch.c` (one batch per pass), `tools/dump_outputs_batch.c`, `tools/fuzz_pipeline_batch.c` (same inputs and output lines as fuzz_pipeline).

## Correctness: exact in every mode
`raw/verify_batch_itl3_{A,B,C,D}` covers golden-v1, novel, OSM, unseen v4, both frozen references and fuzz seeds 1 and 2 (2M each), all through the batch API. **All identical** in:
- A: no switching;
- B: 4 in flight, all yield points;
- C: 4 in flight, random switching at half the yield points (stress);
- D: 2 in flight, all points.
Two failed attempts are kept:
- 1 MB stacks (`raw/attempt1_stack_1mb`; corpora identical, fuzz harness crashed);
- the cause was a buffer overflow in the first fuzz batch harness (fixed; crash report traced to `fz_add`).

## Speed
Machine in interactive use, load average 4–5.
Screen 1 (`raw/screen_analyze2.stdout`): cpi vs ovh (batch path, no switching) vs i4 (4 in flight, all yields):
        v1 median cpi 5.414 ovh 5.402; ovh speedup per round [0.9815, 0.9993, 1.1435, 0.9802, 1.0058, 1.0357] median 1.0026; rounds faster 3/6
        v1 min    cpi 5.314 ovh 5.293; ovh speedup per round [0.9765, 0.997, 1.0466, 0.985, 1.0116, 1.0216] median 1.0043; rounds faster 3/6
        osm median cpi 3.833 ovh 3.73; ovh speedup per round [1.0263, 0.9342, 1.1059, 1.1702, 0.7689, 1.1343] median 1.0661; rounds faster 4/6
        osm min    cpi 3.534 ovh 3.487; ovh speedup per round [1.0, 0.9893, 1.0191, 1.0222, 0.7811, 1.1441] median 1.0095; rounds faster 3/6
        v1 median cpi 5.414 i4 5.768; i4 speedup per round [0.8947, 0.8967, 1.0605, 0.9419, 0.9377, 0.9651] median 0.9398; rounds faster 1/6
        v1 min    cpi 5.314 i4 5.695; i4 speedup per round [0.8966, 0.909, 0.9695, 0.9317, 0.9386, 0.9344] median 0.933; rounds faster 0/6
        osm median cpi 3.833 i4 4.168; i4 speedup per round [0.8923, 0.8461, 0.982, 1.1017, 0.7976, 0.941] median 0.9166; rounds faster 1/6
        osm min    cpi 3.534 i4 3.901; i4 speedup per round [0.8764, 0.8762, 0.9178, 0.9522, 0.8594, 0.9437] median 0.8971; rounds faster 0/6
Screen 2, diagnostic (`raw/screen2_analyze2.stdout`): i4m4 (4 in flight, yield 2 only), i2m7 (2 in flight, all yields):
        v1 median cpi 5.479 i4m4 5.679; i4m4 speedup per round [1.017, 0.9874, 0.9663, 0.9774, 0.9353, 0.9308] median 0.9719; rounds faster 1/6
        v1 min    cpi 5.27 i4m4 5.561; i4m4 speedup per round [0.9791, 0.9594, 0.9447, 0.9637, 0.9356, 0.9362] median 0.9521; rounds faster 0/6
        osm median cpi 3.518 i4m4 3.876; i4m4 speedup per round [0.9036, 0.8844, 0.9201, 0.9187, 0.9435, 0.8743] median 0.9111; rounds faster 0/6
        osm min    cpi 3.417 i4m4 3.704; i4m4 speedup per round [0.9232, 0.9061, 0.917, 0.934, 0.9459, 0.8955] median 0.9201; rounds faster 0/6
        v1 median cpi 5.479 i2m7 5.665; i2m7 speedup per round [1.0604, 0.9874, 0.9503, 0.957, 0.9488, 0.9475] median 0.9536; rounds faster 1/6
        v1 min    cpi 5.27 i2m7 5.533; i2m7 speedup per round [1.0009, 0.9681, 0.9328, 0.9552, 0.9349, 0.949] median 0.9521; rounds faster 1/6
        osm median cpi 3.518 i2m7 3.768; i2m7 speedup per round [0.962, 0.9187, 0.9511, 0.932, 0.918, 0.8835] median 0.9254; rounds faster 0/6
        osm min    cpi 3.417 i2m7 3.655; i2m7 speedup per round [0.9495, 0.9225, 0.9382, 0.9386, 0.9285, 0.8999] median 0.9334; rounds faster 0/6

## Decision
- **Phase 2 gate passed:** batch-path overhead ≈ 0.
- **Phase 3 gate failed:** every interleaved configuration is slower, by 3–7% on golden-v1 and 7–10% on OSM, in nearly every round. Even one switch per address costs more than it hides.
Interpretation (consistent with 137/138; mechanism not separately measured):
- Interleaving makes 2–4 addresses' scratch state and working sets compete for the L1 cache, and each resumed address finds its own lines partly evicted.
- The memory waits it could overlap are fewer than the cold-vs-warm gap suggested, because much of that gap is many small latencies the core already overlaps on its own.
- Phase 0's ideal chain showed the hardware can overlap waits; the parser's access pattern does not leave enough independent long waits for that to pay.
The plan's remaining phases (4–6) are not run. cpi stays the deliverable.
