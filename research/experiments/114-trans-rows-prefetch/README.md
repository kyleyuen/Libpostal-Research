# Experiment 114 — prefetch every line of transition-weight rows only (EXP_ROW_TRANS_ONLY, 128-byte steps)

Date/time/timezone: created 2026-10-04T00:42:52-0700 (`raw/created.txt`); screen 2026-10-04T00:47:43-0700 → 2026-10-04T00:55:12-0700; America/Los_Angeles; commands in `raw/commands.txt`. Status: **accepted** (unseen golden-v9 in 115: not slower)

Basis:
- 112 attributes the largest first-touch cost to weight-row accumulation.
- 113 (all rows, every line) gained on golden-v1 but lost 2–3% on OSM.
- 085 screen 2 showed the state-row extra prefetches cost on OSM-style data (rs8 −1.3%) while transition rows carried the golden-v1 gain.
Change (`raw/crf.diff`, hints only): state rows keep psf's single first-line prefetch; transition rows get every 128-byte line, up to 8 (`rt8`) or 4 (`rt4`) per row. Both caps were chosen before measuring. Switches `raw/switches_rt8.txt` = psf + `-DEXP_ROW_LINES=8 -DEXP_ROW_STEP=128 -DEXP_ROW_TRANS_ONLY`.
A first attempt ran `build_cc.sh` before it was executable (exit 126), so the screen ran without binaries. That attempt is kept in `raw/attempt1_not_executable/`.
Outputs: golden-v1 identical for rt8 and rt4 (sha 45574ee2…). Unseen v9 identical (115). Full verify: see 116.
Screen vs psf (6 rounds, 20 passes; load logged per run in `raw/screen/load.log`; `raw/screen_analyze2.stdout`):
        v1 median psf 5.536 rt8 5.365; rt8 speedup per round [1.0265, 1.0411, 1.0287, 1.0201, 1.0282, 1.0474] median 1.0284; rounds faster 6/6
        v1 min    psf 5.431 rt8 5.269; rt8 speedup per round [1.0293, 1.0427, 1.0294, 1.0214, 1.022, 1.0352] median 1.0293; rounds faster 6/6
        osm median psf 3.571 rt8 3.547; rt8 speedup per round [1.0034, 1.007, 1.0076, 1.0203, 1.0048, 1.0006] median 1.0059; rounds faster 6/6
        osm min    psf 3.494 rt8 3.473; rt8 speedup per round [1.0157, 1.0003, 1.0176, 1.0151, 0.9749, 0.9896] median 1.0077; rounds faster 4/6
        v1 median psf 5.536 rt4 5.406; rt4 speedup per round [1.0276, 1.0357, 1.0151, 1.0148, 1.02, 1.0271] median 1.0235; rounds faster 6/6
        v1 min    psf 5.431 rt4 5.325; rt4 speedup per round [1.0185, 1.0301, 1.0232, 1.0073, 1.0166, 1.0195] median 1.019; rounds faster 6/6
        osm median psf 3.571 rt4 3.559; rt4 speedup per round [0.9933, 0.983, 1.0042, 1.0172, 1.0059, 1.0028] median 1.0035; rounds faster 4/6
        osm min    psf 3.494 rt4 3.48; rt4 speedup per round [0.9986, 0.9901, 1.0173, 1.0023, 0.9913, 0.9911] median 0.995; rounds faster 2/6
Stage timers (`raw/split_analyze.stdout`, 4 alternating runs, psf vs rt8 stats builds):
    dev
       total_parse        psf 5.963/2.558  rt8 5.812/2.580  Δcold -0.151 Δwarm +0.023
       context_fill       psf 1.768/0.378  rt8 1.756/0.380  Δcold -0.012 Δwarm +0.002
       features           psf 1.383/0.740  rt8 1.379/0.740  Δcold -0.004 Δwarm +0.000
       lookups            psf 0.814/0.251  rt8 0.844/0.259  Δcold +0.030 Δwarm +0.008
       accumulate         psf 0.998/0.462  rt8 0.824/0.469  Δcold -0.174 Δwarm +0.007
       viterbi_and_rest   psf 0.471/0.427  rt8 0.478/0.430  Δcold +0.007 Δwarm +0.003
    osm
       total_parse        psf 3.918/2.252  rt8 3.968/2.269  Δcold +0.050 Δwarm +0.017
       context_fill       psf 1.007/0.344  rt8 1.034/0.345  Δcold +0.026 Δwarm +0.001
       features           psf 0.927/0.613  rt8 0.939/0.614  Δcold +0.012 Δwarm +0.001
       lookups            psf 0.491/0.210  rt8 0.524/0.217  Δcold +0.033 Δwarm +0.007
       accumulate         psf 0.645/0.434  rt8 0.618/0.442  Δcold -0.027 Δwarm +0.008
       viterbi_and_rest   psf 0.419/0.386  rt8 0.422/0.386  Δcold +0.003 Δwarm +0.000
Mechanism:
- **Development:** accumulation is 0.174 µs faster cold; the prefetch issue adds 0.03 µs to lookups; net −0.15 µs cold.
- **OSM:** the two roughly cancel (−0.027 / +0.033).
- **Overall:** a real gain on longer, Senzing-derived addresses and about neutral on OSM-style ones.
