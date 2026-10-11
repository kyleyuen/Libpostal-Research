#!/bin/bash
# 159: all P1 correctness checks, serial, every model-loading run through model_slot. Never timed.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/159-p1-exact-phrase-hash"; R="$E/tools/rec.sh"; MS="$W/research/tools/model_slot.sh"
PT="$E/artifacts/ps_test"; TB=/tmp/libpostal-159/tables; ST=/tmp/libpostal-159/struct; O1="$W/research/oracles/v1"; O2="$W/research/oracles/v2"
shasum -a 256 "$PT" "$E/src/ps_exact.c" "$E/src/ps_exact.h" "$E/tools/ps_test.c" $TB/*.ps > "$E/raw/check_hashes.txt"
for c in golden-v1 novel osm-v3 fuzz-s1-200k; do "$R" o2_$c "$MS" "$PT" oracle $TB "$O1/$c" orig; done
for c in fuzz-s2-2m fuzz-s3-2m; do "$R" o2_$c "$MS" "$PT" oracle $TB "$O2/$c" orig; done
"$R" gen_s1 "$MS" "$PT" gen $TB $ST 1 2000000
"$R" gen_s2 "$MS" "$PT" gen $TB $ST 2 2000000
# negative controls: (a) swap the E/L class of the component entry "sk" (matched in golden-v1 a=0); (b) disable routing on the generator
PS_NEG="comp:sk:9" "$R" negctl_flip_golden-v1 "$PT" oracle $TB "$O1/golden-v1"
PS_FORCE_FAST=1 "$R" negctl_forcefast_gen_s1 "$MS" "$PT" gen $TB $ST 1 2000000
echo "$(date '+%Y-%m-%dT%H:%M:%S%z') run_checks DONE" >> "$E/raw/commands.txt"
