# Experiment 091 — Viterbi forward pass without index tracking (EXP_VITERBI_MAXONLY, EXP_VITERBI_FMAX)

Date/time/timezone: created 2026-10-03T10:07:03-0700, America/Los_Angeles. Status: **vmx accepted (unseen golden-v7: +3.3% over pcp, 093)**

## Change (`sources/crf_context.c`, patch `tools/patch_maxonly.py` + FMAX block)
For L = 20 (the model) the NEON forward pass keeps only the running maxima — vmo: the same compare/select as before (identical values), vmx: the hardware maximum (FMAX) — and stores each step's maximum before the state score is added. Backtracking recovers, for the one label on the path at each step, the argmax as the first i whose score, recomputed with the identical expression (prev[i] + st[t][i][j]) + trans[i][j], equals that maximum; 0 if the maximum is still −DBL_MAX (the original then left the zeroed backward edge). Exactness: the original keeps the first strictly greater score, so its argmax is the first index attaining the final maximum. FMAX may return +0 where the scan keeps −0 (max of zeros); zeros of either sign compare equal and give identical sums with non-zero values, so no comparison and no label changes. Per cell and step this removes one select (vmo) and the compare (vmx).

## Checks
`EXP_VITERBI_MAXONLY_VERIFY` builds (abort if no index matches): vmov (compare/select) and vmxv (FMAX) — identical on golden-v1 (45574ee2…), novel (806b8a81…), OSM, unseen v4, frozen references, fuzz seeds 1 and 2 (`raw/verify_vmov/`, `raw/verify_vmxv/`); no abort.

## Results
Screen 1 vmo vs pcp (6 rounds; `raw/screen1_analyze.stdout`):
    v1 pcp [6.488, 6.487, 6.483, 6.502, 6.48, 6.492] median 6.488
    v1 vmo [6.459, 6.428, 6.453, 6.43, 6.492, 6.424] median 6.441
    v1 vmo speedup vs pcp per round [1.0045, 1.0092, 1.0046, 1.0112, 0.9982, 1.0106] median 1.0069
    osm pcp [4.17, 4.168, 4.176, 4.17, 4.182, 4.163] median 4.17
    osm vmo [4.122, 4.125, 4.115, 4.109, 4.096, 4.132] median 4.119
    osm vmo speedup vs pcp per round [1.0116, 1.0104, 1.0148, 1.0148, 1.021, 1.0075] median 1.0132
Mechanism (090 timers on pcp vs vmo, 3 alternating runs; `raw/split_analyze.stdout`):
    dev
      total_parse        pcp 6.840/3.072  vmo 6.803/3.021  Δcold -0.037
      vt_score_setup     pcp 0.196/0.164  vmo 0.195/0.165  Δcold -0.001
      vt_viterbi         pcp 0.387/0.388  vmo 0.339/0.330  Δcold -0.048
      viterbi_and_rest   pcp 0.662/0.619  vmo 0.602/0.549  Δcold -0.060
      features           pcp 1.552/0.812  vmo 1.556/0.815  Δcold +0.004
      accumulate         pcp 1.097/0.504  vmo 1.105/0.526  Δcold +0.008
    osm
      total_parse        pcp 4.539/2.651  vmo 4.506/2.607  Δcold -0.033
      vt_score_setup     pcp 0.170/0.143  vmo 0.169/0.143  Δcold -0.001
      vt_viterbi         pcp 0.348/0.348  vmo 0.301/0.296  Δcold -0.047
      viterbi_and_rest   pcp 0.589/0.555  vmo 0.524/0.486  Δcold -0.065
      features           pcp 1.024/0.657  vmo 1.030/0.659  Δcold +0.006
      accumulate         pcp 0.710/0.464  vmo 0.726/0.486  Δcold +0.016
Screen 2 vmx vs vmo (`raw/screen2_analyze.stdout`):
    v1 vmo [6.465, 6.432, 6.43, 6.436, 6.448, 6.445] median 6.441
    v1 vmx [6.387, 6.363, 6.396, 6.349, 6.348, 6.35] median 6.357
    v1 vmx speedup vs vmo per round [1.0122, 1.0108, 1.0053, 1.0137, 1.0158, 1.015] median 1.013
    osm vmo [4.126, 4.108, 4.125, 4.117, 4.1, 4.111] median 4.114
    osm vmx [4.068, 4.057, 4.053, 4.056, 4.032, 4.046] median 4.054
    osm vmx speedup vs vmo per round [1.0143, 1.0126, 1.0178, 1.015, 1.0169, 1.0161] median 1.0156
vmx vs pcp ≈ +2.0% golden-v1, +2.9% OSM (product of the two screens); larger on OSM, where compute is a larger share.
