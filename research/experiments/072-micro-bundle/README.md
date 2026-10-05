# Experiment 072 — digit-only token fast path (EXP_DIGITS_FAST) and conditional backpointer clear (EXP_BACK_NOCLEAR)

Date/time/timezone: created 2026-10-03T01:40:01-0700, America/Los_Angeles. Status: **dg accepted (small); BACK_NOCLEAR rejected (no added gain)**

Base: 067 `pl` sources and switches (`raw/base_sources_pl.sha256`). Changes (`sources/`):
- `EXP_DIGITS_FAST` (`normalize.c` `add_normalized_token`): a token made only of ASCII 0–9 takes none of the hyphen / full-stop / apostrophe / possessive / alpha-numeric-split branches (each needs a non-digit), so each byte is appended as `DIGIT_CHAR` when `NORMALIZE_TOKEN_REPLACE_DIGITS` is set, else unchanged. Derived from the code, not from data.
- `EXP_BACK_NOCLEAR` (`crf.c`): skip clearing backward edges when every state, state-transition and transition weight is finite and |w| ≤ 1e100 (checked once at load); Viterbi then writes every cell it later reads.
Builds: `mb` = pl + both, `dg` = pl + digits only (same PGO/LTO/mimalloc; `raw/build_*`).

Outputs (`tools/verify.sh`, `raw/verify_{mb,dg}/summary.txt`): both identical on golden-v1 (sha 45574ee2…), novel (806b8a81…), OSM, unseen golden-v4 (vs the original's dump from 069), frozen references, fuzz seeds 1 and 2 (2M each).

Screen (sequential, 6 rotating rounds, 20 passes; `raw/screen_analyze.stdout`):
    v1 pl [6.825, 6.837, 6.8, 6.836, 6.846, 6.814] median 6.831
    v1 mb [6.822, 6.805, 6.795, 6.832, 6.811, 6.814] median 6.812
    v1 dg [6.794, 6.827, 6.78, 6.807, 6.799, 6.796] median 6.798
    v1 mb speedup vs pl per round [1.0004, 1.0047, 1.0007, 1.0006, 1.0051, 1.0] median 1.0007
    v1 dg speedup vs pl per round [1.0046, 1.0015, 1.0029, 1.0043, 1.0069, 1.0026] median 1.0036
    osm pl [4.374, 4.333, 4.325, 4.33, 4.322, 4.302] median 4.328
    osm mb [4.32, 4.319, 4.311, 4.311, 4.317, 4.32] median 4.318
    osm dg [4.305, 4.328, 4.307, 4.284, 4.287, 4.308] median 4.306
    osm mb speedup vs pl per round [1.0125, 1.0032, 1.0032, 1.0044, 1.0012, 0.9958] median 1.0032
    osm dg speedup vs pl per round [1.016, 1.0012, 1.0042, 1.0107, 1.0082, 0.9986] median 1.0062

Decision: `dg` +0.4% golden-v1 (all 6 rounds faster), +0.6% OSM (5/6) → new stack head `dg` (= lf + 067 + 072 digits). The backpointer change adds nothing over `dg` → rejected. Unseen re-measure of `dg` to follow, as the user requires.

Addendum (073, 2026-10-03): stage timers on identical stats builds show context fill −0.009 µs and total −0.004 µs cold for dg vs pl, far below the 0.03 µs end-to-end difference; the +0.4% is therefore within build-to-build layout noise. dg is exact and kept in the stack, but is reported as neutral, not as a measured gain.
