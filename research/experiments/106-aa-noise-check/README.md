# Experiment 106 — A/A noise check after machine cleanup (diagnostic)

Date/time/timezone: created 2026-10-03T23:47:29-0700, America/Los_Angeles. Status: complete

Context: 105's screens scattered ±5–10%. Four long-running CPU users were removed (ACTIVITY_LOG 2026-10-03 23:29–23:47):
- two orphaned VS Code crash reporters, killed;
- a leftover ASan test binary from an earlier Claude session in this project, stopped;
- `icdd`, restarted.
Then the same psf binary was screened against itself (6 rounds, 20 passes, rotating order; `raw/screen_analyze2.stdout`):
        v1 median psfA 5.445 psfB 5.434; psfB speedup per round [0.9994, 1.0137, 1.0063, 1.0097, 0.9974, 0.9951] median 1.0029; rounds faster 3/6
        v1 min    psfA 5.354 psfB 5.343; psfB speedup per round [1.0068, 1.0045, 1.0008, 1.0045, 0.997, 1.0009] median 1.0027; rounds faster 5/6
        osm median psfA 3.561 psfB 3.527; psfB speedup per round [1.0171, 1.0028, 1.0275, 1.0065, 1.0068, 1.0096] median 1.0082; rounds faster 6/6
        osm min    psfA 3.474 psfB 3.438; psfB speedup per round [1.0157, 1.0081, 1.0023, 1.0146, 0.9954, 1.0061] median 1.0071; rounds faster 5/6
Result:
- **Noise.** Identical binaries differ by ≤1% (median ratio) on golden-v1 and 0.8% on OSM, with single rounds up to 2.7%. Decision threshold from now on: about 1.5% with a stage-timer mechanism.
- **Absolute speed.** It improved substantially once the background load was gone: psf golden-v1 5.44 µs vs 6.20 µs in 103, OSM 3.53 vs 3.94. All earlier same-session ratios stay valid. Absolute speedups vs the original are re-measured in 107.
