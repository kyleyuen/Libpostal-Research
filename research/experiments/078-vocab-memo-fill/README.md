# Experiment 078 — eager per-address vocabulary memo fill (EXP_VM_FILL) and reuse of the slot loop's lookups (EXP_VM_REUSE)

Date/time/timezone: created 2026-10-03T02:41:45-0700, America/Los_Angeles. Status: **rejected (slower)**

Change (`sources/address_parser.c`): at token 0, fill every token's vocabulary memo entry in one loop instead of at first use (next-word section, 076); with REUSE, a token whose admin string has identical bytes takes the context-fill slot loop's result. `EXP_VM_REUSE_VERIFY` (`vmv`): 50,923 / 50,136 / 40,416 / 35,553 reused entries on golden-v1 / novel / OSM / unseen v4 and 8,888,627 over 2M fuzz inputs, 0 mismatches; fuzz output identical. vmf/vmr golden-v1 sha identical.
Screen (`raw/screen_analyze.stdout`): vmf −0.5% / −1.0%, vmr −1.0% / −1.3% vs pf2 → rejected (the lazy lookups were already overlapped with surrounding feature work; the eager loop adds a serial phase).
