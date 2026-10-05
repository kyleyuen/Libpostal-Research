# 030 screen: per-address instructions/cycles = (X(102) - X(2)) / (100 * N); warm-pass median µs from the 102-pass runs.
import re, statistics, os
B = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'raw', 'screen_pairs'); N = 10308
def tl(v, p, r, key): return int(re.search(r'(\d+)\s+' + key, open(f'{B}/ic_{v}_p{p}_r{r}.stderr').read()).group(1))
def us(v, r):
    xs = [float(m) for m in re.findall(r'^pass=\d+ us_per_addr=([\d.]+)', open(f'{B}/ic_{v}_p102_r{r}.stdout').read(), re.M)]
    return statistics.median(xs[1:])
med = {}
for v in ('l4', 'pr'):
    for key in ('instructions retired', 'cycles elapsed'):
        xs = [(tl(v, 102, r, key) - tl(v, 2, r, key)) / (100 * N) for r in (1, 2, 3)]
        med[v, key] = statistics.median(xs); print(f'{v} {key}: ' + ' '.join(f'{x:,.0f}' for x in xs) + f'  median {med[v, key]:,.0f}')
    u = [us(v, r) for r in (1, 2, 3)]; med[v, 'us'] = statistics.median(u); print(f'{v} warm median us/addr per rep: ' + ' '.join(f'{x:.3f}' for x in u))
for k in ('instructions retired', 'cycles elapsed', 'us'):
    print(f'pr/l4 {k}: {med["pr", k] / med["l4", k]:.4f}')
