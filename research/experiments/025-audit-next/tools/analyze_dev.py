import re
import statistics as st
from pathlib import Path

raw = Path('/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/025-audit-next/raw/commands')
rx = re.compile(r'^pass=(\d+) us_per_addr=([\d.]+).*cpu_us_per_addr=([\d.]+) nivcsw=(\d+)')
values = {}
for variant in ('fixes', 'borrow'):
    trials = []
    for t in range(1, 6):
        stem = raw / f'dev_t{t}_{variant}'
        assert 'exit=0' in stem.with_suffix('.meta').read_text()
        lines = stem.with_suffix('.stdout').read_text().splitlines()
        passes = [tuple(map(float, m.groups())) for line in lines if (m := rx.match(line))]
        assert len(passes) == 120, (variant, t, len(passes))
        stderr = stem.with_suffix('.stderr').read_text()
        peak = int(re.search(r'(\d+)\s+peak memory footprint', stderr).group(1))
        setup = float(re.search(r'setup_seconds=([\d.]+)', lines[0]).group(1))
        trials.append((passes, peak, setup))
    values[variant] = trials

def p10(xs):
    xs = sorted(xs)
    return xs[int(.1 * (len(xs) - 1))]

print('| variant | pass median µs | P10 µs | CPU median µs | cold median µs | clean median µs (n) | peak max MB | setup median s |')
print('|---|---:|---:|---:|---:|---:|---:|---:|')
for variant, trials in values.items():
    p = [r for trial, _, _ in trials for r in trial]
    clean = [r[1] for r in p if r[3] == 0 and r[1] <= 1.02 * r[2]]
    print(f'| {variant} | {st.median(r[1] for r in p):.3f} | {p10([r[1] for r in p]):.3f} | '
          f'{st.median(r[2] for r in p):.3f} | {st.median(trial[0][0][1] for trial in trials):.3f} | '
          f'{st.median(clean):.3f} ({len(clean)}) | {max(trial[1] for trial in trials)/1e6:.1f} | '
          f'{st.median(trial[2] for trial in trials):.3f} |')

ratios = []
print('\n| trial | fixes median µs | borrow median µs | borrow speedup |')
print('|---:|---:|---:|---:|')
for t in range(5):
    f = st.median(r[1] for r in values['fixes'][t][0])
    b = st.median(r[1] for r in values['borrow'][t][0])
    ratios.append(f / b)
    print(f'| {t+1} | {f:.3f} | {b:.3f} | {f/b:.4f}x |')
print(f'paired_median_speedup={st.median(ratios):.5f}x range={min(ratios):.5f}..{max(ratios):.5f}x')
