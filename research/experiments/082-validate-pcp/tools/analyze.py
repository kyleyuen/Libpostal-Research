# 031: per dataset and round, warm median µs/address (passes 2..20) per binary; paired speedup orig/variant per round; median over rounds.
# Also peak memory footprint (time -l) per binary/dataset (max over rounds).
import re, statistics, os, glob
B = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'raw', 'timing')
V = ['orig', 'lf', 'pcp']; D = ['v1', 'nv', 'osm']; R = [1, 2, 3, 4, 5]
def warm(ds, r, v):
    t = open(f'{B}/{ds}_r{r}_{v}.stdout').read()
    xs = [float(m) for m in re.findall(r'^pass=(?:[2-9]|1\d|20) us_per_addr=([\d.]+)', t, re.M)]
    assert len(xs) == 19, (ds, r, v, len(xs)); return statistics.median(xs)
def peak(ds, r, v):
    return int(re.search(r'(\d+)\s+peak memory footprint', open(f'{B}/{ds}_r{r}_{v}.stderr').read()).group(1))
print('| binary (PGO training) | ' + ' | '.join(f'{d} µs | {d} speedup (range)' for d in D) + ' |')
print('|---|' + '---:|---:|' * len(D))
names = {'orig': 'original', 'lf': 'lf (previous deliverable)', 'pcp': 'pcp = lf + 067 + 072 + 074 pf2 + 077 pcp'}
for v in V:
    cells = []
    for d in D:
        us = [warm(d, r, v) for r in R]; sp = [warm(d, r, 'orig') / warm(d, r, v) for r in R]
        cells.append(f'{statistics.median(us):.3f} | {statistics.median(sp):.3f}× ({min(sp):.2f}–{max(sp):.2f})')
    print(f'| {names[v]} | ' + ' | '.join(cells) + ' |')
print()
print('peak footprint bytes (max over rounds): ' + '; '.join(f'{v} ' + '/'.join(str(max(peak(d, r, v) for r in R)) for d in D) for v in V))
