"""Golden v10 (seventh UNSEEN set, built 2026-10-03; never used for tuning, screening or selection). Copy of golden-v7's builder with cities disjoint from v3-v9; strings in v1/novel/v3-v9 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Omaha",41.2565,-95.9345,"us","A"),("Albuquerque",35.0844,-106.6504,"us","A"),("Richmond",37.5407,-77.4360,"us","A"),("Buffalo",42.8864,-78.8784,"us","A"),
 ("Tucson",32.2226,-110.9747,"us","A"),("Kingston",44.2312,-76.4860,"ca","A"),("Brighton",50.8225,-0.1372,"gb","A"),("Plymouth",50.3755,-4.1427,"gb","A"),
 ("Athlone",53.4239,-7.9407,"ie","A"),("Townsville",-19.2590,146.8169,"au","A"),("Napier",-39.4928,176.9120,"nz","A"),("East London",-33.0153,27.9116,"za","A"),
 ("Limoges",45.8336,1.2611,"fr","F"),("Amiens",49.8941,2.2958,"fr","F"),("Perpignan",42.6887,2.8948,"fr","F"),("Bruges",51.2093,3.2247,"be","F"),
 ("Kassel",51.3127,9.4797,"de","E"),("Erfurt",50.9848,11.0299,"de","E"),("Ulm",48.4011,9.9876,"de","E"),("Wels",48.1575,14.0289,"at","E"),
 ("Winterthur",47.5001,8.7240,"ch","E"),("Nijmegen",51.8126,5.8372,"nl","E"),("Horsens",55.8607,9.8503,"dk","E"),("Norrkoping",58.5877,16.1924,"se","E"),
 ("Bodo",67.2804,14.4049,"no","E"),("Pori",61.4851,21.7974,"fi","E"),("Gliwice",50.2945,18.6714,"pl","E"),("Opole",50.6751,17.9213,"pl","E"),
 ("Pardubice",50.0343,15.7812,"cz","E"),("Kecskemet",46.8964,19.6897,"hu","E"),("Dubrovnik",42.6507,18.0944,"hr","E"),("Lecce",40.3515,18.1750,"it","E"),
 ("Perugia",43.1107,12.3908,"it","E"),("Oviedo",43.3614,-5.8593,"es","E"),("Cadiz",36.5271,-6.2886,"es","E"),("Viseu",40.6566,-7.9125,"pt","E"),
 ("Kalamata",37.0389,22.1142,"gr","E"),("Oradea",47.0465,21.9189,"ro","E"),("Sliven",42.6817,26.3229,"bg","E"),("Gaziantep",37.0662,37.3833,"tr","E"),
 ("Chernivtsi",48.2915,25.9403,"ua","E"),("Tula",54.1931,37.6173,"ru","E"),("Valmiera",57.5385,25.4263,"lv","E"),("Utena",55.4977,25.5992,"lt","E"),
 ("Florianopolis",-27.5954,-48.5480,"br","E"),("Bahia Blanca",-38.7196,-62.2724,"ar","E"),("Morelia",19.7060,-101.1950,"mx","E"),("Valdivia",-39.8142,-73.2459,"cl","E"),
 ("Cartagena",10.3910,-75.4794,"co","E"),("Chiclayo",-6.7714,-79.8409,"pe","E"),
 ("Niigata",37.9161,139.0364,"jp","J"),("Matsuyama",33.8392,132.7657,"jp","J"),("Taitung",22.7583,121.1444,"tw","J"),("Jeonju",35.8242,127.1480,"kr","J"),
 ("Ashdod",31.8014,34.6435,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt", "../golden-v5-osm-unseen/unseen5.txt", "../golden-v6-osm-unseen/unseen6.txt", "../golden-v7-osm-unseen/unseen7.txt", "../golden-v8-osm-unseen/unseen8.txt", "../golden-v9-osm-unseen/unseen9.txt"):   # also disjoint from golden-v3..v6
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
    with open(os.path.join(HERE, "unseen10.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen10.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
