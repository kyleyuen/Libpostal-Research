# Experiment 186 — original parser behavior map and edge corpus

## Purpose and oracle

This is a behavior-coverage map for a possible parser architecture redesign. It
does not assert that outputs are linguistically correct; the contract is to
preserve the original implementation's ordered labels and component bytes.
Finite examples cannot establish equality for every possible input string.
Acceptance is observational: the redesign need not use the same internal steps,
feature-query count, score bits or event order as the original. Trace records
diagnose work and dataflow; their event sequences are not output requirements.
`edge-cases.json` therefore records inputs and options only: it intentionally
contains no predicted or hand-written expected outputs. Generate expected bytes
dynamically from the preserved original libpostal source at commit
`25099c506612b34b23b1bfe286ca6321fcf06f35`, using the versioned original
library/executable under `research/experiments/000-baseline/artifacts` where
available. Do not use optimized-candidate outputs as the oracle.

The registry is a targeted architecture stress corpus, not a country benchmark,
training set, or representative distribution. Cases tagged `hand-authored` are
designed to trigger structural behaviors; source-corpus / prior-record cases
must retain their provenance if added later. No case is claimed tested merely
because it appears here. No parser invocation was made while preparing this
map.

Source citations below are relative to
`research/experiments/179-phrase-recognition-flow/scratch/libpostal-25099c506612b34b23b1bfe286ca6321fcf06f35/`.
That source tree was extracted from
`research/experiments/176-benchmark-contract/artifacts/libpostal-25099c506612b34b23b1bfe286ca6321fcf06f35.tar.gz`,
SHA-256 `7586542af4db343e744f6128a997dd75bf25b3ddc54ed1af3810ea60d2880c3b`.
The archive basename identifies upstream source commit
`25099c506612b34b23b1bfe286ca6321fcf06f35`. The extracted directory inherits
workspace Git metadata, so its local `git rev-parse HEAD` is not the upstream
source identity. Prepend that extracted root to each `src/...` citation.

## Evidence levels

- **Source-proven**: directly follows from the preserved original source at the
  cited path and line. This is not a dynamic run of that behavior.
- **Prior-tested**: an older recorded experiment exercised the named API path or
  input family against the original; it does not imply coverage of every
  registry case.
- **Untested here**: registered for future comparison; output and execution
  status must be collected from the pinned original.
- **Inference**: a proposed risk or likely interaction, not a guarantee.

## Public API and output contract

The public parser API is `libpostal_get_address_parser_default_options`,
`libpostal_parse_address(char *, options)`,
`libpostal_address_parser_response_destroy`, and
`libpostal_parser_print_features(bool)` (`src/libpostal.h:157-168`). The default
options are `{language=NULL, country=NULL}` and `libpostal_parse_address`
forwards them to `address_parser_parse` (`src/libpostal.c:270-282`). A null
address returns NULL before parser lookup (`src/address_parser.c:1691-1692`). A
parser that is not setup also returns NULL with an error log
(`src/address_parser.c:1694-1698`).

The parser normalizes the NUL-terminated input, falls back to the original
address if normalization returns NULL, tokenizes the resulting string, removes
separator and ignorable tokens from the model sequence, and retains separators
for component spacing (`src/address_parser.c:1700-1713`). It then sets both
`language` and `country` to NULL before calling `address_parser_context_fill`
(`src/address_parser.c:1714-1725`). Thus non-null options are public inputs but
are source-proven ignored by this version's parser. Keep them in parity cases;
do not specialize a redesign by country or language.

For a whole input recognized as one qualifying suburb-or-higher component
phrase, or as one postal-code phrase, the parser bypasses sequence prediction
and emits one `(label, normalized-input)` pair (`src/address_parser.c:1727-1778`).
Otherwise adjacent tokens with the same label are joined with a single ASCII
space only when their original token offsets had a gap; label transitions begin
new components (`src/address_parser.c:1787-1829`). The response stores labels
and components as parallel arrays; preserve their order and multiplicity. Do
not canonicalize/reorder labels, collapse repeated labels, sort, or merge
components in the harness.

