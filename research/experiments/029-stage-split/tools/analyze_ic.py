# 029: per-address instructions and cycles = (X(22 passes) - X(2 passes)) / (20 * N) from /usr/bin/time -l, per rep and median.
import re, statistics, sys, os
B = os.path.join(os.path.dirname(__file__), '..', 'raw'); N = 10308
def get(v, p, r, key):
    t = open(f'{B}/ic_{v}_p{p}_r{r}.stderr').read()
    return int(re.search(r'(\d+)\s+' + key, t).group(1))
res = {}
for v in ('orig', 'l4'):
    for key in ('instructions retired', 'cycles elapsed'):
        xs = [(get(v, 22, r, key) - get(v, 2, r, key)) / (20 * N) for r in (1, 2, 3)]
        res[v, key] = statistics.median(xs)
        print(f'{v:5s} {key:21s} per address: ' + ' '.join(f'{x:,.0f}' for x in xs) + f'  median {res[v, key]:,.0f}')
for v in ('orig', 'l4'):
    print(f'{v:5s} IPC (median ratio) {res[v, "instructions retired"] / res[v, "cycles elapsed"]:.2f}')
print(f'instructions ratio orig/l4 {res["orig", "instructions retired"] / res["l4", "instructions retired"]:.3f}')
print(f'cycles ratio orig/l4       {res["orig", "cycles elapsed"] / res["l4", "cycles elapsed"]:.3f}')
