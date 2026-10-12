#!/bin/bash
# usage: fetch_geofabrik.sh OUTDIR CC=GEOFABRIK_PATH ...   e.g. mk=europe/macedonia
# Used after every public Overpass server failed on 2026-10-11 (raw/overpass-fetch-attempt.log). Per country:
#   1. download the Geofabrik extract (.osm.pbf); record SHA-256, bytes and replication timestamp in OUTDIR/CC.source.txt
#   2. take the country's boundary=administrative admin_level=2 relation carrying ISO3166-1=CC from the extract and fetch
#      its full geometry from the OSM API (an extract can cut off offshore boundary ways)
#   3. keep objects tagged addr:housenumber (plus the nodes their ways need), clip them to that boundary, so extracts that
#      cover several countries (gcc-states, ireland-and-northern-ireland, malaysia-singapore-brunei) give one country only
#   4. write objects tagged with both addr:housenumber and addr:street as JSON lines (opl2jsonl.py); delete the extract
out=$1; shift; here=$(cd "$(dirname "$0")" && pwd); UA='libpostal-research-v15 (kyleyuen)'
for spec in "$@"; do
  cc=${spec%%=*}; gp=${spec#*=}; CC=$(echo "$cc" | tr a-z A-Z); f=$out/$cc.jsonl; t=$out/tmp-$cc
  [ -s "$f" ] && { echo "have $cc"; continue; }
  rm -rf "$t"; mkdir -p "$t"; t0=$(date +%s)
  curl -fsSL --retry 5 -A "$UA" -o "$t/x.osm.pbf" "https://download.geofabrik.de/$gp-latest.osm.pbf" || { echo "FAIL $cc download"; continue; }
  { echo "url https://download.geofabrik.de/$gp-latest.osm.pbf"; echo "bytes $(stat -f %z "$t/x.osm.pbf")"
    echo "sha256 $(shasum -a 256 "$t/x.osm.pbf" | cut -c1-64)"
    echo "replication_timestamp $(osmium fileinfo -g header.option.osmosis_replication_timestamp "$t/x.osm.pbf")"; } > "$out/$cc.source.txt"
  rid=$(osmium tags-filter -R -f opl "$t/x.osm.pbf" "r/ISO3166-1=$CC" -o - | grep 'boundary=administrative' | grep 'admin_level=2' | head -1 | cut -d' ' -f1 | tr -d r)
  [ -n "$rid" ] || { echo "FAIL $cc no admin_level=2 ISO3166-1=$CC relation in extract"; continue; }
  curl -fsSL --retry 3 -A "$UA" -o "$t/boundary.osm" "https://api.openstreetmap.org/api/0.6/relation/$rid/full" || { echo "FAIL $cc boundary $rid"; continue; }
  echo "boundary_relation r$rid https://api.openstreetmap.org/api/0.6/relation/$rid/full sha256 $(shasum -a 256 "$t/boundary.osm" | cut -c1-64)" >> "$out/$cc.source.txt"
  osmium tags-filter "$t/x.osm.pbf" nwr/addr:housenumber -o "$t/addr.osm.pbf" --no-progress && rm "$t/x.osm.pbf" \
  && osmium extract -p "$t/boundary.osm" "$t/addr.osm.pbf" -o "$t/clip.osm.pbf" --no-progress \
  && osmium cat -f opl "$t/clip.osm.pbf" | python3 -I "$here/opl2jsonl.py" > "$f.part" && mv "$f.part" "$f" \
  || { echo "FAIL $cc filter/clip"; continue; }
  echo "ok $cc $(($(date +%s)-t0))s $(wc -l < "$f") objects (boundary r$rid)"; rm -rf "$t"
done
