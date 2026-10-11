# Kill rule — pre-registered 2026-10-05T17:16:01-0700, before any build (copied verbatim from plans/REDESIGN_PLAN_2026-10-05.md, phase R0)

### R0 — repair (independent, small)

Fix cpi's `libpostal_parser_print_features` abort (`final/119-cpi/sources/crf.c:793`). Add `libpostal_setup_parser_datadir` to the API probe.

**Gate.** Outputs identical on all sets, timing unchanged within noise, and both APIs behave like the original.

