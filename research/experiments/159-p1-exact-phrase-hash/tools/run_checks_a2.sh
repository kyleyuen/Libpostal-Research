#!/bin/bash
# 159 attempt 2 (tag = low hash bits): all P1 correctness checks again, serial, model-loading runs through model_slot. Never timed.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/159-p1-exact-phrase-hash"; R="$E/tools/rec.sh"; MS="$W/research/tools/model_slot.sh"
PT="$E/artifacts/ps_test"; TB="$E/artifacts/tables"; ST=/tmp/libpostal-159/struct; O1="$W/research/oracles/v1"; O2="$W/research/oracles/v2"
shasum -a 256 "$PT" "$E/src/ps_exact.c" "$E/src/ps_exact.h" "$E/tools/ps_test.c" "$TB"/*.ps > "$E/raw/check_hashes_a2b.txt"
for c in golden-v1 novel osm-v3 fuzz-s1-200k; do "$R" a2b_o2_$c "$MS" "$PT" oracle "$TB" "$O1/$c" orig; done
for c in fuzz-s2-2m fuzz-s3-2m; do "$R" a2b_o2_$c "$MS" "$PT" oracle "$TB" "$O2/$c" orig; done
"$R" a2b_gen_s1 "$MS" "$PT" gen "$TB" $ST 1 2000000
"$R" a2b_gen_s2 "$MS" "$PT" gen "$TB" $ST 2 2000000
PS_NEG="comp:sk:2" "$R" a2b_negctl_flipterm_golden-v1 "$PT" oracle "$TB" "$O1/golden-v1"
PS_FORCE_FAST=1 "$R" a2b_negctl_forcefast_gen_s1 "$MS" "$PT" gen "$TB" $ST 1 2000000
"$R" a2b_smoke_bench_exact "$E/artifacts/ps_bench" exact "$TB" "$O1/osm-v3" 2
"$R" a2b_smoke_bench_orig "$MS" "$E/artifacts/ps_bench" orig "$O1/osm-v3" 2
echo "$(date '+%Y-%m-%dT%H:%M:%S%z') run_checks_a2 (a2b) DONE" >> "$E/raw/commands.txt"
