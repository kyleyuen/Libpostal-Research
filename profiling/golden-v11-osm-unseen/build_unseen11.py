"""Golden v11 (eighth UNSEEN set, built 2026-10-03; never used for tuning, screening or selection). Copy of golden-v7's builder with cities disjoint from v3-v10; strings in v1/novel/v3-v10 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Boise",43.6150,-116.2023,"us","A"),("Spokane",47.6588,-117.4260,"us","A"),("Madison",43.0731,-89.4012,"us","A"),("Hartford",41.7658,-72.6734,"us","A"),
 ("Savannah",32.0809,-81.0912,"us","A"),("Moncton",46.0878,-64.7782,"ca","A"),("Exeter",50.7184,-3.5339,"gb","A"),("Aberdeen",57.1497,-2.0943,"gb","A"),
 ("Wexford",52.3369,-6.4633,"ie","A"),("Cairns",-16.9186,145.7781,"au","A"),("Rotorua",-38.1368,176.2497,"nz","A"),("Kimberley",-28.7282,24.7499,"za","A"),
 ("Poitiers",46.5802,0.3404,"fr","F"),("Besancon",47.2378,6.0241,"fr","F"),("Caen",49.1829,-0.3707,"fr","F"),("Leuven",50.8798,4.7005,"be","F"),
 ("Potsdam",52.3906,13.0645,"de","E"),("Wurzburg",49.7913,9.9534,"de","E"),("Trier",49.7499,6.6371,"de","E"),("Villach",46.6111,13.8558,"at","E"),
 ("Lugano",46.0037,8.9511,"ch","E"),("Leeuwarden",53.2012,5.7999,"nl","E"),("Randers",56.4607,10.0364,"dk","E"),("Umea",63.8258,20.2630,"se","E"),
 ("Alesund",62.4722,6.1495,"no","E"),("Vaasa",63.0951,21.6165,"fi","E"),("Bialystok",53.1325,23.1688,"pl","E"),("Radom",51.4027,21.1471,"pl","E"),
 ("Ceske Budejovice",48.9745,14.4743,"cz","E"),("Sopron",47.6817,16.5845,"hu","E"),("Sibenik",43.7350,15.8952,"hr","E"),("Udine",46.0711,13.2346,"it","E"),
 ("Siena",43.3188,11.3308,"it","E"),("Burgos",42.3439,-3.6969,"es","E"),("Girona",41.9794,2.8214,"es","E"),("Evora",38.5714,-7.9135,"pt","E"),
 ("Chania",35.5138,24.0180,"gr","E"),("Sibiu",45.7983,24.1256,"ro","E"),("Veliko Tarnovo",43.0757,25.6172,"bg","E"),("Trabzon",41.0027,39.7168,"tr","E"),
 ("Poltava",49.5883,34.5514,"ua","E"),("Voronezh",51.6720,39.1843,"ru","E"),("Jurmala",56.9680,23.7704,"lv","E"),("Palanga",55.9175,21.0686,"lt","E"),
 ("Salvador",-12.9777,-38.5016,"br","E"),("Tucuman",-26.8083,-65.2176,"ar","E"),("Oaxaca",17.0732,-96.7266,"mx","E"),("Iquique",-20.2307,-70.1357,"cl","E"),
 ("Santa Marta",11.2408,-74.1990,"co","E"),("Huancayo",-12.0651,-75.2049,"pe","E"),
 ("Nagasaki",32.7503,129.8779,"jp","J"),("Kochi",33.5597,133.5311,"jp","J"),("Hualien",23.9872,121.6015,"tw","J"),("Pohang",36.0190,129.3435,"kr","J"),
 ("Eilat",29.5577,34.9519,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt", "../golden-v5-osm-unseen/unseen5.txt", "../golden-v6-osm-unseen/unseen6.txt", "../golden-v7-osm-unseen/unseen7.txt", "../golden-v8-osm-unseen/unseen8.txt", "../golden-v9-osm-unseen/unseen9.txt", "../golden-v10-osm-unseen/unseen10.txt"):   # also disjoint from golden-v3..v6
        for l in open(os.path.join(HERE, f), encoding="utf-8"): seen.add(l.rstrip("\n"))
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
    with open(os.path.join(HERE, "unseen11.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen11.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
