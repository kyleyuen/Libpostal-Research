# 158 timing request (for the lead; teammates do not time)

Gate decided by this request: R0 kill rule, "timing unchanged within noise" (KILL_RULE.md). Outputs and API parity are already decided (README).

Why a measurement is needed: the change is out of line (`crf_tagger_score_print`, `noinline, cold`), but `crf_tagger_score` is inlined into
`_address_parser_parse`, and the new cold call site changed register allocation/spills there: 344 of ~3,317 normalized disassembly lines differ,
+12 instructions (`raw/disasm/address_parser_parse.diff`). So "unchanged" is not structurally guaranteed. No timing prediction is made.

## Artifacts (SHA-256)

| Variant | bench_ext | libpostal.a |
|---|---|---|
| original | `research/experiments/015-integer-keys/artifacts/bench_ext_baseline` c2ce250646155d6eda91337cac9b9ddab11f3605ff56b70ba59fc1529932c77e | 000-baseline 30de6ef223346735ac3035654ff0d435e5e621dccc94dc77869807dbf146fe22 |
| cpi (119 deliverable) | `research/experiments/119-component-phrase-ids/artifacts/cpi/bench_ext` c5612825963a18be1f4e7869da2bb2dc6fedf3f098189d195bad765247b43c5a | e56d3ae4799966543a5841ebce89f26fb5eeed396d862f6b31e4076354675a4e |
| 158ctl (unchanged-source rebuild) | `research/experiments/158-r0-feature-print/artifacts/158ctl/bench_ext` 105d614a718229f78b8bcb1697ac3c80acd16f853487b97f6c42543a059877a4 | 9cafd120bf5a5dda60739d480debc0a1139fd8dbb17ec6f56203d6bf1aee3ce2 |
| 158fp (candidate) | `research/experiments/158-r0-feature-print/artifacts/158fp/bench_ext` c93d9fa3c051e217aeabae609c3ce34216d60cf21599d6ff29e338d7f1d0dcfa | dbefe56992b517c155127f8e67f222191ee184367383a1a010cdd428c6858ef3 |

Harness source `research/tools/bench_ext.c` 0c4988a2fc1aa00d511a39866b187ce3e8630fe210aad05f2de9eaeb2fe4bf0f; gate `research/tools/env_gate.sh`
6698fc1358c5d4720ceb491a8f04cec06fdddbe46cd2be34f0276a4204a20c34. 158ctl is included so a rebuild effect (new PGO profile / paths) can be
separated from the change: the decisive comparison is **158fp vs 158ctl**; 158fp vs cpi is reported alongside.

## Commands (serial, AC power, `caffeinate -i`, `research/tools/env_gate.sh` must pass before every process)

```sh
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; X="$W/research/experiments"; OUT="$X/158-r0-feature-print/raw/timing"
caffeinate -i "$W/research/tools/run_ext.sh" "$OUT" 5 \
  original="$X/015-integer-keys/artifacts/bench_ext_baseline" \
  cpi="$X/119-component-phrase-ids/artifacts/cpi/bench_ext" \
  158ctl="$X/158-r0-feature-print/artifacts/158ctl/bench_ext" \
  158fp="$X/158-r0-feature-print/artifacts/158fp/bench_ext"
```
(`run_ext.sh`: golden-v1 all and novel, rotating variant order, 5 passes/process. Run `env_gate.sh` before each process per METHOD; if
`run_ext.sh` does not call it, wrap it or use the suite runner with these four variants.) Optional OSM screening, same rotation:
`<bench_ext> "$W/profiling/golden-v3-osm/osm.tsv" 20`.

Footprint (each variant, serially, gated):
```sh
/usr/bin/time -l <bench_ext> "$W/profiling/golden-v2-novel/v1-all.tsv" 5 > <out>.stdout 2> <out>.time
```
Report peak RSS and peak memory footprint, warm median, P10, cold pass 1, setup seconds (`research/tools/analyze_ext.py`).

## Decision rule (R0)

The timing gate passes if the 158fp vs 158ctl paired warm-median ratio lies within the session's noise band (judge by the 158ctl vs cpi
spread in the same session), the cold pass likewise, and peak footprint is not above 158ctl beyond allocator noise. A material slowdown
fails the gate as written; the fallback is to keep cpi and record 158fp as a correctness-maintenance variant (as 025 did).