`address_parser_cli.c` is not a suitable parity harness: it handles
`.language`/`.country`, sets options on parse, prints a JSON object keyed by
label (which loses duplicate labels and ordering semantics), skips empty lines,
and has its own command syntax. `research/experiments/003-batched-lookups/tools/dump_outputs.c`
is closer because it emits ordered pairs, but still reads one text line per
address, skips empty inputs, uses only default options, and cannot represent
embedded NUL or invalid UTF-8. Use a direct API dumper/probe for registry
validation; record raw input bytes, options, returned ordered pairs, stdout,
stderr, and exit status.

## Behavior and risk matrix

| Area | Original source behavior / risk | Evidence and location | Coverage in registry | Status |
|---|---|---|---|---|
| Unicode normalization | UTF-8 normalization and per-token normalization inspect Unicode code points/categories; normalization includes transliteration/alternatives and configurable token transforms in the normalizer. Invalid sequences can stop the local UTF-8 iterator; exact API effect must come from original. | `src/normalize.c:112-145, 200-352, 393-527`; parser fallback at `src/address_parser.c:1700-1704` | composed/decomposed accents, compatibility forms, combining marks, apostrophes, punctuation, Cyrillic, Greek, Arabic, Indic, Hebrew, Thai, Hangul, Hiragana/Katakana/Kanji, mixed scripts, fullwidth/ideographic digits | Source-proven mechanics; exact outcomes untested |
| Token boundaries | The scanner emits distinct types for words, ideographs, numeric/ordinal/Roman numbers, email/URL/phone, punctuation, whitespace/newline and invalid chars (`libpostal.h` token enum). Parser drops separators/ignorable tokens from model sequence while retaining boundary metadata. | `src/libpostal.h:20-45, 45-58`; `src/address_parser.c:1686-1713`; scanner rules in `src/scanner.re` | commas, periods, quotes, slash, ampersand, Unicode dash, tabs/newlines, punctuation-only, URLs/email/phone, alpha-digit boundaries, repeated separators | Source-proven categories; case outputs untested |
| Phrase overlaps | Context fill explores dictionary phrase candidates at token positions; full-input recognized component/postal phrases can take a one-item fast route. Prefix/suffix and overlapping multiword phrases may affect candidate/context scoring. | `src/address_parser.c:687-882, 925-993, 1727-1778` | known-looking city/state/country phrase alone, ambiguous `de`, long phrase with shared prefixes/suffixes, phrase embedded among address tokens, overlapping admin/street terms, repeated phrase | Mechanics source-proven; model-dependent matches untested |
| Unknowns / no tokens | Unknown strings still flow through normalization/tokenization and prediction; empty or punctuation-only text can yield no model tokens, and response/null behavior should be sampled from the original rather than inferred. | `src/address_parser.c:1691-1713, 1779-1837` | empty, whitespace, punctuation-only, gibberish, unseen words, single number, lone symbols | Untested here |
| Length / capacity boundaries | Parsing uses dynamic arrays but also fixed-size/path-dependent structures in model routines; prior test includes 41 tokens, which exceeded default CRF context (10) and initial lean capacity (32). There is no universal maximum length proven here. | Original `src/address_parser.c` parse/context path; lifecycle probe description in `research/experiments/158-r0-feature-print/tools/api_probe2.c`; 41-token prior API cases in 158/161 | 0/1/2 tokens, 9/10/11, 31/32/33, 63/64/65, 127+ tokens, repeated long tokens, long single token, long UTF-8 bytes | 41 tokens prior-tested; other boundaries untested; boundary list is design coverage, not fixed-capacity assertion |
| Options: language/country | Defaults are NULL; API accepts pointers, but parse sets both to NULL before context fill, so current parser source ignores them. Null, common codes, unknown/lower/upper/malformed strings are relevant API parity combinations (avoid null-pointer dereference assumptions in candidate). | `src/libpostal.c:270-282`; `src/address_parser.c:1714-1725` | NULL/NULL, en/US, es/ES, de/DE, zh/HK, ja/JP, xx/ZZ, uppercase/lowercase, empty strings | Ignore semantics source-proven; option combinations untested here |
| Ordered labels, duplicate semantics | Response consists of ordered parallel arrays; component assembly merges adjacent equal labels and preserves separated repeats as separate components. Direct-return phrase path emits one label/component. | `src/libpostal.h:150-168`; `src/address_parser.c:1727-1829` | Inputs likely to produce multiple same-label runs, duplicate-looking components and whole-phrase route | Contract from source; actual model routes untested |
| Errors and invalid API bytes | NULL address returns NULL. Not-setup parser logs/returns NULL. Public API consumes C strings: embedded NUL truncates by C semantics; malformed UTF-8 and NULL option pointers need guarded direct probes. Passing arbitrary invalid pointers is undefined and is not a valid differential test. | `src/address_parser.c:1691-1704`; `src/libpostal.c:274-282` | `invalid-api` entries use base64 and `safe_to_call:false`; run only in a separately guarded probe after confirming allocation/termination, never through line CLI | Null-address/setup source-proven; other invalid input effects untested |
| Feature-print API | Public `libpostal_parser_print_features(true/false)` is independently observable and had regressed in `cpi`; experiment 158's `158fp` matched the original stream, while 158ctl aborted. It must be included when claiming broader public API parity, although it is not parse output. | `libpostal.h:168`; `research/experiments/150-independent-professor-audit/README.md`; `research/experiments/158-r0-feature-print/README.md` and `raw/api/compare.txt` | API probe: default/feature on/off, parser datadir, lifecycle sequences; registry cases exercise parse only | Prior-tested for original and 158fp; not executed in this task |
| Setup / teardown lifecycle | Public setup_parser and teardown paths are distinct from per-address behavior. Original passed recorded default, feature print, datadir, lifecycle, lifecycle-disable, 41-token lifecycle, bad datadir/recovery and datadir lifecycle probes. `cpi` teardown→re-setup memory-safety defect was reproduced; 161 fixed it and has recorded ASan/lifecycle checks. | 158 `tools/api_probe2.c`, `raw/api`; 161 `README.md`; public setup API in `src/libpostal.h` | Separate API probe modes; not encoded as address records | Prior-tested; repeated whole-lib `libpostal_teardown`/`libpostal_setup` remains specifically untested in historical 161 report |
| Float order and ties | CRF scoring and Viterbi decisions can depend on accumulation order and tie handling; arithmetic may change freely if ordered parser output remains byte-identical. Reordered or approximate scoring may use a certified margin bound and exact fallback for uncertain cases. 151's certification applied to its tested sets and is not universal proof for a new architecture or all strings. | `research/experiments/151-optimality-bound/PLAN.md` and record; `src/crf.c` original scoring/prediction | identical-token permutation, repeated token, symmetric unknowns, duplicated addresses, long ambiguous inputs; true tied/near-tied paths require original model execution or an equivalent proof | Output parity is required; internal score bits/order are not. Historical tested coverage only; universal ties remain unenumerated and unproven |
| Scoring/debug output | Feature print path emits observable text and may visit string-feature routes separately from optimized parse scoring. Do not derive parse equality from score equality or debug print alone. | original `src/crf.c`; experiments 150/158 | dedicated API/debug regression probe, outside correctness corpus | Prior-tested, with historical candidate regression documented |

