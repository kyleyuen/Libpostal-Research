import re
import statistics as st
from pathlib import Path

raw = Path(__file__).resolve().parents[1] / 'raw' / 'commands'
rx = re.compile(r'^pass=(\d+) us_per_addr=([\d.]+).*cpu_us_per_addr=([\d.]+) nivcsw=(\d+)')


def p10(xs):
    xs = sorted(xs)
    return xs[int(.1 * (len(xs) - 1))]


for dataset in ('v1-all', 'novel'):
    data = {}
    for variant in ('fixes', 'borrow'):
        trials = []
        for number in range(1, 4):
            stem = raw / f'confirm_{dataset}_t{number}_{variant}'
            assert 'exit=0' in stem.with_suffix('.meta').read_text()
            lines = stem.with_suffix('.stdout').read_text().splitlines()
            passes = [tuple(map(float, m.groups())) for line in lines if (m := rx.match(line))]
            assert len(passes) == 120, (dataset, variant, number, len(passes))
            stderr = stem.with_suffix('.stderr').read_text()
            peak = int(re.search(r'(\d+)\s+peak memory footprint', stderr).group(1))
            setup = float(re.search(r'setup_seconds=([\d.]+)', lines[0]).group(1))
            trials.append((passes, peak, setup))
        data[variant] = trials
    print(f'\n{dataset}')
    print('| variant | warm median µs | warm P10 µs | CPU median µs | clean median µs (n) | cold median µs | peak max MB | setup median s |')
    print('|---|---:|---:|---:|---:|---:|---:|---:|')
    for variant, trials in data.items():
        warm = [r for passes, _, _ in trials for r in passes[1:]]
        clean = [r[1] for r in warm if r[3] == 0 and r[1] <= 1.02 * r[2]]
        print(f'| {variant} | {st.median(r[1] for r in warm):.3f} | {p10([r[1] for r in warm]):.3f} | '
              f'{st.median(r[2] for r in warm):.3f} | {st.median(clean):.3f} ({len(clean)}) | '
              f'{st.median(passes[0][1] for passes, _, _ in trials):.3f} | '
              f'{max(peak for _, peak, _ in trials) / 1e6:.1f} | '
              f'{st.median(setup for _, _, setup in trials):.3f} |')
    print('| trial | fixes warm median µs | borrow warm median µs | borrow speedup |')
    print('|---:|---:|---:|---:|')
    ratios = []
    for i in range(3):
        f = st.median(r[1] for r in data['fixes'][i][0][1:])
        b = st.median(r[1] for r in data['borrow'][i][0][1:])
        ratios.append(f / b)
        print(f'| {i+1} | {f:.3f} | {b:.3f} | {f/b:.4f}x |')
    print(f'paired_median_speedup={st.median(ratios):.5f}x range={min(ratios):.5f}..{max(ratios):.5f}x')
