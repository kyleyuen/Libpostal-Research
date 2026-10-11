#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../../../.." && pwd)
cd "$root"
echo "probe=166 offline CRF re-encoding evidence"
date '+timestamp=%Y-%m-%dT%H:%M:%S%z'
git rev-parse HEAD
shasum -a 256 research/experiments/160-p2-records-in-cpi/raw/gen_records/stdout.txt research/experiments/160-p2-records-in-cpi/raw/verify_160c.stdout research/oracles/v1/SHA256SUMS
(cd research/oracles/v1 && shasum -a 256 -c SHA256SUMS) | tail -20
for c in golden-v1 novel osm-v3 fuzz-s1-200k; do
  gzip -cd "research/oracles/v1/$c/O3i.txt.gz" | awk -v c="$c" '{n++; if ($5 == "-") miss++; else hit++} END {printf "O3i corpus=%s rows=%d hits=%d misses=%d\n", c,n,hit,miss}'
done
sed -n '1,120p' research/experiments/160-p2-records-in-cpi/raw/gen_records/stdout.txt
sed -n '1,80p' research/experiments/160-p2-records-in-cpi/raw/verify_160c.stdout
