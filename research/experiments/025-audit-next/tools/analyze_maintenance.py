import re
import statistics as st
from pathlib import Path

raw = Path(__file__).resolve().parents[1] / 'raw' / 'commands'
rx = re.compile(r'^pass=(\d+) us_per_addr=([\d.]+).*cpu_us_per_addr=([\d.]+) nivcsw=(\d+)')


def p10(xs):
    xs = sorted(xs)
    return xs[int(.1 * (len(xs) - 1))]


for dataset in ('v1-all', 'novel'):
    values = {}
    for variant in ('control', 'fixes'):
        trials = []
        for t in range(1, 4):
            stem = raw / f'maintenance_{dataset}_t{t}_{variant}'
            assert 'exit=0' in stem.with_suffix('.meta').read_text()
            lines = stem.with_suffix('.stdout').read_text().splitlines()
            passes = [tuple(map(float, m.groups())) for line in lines if (m := rx.match(line))]
            assert len(passes) == 120
            peak = int(re.search(r'(\d+)\s+peak memory footprint', stem.with_suffix('.stderr').read_text()).group(1))
            trials.append((passes, peak))
        values[variant] = trials
    print(f'\n{dataset}')
    print('| variant | warm median µs | P10 µs | CPU median µs | cold median µs | peak max bytes |')
    print('|---|---:|---:|---:|---:|---:|')
    for variant, trials in values.items():
        warm = [r for passes, _ in trials for r in passes[1:]]
        print(f'| {variant} | {st.median(r[1] for r in warm):.3f} | {p10([r[1] for r in warm]):.3f} | '
              f'{st.median(r[2] for r in warm):.3f} | {st.median(passes[0][1] for passes, _ in trials):.3f} | '
              f'{max(peak for _, peak in trials)} |')
    ratios = []
    for t in range(3):
        c = st.median(r[1] for r in values['control'][t][0][1:])
        f = st.median(r[1] for r in values['fixes'][t][0][1:])
        ratios.append(c / f)
        print(f'trial={t+1} control_us={c:.3f} fixes_us={f:.3f} fixes_speedup={c/f:.5f}x')
    print(f'paired_median_speedup={st.median(ratios):.5f}x range={min(ratios):.5f}..{max(ratios):.5f}x')
