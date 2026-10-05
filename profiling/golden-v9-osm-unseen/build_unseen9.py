"""Golden v9 (sixth UNSEEN set, built 2026-10-03; never used for tuning, screening or selection). Copy of golden-v7's builder with cities disjoint from v3-v8; strings in v1/novel/v3-v8 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Salt Lake City",40.7608,-111.8910,"us","A"),("Tampa",27.9506,-82.4572,"us","A"),("Indianapolis",39.7684,-86.1581,"us","A"),("Detroit",42.3314,-83.0458,"us","A"),
 ("Louisville",38.2527,-85.7585,"us","A"),("Regina",50.4452,-104.6189,"ca","A"),("Leicester",52.6369,-1.1398,"gb","A"),("Southampton",50.9097,-1.4044,"gb","A"),
 ("Sligo",54.2766,-8.4761,"ie","A"),("Darwin",-12.4634,130.8456,"au","A"),("Nelson",-41.2706,173.2840,"nz","A"),("Pietermaritzburg",-29.6006,30.3794,"za","A"),
 ("Clermont-Ferrand",45.7772,3.0870,"fr","F"),("Le Mans",48.0061,0.1996,"fr","F"),("Brest",48.3904,-4.4861,"fr","F"),("Charleroi",50.4108,4.4446,"be","F"),
 ("Freiburg",47.9990,7.8421,"de","E"),("Regensburg",49.0134,12.1016,"de","E"),("Rostock",54.0924,12.0991,"de","E"),("St. Polten",48.2047,15.6256,"at","E"),
 ("St. Gallen",47.4245,9.3767,"ch","E"),("Maastricht",50.8514,5.6910,"nl","E"),("Vejle",55.7113,9.5357,"dk","E"),("Vasteras",59.6099,16.5448,"se","E"),
 ("Kristiansand",58.1467,7.9956,"no","E"),("Kuopio",62.8924,27.6770,"fi","E"),("Kielce",50.8661,20.6286,"pl","E"),("Olsztyn",53.7784,20.4801,"pl","E"),
 ("Liberec",50.7671,15.0562,"cz","E"),("Nyiregyhaza",47.9554,21.7167,"hu","E"),("Varazdin",46.3057,16.3366,"hr","E"),("Trieste",45.6495,13.7768,"it","E"),
 ("Parma",44.8015,10.3279,"it","E"),("Pamplona",42.8125,-1.6458,"es","E"),("Santander",43.4623,-3.8099,"es","E"),("Funchal",32.6669,-16.9241,"pt","E"),
 ("Ioannina",39.6650,20.8537,"gr","E"),("Craiova",44.3302,23.7949,"ro","E"),("Pleven",43.4170,24.6067,"bg","E"),("Konya",37.8746,32.4932,"tr","E"),
 ("Vinnytsia",49.2331,28.4682,"ua","E"),("Samara",53.1959,50.1002,"ru","E"),("Rezekne",56.5099,27.3331,"lv","E"),("Marijampole",54.5593,23.3541,"lt","E"),
 ("Fortaleza",-3.7319,-38.5267,"br","E"),("Salta",-24.7821,-65.4232,"ar","E"),("Leon",21.1250,-101.6860,"mx","E"),("La Serena",-29.9027,-71.2519,"cl","E"),
 ("Manizales",5.0703,-75.5138,"co","E"),("Piura",-5.1945,-80.6328,"pe","E"),
 ("Okayama",34.6551,133.9195,"jp","J"),("Kumamoto",32.8031,130.7079,"jp","J"),("Chiayi",23.4801,120.4491,"tw","J"),("Ulsan",35.5384,129.3114,"kr","J"),
 ("Rishon LeZion",31.9730,34.7925,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt", "../golden-v5-osm-unseen/unseen5.txt", "../golden-v6-osm-unseen/unseen6.txt", "../golden-v7-osm-unseen/unseen7.txt", "../golden-v8-osm-unseen/unseen8.txt"):   # also disjoint from golden-v3..v6
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
    with open(os.path.join(HERE, "unseen9.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen9.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
