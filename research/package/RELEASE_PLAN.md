# Private clone package plan

The primary review artifact is the private Git repository clone. No ZIP or
release download is required for the current 203 result. The repository owner
controls staging, commit, push, and visibility.

## Clone-resident 203 evidence

The versioned allowlist is `git-add-candidates.tsv`; the inventory snapshot is
`inventory-summary.json`. The current proposal contains 12,177 paths totaling
347,461,693 bytes. It does not hash or copy files. The updated list drops one
Python bytecode cache and adds the original-000 harness executables and hash
list used by the verifier, plus the two pinned original fuzz streams; restage
the updated allowlist and summary. It explicitly
overrides broad ignore rules for the
promoted 203 package, original-000 artifacts, and clone-verification evidence.

- Accepted candidate source/build bundle and manifests:
  `research/final/203-cpi-memory/` (97 total files: 95 covered by its
  `SHA256SUMS`, plus the manifest and sidecar; about 33.6 MB; largest file is
  under 9 MB).
- Original 000 executable/library artifacts:
  `research/experiments/000-baseline/artifacts/` (each proposed file below
  50 MB).
- Original-000 comparison harnesses used by the verifier:
  `research/experiments/182-benchmark-suite-prep/artifacts/original-000/`
  (three executables and a hash list; each executable is below 10 MB).
- Frozen datasets and source identities: `profiling/` inputs selected by the
  three 203 suite configs, including `profiling/golden-v14-osm-unseen/`.
- Candidate JSONL outputs: `research/suite/results/203-stream-output-v1/correctness/`.
- Original 000 JSONL outputs copied without modification to
  `research/suite/results/203-stream-output-v1/clone-evidence/original/`.
  `clone-evidence/manifest.json` records the original cache source path, input
  hash, row count, output hashes, and byte-identical status. The 14 pairs cover
  130,341 rows and each complete file is below GitHub's 100 MB limit.
- Timing evidence (84 role captures) and group metadata:
  `research/suite/results/203-stream-gold-novel-v1/`,
  `research/suite/results/203-stream-osm-v3-v13-v1/`, and
  `research/suite/results/203-stream-v14-v1/`. Their `timing/` children hold
  stdout, stderr, and exit records; root files include configs, inputs/build
  identities, environment, gates, reports, and paired memory results.
- Steady-footprint and live-thread evidence:
  `research/experiments/203-cpi-memory-regression/raw/stream-steady-thread-v1/`.
- The 203 experiment record, final report, audits, and source helper are
  proposed by the same inventory for Git review.
- Pinned original fuzz streams required by clone validation:
  `research/experiments/011-normalize-fastpaths/raw/fuzz/base_s1.txt` and
  `base_s2.txt`.
- Current clone documentation/provenance hashes are in
  `research/package/CLONE_REVIEW_MANIFEST-v1.sha256` and its `.sha256` sidecar;
  older final-document manifests remain unchanged as historical snapshots.

The independent clone verifier is
`research/tools/verify_203_clone_evidence.py`. Run it from the repository root
after cloning; it uses only the Python standard library and checks promoted
manifests, input identity/row counts, full output byte equality, timing exits
and paired peak fields, and live one-thread samples. It does not load the
parser or model.

The verifier passed against the current working tree: 95 promoted-manifest
files and 16 source files, 14 output pairs covering 130,341 rows, 84 natural
exit timing captures, 42 paired negative peak-footprint and RSS deltas, and
36 live one-thread samples across six captures. It also validates hashes,
input identities, active-stream markers, and executable identities.

## Exclusions and retained history

`git-add-candidates.tsv` is a proposal, not an executed `git add`. The full
inventory preserves the experiment record history and small logs. Local
`.git`, nested upstream checkout, model cache, object/profile intermediates,
and temporary locks are excluded. Retrieve upstream source at the pinned
revision and model files from the pinned relative-path/hash manifest when
running setup. Large historical artifacts/raw traces listed in
`release-paths.tsv` remain separately identified; none are required to inspect
or verify the current 203 clone evidence. Nothing is deleted or rewritten.

The old `BUNDLE-MANIFEST-v1/v2.json` and any prior release lists are historical
planning artifacts; they are not the current clone completeness contract.
