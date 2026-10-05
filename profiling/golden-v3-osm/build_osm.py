"""Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("New York",40.7484,-73.9857,"us","A"),("Chicago",41.8818,-87.6231,"us","A"),("Los Angeles",34.0522,-118.2437,"us","A"),("Houston",29.7604,-95.3698,"us","A"),
 ("Seattle",47.6097,-122.3331,"us","A"),("Toronto",43.6532,-79.3832,"ca","A"),("Montreal",45.5017,-73.5673,"ca","A"),("London",51.5145,-0.0928,"gb","A"),
 ("Manchester",53.4808,-2.2426,"gb","A"),("Dublin",53.3498,-6.2603,"ie","A"),("Sydney",-33.8688,151.2093,"au","A"),("Melbourne",-37.8136,144.9631,"au","A"),
 ("Auckland",-36.8485,174.7633,"nz","A"),("Cape Town",-33.9249,18.4241,"za","A"),("Mumbai",19.0760,72.8777,"in","A"),("Singapore",1.2830,103.8510,"sg","A"),
 ("Paris",48.8566,2.3522,"fr","F"),("Lyon",45.7640,4.8357,"fr","F"),("Brussels",50.8503,4.3517,"be","F"),("Luxembourg",49.6116,6.1319,"lu","F"),
 ("Berlin",52.5200,13.4050,"de","E"),("Munich",48.1374,11.5755,"de","E"),("Hamburg",53.5511,9.9937,"de","E"),("Vienna",48.2082,16.3738,"at","E"),
 ("Zurich",47.3769,8.5417,"ch","E"),("Amsterdam",52.3676,4.9041,"nl","E"),("Rotterdam",51.9244,4.4777,"nl","E"),("Copenhagen",55.6761,12.5683,"dk","E"),
 ("Stockholm",59.3293,18.0686,"se","E"),("Oslo",59.9139,10.7522,"no","E"),("Helsinki",60.1699,24.9384,"fi","E"),("Warsaw",52.2297,21.0122,"pl","E"),
 ("Krakow",50.0647,19.9450,"pl","E"),("Prague",50.0755,14.4378,"cz","E"),("Bratislava",48.1486,17.1077,"sk","E"),("Budapest",47.4979,19.0402,"hu","E"),
 ("Ljubljana",46.0569,14.5058,"si","E"),("Zagreb",45.8150,15.9819,"hr","E"),("Rome",41.9028,12.4964,"it","E"),("Milan",45.4642,9.1900,"it","E"),
 ("Madrid",40.4168,-3.7038,"es","E"),("Barcelona",41.3851,2.1734,"es","E"),("Lisbon",38.7223,-9.1393,"pt","E"),("Athens",37.9838,23.7275,"gr","E"),
 ("Bucharest",44.4268,26.1025,"ro","E"),("Sofia",42.6977,23.3219,"bg","E"),("Istanbul",41.0082,28.9784,"tr","E"),("Kyiv",50.4501,30.5234,"ua","E"),
 ("Moscow",55.7558,37.6173,"ru","E"),("Vilnius",54.6872,25.2797,"lt","E"),("Riga",56.9496,24.1052,"lv","E"),("Tallinn",59.4370,24.7536,"ee","E"),
 ("Sao Paulo",-23.5505,-46.6333,"br","E"),("Buenos Aires",-34.6037,-58.3816,"ar","E"),("Mexico City",19.4326,-99.1332,"mx","E"),("Santiago",-33.4489,-70.6693,"cl","E"),
 ("Bogota",4.7110,-74.0721,"co","E"),("Tokyo",35.6812,139.7671,"jp","J"),("Osaka",34.6937,135.5023,"jp","J"),("Taipei",25.0330,121.5654,"tw","J"),
 ("Seoul",37.5665,126.9780,"kr","J"),("Beijing",39.9042,116.4074,"cn","J"),("Tel Aviv",32.0853,34.7818,"il","E"),("Dubai",25.2048,55.2708,"ae","A"),
]
HERE = os.path.dirname(os.path.abspath(__file__)); RAW = os.path.join(HERE, "raw")
def fname(c): return os.path.join(RAW, c[0].lower().replace(" ", "_") + ".json")
def fetch():
    os.makedirs(RAW, exist_ok=True)
    for c in CITIES:
        f = fname(c)
        if os.path.exists(f): continue
        q = f'[out:json][timeout:90];nwr(around:700,{c[1]},{c[2]})["addr:housenumber"]["addr:street"];out tags 400;'
        for attempt in range(4):
            try:
                ep = ["https://maps.mail.ru/osm/tools/overpass/api/interpreter", "https://overpass-api.de/api/interpreter"][attempt % 2]   # main server returned 504s on 2026-10-01
                req = urllib.request.Request(ep, data=urllib.parse.urlencode({"data": q}).encode(),
                    headers={"User-Agent": "libpostal-runtime-research/1.0 (academic benchmark)", "Accept": "application/json"})
                body = urllib.request.urlopen(req, timeout=120).read()
                json.loads(body); open(f, "wb").write(body); print("ok", c[0], len(json.loads(body)["elements"]), flush=True); break
            except Exception as e:
                print("retry", c[0], attempt, repr(e)[:80], flush=True); time.sleep(20 * (attempt + 1))
        time.sleep(4)
def assemble(t, c):
    hn, st = t.get("addr:housenumber", "").strip(), t.get("addr:street", "").strip()
    pc, city, state = t.get("addr:postcode", "").strip(), (t.get("addr:city") or c[0]).strip(), t.get("addr:state", "").strip()
    if not hn or not st: return None
    k = c[4]
    if k == "E": s = f"{st} {hn}, {pc} {city}" if pc else f"{st} {hn}, {city}"
    elif k == "F": s = f"{hn} {st}, {pc} {city}" if pc else f"{hn} {st}, {city}"
    elif k == "A": tail = " ".join(x for x in (state, pc) if x); s = f"{hn} {st}, {city}" + (f" {tail}" if tail else "")
    else: s = " ".join(x for x in (pc, city, st, hn) if x)
    s = " ".join(s.split())
    return s if s and "\t" not in s and "\n" not in s else None
def build():
    seen = set()
    for f in ("../golden-v1/all.txt",):
        seen |= {l.rstrip("\n") for l in open(os.path.join(HERE, f), encoding="utf-8")}
    for l in open(os.path.join(HERE, "../golden-v2-novel/novel.tsv"), encoding="utf-8"): seen.add(l.rstrip("\n").split("\t", 1)[1])
    rows, dropped_overlap = [], 0
    for c in CITIES:
        f = fname(c)
        if not os.path.exists(f): print("missing", c[0]); continue
        cand = {}
        for e in json.load(open(f, encoding="utf-8"))["elements"]:
            s = assemble(e.get("tags", {}), c)
            if s: cand[s] = 1
        picked = sorted(cand, key=lambda s: hashlib.sha256(s.encode()).hexdigest())
        kept = []
        for s in picked:
            if s in seen: dropped_overlap += 1; continue
            kept.append(s); seen.add(s)
            if len(kept) >= PER_CITY: break
        rows += [(c[3], s) for s in kept]
    rows.sort(key=lambda r: hashlib.sha256(r[1].encode()).hexdigest())
    with open(os.path.join(HERE, "osm.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "osm.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
