# Experiment 178 — original ordered event scoring

Date/time/timezone: 2026-10-05T23:41:50-0700 PDT, pre-registered before candidate source editing.
Status: **feasible vertical slice; not a speed result**.

## Hypothesis and change

Starting from experiment 142's preserved-original-source derivative, retain the trained model, original trie lookup, feature construction, score addition order and Viterbi implementation. The smallest vertical slice will copy each already resolved ordered state or transition event into reusable per-address event scratch, then consume that stream with the original CSR weights. Allocation failure uses the original loop for that address. This is neither cpi nor 160's compiled-record representation: no dictionary replacement, model re-encoding, integer key routing, phrase search replacement or changed arithmetic.

Base commit: `181737d30977adc9d287e3f7a1cfafd7277525d4`. Source base: 142 oracle source (`raw/base_hashes.txt`), which itself derives from original libpostal commit `25099c50`; candidate patch and complete source copy remain in this directory.

## Gates and planned evidence

Compare O3i and O4 to immutable 142 original oracles; compare final parser bytes on development, golden-v1, novel, OSM v3 and a bounded fuzz sample where feasible. Verify one thread by source scan and `ps -M`; collect memory only as a diagnostic. Model-loading commands use `research/tools/model_slot.sh`. No benchmark timing or speed claim is authorized.

## All measured trials

No speed trial was run or interpreted.

- A candidate `dump_outputs` process had one `ps -M` row (`raw/thread_memory.psM.txt`). Static source scan is retained in `raw/thread_static_scan.txt`.
- A single `/usr/bin/time -l` golden-v1 run reports maximum RSS 2,081,898,496 bytes. A live `footprint -p` snapshot reports 72 MB current footprint. These two measurements use different operating-system metrics and moments; they are diagnostic only and do not establish a memory claim.

## Correctness and interpretation

- **Original event oracle:** O3i exactly matches on golden-v1 (702,370 records), novel (713,995), OSM v3 (546,890), and fuzz seed 1 first 200,000 inputs (14,030,294). O3i includes misses and duplicates.
- **Score oracle:** O4 exactly matches on golden-v1 (151,312 rows), novel (147,498), OSM v3 (122,314), and fuzz (2,723,644), showing the original f64 additions occurred in the same order.
- **Final outputs:** candidate bytes match 142's original controls on golden-v1, novel and OSM v3. The candidate's first 200,000 fuzz output lines match the original 142 run prefix.
- The initial run accidentally set `EVENT_ORACLE_DIR` instead of the inherited original hook's `ORACLE_DIR`; it exited 0 but produced no dumps and is retained as `commands.tsv` plus failed-comparator files. The corrected run is `commands_retry.tsv` and `oracle_retry_summary.txt`.
- The first build failed because C preprocessor macro arguments split a compound-literal initializer; its logs are retained in `build.*`. The retry replaces that initializer with field assignments and exits 0.

## Decision

The vertical slice is feasible: an original-source parser can represent resolved ordered events explicitly and consume them through the original CSR scorer with identical O3i, O4 and tested output. The remaining blocker to a distinct architecture is not semantics at this seam: this slice still constructs string features and performs the original trie lookups before event emission. A subsequent architecture must move event production upstream while retaining these exact oracle checks and an original fallback. This worker owns only this experiment directory; shared ledger, activity log and professor report are out of scope.
