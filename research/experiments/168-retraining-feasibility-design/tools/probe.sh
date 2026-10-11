#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../../../.." && pwd)
cd "$root"
src=research/experiments/026-cross-platform/windows/upstream/src/address_parser_train.c
echo "probe=168 retraining boundary evidence"
date '+timestamp=%Y-%m-%dT%H:%M:%S%z'
git rev-parse HEAD
shasum -a 256 "$src" research/experiments/026-cross-platform/windows/upstream/src/crf_trainer.c research/experiments/026-cross-platform/windows/upstream/src/crf_trainer_averaged_perceptron.c research/experiments/153-record-format/raw/a2.stdout research/experiments/160-p2-records-in-cpi/raw/verify_160c.stdout
rg -n 'DEFAULT_ITERATIONS|DEFAULT_MIN_UPDATES|DEFAULT_MODEL_TYPE|address_parser_train\(|address_parser_train_epoch|address_parser_finalize_model|shuffle_file_chunked_size|USAGE' "$src"
awk '/^state_features.tsv:|^trans_features.tsv:|^vocab.tsv:|^verify:/' research/experiments/153-record-format/raw/a2.stdout
awk '/^fuzz s[12] IDENTICAL|^golden-v1|^novel|^osm-v3|^unseen-v[4-9]|^holdout-150|^dad-150|^frozen/' research/experiments/160-p2-records-in-cpi/raw/verify_160c.stdout
printf 'tsv_or_jsonl_resource_files='; find research/experiments/026-cross-platform/windows/upstream -type f \( -name '*.tsv' -o -name '*.jsonl' \) | wc -l
printf 'labeled_parser_training_files_by_name='; find research/experiments/026-cross-platform/windows/upstream -type f \( -iname '*address*train*' -o -iname '*parser*train*' \) ! -name '*.c' ! -name '*.h' ! -name '*.Po' | wc -l
