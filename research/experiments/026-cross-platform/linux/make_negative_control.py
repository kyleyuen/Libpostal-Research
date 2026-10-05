import json
import sys
from pathlib import Path

src, dst = map(Path, sys.argv[1:3])
if dst.exists():
    raise SystemExit(f"exists: {dst}")
with src.open(encoding="utf-8") as inp, dst.open("w", encoding="utf-8") as out:
    first = json.loads(next(inp))
    first["components"][0][0] = "deliberate_wrong_label"
    out.write(json.dumps(first, ensure_ascii=False, separators=(",", ":")) + "\n")
    out.writelines(inp)
print(dst)
