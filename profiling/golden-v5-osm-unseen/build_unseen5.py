"""Golden v5 (second UNSEEN set, built 2026-10-03 overnight; never used for tuning, screening or selection). Copy of golden-v4's builder with a city list disjoint from v3 and v4; strings in v1/novel/v3/v4 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Atlanta",33.7490,-84.3880,"us","A"),("Austin",30.2672,-97.7431,"us","A"),("Portland",45.5152,-122.6784,"us","A"),("Pittsburgh",40.4406,-79.9959,"us","A"),
 ("Calgary",51.0447,-114.0719,"ca","A"),("Halifax",44.6488,-63.5752,"ca","A"),("Glasgow",55.8642,-4.2518,"gb","A"),("Leeds",53.8008,-1.5491,"gb","A"),
 ("Galway",53.2707,-9.0568,"ie","A"),("Adelaide",-34.9285,138.6007,"au","A"),("Christchurch",-43.5321,172.6362,"nz","A"),("Durban",-29.8587,31.0218,"za","A"),
 ("Bordeaux",44.8378,-0.5792,"fr","F"),("Nantes",47.2184,-1.5536,"fr","F"),("Strasbourg",48.5734,7.7521,"fr","F"),("Ghent",51.0543,3.7174,"be","F"),
 ("Stuttgart",48.7758,9.1829,"de","E"),("Dresden",51.0504,13.7373,"de","E"),("Bremen",53.0793,8.8017,"de","E"),("Salzburg",47.8095,13.0550,"at","E"),
 ("Basel",47.5596,7.5886,"ch","E"),("The Hague",52.0705,4.3007,"nl","E"),("Odense",55.4038,10.4024,"dk","E"),("Malmo",55.6050,13.0038,"se","E"),
 ("Trondheim",63.4305,10.3951,"no","E"),("Turku",60.4518,22.2666,"fi","E"),("Poznan",52.4064,16.9252,"pl","E"),("Lodz",51.7592,19.4560,"pl","E"),
 ("Ostrava",49.8209,18.2625,"cz","E"),("Szeged",46.2530,20.1414,"hu","E"),("Rijeka",45.3271,14.4422,"hr","E"),("Bologna",44.4949,11.3426,"it","E"),
 ("Florence",43.7696,11.2558,"it","E"),("Bilbao",43.2630,-2.9350,"es","E"),("Malaga",36.7213,-4.4214,"es","E"),("Braga",41.5454,-8.4265,"pt","E"),
 ("Patras",38.2466,21.7346,"gr","E"),("Iasi",47.1585,27.6014,"ro","E"),("Varna",43.2141,27.9147,"bg","E"),("Ankara",39.9334,32.8597,"tr","E"),
 ("Odesa",46.4825,30.7233,"ua","E"),("Kazan",55.7963,49.1088,"ru","E"),("Daugavpils",55.8714,26.5161,"lv","E"),("Klaipeda",55.7033,21.1443,"lt","E"),
 ("Curitiba",-25.4284,-49.2733,"br","E"),("Rosario",-32.9442,-60.6505,"ar","E"),("Monterrey",25.6866,-100.3161,"mx","E"),("Valparaiso",-33.0472,-71.6127,"cl","E"),
 ("Cali",3.4516,-76.5320,"co","E"),("Quito",-0.1807,-78.4678,"ec","E"),
 ("Sapporo",43.0618,141.3545,"jp","J"),("Kyoto",35.0116,135.7681,"jp","J"),("Taichung",24.1477,120.6736,"tw","J"),("Daegu",35.8714,128.6014,"kr","J"),
 ("Jerusalem",31.7683,35.2137,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt"):   # also disjoint from golden-v3 and golden-v4
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
    with open(os.path.join(HERE, "unseen5.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen5.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
