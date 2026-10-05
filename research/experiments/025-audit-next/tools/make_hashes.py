import hashlib
from pathlib import Path

experiment = Path(__file__).resolve().parents[1]
files = [experiment / 'source.patch',
         *sorted((experiment / 'sources').glob('*')),
         *sorted((experiment / 'tools').glob('*')),
         experiment / 'data' / 'development.tsv']
for variant in ('control', 'fixes', 'borrow'):
    files.extend(experiment / 'artifacts' / variant / name for name in
                 ('libpostal.a', 'bench_ext', 'dump_outputs', 'fuzz_pipeline', 'hashes.txt'))
with (experiment / 'raw' / 'final_hashes.txt').open('w') as out:
    for path in files:
        if not path.is_file():
            continue
        h = hashlib.sha256()
        with path.open('rb') as src:
            while chunk := src.read(1024 * 1024):
                h.update(chunk)
        out.write(f'{h.hexdigest()}  {path.relative_to(experiment.parents[2])}\n')
