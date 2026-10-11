# Independent audit holdout v1 (2026-10-05)

This frozen dataset was assembled for a **second-opinion Codex audit** after the
`cpi` candidate and the repository's benchmark suite had been selected. It is
separate from golden-v1, novel, and the OpenStreetMap sets. The original timing
and correctness runs used identical files under
`/private/tmp/libpostal-professor-audit-20261005/`; this directory preserves
their tested bytes. The associated results are in
`research/experiments/150-independent-professor-audit/`.

## Inputs

- `rrad-source.json`: the full 3,220-address [RRAD](https://github.com/EthanRBrown/rrad)
  `addresses-us-all.json` source, fetched from the URL in `metadata.json`.
  RRAD says its addresses come from OpenAddresses and are public domain.
- `rrad-real.txt`: 3,220 unique address strings, with no exact matches in the
  eleven previously measured sets. For each source record, nonempty
  `address1`, `address2`, `city`, `state`, and `postalCode` fields were trimmed,
  joined with spaces, and internal whitespace collapsed. Strings were shuffled
  with Python 3.14.4 `random.Random(15725397191413253593)`.
- `edge-cases.txt`: 78 nonempty hand-built and transformed strings covering
  scripts, combining characters, emoji, full-width and Arabic/Devanagari digits,
  units, postal codes, spacing, punctuation, and unusually long strings. The
  30 base cases were augmented with upper/lowercase versions of the first 24.
- `audit-input.txt` and `audit-input.tsv`: the 3,220 real and 78 edge strings
  shuffled together in a fixed order. The TSV's first field is a category tag
  (`US`, `MULTI`, `UNIT`, `FORMAT`, `UNICODE`, or `MALFORM`), not a country label
  for the edge cases. These were the exact inputs for the 15 paired trials.
- `dad-international.txt`: 45 additional real addresses from nine files in
  [Dummy Address Data](https://github.com/justintime50/dad), across Australia,
  Canada, China, Germany, Spain, France, the UK, and Mexico. These were used
  for a separate exact-output check, not the timing trials. The raw files,
  source URLs and hashes are preserved in `dad-source/` and `dad-sources.json`.
  Their MIT license is preserved as `dad-LICENSE`.

`metadata.json` records the RRAD source SHA-256, shuffle seed and counts.
`SHA256SUMS` fixes every stored input and source byte. No reserved v12/v13
dataset was read or spent. This set adds source diversity, but RRAD is a US-only
sample and the international addendum is small; it cannot represent every
address or every parser edge case.
