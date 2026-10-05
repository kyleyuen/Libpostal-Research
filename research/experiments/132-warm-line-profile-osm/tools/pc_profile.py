"""112: leaf-PC histograms from xctrace CPU Profiler exports (cold, warm); per-PC µs/address = share x run's µs/address; cold - warm by source line (atos)."""
import re, sys, subprocess, statistics as st, collections, xml.etree.ElementTree as ET
def load(n):
    root = ET.parse(f"raw/xc_{n}_cpu.xml").getroot(); bt, fr, binl = {}, {}, {}
    c = collections.Counter(); tot = 0
    for row in root.iter("row"):
        b = row.find("backtrace")
        if b is None: continue
        if "ref" in b.attrib: leaf = bt[b.attrib["ref"]]
        else:
            f = b.find("frame")
            if "ref" in f.attrib: leaf = fr[f.attrib["ref"]]
            else:
                bi = f.find("binary"); name = bi.attrib.get("name") if bi is not None and "name" in bi.attrib else (binl.get(bi.attrib["ref"]) if bi is not None else None)
                if bi is not None and "id" in bi.attrib: binl[bi.attrib["id"]] = bi.attrib.get("name"); 
                if bi is not None and bi.attrib.get("name") == "bench_ext": load.base = int(bi.attrib["load-addr"], 16)
                leaf = (f.attrib.get("name"), int(f.attrib["addr"], 16), name); fr[f.attrib["id"]] = leaf
                for g in b.findall("frame")[1:]:
                    if "id" in g.attrib: fr[g.attrib["id"]] = (g.attrib.get("name"), int(g.attrib["addr"], 16), None)
            bt[b.attrib["id"]] = leaf
        c[leaf] += 1; tot += 1
    us = st.median(float(u) for p, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", open(f"raw/xc_{n}_bench.stdout").read(), re.M) if int(p) >= 2)
    base = int(re.search(r'name="bench_ext"[^>]*load-addr="(0x[0-9a-f]+)"', open(f"raw/xc_{n}_cpu.xml").read()).group(1), 16); out = collections.Counter()   # 132: load address from any frame
    for (fn, a, b), k in c.items(): out[(fn, a - base if b == "bench_ext" else None)] += k / tot * us
    return out, us, tot
C, cu, ct = load("cold"); Wm, wu, wt = load("warm")
print(f"cold {cu:.3f} µs/addr ({ct} samples), warm {wu:.3f} µs/addr ({wt} samples)")
keys = set(C) | set(Wm); rows = sorted(((C.get(k, 0) - Wm.get(k, 0), k) for k in keys), reverse=True)
top = [r for r in rows if r[1][1] is not None][:60]
addrs = [hex(0x100000000 + k[1] - 1) for _, k in top]   # leaf addr may be +1 (pc of next); use -1 for line lookup
lines = subprocess.run(["atos", "-o", "../119-component-phrase-ids/artifacts/cpi/bench_ext", "-l", "0x100000000"] + addrs, capture_output=True, text=True).stdout.strip().split("\n")
byline = collections.Counter()
for (d, k), l in zip(top, lines): byline[re.sub(r"^.*\((?:in bench_ext\) \()?", "", l).rstrip(")") + "  [" + (k[0] or "?") + "]"] += d
print("top source lines by cold-warm µs/address (60 hottest PCs aggregated):")
for l, d in byline.most_common(30): print(f"  {d:6.3f}  {l}")