## Corpus structure and use

`edge-cases.json` is UTF-8 JSON, schema version 1. Every record has a stable
`id`, `category`, `provenance`, `scope`, `input_utf8` or `input_base64`, explicit
nullable parser options, rationale, evidence status and `safe_to_call`. A
dedicated `safe-api-null` record represents the defined public NULL-address
call; malformed byte sequences remain `invalid-api` and unsafe. Valid UTF-8 cases can be sent to the direct public parser API after setup. Empty
strings are valid API inputs even though the existing line-based dumpers skip
them. `invalid-api` examples are intentionally not safe to call in the ordinary
runner. JSON/base64 preserves raw bytes, but cannot make an invalid pointer,
unterminated C string, or arbitrary malformed object safe.

No expected output field exists. For every safe case, the original oracle run
must produce the actual ordered pair list or a captured NULL/error/exit result;
preserve that output in a separately versioned raw result file with original
binary/model hashes and exact invocation. Candidate comparisons consume those
captured original results. An output difference rejects byte-identity until
explained; a prediction that a case “should” have a certain label is not an
oracle.

Registry counts after adding the London and SW1A 1AA cases: 82 records total,
78 safe public-API records and 4 unsafe malformed-byte records. The shared
selector returns `(case, trace_safe)` for all safe records; use it instead of
duplicating eligibility logic. No real original/control/diagnostic parse output
has been captured for the complete registry yet.

