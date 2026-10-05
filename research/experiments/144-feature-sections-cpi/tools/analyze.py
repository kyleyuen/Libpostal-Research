#!/usr/bin/env python3
# 144: median over 4 runs per stage (cold/warm) for fss and css; feature sections fs_* from fss only.
import re, glob, statistics as st, os
B = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'raw', 'split')
for ds in ('dev', 'osm', 'u11'):
    R = {}
    for v in ('css', 'fss'):
        S = {}
        for p in sorted(glob.glob(f'{B}/{ds}_r*_{v}.stdout')):
            for line in open(p):
                m = re.match(r'(\S+) cold_us=([\d.]+) warm_us=([\d.]+)', line)
                if m: S.setdefault(m.group(1), []).append((float(m.group(2)), float(m.group(3))))
        R[v] = {k: (st.median(x[0] for x in vv), st.median(x[1] for x in vv), len(vv)) for k, vv in S.items()}
    print(f'{ds}')
    for k in ('total_parse', 'context_fill', 'features', 'lookups', 'accumulate', 'viterbi_and_rest'):
        c, f = R['css'][k], R['fss'][k]
        print(f'   {k:18} css {c[0]:.3f}/{c[1]:.3f}  fss {f[0]:.3f}/{f[1]:.3f}  (timer overhead {f[0]-c[0]:+.3f}/{f[1]-c[1]:+.3f}) n={c[2]},{f[2]}')
    tot = 0
    for k, v in R['fss'].items():
        if k.startswith('fs_'): print(f'   {k:18} {v[0]:.3f}/{v[1]:.3f}'); tot += v[0]
    print(f'   sum fs_* cold {tot:.3f} vs fss features cold {R["fss"]["features"][0]:.3f}')
