"""Golden v4 (UNSEEN set, built 2026-10-03 for the user-requested re-measure; never used for tuning, screening or selection). Copy of golden-v3's builder with a disjoint city list.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Boston",42.3601,-71.0589,"us","A"),("San Francisco",37.7749,-122.4194,"us","A"),("Philadelphia",39.9526,-75.1652,"us","A"),("Denver",39.7392,-104.9903,"us","A"),
 ("Minneapolis",44.9778,-93.2650,"us","A"),("Vancouver",49.2827,-123.1207,"ca","A"),("Ottawa",45.4215,-75.6972,"ca","A"),("Edinburgh",55.9533,-3.1883,"gb","A"),
 ("Bristol",51.4545,-2.5879,"gb","A"),("Cork",51.8985,-8.4756,"ie","A"),("Brisbane",-27.4698,153.0251,"au","A"),("Perth",-31.9523,115.8613,"au","A"),
 ("Wellington",-41.2865,174.7762,"nz","A"),("Johannesburg",-26.2041,28.0473,"za","A"),
 ("Marseille",43.2965,5.3698,"fr","F"),("Toulouse",43.6047,1.4442,"fr","F"),("Lille",50.6292,3.0573,"fr","F"),("Antwerp",51.2194,4.4025,"be","F"),
 ("Cologne",50.9375,6.9603,"de","E"),("Frankfurt",50.1109,8.6821,"de","E"),("Leipzig",51.3397,12.3731,"de","E"),("Graz",47.0707,15.4395,"at","E"),
 ("Geneva",46.2044,6.1432,"ch","E"),("Utrecht",52.0907,5.1214,"nl","E"),("Aarhus",56.1629,10.2039,"dk","E"),("Gothenburg",57.7089,11.9746,"se","E"),
 ("Bergen",60.3913,5.3221,"no","E"),("Tampere",61.4978,23.7610,"fi","E"),("Gdansk",54.3520,18.6466,"pl","E"),("Wroclaw",51.1079,17.0385,"pl","E"),
 ("Brno",49.1951,16.6068,"cz","E"),("Kosice",48.7164,21.2611,"sk","E"),("Debrecen",47.5316,21.6273,"hu","E"),("Split",43.5081,16.4402,"hr","E"),
 ("Turin",45.0703,7.6869,"it","E"),("Naples",40.8518,14.2681,"it","E"),("Valencia",39.4699,-0.3763,"es","E"),("Seville",37.3891,-5.9845,"es","E"),
 ("Porto",41.1579,-8.6291,"pt","E"),("Thessaloniki",40.6401,22.9444,"gr","E"),("Cluj-Napoca",46.7712,23.6236,"ro","E"),("Plovdiv",42.1354,24.7453,"bg","E"),
 ("Izmir",38.4237,27.1428,"tr","E"),("Lviv",49.8397,24.0297,"ua","E"),("Saint Petersburg",59.9311,30.3609,"ru","E"),("Kaunas",54.8985,23.9036,"lt","E"),
 ("Tartu",58.3776,26.7290,"ee","E"),("Rio de Janeiro",-22.9068,-43.1729,"br","E"),("Cordoba",-31.4201,-64.1888,"ar","E"),("Guadalajara",20.6597,-103.3496,"mx","E"),
 ("Medellin",6.2442,-75.5812,"co","E"),("Lima",-12.0464,-77.0428,"pe","E"),("Montevideo",-34.9011,-56.1645,"uy","E"),
 ("Nagoya",35.1815,136.9066,"jp","J"),("Fukuoka",33.5904,130.4017,"jp","J"),("Kaohsiung",22.6273,120.3014,"tw","J"),("Busan",35.1796,129.0756,"kr","J"),
 ("Haifa",32.7940,34.9896,"il","E"),
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
    for l in open(os.path.join(HERE, "../golden-v3-osm/osm.txt"), encoding="utf-8"): seen.add(l.rstrip("\n"))   # also disjoint from golden-v3
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
    with open(os.path.join(HERE, "unseen.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
