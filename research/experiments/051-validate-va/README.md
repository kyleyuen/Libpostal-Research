# Experiment 051 — full same-session validation of 048 `va`

Date/time/timezone: created 2026-10-02T17:44:59-0700; timing 2026-10-02T17:45:42-0700 to 2026-10-02T17:57:05-0700 (host clock), America/Los_Angeles.
Status: **accepted — `va` becomes the deliverable** (`final/048-va/`)

`va` = `l3` sources and switches + `EXP_VITERBI20` + `EXP_TRANS_ALIAS` (048), PGO trained on golden-v1 development (031 showed the training set does not bias speed), LTO, mimalloc. Binary/library/source hashes `raw/va_hashes.txt`.

## Correctness (`raw/verify_summary.txt`, run in parallel before timing)
Byte-identical to the original on golden-v1 (sha 45574ee2…), novel (806b8a81…) and OSM (vs 017's original dump); both frozen reference files identical; 2 × 2,000,000 fuzzed inputs identical to 011's original outputs. One thread at 3–18 s (`raw/threads.txt`).

## Speed (sequential per 034, 5 rotating rounds, 20 passes, median of warm passes 2–20, paired with the original each round; 45 runs exit 0; `raw/analyze.stdout`)
    | binary (PGO training) | v1 µs | v1 speedup (range) | nv µs | nv speedup (range) | osm µs | osm speedup (range) |
    |---|---:|---:|---:|---:|---:|---:|
    | original | 33.716 | 1.000× (1.00–1.00) | 34.104 | 1.000× (1.00–1.00) | 21.271 | 1.000× (1.00–1.00) |
    | l3 (deliverable) | 7.820 | 4.321× (4.30–4.35) | 7.916 | 4.316× (4.31–4.52) | 4.957 | 4.291× (4.27–4.56) |
    | va = l3 + Viterbi-20 + trans alias | 7.447 | 4.534× (4.51–4.58) | 7.512 | 4.545× (4.52–4.91) | 4.622 | 4.617× (4.57–4.71) |
    
    peak footprint bytes (max over rounds): orig 2613496016/2613856488/2612791528; l3 2380351744/2380335336/2370291920; va 2380335336/2380335336/2370291920

## Memory
Peak footprint `va` 2,380.3 / 2,380.3 / 2,370.3 MB vs original 2,613.5 / 2,613.9 / 2,612.8 MB. Steady footprint (`raw/steady/steady.txt`): original 2,492 / 2,492 MB, `l3` 2,141 / 2,141 MB, `va` 2,141 / 2,141 MB (also below the original's 2,172 MB state measured in 031–032).

## Decision
Accepted: `va` is 4.53× / 4.55× / 4.62× (golden-v1 / novel / OSM) vs `l3`'s 4.32× / 4.32× / 4.29× in the same session, meeting every rule.
