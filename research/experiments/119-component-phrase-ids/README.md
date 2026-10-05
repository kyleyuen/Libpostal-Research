# Experiment 119 — component-phrase features as precomputed feature ids (EXP_CP_IDS; first piece of idea F)

Date/time/timezone: created 2026-10-04T01:21:21-0700 (`raw/created.txt`); screen 2026-10-04T01:24:25-0700 → 2026-10-04T01:29:52-0700; America/Los_Angeles; commands in `raw/commands.txt`. Status: **accepted** (unseen golden-v10 in 120: +4.7%, every round)

## Change
Based on 117 and 118; diffs in `raw/address_parser.diff` and `raw/crf.diff`.
- **Refactor.** The component-phrase block of `address_parser_features` (`phrase`, the per-type phrase features, `commonly …`) is moved, unchanged, into `ap_cp_emit`.
- **Load.** The component trie is enumerated depth-first (4,012,750 keys). For each key K with data id d, `ap_cp_emit(K, phrase_types[d])` runs into a scratch array, and each produced key is looked up in the state dictionary with `fd_lookup` (text form, which 059 showed equals the name-byte lookup). The ids found are stored in emission order. Keys the dictionary lacks are left out, because they add nothing to any score.
  - Table: 4,012,750 entries, 6,519,618 ids, 47.8 MB, built in 2.2 s.
  - Model data only; no benchmark data involved.
- **Parse.** When the runtime phrase string's length equals strlen(K) (both ≤ 255), the stored ids are pushed as already-resolved integer keys (015/023 mechanism) instead of building, hashing and looking up the strings.
  - Why the length guard is enough: the trie match consumes the token bytes in order with "" or " " between tokens, so equal lengths mean equal strings (118).
  - Every other case runs `ap_cp_emit` as before.
  - `add_word_feature` is cleared exactly as before.
- **Header issue.** The first build failed because `address_parser.c` sees the original `crf.h`; an accessor was added in `crf.c`. The failed attempt is kept in `raw/attempt1_crf_h/`.
Switches: `raw/switches_cpi.txt` = rt8 + `-DEXP_CP_IDS`.

## Correctness
`cpv` (+ `EXP_CP_IDS_VERIFY`: every enumerated key is checked with `trie_get_data`; every table use regenerates the strings, looks them up and compares the ids in order) and `cpi`: all identical on golden-v1, novel, OSM, unseen v4, frozen references and fuzz seeds 1 and 2. The verify build checked about 12.8M table uses (27,542 / 31,345 / 38,451 / 40,132 on the corpora; 6,342,947 + 6,349,866 on fuzz) with 0 mismatches (`raw/verify_*`).

## Speed
Screen vs rt8 (6 rounds; `raw/screen_analyze2.stdout`):
        v1 median rt8 5.399 cpi 5.249; cpi speedup per round [1.0322, 1.0257, 1.0242, 1.0388, 1.0263, 1.0285] median 1.0274; rounds faster 6/6
        v1 min    rt8 5.319 cpi 5.169; cpi speedup per round [1.038, 1.0197, 1.0316, 1.0314, 1.0299, 1.0216] median 1.0307; rounds faster 6/6
        osm median rt8 3.576 cpi 3.372; cpi speedup per round [1.0426, 1.0549, 1.07, 1.0498, 1.0564, 1.0716] median 1.0556; rounds faster 6/6
        osm min    rt8 3.468 cpi 3.29; cpi speedup per round [1.0466, 1.0351, 1.0718, 1.0589, 1.0577, 1.031] median 1.0522; rounds faster 6/6
Stage timers (`raw/split_analyze.stdout`, 4 alternating runs):
    dev
       total_parse        rt8 5.842/2.582  cpi 5.636/2.471  Δcold -0.206 Δwarm -0.111
       context_fill       rt8 1.782/0.380  cpi 1.755/0.377  Δcold -0.027 Δwarm -0.003
       features           rt8 1.385/0.742  cpi 1.397/0.688  Δcold +0.011 Δwarm -0.054
       lookups            rt8 0.845/0.259  cpi 0.677/0.202  Δcold -0.167 Δwarm -0.057
       accumulate         rt8 0.825/0.469  cpi 0.818/0.472  Δcold -0.007 Δwarm +0.003
       viterbi_and_rest   rt8 0.480/0.429  cpi 0.475/0.430  Δcold -0.005 Δwarm +0.001
    osm
       total_parse        rt8 3.940/2.275  cpi 3.837/2.176  Δcold -0.103 Δwarm -0.099
       context_fill       rt8 1.016/0.347  cpi 1.041/0.347  Δcold +0.024 Δwarm -0.000
       features           rt8 0.933/0.615  cpi 0.901/0.571  Δcold -0.032 Δwarm -0.043
       lookups            rt8 0.520/0.217  cpi 0.428/0.166  Δcold -0.092 Δwarm -0.051
       accumulate         rt8 0.617/0.442  cpi 0.623/0.442  Δcold +0.005 Δwarm +0.000
       viterbi_and_rest   rt8 0.423/0.387  cpi 0.422/0.386  Δcold -0.002 Δwarm -0.001
Lookups are 0.17 µs (dev) / 0.09 µs (OSM) faster cold and about 0.05 µs faster warm; feature generation is about 0.05 µs faster warm. Both are compute and cache savings and hold on OSM-style data.
Memory: bench peak RSS unchanged (2,543,124,480 B); the table fits under the load-time peak. The validation (121) records the footprint.
