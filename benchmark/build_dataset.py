#!/usr/bin/env python3
"""Build a reviewer test set (country<TAB>address TSV) from open address data. Standard library only.

usage: build_dataset.py --out v15.tsv [--per-country N] [--min-per-country M] [--seed S] INPUT [INPUT ...]

Each INPUT is CC=PATH (country code given; it overrides any country field in the records) or PATH (country taken
from the record, or from an OpenAddresses path such as .../us/ca/city.geojson). Files may be gzip-compressed. Accepted records:
  OpenAddresses  GeoJSON lines (batch output) with properties number/street/unit/city/district/region/postcode,
                 or the classic CSV with NUMBER,STREET,UNIT,CITY,DISTRICT,REGION,POSTCODE columns
  Overture       addresses exported to CSV or newline JSON, e.g. with DuckDB:
                   COPY (SELECT country, number, street, unit, postcode, address_levels FROM read_parquet(
                     's3://overturemaps-us-west-2/release/<release>/theme=addresses/type=*/*', hive_partitioning=1)
                     WHERE country = 'NZ') TO 'nz.jsonl' (FORMAT JSON);
  OpenStreetMap  `osmium export -f geojsonseq` output whose properties carry addr:* tags

Each record becomes one line in the usual local order (simplified; see ORDER): "street number, postcode city" for
most of Europe and Latin America, "number street, city region postcode" for English-speaking and some other
countries, and "postcode region city street number" for Japan, Korea, China, Taiwan and Hong Kong. Records without a
street or a house number (or whose street or number has no letter or digit) are skipped. Up to N addresses per country are kept by seeded reservoir sampling (same
inputs and seed -> same file); a string already in the kept sample is not added twice, so memory stays O(N) per
country even for very large inputs. Writes OUT and OUT.manifest.json (per-country counts, input SHA-256 values,
seed, skips). Countries below M are listed as short; nothing is padded or synthesized.
"""
import argparse, csv, gzip, hashlib, json, os, random, re, sys, time

NUMBER_FIRST = set("us ca au nz gb ie im jm tt pr za in pk bd my sg ph fr be lu nc re pf il ae qa kw eg tz".split())
EAST_ASIA = set("jp kr cn tw hk".split())
ORDER = ("number street first: " + " ".join(sorted(NUMBER_FIRST)) + "; postcode-first east Asian order: "
         + " ".join(sorted(EAST_ASIA)) + "; all others: street number, postcode city")


def opener(path):
    return gzip.open(path, "rt", encoding="utf-8", errors="replace") if path.endswith(".gz") else open(path, encoding="utf-8", errors="replace")


def clean(v):
    if isinstance(v, (int, float)): v = str(v)
    if not isinstance(v, str): return ""
    return re.sub(r"\s+", " ", v).strip()


def fields(p):
    """Normalize one record's properties from any accepted source to a common dict."""
    g = lambda *ks: next((clean(p[k]) for k in ks if k in p and clean(p[k])), "")
    levels = p.get("address_levels")
    if isinstance(levels, str):
        try: levels = json.loads(levels)
        except ValueError: levels = None
    lv = [clean(x.get("value")) for x in levels if isinstance(x, dict)] if isinstance(levels, list) else []
    return {"cc": g("country", "addr:country").lower()[:2],
            "number": g("number", "NUMBER", "addr:housenumber"), "street": g("street", "STREET", "addr:street"),
            "unit": g("unit", "UNIT", "addr:unit", "addr:flats"), "postcode": g("postcode", "POSTCODE", "addr:postcode"),
            "city": g("city", "CITY", "addr:city") or (lv[-1] if lv else ""),
            "region": g("region", "REGION", "addr:state", "addr:province") or (lv[0] if len(lv) > 1 else "")}


def fmt(cc, f):
    n, s, u, pc, c, r = f["number"], f["street"], f["unit"], f["postcode"], f["city"], f["region"]
    j = lambda sep, *xs: sep.join(x for x in xs if x)
    if cc in EAST_ASIA:
        return j(" ", pc, r, c, s, n, u)
    if cc in NUMBER_FIRST:
        return j(", ", j(" ", n, s, u), c, j(" ", r, pc))
    return j(", ", j(" ", s, n, u), j(" ", pc, c))


