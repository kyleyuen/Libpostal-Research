#!/bin/bash
# Verify c2s1, c3, c4, then wait for c2s2's final build line in raw/compile_times.txt (written only after its last link) and verify it.
cd "$(dirname "$0")/.."
for v in c2s1 c3 c4; do tools/verify.sh $v > raw/verify_$v.log 2>&1; done
until grep -q "^c2s2: " raw/compile_times.txt; do sleep 15; done
tools/verify.sh c2s2 > raw/verify_c2s2.log 2>&1
