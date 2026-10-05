import json
from pathlib import Path

root = Path('/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/025-audit-next/raw')
src = root / 'commands/borrow_v1_r1.stdout'
dst = root / 'negative_control.jsonl'
with src.open(encoding='utf-8') as inp, dst.open('w', encoding='utf-8') as out:
    first = json.loads(next(inp))
    first['components'][0][0] = 'deliberate_wrong_label'
    out.write(json.dumps(first, ensure_ascii=False, separators=(',', ':')) + '\n')
    for line in inp:
        out.write(line)
print(dst)
