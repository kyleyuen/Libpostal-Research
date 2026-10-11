#!/bin/bash
# 152: correctness only (never timed). ctl (142 original build) vs cert (EXP_CERT) on every non-reserved set + 2 fresh fuzz seeds x 2M.
# The cert build must produce identical outputs; its stderr carries the certificate counts. Up to 4 jobs at once.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/152-certificate-proof"; B="$E/raw/runs"; P="$W/profiling"
CTL="$W/research/experiments/142-stage-oracles/artifacts/ctl"; CER="$E/artifacts/cert"; mkdir -p "$B"
run() { local n=$1; shift; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') START $n" >> "$B/run.log"; "$@" > "$B/$n.out" 2> "$B/$n.err"; echo $? > "$B/$n.exit"; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') END $n exit=$(cat "$B/$n.exit")" >> "$B/run.log"; }
SETS="v1:golden-v1/all.txt nv:golden-v2-novel/novel.txt osm:golden-v3-osm/osm.txt u4:golden-v4-osm-unseen/unseen.txt u5:golden-v5-osm-unseen/unseen5.txt u6:golden-v6-osm-unseen/unseen6.txt u7:golden-v7-osm-unseen/unseen7.txt u8:golden-v8-osm-unseen/unseen8.txt u9:golden-v9-osm-unseen/unseen9.txt u10:golden-v10-osm-unseen/unseen10.txt u11:golden-v11-osm-unseen/unseen11.txt aud:independent-audit-v1/audit-input.txt dad:independent-audit-v1/dad-international.txt"
n=0
for s in $SETS; do k=${s%%:*}; f="$P/${s#*:}"
  run ctl_$k "$CTL/dump_outputs" "$f" & run cert_$k "$CER/dump_outputs" "$f" & n=$((n+2)); [ $n -ge 4 ] && { wait; n=0; }
done; wait
for sd in 2 3; do run ctl_fz$sd "$CTL/fuzz_pipeline" $sd 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" &
  run cert_fz$sd "$CER/fuzz_pipeline" $sd 2000000 "$P/golden-v1/all.txt" "$P/golden-v2-novel/novel.txt" & done; wait
{ for s in $SETS fz2:x fz3:x; do k=${s%%:*}; if cmp -s "$B/ctl_$k.out" "$B/cert_$k.out"; then r=IDENTICAL; else r=DIFFERENT; fi
  echo "$k outputs=$r ctl_exit=$(cat "$B/ctl_$k.exit") cert_exit=$(cat "$B/cert_$k.exit") $(grep "^CERT" "$B/cert_$k.err")"; grep -c "CERT VIOLATION" "$B/cert_$k.err" | sed "s/^/  violations_lines=/"; done; } > "$E/raw/summary.txt"
date '+%Y-%m-%dT%H:%M:%S%z' > "$B/DONE"