### Real smoke evidence (2026-10-07)

Four trace-safe cases were run sequentially through the pinned original, the
unchanged-source release control and the diagnostic build: `city-london`,
`postcode-uk-sw1a-1aa`, `empty`, and `punctuation-only`. All three output files
are byte-identical (SHA-256
`0713c9a84a7c7b648fef61e9d320889fb50122c456e506df34969c406529c544`). The
original outputs include `city=london`, `postcode=sw1a 1aa`, a successful empty
response with zero pairs, and the punctuation-only input's unusual
`house=/\\ !?` result. That last result is preserved exactly as observed, not
treated as a semantic oracle or normalized away.

The diagnostic trace has four address summaries, 74 query events, and zero
overflow; the trace SHA-256 is
`d11be25662bed6dd346670180cca428739cf2a2715af3d4eee692f2420140f0c`. Thread
samples were all one thread (14 original, 14 release-control, 13 diagnostic
samples). This is smoke evidence only: no speed or memory claim follows. Raw
outputs, command/environment records, thread samples and analysis are in
`results/smoke-20261007-final3/api-trace-smoke-v1/`; summary is
`results/smoke-20261007-final3/results.json`.

### Full safe API parity (2026-10-07)

The complete safe registry was then run through the pinned original,
release-control and diagnostic libraries. All **78/78** emitted ordered
response records are byte-identical across the three builds; common output
SHA-256 is
`bfcd44cb99eabc2d9fc86cb0dc60816a8877b932595e434180494314ad1fb665`.
Input SHA-256 is
`8b5af8831ffff538f891bd0831cee5b42b05ef0186331db6b00500fba8edac89`.
The process command exited 0 at 2026-10-07 17:01:59 PDT, with empty stderr.
Each build had thirteen thread samples, all reporting one thread. Four unsafe
malformed-byte records were excluded; the run does not claim behavior for
those byte sequences or for all possible parser inputs. Output, input/build/
model manifest and run evidence are under
`results/newedge-api-20261007-final3/`; the fixture command/exit record is
`raw/fixtures/newedge-api-command-20261007.json`.

The 78-case run is API output parity only: it did not generate a per-address
trace. Trace coverage remains the four smoke inputs above (74 query events,
zero overflow), so operation counts from those four must not be presented as a
full-registry profile. Neither run is a speed or memory acceptance benchmark.
The `evidence_status` values in the frozen manifest record authoring-time
knowledge; these run records are the measured evidence for the 78 cases.

The no-model fake-library fixture check passed on 2026-10-07 23:53:53 UTC; its
six check results and exits are preserved in `raw/edge-harness-fakecheck.json`.
It validates harness plumbing only. Even with 78-case equality, universal
byte identity is not proven; retain the golden-v1 hard gate and treat this
edge corpus as targeted supplemental coverage.

The direct public-API capture harness is `tools/edge_api_dump.py`. For one
versioned original/control/candidate shared library, run:

```sh
python3 research/experiments/186-architecture-benchmark/tools/edge_api_dump.py \
  --library research/experiments/000-baseline/artifacts/libpostal.dylib \
  --datadir /path/to/pinned/model/data \
  --cases research/experiments/186-architecture-benchmark/edge-cases.json \
  --output-jsonl research/experiments/186-architecture-benchmark/raw/original-edge-outputs.jsonl
```

For existing address-per-line workloads, replace `--cases FILE` with
`--input-txt FILE`; that mode follows the existing corpus convention and skips
empty lines. The output path is created exclusively, so an existing capture is
never overwritten. To compare builds, write each to a distinct path and compare
the ordered JSON records or their raw file hashes. `--compare-jsonl ORIGINAL`
also does an exact byte comparison against an existing capture and exits 1 on
any difference; use a fresh candidate `--output-jsonl` path.

