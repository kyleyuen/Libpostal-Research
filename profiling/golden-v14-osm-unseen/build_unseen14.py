"""Build the one-use golden-v14 public OSM corpus from a fixed, new city list.

Selection is global and city-based: no parser output, model contents, or
candidate timing is consulted. Addresses are assembled with the existing
country-format templates, bounded at 200 distinct rows per city, then exact
duplicates of every retained prior corpus are removed.
"""
import hashlib
import json
import os
import time
import urllib.parse
import urllib.request
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
RAW = os.path.join(HERE, "raw")
PER_CITY = 200
API_URLS = [
    "https://maps.mail.ru/osm/tools/overpass/api/interpreter",
    "https://overpass-api.de/api/interpreter",
]
# (city, latitude, longitude, ISO-2 country, address format)
# A: North America/UK/Oceania/Africa order; E: street-first European/international;
# F: house-number-first France/Belgium; J: Japanese/Taiwanese/Korean order.
CITIES = [
    ("Fort Collins", 40.5853, -105.0844, "us", "A"),
    ("Eugene", 44.0521, -123.0868, "us", "A"),
    ("Knoxville", 35.9606, -83.9207, "us", "A"),
    ("Worcester", 42.2626, -71.8023, "us", "A"),
    ("Gatineau", 45.4765, -75.7013, "ca", "A"),
    ("Kamloops", 50.6745, -120.3273, "ca", "A"),
    ("Dundee", 56.4620, -2.9707, "gb", "A"),
    ("Cambridge", 52.2053, 0.1218, "gb", "A"),
    ("Ennis", 52.8436, -8.9864, "ie", "A"),
    ("Launceston", -41.4332, 147.1448, "au", "A"),
    ("Wollongong", -34.4278, 150.8931, "au", "A"),
    ("New Plymouth", -39.0556, 174.0752, "nz", "A"),
    ("George", -33.9640, 22.4617, "za", "A"),
    ("Troyes", 48.2973, 4.0744, "fr", "F"),
    ("Lorient", 47.7483, -3.3702, "fr", "F"),
    ("Hasselt", 50.9307, 5.3325, "be", "F"),
    ("Mainz", 49.9929, 8.2473, "de", "E"),
    ("Weimar", 50.9795, 11.3235, "de", "E"),
    ("Aachen", 50.7753, 6.0839, "de", "E"),
    ("Steyr", 48.0427, 14.4213, "at", "E"),
    ("Eisenstadt", 47.8457, 16.5233, "at", "E"),
    ("Sion", 46.2331, 7.3606, "ch", "E"),
    ("Amersfoort", 52.1561, 5.3878, "nl", "E"),
    ("Silkeborg", 56.1700, 9.5500, "dk", "E"),
    ("Karlstad", 59.3793, 13.5036, "se", "E"),
    ("Lillehammer", 61.1153, 10.4662, "no", "E"),
    ("Kotka", 60.4664, 26.9458, "fi", "E"),
    ("Plock", 52.5463, 19.7065, "pl", "E"),
    ("Krosno", 49.6880, 21.7700, "pl", "E"),
    ("Znojmo", 48.8555, 16.0488, "cz", "E"),
    ("Szolnok", 47.1621, 20.1825, "hu", "E"),
    ("Sisak", 45.4872, 16.3760, "hr", "E"),
    ("Arezzo", 43.4633, 11.8796, "it", "E"),
    ("Aosta", 45.7370, 7.3200, "it", "E"),
    ("Tarragona", 41.1189, 1.2445, "es", "E"),
    ("Avila", 40.6565, -4.6818, "es", "E"),
    ("Portimao", 37.1386, -8.5373, "pt", "E"),
    ("Serres", 41.0909, 23.5413, "gr", "E"),
    ("Ploiesti", 44.9367, 26.0139, "ro", "E"),
    ("Montana", 43.4085, 23.2250, "bg", "E"),
    ("Afyonkarahisar", 38.7500, 30.5500, "tr", "E"),
    ("Zhytomyr", 50.2547, 28.6587, "ua", "E"),
    ("Ryazan", 54.6095, 39.7126, "ru", "E"),
    ("Kuldiga", 56.9680, 21.9700, "lv", "E"),
    ("Kedainiai", 55.2870, 23.9700, "lt", "E"),
    ("Trencin", 48.8945, 18.0444, "sk", "E"),
    ("Celje", 46.2367, 15.2677, "si", "E"),
    ("Niteroi", -22.8832, -43.1034, "br", "E"),
    ("Posadas", -27.3621, -55.9009, "ar", "E"),
    ("Saltillo", 25.4232, -100.9910, "mx", "E"),
    ("Chillan", -36.6066, -72.1034, "cl", "E"),
    ("Popayan", 2.4448, -76.6147, "co", "E"),
    ("Pucallpa", -8.3791, -74.5539, "pe", "E"),
    ("Paysandu", -32.3214, -58.0757, "uy", "E"),
    ("Nara", 34.6851, 135.8049, "jp", "J"),
    ("Miaoli", 24.5600, 120.8200, "tw", "J"),
    ("Mokpo", 34.8118, 126.3922, "kr", "J"),
    ("Akko", 32.9236, 35.0700, "il", "E"),
    ("Jaipur", 26.9124, 75.7873, "in", "E"),
    ("Chengdu", 30.5728, 104.0668, "cn", "E"),
    ("Chiang Mai", 18.7883, 98.9853, "th", "E"),
    ("Cebu City", 10.3157, 123.8854, "ph", "E"),
    ("Bandung", -6.9175, 107.6191, "id", "E"),
    ("Da Nang", 16.0544, 108.2022, "vn", "E"),
    ("Alexandria", 31.2000, 29.9187, "eg", "E"),
    ("Fes", 34.0331, -5.0003, "ma", "E"),
    ("Nakuru", -0.3031, 36.0800, "ke", "E"),
]
# Fixed globally distributed subset drawn from the original 67-city pool after
# bounding acquisition time; selected without parser outputs or candidate
# measurements; 15 cities across 14 countries.
SELECTED_CITY_NAMES = {
    "Fort Collins", "Eugene", "Gatineau", "Troyes", "Mainz", "Plock",
    "Launceston", "New Plymouth", "Nakuru", "Niteroi", "Popayan", "Nara",
    "Jaipur", "Chengdu", "Da Nang",
}
SELECTED_CITIES = [city for city in CITIES if city[0] in SELECTED_CITY_NAMES]

