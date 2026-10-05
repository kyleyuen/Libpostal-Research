#!/usr/bin/env python3
# 148: per-variant warm µs/address by innermost function and by source line (all bench_ext leaf PCs resolved with atos -i, batched),
# then ipr - mean(cpi, cpi2), with cpi - cpi2 as the noise floor.
import re, subprocess, statistics as st, collections, xml.etree.ElementTree as ET
X = '/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments'
BIN = {'cpi': f'{X}/119-component-phrase-ids/artifacts/cpi/bench_ext', 'cpi2': f'{X}/119-component-phrase-ids/artifacts/cpi/bench_ext', 'ipr': f'{X}/122-int-pairs-retest/artifacts/ipr/bench_ext'}
def prof(n):
    raw = open(f'raw/xc_{n}_cpu.xml').read()
    binname = dict(re.findall(r'<binary id="(\d+)" name="([^"]*)"', raw))
    base = int(re.search(r'name="bench_ext"[^>]*load-addr="(0x[0-9a-f]+)"', raw).group(1), 16)
    root = ET.fromstring(raw); bt, fr = {}, {}; c = collections.Counter(); tot = 0
    for row in root.iter('row'):
        b = row.find('backtrace')
        if b is None: continue
        if 'ref' in b.attrib: leaf = bt[b.attrib['ref']]
        else:
            f = b.find('frame')
            if 'ref' in f.attrib: leaf = fr[f.attrib['ref']]
            else:
                bi = f.find('binary'); bn = (bi.attrib.get('name') or binname.get(bi.attrib.get('ref'))) if bi is not None else None
                leaf = (f.attrib.get('name'), int(f.attrib['addr'], 16), bn); fr[f.attrib['id']] = leaf
            for g in b.findall('frame')[1:]:
                if 'id' in g.attrib:
                    gi = g.find('binary'); gn = (gi.attrib.get('name') or binname.get(gi.attrib.get('ref'))) if gi is not None else None
                    fr[g.attrib['id']] = (g.attrib.get('name'), int(g.attrib['addr'], 16), gn)
            bt[b.attrib['id']] = leaf
        c[leaf] += 1; tot += 1
    us = st.median(float(u) for p, u in re.findall(r'^pass=(\d+) us_per_addr=([\d.]+)', open(f'raw/xc_{n}_bench.stdout').read(), re.M) if int(p) >= 2)
    offs = collections.Counter(); fn_out = collections.Counter()
    for (fn, a, bn), k in c.items():
        if bn == 'bench_ext': offs[a - base] += k / tot * us
        else: fn_out[f'[sys] {fn}'] += k / tot * us
    keys = list(offs); lines, funcs = collections.Counter(), collections.Counter(fn_out)
    for i in range(0, len(keys), 400):
        chunk = keys[i:i + 400]
        out = subprocess.run(['atos', '-i', '-o', BIN[n], '-l', '0x100000000'] + [hex(0x100000000 + o - 1) for o in chunk], capture_output=True, text=True).stdout
        # atos -i prints one or more lines per address (innermost first), blank-line separated groups are not guaranteed; take first line per address by re-querying when counts mismatch
        groups = out.strip().split('\n\n') if '\n\n' in out else None
        res = groups if groups and len(groups) == len(chunk) else None
        if res is None:
            res = [subprocess.run(['atos', '-i', '-o', BIN[n], '-l', '0x100000000', hex(0x100000000 + o - 1)], capture_output=True, text=True).stdout for o in chunk]
        for o, r in zip(chunk, res):
            first = r.strip().split('\n')[0]
            m = re.search(r'^(\S+).*\(([^()]+:\d+)\)', first)
            fn, ln = (m.group(1), m.group(2)) if m else (first[:40], '?')
            lines[f'{ln} [{fn}]'] += offs[o]; funcs[fn] += offs[o]
    return us, tot, funcs, lines
P = {n: prof(n) for n in ('cpi', 'cpi2', 'ipr')}
for n, (us, tot, f, l) in P.items(): print(f'{n}: warm us/address {us:.3f}, samples {tot}')
def diff(idx, k=25):
    a, b, i = P['cpi'][idx], P['cpi2'][idx], P['ipr'][idx]
    keys = set(a) | set(b) | set(i)
    rows = [(i.get(x, 0) - (a.get(x, 0) + b.get(x, 0)) / 2, a.get(x, 0) - b.get(x, 0), x) for x in keys]
    for d, noise, x in sorted(rows, key=lambda r: -abs(r[0]))[:k]: print(f'   {d:+.4f}  (cpi-cpi2 {noise:+.4f})  {x}')
print('\nby innermost function: ipr - mean(cpi, cpi2), us/address'); diff(2)
print('\nby source line'); diff(3, 30)
