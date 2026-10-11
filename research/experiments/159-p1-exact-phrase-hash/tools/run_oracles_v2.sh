#!/bin/bash
# 159: v2 O1/O2 stage oracles from the ORIGINAL library (142 `orc` build, fprintf-only EXP_ORACLE hooks, hashes verified first),
# fuzz seeds 2 and 3 x 2,000,000 inputs (same corpora as 142/121: golden-v1 all + novel). Serial, through model_slot.
# O1/O2 are written into FIFOs read by `gzip -n`; O3k/O3i/O4 are symlinks to /dev/null (not needed by P1; 142's fuzz O4 was 1.27 GB gz per 200k).
set -u
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/159-p1-exact-phrase-hash"; A="$W/research/experiments/142-stage-oracles/artifacts/orc"
O="$W/research/oracles/v2"; R="$E/raw/oracles_v2"; mkdir -p "$O" "$R"
( cd "$A" && shasum -a 256 -c <(head -3 hashes.txt) ) > "$R/orc_hash_check.txt" 2>&1 || { echo "orc hash check FAILED" >> "$R/run.log"; exit 1; }
for s in 2 3; do
  d="$O/fuzz-s$s-2m"; mkdir -p "$d"; rm -f "$d/O1.txt" "$d/O2.txt" "$d/O3k.txt" "$d/O3i.txt" "$d/O4.txt"
  mkfifo "$d/O1.txt" "$d/O2.txt"; for f in O3k O3i O4; do ln -s /dev/null "$d/$f.txt"; done
  gzip -n -c < "$d/O1.txt" > "$d/O1.txt.gz" & g1=$!
  gzip -n -c < "$d/O2.txt" > "$d/O2.txt.gz" & g2=$!
  echo "$(date '+%Y-%m-%dT%H:%M:%S%z') START seed $s: ORACLE_DIR=$d $A/fuzz_pipeline $s 2000000 golden-v1/all.txt golden-v2-novel/novel.txt" >> "$R/run.log"
  ORACLE_DIR="$d" "$W/research/tools/model_slot.sh" "$A/fuzz_pipeline" $s 2000000 "$W/profiling/golden-v1/all.txt" "$W/profiling/golden-v2-novel/novel.txt" > "$R/fz$s.stdout" 2> "$R/fz$s.stderr"
  rc=$?; wait $g1; e1=$?; wait $g2; e2=$?
  echo "$(date '+%Y-%m-%dT%H:%M:%S%z') END seed $s exit=$rc gzipO1=$e1 gzipO2=$e2" >> "$R/run.log"
  rm -f "$d/O1.txt" "$d/O2.txt" "$d/O3k.txt" "$d/O3i.txt" "$d/O4.txt"
done
( cd "$O" && shasum -a 256 fuzz-s2-2m/O1.txt.gz fuzz-s2-2m/O2.txt.gz fuzz-s3-2m/O1.txt.gz fuzz-s3-2m/O2.txt.gz > SHA256SUMS )
for f in fuzz-s2-2m/O1 fuzz-s2-2m/O2 fuzz-s3-2m/O1 fuzz-s3-2m/O2; do printf '%s  %s\n' "$(gzip -dc "$O/$f.txt.gz" | shasum -a 256 | cut -d' ' -f1)" "$f.txt"; done > "$O/SHA256SUMS.uncompressed"
cmp "$R/fz2.stdout" "$W/research/experiments/011-normalize-fastpaths/raw/fuzz/base_s2.txt" > "$R/fz2_vs_base_s2.txt" 2>&1; echo "cmp exit $?" >> "$R/fz2_vs_base_s2.txt"
echo "$(date '+%Y-%m-%dT%H:%M:%S%z') DONE" >> "$R/run.log"
