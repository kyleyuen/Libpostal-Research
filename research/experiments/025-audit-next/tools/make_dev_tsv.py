from pathlib import Path

root = Path('/Users/kyle/Desktop/HW/CSCI 201 Reserach')
wanted = set((root / 'profiling/golden-v1/development.txt').read_text().splitlines())
rows = (root / 'profiling/golden-v2-novel/v1-all.tsv').read_text().splitlines()
selected = [row for row in rows if row.partition('\t')[2] in wanted]
assert len(wanted) == 10308 and len(selected) == 10308
out = root / 'research/experiments/025-audit-next/data/development.tsv'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text('\n'.join(selected) + '\n')
print(f'rows={len(selected)} output={out}')
