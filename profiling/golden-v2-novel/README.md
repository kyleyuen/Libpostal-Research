# Novel benchmark v2 (generalization set)

Derived deterministically by `build_novel.py` from golden-v1's structured source fields (golden-v1 files are not modified).
12,863 address strings, **none of which appear in golden-v1**, re-assembled from components in six component orders, four separators,
mixed casing (upper/lower/original), 25% with one road-name typo (deletion/transposition, creating unseen tokens), plus per-record country codes
(`novel.tsv`: `country<TAB>address`). `v1-all.tsv` is golden-v1/all.txt in the same order with country labels.

Purpose: the hidden evaluation will use a new dataset for both the reference and our build. Anything tuned to golden-v1 might not transfer, so every
candidate is also timed on this set (and on a cold first pass, see METHOD.md). This set is not ground truth and carries no annotations; correctness is
byte-identical output to the preserved baseline library. Checksums: `SHA256SUMS`. Source attribution and license obligations of golden-v1 apply.
