# 161 timing request (for the lead; teammates do not time)

Decides kill-rule item 4: "Timing is unchanged within noise against 158fp: same session." No other item depends on timing.

Structural evidence (not a measurement): the fix is teardown-only. Per-function normalized disassembly of `dump_outputs`, 161fx vs 158fp
(`raw/disasm/differing_functions.txt`, `function_diffs.txt`):
- `_address_parser_parse`, `_address_parser_features` and `_is_valid_dictionary_phrase` differ only in `mov w8, #<__LINE__>` immediates of
  error-log calls (+4, from the forward declaration added near the top of address_parser.c). The instruction sequences are otherwise identical.
- All other differences are in setup or teardown functions: `_address_parser_destroy`, `_address_parser_load` (`__LINE__` only), `_cp_build`,
  `_crf_destroy`, `_crf_int_keys_enable`, `_libpostal_teardown`, plus the three new functions.
- Code and data addresses shift, so alignment and layout effects are possible. No prediction is made.

## Artifacts (SHA-256)

| Variant | bench_ext | libpostal.a |
|---|---|---|
| 158fp (control, 158 candidate) | `research/experiments/158-r0-feature-print/artifacts/158fp/bench_ext` c93d9fa3c051e217aeabae609c3ce34216d60cf21599d6ff29e338d7f1d0dcfa | dbefe56992b517c155127f8e67f222191ee184367383a1a010cdd428c6858ef3 |
| 161fx (candidate) | `research/experiments/161-resetup-defect/artifacts/161fx/bench_ext` 7c4771d7f7352c9e2c2f727ff6e39f0f00f879ea2a8f7be7756cddfc8fb87dc1 | f5fb2fcd552150dc99b7d2578fae2d602a2567199ecfadc36eb98db21aa28799 |

Both builds use the same recipe: `build_cc.sh`, the 119 switches, PGO on golden-v1 development, clang -O2 -flto, and mimalloc. If you run 158 in
the same session, add original, cpi and 158ctl as in 158's TIMING_REQUEST.md, so both gates come from one session.

## Commands (serial, AC power, `caffeinate -i`, `research/tools/env_gate.sh` before every process)

```sh
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; OUT="$X/161-resetup-defect/raw/timing"
caffeinate -i "$W/research/tools/run_ext.sh" "$OUT" 5 \
  158fp="$X/158-r0-feature-print/artifacts/158fp/bench_ext" \
  161fx="$X/161-resetup-defect/artifacts/161fx/bench_ext"
```
Optionally add original (`$X/015-integer-keys/artifacts/bench_ext_baseline`) for a speedup figure. Footprint, each variant serially and gated:
`/usr/bin/time -l <bench_ext> "$W/profiling/golden-v2-novel/v1-all.tsv" 5`. Report warm median, P10, cold pass 1, setup seconds and
peak footprint (`research/tools/analyze_ext.py`).

## Decision rule

Item 4 passes if the paired 161fx/158fp warm-median ratio and the cold pass are within the session's noise, and peak footprint is not above 158fp
beyond allocator noise. Setup seconds should be unchanged: setup code differs only in teardown resets and the `cp_build` failure path.
