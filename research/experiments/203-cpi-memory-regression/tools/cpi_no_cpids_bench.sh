#!/bin/sh
set -eu
exec env CP_IDS_DISABLE=1 \
  "/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/119-component-phrase-ids/artifacts/cpi/bench_ext" "$@"
