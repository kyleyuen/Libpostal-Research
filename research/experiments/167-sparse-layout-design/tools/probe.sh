#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../../../.." && pwd)
cd "$root"
echo "probe=167 sparse layout evidence"
date '+timestamp=%Y-%m-%dT%H:%M:%S%z'
git rev-parse HEAD
shasum -a 256 research/experiments/135-weight-compressibility-probe/raw/probe.stderr research/experiments/153-record-format/raw/a2.stdout research/experiments/162-joint-timing/README.md
awk '/^WD_PROBE (state|trans) / {print}' research/experiments/135-weight-compressibility-probe/raw/probe.stderr
awk '/^entries |^verify:|^sizes MB:/' research/experiments/153-record-format/raw/a2.stdout
for c in golden-v1 novel osm-v3 fuzz-s1-200k; do
  gzip -cd "research/oracles/v1/$c/O3i.txt.gz" | awk -v c="$c" '{n++; if ($5 == "-") miss++; else hit++} END {printf "O3i corpus=%s rows=%d hit_rate=%.4f miss_rate=%.4f\n",c,n,100*hit/n,100*miss/n}'
done
sed -n '/160b |/,/160c |/p' research/experiments/162-joint-timing/README.md
