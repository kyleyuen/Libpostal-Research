# Experiment 059 — feature keys carry a 1-byte name id instead of the name text (on 056 `ftr`)

Date/time/timezone: created 2026-10-02T19:48:15-0700; screen 2026-10-02T19:50:20-0700 to 2026-10-02T19:53:50-0700, America/Los_Angeles.
Status: **accepted into the stack** (`nb`); full validation pending

## Change (switch `EXP_NAME_BYTE`; `features.c`, new `fa_nb.h`, `address_parser.c` macro, `crf.c`)
Feature keys for the scoring pass were written as `name|value`; the lookup then split at '|' (memchr), hashed the name and probed the dictionary's name table to get a 1-byte name id. Now, when the feature name is a string literal known to the array's dictionary, the key is written as [0x01][name id | 0x80 if value parts follow][value parts, joined and stripped exactly as before]; the lookup takes the id directly. The name id per call site and dictionary is a model constant cached in a static at that call site (keyed by the dictionary pointer). Unknown names, non-literal names and other arrays keep the text form; the arrays are reset after the scoring pass so no other path sees the marker. 0x01 cannot begin a text key, and the dictionary's own split of the text form yields the same (id, flag, rest), so every lookup returns the same feature id.

## Checks
Build `nb` (`l3`-procedure PGO/LTO/mimalloc; switches `raw/switches.txt`): output byte-identical to the original on golden-v1, novel, OSM and 2 × 2,000,000 fuzzed inputs (`raw/verify_summary.txt`).

## Screen (sequential, alternating, golden-v1 and OSM, 20 passes, 4 rounds; `raw/screen_analyze.stdout`)
    v1 ftr [7.306, 7.318, 7.338, 7.299] median 7.312
    v1 nb [7.079, 7.091, 7.114, 7.099] median 7.095
    v1 nb vs ftr per round [1.0321, 1.032, 1.0315, 1.0282]
    osm ftr [4.719, 4.724, 4.704, 4.723] median 4.721
    osm nb [4.559, 4.551, 4.533, 4.541] median 4.546
    osm nb vs ftr per round [1.0351, 1.038, 1.0377, 1.0401]

+2.8–3.2% golden-v1 and +3.5–4.0% OSM over `ftr` in every round.
