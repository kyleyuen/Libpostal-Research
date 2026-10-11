#!/usr/bin/env python3
"""Stream exact candidate comparisons without editing preserved references."""
import hashlib
from pathlib import Path

root = Path(__file__).resolve().parents[4]
out = root / 'research/experiments/173-shared-token-identity/raw/verify'

def digest(path):
    h = hashlib.sha256()
    n = 0
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
            n += chunk.count(b'\n')
    return h.hexdigest(), n

checks = [
    ('golden', out / 'golden.stdout', '45574ee2b1972f1acf2dbf53eb1fc2c25f8e1972cd5c22e8071fe3606c5c8c63'),
    ('novel', out / 'novel.stdout', '806b8a81603ea9adf3af788334dc9b7d577cbb8a13753dd591b535f4fb896aae'),
]
for label, path, expected in checks:
    got, lines = digest(path)
    print(label, lines, got, 'PASS' if got == expected else 'FAIL')
    assert got == expected

pairs = [
    ('osm', out / 'osm.stdout', root / 'research/experiments/017-loadtime-precompute/raw/v3check/outputs_baseline.jsonl'),
    ('fuzz-200k', out / 'fuzz_s1_200k.stdout', out / 'fuzz_s1_200k.original_prefix'),
    ('fuzz-2m', out / 'fuzz_s1_2m.candidate.stdout', out / 'fuzz_s1_2m.original.stdout'),
    ('golden-noverify', out / 'golden.noverify.stdout', out / 'golden.stdout'),
    ('fuzz-200k-noverify', out / 'fuzz_s1_200k.noverify.stdout', out / 'fuzz_s1_200k.stdout'),
]
for label, a, b in pairs:
    ah, an = digest(a)
    bh, bn = digest(b)
    print(label, an, ah, 'PASS' if (ah, an) == (bh, bn) else 'FAIL')
    assert (ah, an) == (bh, bn)

# Negative control: the comparator must flag a single-byte perturbation.
with (out / 'golden.stdout').open('rb') as f:
    original = f.read()
assert original
mutant_hash = hashlib.sha256(bytes([original[0] ^ 1]) + original[1:]).hexdigest()
assert mutant_hash != digest(out / 'golden.stdout')[0]
print('negative-control one-byte perturbation: DETECTED')