Output is deterministic JSONL in manifest order, with one record per safe case:
`{case_id,category,input_utf8|input_kind,options,trace_safe,status,ordered_pairs}`. `trace_safe` is false for a null pointer or embedded NUL; those records remain in the API correctness lane. `status` is
`"ok"` or `"null"`; `ordered_pairs` is an ordered array of `[label,component]`
tuples or JSON null for a NULL response. The harness uses the library's public
API through `ctypes`, preserves empty strings/newlines/options, and omits every
record outside safe valid-UTF-8 scope except the explicit source-proven
NULL-address API probe. `input_utf8` records may include an embedded NUL; the
caller passes a terminated C buffer and the original API observes normal C
string termination. Options must each be NULL or a UTF-8 string; embedded NUL
option records explicitly exercise C-string truncation. It runs in a visible Python process;
diagnostic trace collection must be configured by the caller/environment and
is not implemented in this correctness dumper. Redirect and retain stdout,
stderr and exit status alongside output JSONL and binary/model hashes. No
result is claimed captured until the command has actually run. Malformed-byte
probes require a separate guarded process-level harness; this dumper has no
unsafe-byte opt-in.

Importable runner helpers `validate_cases(cases)` and
`select_safe_cases(cases)` live in `tools/edge_api_dump.py`;
`select_safe_cases` returns manifest-order `(case, trace_safe)` pairs and is
the shared filter for correctness/trace case selection. The trace-safe flag is
false for embedded-NUL inputs and the explicit NULL-address call.

The same harness has a diagnostic-only `--trace` mode for an instrumented
shared library. It retains only trace-safe records, sets `TRACE_JSONL`,
`TRACE_CASE_ID`, and `TRACE_EXPECTED_ADDRESSES` before loading the library,
calls exported `diag_trace_finish()` after the parses, and returns failure if
the diagnostic reports a failed/incomplete trace. Optional
`--trace-case-ids ID1,ID2,...` chooses a small named subset in the specified
order; for example `city-london,postcode-uk-sw1a-1aa,empty,punctuation-only`
spans a named-place string, an alpha-numeric postcode, empty text and
punctuation-only text. Keep this separate from the complete API correctness
capture. Trace JSONL path creation is exclusive. `--trace` requires
`--trace-jsonl` or pre-set `TRACE_JSONL`; if trace is requested, output path
must be new.

For the existing text-based diagnostic dumper, materialize a small default-
options subset and a sidecar mapping with:

```sh
python3 research/experiments/186-architecture-benchmark/tools/materialize_trace_inputs.py \
  --cases research/experiments/186-architecture-benchmark/edge-cases.json \
  --count 5 \
  --output-txt research/experiments/186-architecture-benchmark/raw/edge-trace-smoke.txt \
  --output-meta research/experiments/186-architecture-benchmark/raw/edge-trace-smoke.json
```

It selects the first manifest-order trace-safe, non-empty, single-line cases
with NULL/NULL options, and stores their IDs, category, source position, exact
input text, byte count and SHA-256 beside the TXT. This is a separate trace
subset; it cannot cover non-default options, empty strings or API-only cases.

Include these registry cases alongside all golden-v1 inputs in any redesign
acceptance gate. The edge registry supplements coverage and does not weaken the
hard golden-v1 and single-thread constraints. Avoid turning this corpus into
country-specific shortcuts or hard-coded address/output tables.

## Explicit coverage gaps

- No exhaustive inputs, arbitrary C object/pointer states, all malformed UTF-8
  forms, all Unicode code points, or all model phrase overlaps are enumerated.
- No corpus outputs were run or captured during creation of this document.
- Source mechanics do not establish which entries match dictionaries or which
  Viterbi ties occur; those require execution and/or instrumentation against the
  pinned original model.
- Most international strings below are structural script/normalization probes,
  not verified locale examples. Hong Kong/CJK entries are included as one
  cross-script slice and do not justify a special-case path.
- The architecture's final behavior must be compared against the preserved
  original on golden-v1 and these added cases, while staying single-threaded.
