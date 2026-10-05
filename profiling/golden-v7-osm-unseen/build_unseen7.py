"""Golden v7 (fourth UNSEEN set, built 2026-10-03; never used for tuning, screening or selection). Copy of golden-v6's builder with cities disjoint from v3-v6; strings in v1/novel/v3-v6 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Charlotte",35.2271,-80.8431,"us","A"),("San Diego",32.7157,-117.1611,"us","A"),("Columbus",39.9612,-82.9988,"us","A"),("New Orleans",29.9511,-90.0715,"us","A"),
 ("Edmonton",53.5461,-113.4938,"ca","A"),("Victoria",48.4284,-123.3656,"ca","A"),("Birmingham",52.4862,-1.8904,"gb","A"),("Newcastle",54.9783,-1.6178,"gb","A"),
 ("Waterford",52.2593,-7.1101,"ie","A"),("Canberra",-35.2809,149.1300,"au","A"),("Dunedin",-45.8788,170.5028,"nz","A"),("Port Elizabeth",-33.9608,25.6022,"za","A"),
 ("Grenoble",45.1885,5.7245,"fr","F"),("Dijon",47.3220,5.0415,"fr","F"),("Reims",49.2583,4.0317,"fr","F"),("Namur",50.4674,4.8720,"be","F"),
 ("Mannheim",49.4875,8.4660,"de","E"),("Bonn",50.7374,7.0982,"de","E"),("Munster",51.9607,7.6261,"de","E"),("Linz",48.3069,14.2858,"at","E"),
 ("Lausanne",46.5197,6.6323,"ch","E"),("Groningen",53.2194,6.5665,"nl","E"),("Esbjerg",55.4765,8.4594,"dk","E"),("Linkoping",58.4108,15.6214,"se","E"),
 ("Tromso",69.6492,18.9553,"no","E"),("Jyvaskyla",62.2426,25.7473,"fi","E"),("Szczecin",53.4285,14.5528,"pl","E"),("Bydgoszcz",53.1235,18.0084,"pl","E"),
 ("Olomouc",49.5938,17.2509,"cz","E"),("Miskolc",48.1035,20.7784,"hu","E"),("Zadar",44.1194,15.2314,"hr","E"),("Verona",45.4384,10.9916,"it","E"),
 ("Bari",41.1171,16.8719,"it","E"),("Murcia",37.9922,-1.1307,"es","E"),("Valladolid",41.6523,-4.7245,"es","E"),("Faro",37.0194,-7.9322,"pt","E"),
 ("Larissa",39.6390,22.4191,"gr","E"),("Brasov",45.6427,25.5887,"ro","E"),("Ruse",43.8356,25.9657,"bg","E"),("Antalya",36.8969,30.7133,"tr","E"),
 ("Dnipro",48.4647,35.0462,"ua","E"),("Yekaterinburg",56.8389,60.6057,"ru","E"),("Jelgava",56.6511,23.7214,"lv","E"),("Panevezys",55.7348,24.3575,"lt","E"),
 ("Recife",-8.0476,-34.8770,"br","E"),("La Plata",-34.9205,-57.9536,"ar","E"),("Merida",20.9674,-89.5926,"mx","E"),("Temuco",-38.7359,-72.5904,"cl","E"),
 ("Bucaramanga",7.1193,-73.1227,"co","E"),("Trujillo",-8.1116,-79.0288,"pe","E"),
 ("Kobe",34.6901,135.1955,"jp","J"),("Kanazawa",36.5613,136.6562,"jp","J"),("Hsinchu",24.8138,120.9675,"tw","J"),("Gwangju",35.1595,126.8526,"kr","J"),
 ("Kiryat Ata",32.8156,35.0369,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt", "../golden-v5-osm-unseen/unseen5.txt", "../golden-v6-osm-unseen/unseen6.txt"):   # also disjoint from golden-v3..v6
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
    with open(os.path.join(HERE, "unseen7.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen7.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