def city_file(city):
    return os.path.join(RAW, city[0].lower().replace(" ", "_") + ".json")

def fetch():
    os.makedirs(RAW, exist_ok=True)
    for city in SELECTED_CITIES:
        path = city_file(city)
        if os.path.exists(path):
            continue
        query = (f'[out:json][timeout:90];nwr(around:700,{city[1]},{city[2]})'
                 '["addr:housenumber"]["addr:street"];out tags 400;')
        for attempt in range(4):
            try:
                endpoint = API_URLS[attempt % len(API_URLS)]
                req = urllib.request.Request(endpoint,
                    data=urllib.parse.urlencode({"data": query}).encode(),
                    headers={"User-Agent": "libpostal-runtime-research/1.0 (academic benchmark)",
                             "Accept": "application/json"})
                body = urllib.request.urlopen(req, timeout=120).read()
                parsed = json.loads(body)
                with open(path, "wb") as out:
                    out.write(body)
                print("ok", city[0], len(parsed.get("elements", [])), endpoint, flush=True)
                break
            except Exception as exc:
                print("retry", city[0], attempt + 1, repr(exc)[:120], flush=True)
                if attempt == 3:
                    break
                time.sleep(20 * (attempt + 1))
        time.sleep(4)

def assemble(tags, city):
    hn = tags.get("addr:housenumber", "").strip()
    street = tags.get("addr:street", "").strip()
    if not hn or not street:
        return None
    postcode = tags.get("addr:postcode", "").strip()
    locality = (tags.get("addr:city") or city[0]).strip()
    state = tags.get("addr:state", "").strip()
    kind = city[4]
    if kind == "E":
        value = f"{street} {hn}, {postcode} {locality}" if postcode else f"{street} {hn}, {locality}"
    elif kind == "F":
        value = f"{hn} {street}, {postcode} {locality}" if postcode else f"{hn} {street}, {locality}"
    elif kind == "A":
        tail = " ".join(v for v in (state, postcode) if v)
        value = f"{hn} {street}, {locality}" + (f" {tail}" if tail else "")
    else:
        value = " ".join(v for v in (postcode, locality, street, hn) if v)
    value = " ".join(value.split())
    return value if value and "\t" not in value and "\n" not in value else None

def previous_strings():
    paths = [
        "../golden-v1/all.txt",
        "../golden-v2-novel/novel.tsv",
        "../golden-v3-osm/osm.txt",
        "../golden-v4-osm-unseen/unseen.txt",
        "../golden-v5-osm-unseen/unseen5.txt",
        "../golden-v6-osm-unseen/unseen6.txt",
        "../golden-v7-osm-unseen/unseen7.txt",
        "../golden-v8-osm-unseen/unseen8.txt",
        "../golden-v9-osm-unseen/unseen9.txt",
        "../golden-v10-osm-unseen/unseen10.txt",
        "../golden-v11-osm-unseen/unseen11.txt",
        "../golden-v12-osm-unseen/unseen12.txt",
        "../golden-v13-osm-unseen/unseen13.txt",
    ]
    seen = set()
    for rel in paths:
        path = os.path.normpath(os.path.join(HERE, rel))
        if not os.path.isfile(path):
            raise FileNotFoundError(path)
        with open(path, encoding="utf-8") as source:
            for line in source:
                line = line.rstrip("\n")
                seen.add(line.split("\t", 1)[1] if rel.endswith("novel.tsv") else line)
    return seen