def records(path):
    with opener(path) as fh:
        head = fh.read(4096); fh.seek(0)
        if head.lstrip().startswith(("{", "\x1e")):
            for line in fh:
                line = line.strip().lstrip("\x1e")
                if not line: continue
                try: o = json.loads(line)
                except ValueError: yield None; continue
                yield o.get("properties", o) if isinstance(o, dict) else None
        else:
            yield from csv.DictReader(fh)


def cc_from_path(path):
    m = re.search(r"(?:^|/)([a-z]{2})/", path.replace(os.sep, "/").lower())
    return m.group(1) if m else ""


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", required=True); ap.add_argument("--per-country", type=int, default=100000)
    ap.add_argument("--min-per-country", type=int, default=20000); ap.add_argument("--seed", type=int, default=20261011)
    ap.add_argument("inputs", nargs="+")
    a = ap.parse_args()
    rnd = random.Random(a.seed); keep, kept, stats, inputs = {}, {}, {}, []
    for spec in a.inputs:
        cc0, path = (spec.split("=", 1) if re.match(r"^[A-Za-z]{2}=", spec) else ("", spec))
        cc0 = cc0.lower() or cc_from_path(path)
        h = hashlib.sha256()
        with open(path, "rb") as fb:
            for b in iter(lambda: fb.read(1 << 20), b""): h.update(b)
        n_in = n_skip = 0
        for p in records(path):
            n_in += 1
            if not isinstance(p, dict): n_skip += 1; continue
            f = fields(p); cc = cc0 or f["cc"]   # a given CC= wins: OSM addr:country tags include typos (VI in Vietnam)
            # a street or number without any letter or digit ("-", "- -" in OSM) is a placeholder, not an address; libpostal returns
            # no components for such a line, which the timing harness rejects
            if not re.fullmatch(r"[a-z]{2}", cc) or not re.search(r"\w", f["street"]) or not re.search(r"\w", f["number"]): n_skip += 1; continue
            s = fmt(cc, f)
            st = stats.setdefault(cc, {"usable_records": 0, "duplicates_of_kept": 0})
            k, ks = keep.setdefault(cc, []), kept.setdefault(cc, set())
            if s in ks: st["duplicates_of_kept"] += 1; continue
            st["usable_records"] += 1
            if len(k) < a.per_country:
                k.append(s); ks.add(s)
            else:
                i = rnd.randrange(st["usable_records"])
                if i < a.per_country:
                    ks.discard(k[i]); k[i] = s; ks.add(s)
        inputs.append({"input": spec, "country": cc0 or "from records", "sha256": h.hexdigest(), "records": n_in, "skipped": n_skip})
        print(f"{spec}: {n_in:,} records, {n_skip:,} skipped (no street/number or unparseable)", file=sys.stderr)
    with open(a.out, "w", encoding="utf-8") as o:
        for cc in sorted(keep):
            for s in keep[cc]: o.write(f"{cc}\t{s}\n")
    countries = {cc: {"kept": len(keep[cc]), **stats[cc]} for cc in sorted(keep)}
    short = sorted(cc for cc, x in countries.items() if x["kept"] < a.min_per_country)
    man = {"created": time.strftime("%Y-%m-%dT%H:%M:%S%z"), "out": os.path.abspath(a.out), "seed": a.seed, "per_country": a.per_country,
           "min_per_country": a.min_per_country, "order_rules": ORDER, "total": sum(x["kept"] for x in countries.values()),
           "countries": countries, "below_minimum": short, "inputs": inputs}
    json.dump(man, open(a.out + ".manifest.json", "w"), indent=1)
    print(f"{a.out}: {man['total']:,} addresses, {len(countries)} countries; below {a.min_per_country:,}: {', '.join(short) or 'none'}", file=sys.stderr)


if __name__ == "__main__":
    main()
