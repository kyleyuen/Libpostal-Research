"""Golden v8 (fifth UNSEEN set, built 2026-10-03; never used for tuning, screening or selection). Copy of golden-v7's builder with cities disjoint from v3-v7; strings in v1/novel/v3-v7 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Sacramento",38.5816,-121.4944,"us","A"),("Kansas City",39.0997,-94.5786,"us","A"),("Cincinnati",39.1031,-84.5120,"us","A"),("Milwaukee",43.0389,-87.9065,"us","A"),
 ("Raleigh",35.7796,-78.6382,"us","A"),("Saskatoon",52.1332,-106.6700,"ca","A"),("Sheffield",53.3811,-1.4701,"gb","A"),("Nottingham",52.9548,-1.1581,"gb","A"),
 ("Kilkenny",52.6541,-7.2448,"ie","A"),("Geelong",-38.1499,144.3617,"au","A"),("Tauranga",-37.6878,176.1651,"nz","A"),("Bloemfontein",-29.0852,26.1596,"za","A"),
 ("Angers",47.4784,-0.5632,"fr","F"),("Tours",47.3941,0.6848,"fr","F"),("Metz",49.1193,6.1757,"fr","F"),("Mons",50.4542,3.9567,"be","F"),
 ("Karlsruhe",49.0069,8.4037,"de","E"),("Augsburg",48.3705,10.8978,"de","E"),("Kiel",54.3233,10.1228,"de","E"),("Klagenfurt",46.6247,14.3053,"at","E"),
 ("Lucerne",47.0502,8.3093,"ch","E"),("Haarlem",52.3874,4.6462,"nl","E"),("Roskilde",55.6415,12.0803,"dk","E"),("Orebro",59.2753,15.2134,"se","E"),
 ("Drammen",59.7439,10.2045,"no","E"),("Lahti",60.9827,25.6612,"fi","E"),("Torun",53.0138,18.5984,"pl","E"),("Rzeszow",50.0412,21.9991,"pl","E"),
 ("Hradec Kralove",50.2092,15.8328,"cz","E"),("Gyor",47.6875,17.6504,"hu","E"),("Pula",44.8666,13.8496,"hr","E"),("Padua",45.4064,11.8768,"it","E"),
 ("Catania",37.5079,15.0830,"it","E"),("Granada",37.1773,-3.5986,"es","E"),("Salamanca",40.9701,-5.6635,"es","E"),("Aveiro",40.6405,-8.6538,"pt","E"),
 ("Volos",39.3622,22.9420,"gr","E"),("Constanta",44.1598,28.6348,"ro","E"),("Stara Zagora",42.4258,25.6345,"bg","E"),("Eskisehir",39.7767,30.5206,"tr","E"),
 ("Zaporizhzhia",47.8388,35.1396,"ua","E"),("Nizhny Novgorod",56.2965,43.9361,"ru","E"),("Ventspils",57.3894,21.5606,"lv","E"),("Alytus",54.3964,24.0414,"lt","E"),
 ("Belo Horizonte",-19.9167,-43.9345,"br","E"),("Mar del Plata",-38.0055,-57.5426,"ar","E"),("Queretaro",20.5888,-100.3899,"mx","E"),("Antofagasta",-23.6509,-70.3975,"cl","E"),
 ("Pereira",4.8133,-75.6961,"co","E"),("Cusco",-13.5320,-71.9675,"pe","E"),
 ("Yokohama",35.4437,139.6380,"jp","J"),("Hamamatsu",34.7108,137.7261,"jp","J"),("Keelung",25.1276,121.7392,"tw","J"),("Daejeon",36.3504,127.3845,"kr","J"),
 ("Netanya",32.3215,34.8532,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt", "../golden-v5-osm-unseen/unseen5.txt", "../golden-v6-osm-unseen/unseen6.txt", "../golden-v7-osm-unseen/unseen7.txt"):   # also disjoint from golden-v3..v6
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
    with open(os.path.join(HERE, "unseen8.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen8.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
