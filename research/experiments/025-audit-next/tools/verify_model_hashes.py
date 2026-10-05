import hashlib
import json
from pathlib import Path

manifest = json.loads(Path('profiling/manifest.json').read_text())
for item in manifest['model_files']:
    path = Path(item['path'])
    digest = hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()
    print(f'{digest}  {path}  expected={item["sha256"]}  match={digest == item["sha256"]}')
    assert digest == item['sha256']
