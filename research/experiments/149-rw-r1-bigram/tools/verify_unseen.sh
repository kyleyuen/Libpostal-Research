#!/bin/bash
# 149: candidate dumps on unseen v5-v11 vs the original library's dump_orig.stdout of each set's remeasure experiment. usage: verify_unseen.sh VARIANT
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/149-rw-r1-bigram"; X="$E/.."; P="$E/../../../profiling"; v=$1; o="$E/raw/verify_$v/unseen"; mkdir -p "$o"
R="5:080-unseen5-remeasure 6:086-unseen6-remeasure 7:093-unseen7-remeasure 8:102-unseen8-remeasure 9:115-unseen9-remeasure 10:120-unseen10-remeasure 11:134-unseen11-confirm-cpi"
i=0; for x in $R; do n=${x%%:*}; "$E/artifacts/$v/dump_outputs" "$P/golden-v$n-osm-unseen/unseen$n.txt" > "$o/u$n.jsonl" 2> "$o/u$n.err" & i=$((i+1)); [ $((i % 4)) -eq 0 ] && wait; done; wait
for x in $R; do n=${x%%:*}; e=${x#*:}; cmp -s "$o/u$n.jsonl" "$X/$e/raw/dump_orig.stdout" && echo "unseen-v$n IDENTICAL ($(wc -l < "$o/u$n.jsonl" | tr -d ' ') lines)" || echo "unseen-v$n DIFFERENT"; done
