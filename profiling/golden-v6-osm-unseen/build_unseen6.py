"""Golden v6 (third UNSEEN set, built 2026-10-03; never used for tuning, screening or selection). Copy of golden-v5's builder with cities disjoint from v3, v4 and v5; strings in v1/novel/v3/v4/v5 removed.
Golden v3 (independent held-out set): real addresses fetched from OpenStreetMap via the Overpass API (not from the Senzing data behind golden-v1 and
the novel set). For each city: up to 400 tagged objects with addr:housenumber + addr:street within 700 m of the centre (2 km timed out on dense cities); addresses are assembled from the
tags in the country's usual order (template per country, below); a deterministic hash keeps at most PER_CITY distinct addresses per city; strings
that appear in golden-v1 or the novel set are dropped. Raw JSON responses are kept in raw/. Data (c) OpenStreetMap contributors, ODbL.
usage: python3 build_osm.py fetch   (downloads raw/, polite pacing)   |   python3 build_osm.py build   (writes osm.tsv, osm.txt, SHA256SUMS)"""
import hashlib, json, os, sys, time, urllib.parse, urllib.request
PER_CITY = 200
# (city, lat, lon, cc, template); templates: E = "{street} {hn}, {pc} {city}", F = "{hn} {street}, {pc} {city}", A = "{hn} {street}, {city} {pc}" (+state), J = "{pc} {city} {street} {hn}"
CITIES = [
 ("Dallas",32.7767,-96.7970,"us","A"),("Phoenix",33.4484,-112.0740,"us","A"),("Nashville",36.1627,-86.7816,"us","A"),("Baltimore",39.2904,-76.6122,"us","A"),
 ("Winnipeg",49.8951,-97.1384,"ca","A"),("Quebec City",46.8139,-71.2080,"ca","A"),("Liverpool",53.4084,-2.9916,"gb","A"),("Cardiff",51.4816,-3.1791,"gb","A"),
 ("Limerick",52.6638,-8.6267,"ie","A"),("Hobart",-42.8821,147.3272,"au","A"),("Hamilton",-37.7870,175.2793,"nz","A"),("Pretoria",-25.7479,28.2293,"za","A"),
 ("Montpellier",43.6108,3.8767,"fr","F"),("Rennes",48.1173,-1.6778,"fr","F"),("Nice",43.7102,7.2620,"fr","F"),("Liege",50.6326,5.5797,"be","F"),
 ("Nuremberg",49.4521,11.0767,"de","E"),("Hannover",52.3759,9.7320,"de","E"),("Dusseldorf",51.2277,6.7735,"de","E"),("Innsbruck",47.2692,11.4041,"at","E"),
 ("Bern",46.9480,7.4474,"ch","E"),("Eindhoven",51.4416,5.4697,"nl","E"),("Aalborg",57.0488,9.9217,"dk","E"),("Uppsala",59.8586,17.6389,"se","E"),
 ("Stavanger",58.9700,5.7331,"no","E"),("Oulu",65.0121,25.4651,"fi","E"),("Katowice",50.2649,19.0238,"pl","E"),("Lublin",51.2465,22.5684,"pl","E"),
 ("Plzen",49.7384,13.3736,"cz","E"),("Pecs",46.0727,18.2323,"hu","E"),("Osijek",45.5550,18.6955,"hr","E"),("Genoa",44.4056,8.9463,"it","E"),
 ("Palermo",38.1157,13.3615,"it","E"),("Zaragoza",41.6488,-0.8891,"es","E"),("Alicante",38.3452,-0.4810,"es","E"),("Coimbra",40.2033,-8.4103,"pt","E"),
 ("Heraklion",35.3387,25.1442,"gr","E"),("Timisoara",45.7489,21.2087,"ro","E"),("Burgas",42.5048,27.4626,"bg","E"),("Bursa",40.1885,29.0610,"tr","E"),
 ("Kharkiv",49.9935,36.2304,"ua","E"),("Novosibirsk",55.0084,82.9357,"ru","E"),("Liepaja",56.5047,21.0108,"lv","E"),("Siauliai",55.9349,23.3137,"lt","E"),
 ("Porto Alegre",-30.0346,-51.2177,"br","E"),("Mendoza",-32.8895,-68.8458,"ar","E"),("Puebla",19.0414,-98.2063,"mx","E"),("Concepcion",-36.8201,-73.0444,"cl","E"),
 ("Barranquilla",10.9685,-74.7813,"co","E"),("Arequipa",-16.4090,-71.5375,"pe","E"),
 ("Sendai",38.2682,140.8694,"jp","J"),("Hiroshima",34.3853,132.4553,"jp","J"),("Tainan",22.9999,120.2270,"tw","J"),("Incheon",37.4563,126.7052,"kr","J"),
 ("Beersheba",31.2520,34.7915,"il","E"),
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
    for f in ("../golden-v3-osm/osm.txt", "../golden-v4-osm-unseen/unseen.txt", "../golden-v5-osm-unseen/unseen5.txt"):   # also disjoint from golden-v3, v4, v5
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
    with open(os.path.join(HERE, "unseen6.tsv"), "w", encoding="utf-8") as o: o.writelines(f"{cc}\t{s}\n" for cc, s in rows)
    with open(os.path.join(HERE, "unseen6.txt"), "w", encoding="utf-8") as o: o.writelines(f"{s}\n" for cc, s in rows)
    na = sum(any(ord(ch) > 127 for ch in s) for _, s in rows)
    print(f"rows={len(rows)} countries={len({r[0] for r in rows})} non_ascii={na} dropped_overlap={dropped_overlap}")
if __name__ == "__main__": fetch() if sys.argv[1:] == ["fetch"] else build()