def build():
    seen = previous_strings()
    prior_count = len(seen)
    rows = []
    missing = []
    overlap_count = 0
    raw_sources = []
    for city in SELECTED_CITIES:
        path = city_file(city)
        if not os.path.isfile(path):
            missing.append(city[0])
            continue
        with open(path, encoding="utf-8") as raw:
            response = json.load(raw)
            elements = response.get("elements", [])
        with open(path, "rb") as raw:
            raw_bytes = raw.read()
        raw_sources.append({
            "city": city[0], "country": city[3],
            "coordinates": [city[1], city[2]],
            "raw_file": "raw/" + os.path.basename(path),
            "raw_sha256": hashlib.sha256(raw_bytes).hexdigest(),
            "raw_bytes": len(raw_bytes), "response_elements": len(elements),
            "file_mtime_utc": datetime.fromtimestamp(os.stat(path).st_mtime, timezone.utc).isoformat(),
        })
        candidates = {s for e in elements if (s := assemble(e.get("tags", {}), city))}
        ordered = sorted(candidates, key=lambda s: hashlib.sha256(s.encode()).hexdigest())
        kept = []
        for address in ordered:
            if address in seen:
                overlap_count += 1
                continue
            kept.append(address)
            seen.add(address)
            if len(kept) == PER_CITY:
                break
        rows.extend((city[3], address, city[0]) for address in kept)
    rows.sort(key=lambda row: hashlib.sha256(row[1].encode()).hexdigest())
    with open(os.path.join(HERE, "unseen14.tsv"), "w", encoding="utf-8") as out:
        out.writelines(f"{cc}\t{address}\n" for cc, address, _city in rows)
    with open(os.path.join(HERE, "unseen14.txt"), "w", encoding="utf-8") as out:
        out.writelines(f"{address}\n" for _cc, address, _city in rows)
    report = {
        "dataset_id": "golden-v14-osm-unseen",
        "built_at_utc": datetime.now(timezone.utc).isoformat(),
        "source": "OpenStreetMap via Overpass API; raw city responses in raw/",
        "selection": "fixed SELECTED_CITY_NAMES subset of the original 67-city pool, narrowed to bound acquisition time; chosen without parser outputs or candidate measurements; all selected cities absent from prior v3-v13 city lists",
        "input_sets_checked": 13,
        "input_set_paths": [
            "../golden-v1/all.txt", "../golden-v2-novel/novel.tsv",
            "../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt",
            "../golden-v5-osm-unseen/unseen5.txt", "../golden-v6-osm-unseen/unseen6.txt",
            "../golden-v7-osm-unseen/unseen7.txt", "../golden-v8-osm-unseen/unseen8.txt",
            "../golden-v9-osm-unseen/unseen9.txt", "../golden-v10-osm-unseen/unseen10.txt",
            "../golden-v11-osm-unseen/unseen11.txt", "../golden-v12-osm-unseen/unseen12.txt",
            "../golden-v13-osm-unseen/unseen13.txt",
        ],
        "selected_cities": [{"city": c[0], "country": c[3], "coordinates": [c[1], c[2]]} for c in SELECTED_CITIES],
        "raw_sources": raw_sources,
        "prior_unique_address_count": prior_count,
        "new_rows": len(rows),
        "country_count": len({row[0] for row in rows}),
        "city_count_with_rows": len({row[2] for row in rows}),
        "rows_by_country": {cc: sum(1 for row in rows if row[0] == cc) for cc in sorted({row[0] for row in rows})},
        "rows_by_city": {city: sum(1 for row in rows if row[2] == city) for city in sorted({row[2] for row in rows})},
        "exact_overlap_candidates_removed": overlap_count,
        "missing_city_responses": missing,
        "output_sha256": {},
    }
    for name in ("unseen14.txt", "unseen14.tsv"):
        with open(os.path.join(HERE, name), "rb") as source:
            report["output_sha256"][name] = hashlib.sha256(source.read()).hexdigest()
    with open(os.path.join(HERE, "provenance.json"), "w", encoding="utf-8") as out:
        json.dump(report, out, indent=2, ensure_ascii=False)
        out.write("\n")
    print(f"rows={len(rows)} countries={report['country_count']} cities={report['city_count_with_rows']} "
          f"overlap_removed={overlap_count} missing={len(missing)}")
    print(json.dumps(report["output_sha256"], sort_keys=True))

if __name__ == "__main__":
    import sys
    if sys.argv[1:] == ["fetch"]:
        fetch()
    elif sys.argv[1:] == ["build"]:
        build()
    else:
        raise SystemExit("usage: python3 build_unseen14.py fetch|build")
