# Experiment 203 output evidence: independent audit

Audit scope: saved records only. No parser, model, build, timing, or memory job was run for this review.

## Serialized corpus outputs

`research/suite/results/203-stream-output-v1/` records an integrity/correctness/report-only run (`invocation.json`); it did not run the timing, memory, or thread stages. The manifest lists 14 datasets totaling 130,341 address lines. I independently checked every listed TXT/TSV hash and the seven model-file hashes against the files on disk. All matched. The exact candidate artifact hashes also match the manifest: `bench_ext` `8bb0a15c…`, `dump_outputs` `50e769bc…`, and `fuzz_pipeline` `76d3943c…`; original comparator artifacts are `a4968702…`, `578a6267…`, and `5a657303…`, respectively. The candidate `address_parser.c` and `stream_postcode_graph.h` hashes (`ca2e56f6…` and `ba25c928…`) match the corresponding source entries in the candidate artifact `hashes.txt`.

The correctness stage produced 14 fresh candidate JSONL dumps. Their line counts sum to 130,341. I compared each file byte-for-byte with the matching cached `original-000` JSONL; all 14 pairs matched. The run log shows 16 correctness jobs total: those 14 candidate dumps and two candidate fuzz jobs. No original dump job ran in this invocation. The comparator files are cached outputs keyed by the preserved original dumper hash and each input hash. The v14 original cache had been generated earlier by the fresh baseline dump recorded in `research/suite/results/202-fresh-v14-correctness-v1/run.log`; this 203 check reused it.

The runner also reports `RESULT: IDENTICAL` against both frozen references configured for golden-v1 and novel. Its one-byte negative control deliberately mutates the cached golden baseline bytes in memory and confirms the SHA-256 changes; this validates mismatch detection, not parser behavior.

Every candidate job’s stderr has exactly one identical active-stream marker: `PC_COMPRESS_STREAM active=1 rows=5483892 edges=122300179 bytes=151033002`. This holds for all 14 corpus dumps and both fuzz jobs (16/16), confirming that each candidate process used the streaming postcode graph path. The captured candidate and comparator executable hashes in the run manifest matched the live artifact files. The candidate source hashes also match its artifact record.

## Fuzz comparison and limits

The candidate ran two 2,000,000-case fuzz jobs, seeds 1 and 2. Each produced a 2,000,000-line digest/status stream. Both whole-file SHA-256 values match the corresponding frozen original references, `research/experiments/011-normalize-fastpaths/raw/fuzz/base_s1.txt` and `base_s2.txt`; the candidate streams also match those reference hashes on disk. The original fuzz pipeline was not rerun here because the configured 011 references already existed. This is digest-stream agreement for those seeds and configured mutation corpora, not byte-by-byte output comparison of each parser result and not a universal-input proof.

The saved correctness evidence supports exact serialized output equality on these 14 listed datasets and digest-stream equality on the two referenced fuzz runs. It does not establish timing, thread-count, peak-memory, or steady-memory acceptance. In particular, no memory-gate conclusion should be drawn from this correctness-only run.
