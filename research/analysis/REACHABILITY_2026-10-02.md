# Can this parser reach 7.8× under the project rules? (evening of 2026-10-02)

Status: analysis of measured data, written after experiments 029–050. Every number is MEASURED (with its experiment) or labelled as an estimate. No estimate here is a speed result.

## Target
The user's target is 90% of the realistic ceiling: 7.8× the original, i.e. about **4.3 µs per address** on golden-v1 (original ≈33.7–34.5 µs in same-session runs). The deliverable `l3` measures ≈7.8 µs (4.3–4.4×); the best verified candidate `va` (048) screens at ≈7.45 µs (≈4.5×).

## The bounds that say "possible on paper"
- Realistic ceiling (035 level 2: feature keys replayed, every dictionary lookup still paid; a probe, not a design): 3.91 µs = **8.7×**.
- Warm bound (041: the unmodified `l3` parsing an address whose data is already cached): 3.67 µs = **9.2×**.
Both lie below 4.3 µs, so 7.8× is not excluded by arithmetic.

## What would have to go
From `va` (≈7.45 µs) to 4.3 µs: **≈3.15 µs must be removed.** 043 splits `l3`'s time into warm compute (≈3.7 µs) and first-touch memory cost (≈4.1–4.5 µs; same-host co-runners included):

| part | first-touch µs | warm µs | source |
|---|---:|---:|---|
| context fill (phrase searches, normalization) | 1.69 | 0.57 | 043 |
| feature generation | 1.23 | 0.88 | 043 |
| dictionary lookups | 0.72 | 0.49 | 043 |
| weight accumulation | 0.61 | 0.51 | 043 |
| Viterbi + rest | 0.05 | 0.96 → ≈0.6 after 048 | 043, 047, 048 |
| tokenize, normalize, response | 0.22 | 0.30 | 043 |

## Upper bounds of every lever identified (optimistic, ESTIMATES)
- **All string handling removed** (an integer pipeline from tokens to feature ids): libc string/memory routines are ≈21% of warm time and the string-building functions and key splitting another ≈10% (049) → at most ≈1.0–1.2 µs of warm compute.
- **Further Viterbi/setup**: ≈0.2 µs (048 already took ≈0.35 µs; 050's lazy rows lost).
- **Dense phrase index** for the component and postal tries (≈1.29 µs first-touch): perhaps half → ≈0.65 µs, but 039 found such a table needs ≈30–50 MB, above `l3`'s ≈31 MB steady-memory margin; it must replace, not add to, existing structures.
- **Fused dictionary key + weight row records** (lookups + accumulation ≈1.33 µs first-touch): removes the row-index line and some key lines (≈40 of ≈180 lines per address, 045) → perhaps ≈0.3–0.5 µs.
- **Feature-generation first-touch** (≈1.23 µs): its memory is largely not explained by the traced structures (045); no specific lever identified → unknown, perhaps ≈0.3 µs.

Sum of optimistic upper bounds: ≈1.1 + 0.2 + 0.65 + 0.4 + 0.3 ≈ **2.65 µs**, below the ≈3.15 µs needed, and reaching even that requires rewriting feature generation, the dictionaries and the phrase search together (a new inference engine, weeks of work), with the phrase-index memory problem unsolved.

## Evidence that cheap levers are exhausted
Rejected or neutral in this project because their gain did not appear when measured: interleaved/early prefetch and latency-hiding (004, 012 iC, 027, 037, 046), string-to-integer re-keying (015 pairs, 020 g2, 030), lazy buffers (016 T8, 050), dictionary density beyond `l3` (031: memory rule). Accepted today: 048 `va` (+≈4.7%).

## Conclusion (for the user's decision)
7.8× is possible on paper but, with every lever identified so far at its optimistic bound, the projected best is ≈4.8–5.5 µs (≈6.1–7.0×), short of 4.3 µs, and it requires a ground-up rewrite. The honest options are: (a) accept `va` (≈4.5×) as the new deliverable after validation; (b) commit to the multi-week integer-pipeline + dense-layout rewrite with an expected ≈6–7×; (c) re-measure on a quiet machine (co-runners at 100–170% CPU share the L2 during all of today's runs), which could change the first-touch share.
