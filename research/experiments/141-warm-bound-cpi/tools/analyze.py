#!/usr/bin/env python3
# 141 analysis. Per run: median of passes 2..20. Per variant/dataset: median over rounds (and min/max).
# Speedup = orig_ext / cpi_ext (same harness). Warm bound = orig_ext / cpi_warm (088's cross-harness ratio).
# Stage buckets come from bench_split only (median over runs per stage).
import re, statistics as st, glob, os
B = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'raw')
med = st.median

def passes(path, key):
    v = {}
    for line in open(path):
        m = re.match(r'pass=(\d+) (?:.*? )?' + key + r'=([\d.]+)', line)
        if m: v[int(m.group(1))] = float(m.group(2))
    return [v[p] for p in sorted(v) if p >= 2]

def rss(path):
    for line in open(path):
        m = re.match(r'\s*(\d+)\s+maximum resident set size', line)
        if m: return int(m.group(1))

def ok(p): return open(p.rsplit('.', 1)[0] + '.exit').read().strip() == '0'

DS = ['dev', 'osm', 'u11']
VAR = [('orig_ext', 'orig_ext', 'us_per_addr'), ('cpi_ext', 'cpi_ext', 'us_per_addr'), ('cpi_warm', 'cpi_warm', 'warm_us_per_addr'),
       ('cpi_first', 'cpi_warm', 'first_call_us_per_addr'), ('orig_warm', 'orig_warm', 'warm_us_per_addr'), ('orig_first', 'orig_warm', 'first_call_us_per_addr')]
res = {}
for ds in DS:
    for name, f, key in VAR:
        res[ds, name] = [med(passes(p, key)) for p in sorted(glob.glob(f'{B}/timing/{ds}_r*_{f}.stdout')) if ok(p)]

print('timing (µs/address; per-run median of passes 2-20; median [min-max] over runs)')
for ds in DS:
    for name, _, _ in VAR:
        v = res[ds, name]
        print(f'  {ds:4} {name:10} {med(v):7.3f} [{min(v):.3f}-{max(v):.3f}] n={len(v)}')
print()
rr = lambda a, b: ' '.join(f'{x / y:.2f}' for x, y in zip(a, b))
print('speedups vs original bench_ext (per-round ratios in brackets)')
for ds in DS:
    o, c, w, ow = res[ds, 'orig_ext'], res[ds, 'cpi_ext'], res[ds, 'cpi_warm'], res[ds, 'orig_warm']
    print(f'  {ds:4} cpi normal {med(o) / med(c):5.2f}x [{rr(o, c)}]  warm bound {med(o) / med(w):5.2f}x [{rr(o, w)}]'
          f'  | same-harness warm/warm {med(ow) / med(w):5.2f}x | 7.8x needs {med(o) / 7.8:.3f} us (cpi {med(c):.3f}, warm {med(w):.3f})'
          f' | first-touch share {(med(c) - med(w)) / med(c) * 100:.0f}%')
print()
print('peak RSS (bytes, max over runs, /usr/bin/time -l)')
for ds in DS:
    for name in ['orig_ext', 'cpi_ext', 'cpi_warm', 'orig_warm']:
        v = [rss(p) for p in sorted(glob.glob(f'{B}/timing/{ds}_r*_{name}.stderr'))]
        print(f'  {ds:4} {name:9} {max(v)}')
print()
STAGES = ['total_parse', 'normalize', 'tokenize', 'context_fill', 'features', 'lookups', 'accumulate', 'viterbi_and_rest', 'response', 'predict_total']
print('stage split (bench_split = cpi + EXP_STAGE_STATS; µs/address; median over runs; cold / warm)')
for ds in DS:
    S = {}
    files = [p for p in sorted(glob.glob(f'{B}/split/{ds}_r*.stdout')) if ok(p)]
    for p in files:
        for line in open(p):
            m = re.match(r'(\S+) cold_us=([\d.]+) warm_us=([\d.]+)', line)
            if m: S.setdefault(m.group(1), []).append((float(m.group(2)), float(m.group(3))))
    g = lambda s, i: med([x[i] for x in S[s]])
    print(f'  {ds} (runs={len(files)})')
    for s in STAGES:
        if s in S: print(f'    {s:18} {g(s, 0):6.3f} / {g(s, 1):6.3f}   cold-warm {g(s, 0) - g(s, 1):6.3f}')
    for i, lab in ((0, 'cold'), (1, 'warm')):
        tot = g('total_parse', i)
        front = sum(g(s, i) for s in ('context_fill', 'features', 'lookups'))
        fixed = sum(g(s, i) for s in ('accumulate', 'viterbi_and_rest'))
        other = sum(g(s, i) for s in ('normalize', 'tokenize', 'response'))
        print(f'    {lab}: front {front:.3f} ({front / tot * 100:.0f}%)  fixed(acc+vit) {fixed:.3f} ({fixed / tot * 100:.0f}%)  other {other:.3f} ({other / tot * 100:.0f}%)'
              f'  sum {front + fixed + other:.3f} vs total {tot:.3f} (unattributed {tot - front - fixed - other:+.3f})')
