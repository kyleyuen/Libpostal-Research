#!/bin/bash
# usage: fetch_osm.sh OUTDIR CC...   OpenStreetMap objects tagged addr:housenumber + addr:street inside the country's
# ISO3166-1 admin_level=2 area, from a public Overpass API (default: the VK maps.mail.ru mirror; overpass-api.de answered 504 "too busy" on 2026-10-11), converted to one JSON object per line (osm2jsonl.py).
# Countries with more than 400k such objects in the 2026-10-11 count (or no count) are capped at the first 300k objects
# in Overpass output order (ponytail: not a random sample of the country; the reservoir sample is drawn from these).
out=$1; shift; here=$(cd "$(dirname "$0")" && pwd)
for cc in "$@"; do
  f=$out/$cc.jsonl; [ -s "$f" ] && { echo "have $cc"; continue; }
  case $cc in md|ro|hu|gb|in|my|cn) lim=" 300000";; *) lim="";; esac
  CC=$(echo "$cc" | tr a-z A-Z)
  q="[out:json][timeout:600];area[\"ISO3166-1\"=\"$CC\"][admin_level=2]->.a;nwr[\"addr:housenumber\"][\"addr:street\"](area.a);out tags$lim;"
  for try in 1 2 3; do
    t0=$(date +%s)
    code=$(curl -sS -m 1500 -A 'libpostal-research-v15 (kyleyuen)' --data-urlencode "data=$q" -o "$out/$cc.json.part" -w '%{http_code}' "${OVERPASS:-https://maps.mail.ru/osm/tools/overpass/api/interpreter}")
    if [ "$code" = 200 ] && python3 -I "$here/osm2jsonl.py" "$out/$cc.json.part" "$f"; then
      echo "ok $cc try=$try $(($(date +%s)-t0))s $(wc -l < "$f") objects query=$q"; rm -f "$out/$cc.json.part"; break
    fi
    echo "fail $cc try=$try http=$code $(($(date +%s)-t0))s $(head -c 300 "$out/$cc.json.part" | tr '\n' ' ')"; rm -f "$f"; sleep 60
  done
  sleep 20
done
