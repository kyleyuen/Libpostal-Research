"""Golden v12 (ninth UNSEEN set, built 2026-10-03; never used for tuning, screening or selection). Copy of golden-v7's builder with cities disjoint from v3-v11; strings in v1/novel/v3-v11 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Fresno",36.7378,-119.7871,"us","A"),("Little Rock",34.7465,-92.2896,"us","A"),("Des Moines",41.5868,-93.6250,"us","A"),("Providence",41.8240,-71.4128,"us","A"),
 ("Anchorage",61.2181,-149.9003,"us","A"),("Sherbrooke",45.4042,-71.8929,"ca","A"),("York",53.9600,-1.0873,"gb","A"),("Swansea",51.6214,-3.9436,"gb","A"),
 ("Tralee",52.2713,-9.6999,"ie","A"),("Toowoomba",-27.5598,151.9507,"au","A"),("Palmerston North",-40.3523,175.6082,"nz","A"),("Polokwane",-23.9045,29.4689,"za","A"),
 ("Nancy",48.6921,6.1844,"fr","F"),("Orleans",47.9030,1.9093,"fr","F"),("Pau",43.2951,-0.3708,"fr","F"),("Tournai",50.6056,3.3878,"be","F"),
 ("Oldenburg",53.1435,8.2146,"de","E"),("Jena",50.9271,11.5892,"de","E"),("Koblenz",50.3569,7.5890,"de","E"),("Dornbirn",47.4125,9.7417,"at","E"),
 ("Fribourg",46.8065,7.1620,"ch","E"),("Zwolle",52.5168,6.0830,"nl","E"),("Kolding",55.4904,9.4722,"dk","E"),("Jonkoping",57.7826,14.1618,"se","E"),
 ("Fredrikstad",59.2181,10.9298,"no","E"),("Joensuu",62.6010,29.7636,"fi","E"),("Zielona Gora",51.9356,15.5062,"pl","E"),("Tarnow",50.0121,20.9858,"pl","E"),
 ("Zlin",49.2265,17.6707,"cz","E"),("Eger",47.9025,20.3772,"hu","E"),("Karlovac",45.4929,15.5553,"hr","E"),("Bergamo",45.6983,9.6773,"it","E"),
 ("Ravenna",44.4184,12.2035,"it","E"),("Logrono",42.4627,-2.4449,"es","E"),("Badajoz",38.8794,-6.9707,"es","E"),("Setubal",38.5244,-8.8882,"pt","E"),
 ("Kavala",40.9396,24.4069,"gr","E"),("Galati",45.4353,28.0080,"ro","E"),("Shumen",43.2712,26.9361,"bg","E"),("Samsun",41.2867,36.3300,"tr","E"),
 ("Ternopil",49.5535,25.5948,"ua","E"),("Kaliningrad",54.7104,20.4522,"ru","E"),("Cesis",57.3119,25.2706,"lv","E"),("Telsiai",55.9814,22.2472,"lt","E"),
 ("Natal",-5.7945,-35.2110,"br","E"),("Neuquen",-38.9516,-68.0591,"ar","E"),("Hermosillo",29.0729,-110.9559,"mx","E"),("Rancagua",-34.1708,-70.7444,"cl","E"),
 ("Armenia",4.5339,-75.6811,"co","E"),("Tacna",-18.0146,-70.2536,"pe","E"),
 ("Akita",39.7200,140.1025,"jp","J"),("Toyama",36.6959,137.2137,"jp","J"),("Yilan",24.7570,121.7533,"tw","J"),("Cheongju",36.6424,127.4890,"kr","J"),
 ("Nahariya",33.0089,35.0981,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt", "../golden-v5-osm-unseen/unseen5.txt", "../golden-v6-osm-unseen/unseen6.txt", "../golden-v7-osm-unseen/unseen7.txt", "../golden-v8-osm-unseen/unseen8.txt", "../golden-v9-osm-unseen/unseen9.txt", "../golden-v10-osm-unseen/unseen10.txt", "../golden-v11-osm-unseen/unseen11.txt"):   # also disjoint from golden-v3..v6
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
    with open(os.path.join(HERE, "unseen12.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen12.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
