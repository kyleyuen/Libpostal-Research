"""Golden v13 (tenth UNSEEN set, fetch started 2026-10-04 for REWRITE_PLAN M5; never used for tuning, screening or selection). Copy of golden-v12's builder with 65 cities disjoint from v3-v12 (checked by script); strings in v1/novel/v3-v12 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Tulsa",36.1540,-95.9928,"us","A"),("Wichita",37.6872,-97.3301,"us","A"),("Chattanooga",35.0456,-85.3097,"us","A"),("Syracuse",43.0481,-76.1474,"us","A"),
 ("Reno",39.5296,-119.8138,"us","A"),("Kelowna",49.8880,-119.4960,"ca","A"),("Thunder Bay",48.3809,-89.2477,"ca","A"),("Norwich",52.6309,1.2974,"gb","A"),
 ("Bath",51.3811,-2.3590,"gb","A"),("Drogheda",53.7179,-6.3561,"ie","A"),("Ballarat",-37.5622,143.8503,"au","A"),("Bendigo",-36.7570,144.2794,"au","A"),
 ("Whangarei",-35.7251,174.3237,"nz","A"),("Stellenbosch",-33.9321,18.8602,"za","A"),
 ("Rouen",49.4432,1.0999,"fr","F"),("Avignon",43.9493,4.8055,"fr","F"),("Annecy",45.8992,6.1294,"fr","F"),("Verviers",50.5891,5.8623,"be","F"),
 ("Gottingen",51.5413,9.9158,"de","E"),("Heidelberg",49.3988,8.6724,"de","E"),("Bamberg",49.8988,10.9028,"de","E"),("Wiener Neustadt",47.8151,16.2465,"at","E"),
 ("Biel",47.1368,7.2468,"ch","E"),("Delft",52.0116,4.3571,"nl","E"),("Breda",51.5719,4.7683,"nl","E"),("Svendborg",55.0598,10.6068,"dk","E"),
 ("Lund",55.7047,13.1910,"se","E"),("Gavle",60.6749,17.1413,"se","E"),("Tonsberg",59.2675,10.4076,"no","E"),("Lappeenranta",61.0587,28.1887,"fi","E"),
 ("Gniezno",52.5348,17.5826,"pl","E"),("Legnica",51.2070,16.1619,"pl","E"),("Jihlava",49.3961,15.5912,"cz","E"),("Szekesfehervar",47.1860,18.4221,"hu","E"),
 ("Slavonski Brod",45.1603,18.0156,"hr","E"),("Modena",44.6471,10.9252,"it","E"),("Pisa",43.7228,10.4017,"it","E"),("Trento",46.0748,11.1217,"it","E"),
 ("Almeria",36.8340,-2.4637,"es","E"),("Caceres",39.4753,-6.3724,"es","E"),("Leiria",39.7436,-8.8071,"pt","E"),("Chalkida",38.4636,23.5994,"gr","E"),
 ("Arad",46.1866,21.3123,"ro","E"),("Haskovo",41.9344,25.5554,"bg","E"),("Kayseri",38.7205,35.4826,"tr","E"),("Lutsk",50.7472,25.3254,"ua","E"),
 ("Tver",56.8587,35.9176,"ru","E"),("Tukums",56.9670,23.1554,"lv","E"),("Taurage",55.2522,22.2897,"lt","E"),("Zilina",49.2231,18.7394,"sk","E"),
 ("Maribor",46.5547,15.6459,"si","E"),("Parnu",58.3859,24.4971,"ee","E"),("Londrina",-23.3045,-51.1696,"br","E"),("Joinville",-26.3045,-48.8487,"br","E"),
 ("Santa Fe",-31.6107,-60.6973,"ar","E"),("Chihuahua",28.6353,-106.0889,"mx","E"),("Talca",-35.4264,-71.6554,"cl","E"),("Pasto",1.2136,-77.2811,"co","E"),
 ("Ica",-14.0678,-75.7286,"pe","E"),("Salto",-31.3833,-57.9667,"uy","E"),
 ("Kagoshima",31.5966,130.5571,"jp","J"),("Utsunomiya",36.5551,139.8828,"jp","J"),("Changhua",24.0809,120.5385,"tw","J"),("Changwon",35.2280,128.6811,"kr","J"),
 ("Rehovot",31.8928,34.8113,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt", "../golden-v5-osm-unseen/unseen5.txt", "../golden-v6-osm-unseen/unseen6.txt", "../golden-v7-osm-unseen/unseen7.txt", "../golden-v8-osm-unseen/unseen8.txt", "../golden-v9-osm-unseen/unseen9.txt", "../golden-v10-osm-unseen/unseen10.txt", "../golden-v11-osm-unseen/unseen11.txt", "../golden-v12-osm-unseen/unseen12.txt"):   # also disjoint from golden-v3..v6
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
    with open(os.path.join(HERE, "unseen13.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen13.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
