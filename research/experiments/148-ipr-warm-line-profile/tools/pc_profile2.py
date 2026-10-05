"""132: leaf-PC histogram from an xctrace CPU Profiler export, resolving binary refs from the whole file; µs/address by innermost source line (atos -i)."""
import re, sys, subprocess, statistics as st, collections, xml.etree.ElementTree as ET
n = sys.argv[1]; B = sys.argv[2]
raw = open(f"raw/xc_{n}_cpu.xml").read()
binname = dict(re.findall(r'<binary id="(\d+)" name="([^"]*)"', raw))
base = int(re.search(r'name="bench_ext"[^>]*load-addr="(0x[0-9a-f]+)"', raw).group(1), 16)
root = ET.fromstring(raw); bt, fr = {}, {}; c = collections.Counter(); tot = 0
for row in root.iter("row"):
    b = row.find("backtrace")
    if b is None: continue
    if "ref" in b.attrib: leaf = bt[b.attrib["ref"]]
    else:
        f = b.find("frame")
        if "ref" in f.attrib: leaf = fr[f.attrib["ref"]]
        else:
            bi = f.find("binary"); bn = None
            if bi is not None: bn = bi.attrib.get("name") or binname.get(bi.attrib.get("ref"))
            leaf = (f.attrib.get("name"), int(f.attrib["addr"], 16), bn); fr[f.attrib["id"]] = leaf
        for g in b.findall("frame")[1:]:
            if "id" in g.attrib:
                gi = g.find("binary"); gn = (gi.attrib.get("name") or binname.get(gi.attrib.get("ref"))) if gi is not None else None
                fr[g.attrib["id"]] = (g.attrib.get("name"), int(g.attrib["addr"], 16), gn)
        bt[b.attrib["id"]] = leaf
    c[leaf] += 1; tot += 1
us = st.median(float(u) for p, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", open(f"raw/xc_{n}_bench.stdout").read(), re.M) if int(p) >= 2)
inb = collections.Counter(); outside = collections.Counter()
for (fn, a, bn), k in c.items():
    if bn == "bench_ext": inb[a - base] += k / tot * us
    else: outside[fn] += k / tot * us
top = sorted(inb.items(), key=lambda x: -x[1])[:300]
by = collections.Counter()
for off, d in top:
    out = subprocess.run(["atos", "-i", "-o", B, "-l", "0x100000000", hex(0x100000000 + off - 1)], capture_output=True, text=True).stdout.strip().split("\n")
    m = re.search(r"^(\S+).*\(([^()]+:\d+)\)", out[0]); by[f"{m.group(2)}  [{m.group(1)}]" if m else out[0]] += d
print(f"{n}: µs/address {us:.3f} ({tot} samples); in bench_ext {sum(inb.values()):.3f}; outside {sum(outside.values()):.3f}: " + ", ".join(f"{k} {v:.3f}" for k, v in outside.most_common(6)))
for l, d in by.most_common(35): print(f"  {d:6.3f}  {l}")
