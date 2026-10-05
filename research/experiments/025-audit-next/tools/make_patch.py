import difflib
from pathlib import Path

experiment = Path(__file__).resolve().parents[1]
base = experiment.parents[1] / 'final' / '021-l4' / 'sources'
with (experiment / 'source.patch').open('w') as out:
    for name in ('address_parser.c', 'crf.c'):
        old = (base / name).read_text().splitlines(keepends=True)
        new = (experiment / 'sources' / name).read_text().splitlines(keepends=True)
        out.writelines(difflib.unified_diff(old, new,
            fromfile=f'final/021-l4/sources/{name}',
            tofile=f'experiments/025-audit-next/sources/{name}'))
