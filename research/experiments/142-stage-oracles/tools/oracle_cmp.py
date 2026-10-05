#!/usr/bin/env python3
# 142 oracle comparator (explains failures; `cmp` decides). usage: oracle_cmp.py REFERENCE CANDIDATE   (plain or .gz)
# Prints line counts, mismatching-line count, and the first differing record (address a, token t, both lines). Exit 0 iff identical.
import gzip, sys

def opener(p): return gzip.open(p, 'rb') if p.endswith('.gz') else open(p, 'rb')

def main(ref, cand):
    n = bad = 0; first = None
    with opener(ref) as fr, opener(cand) as fc:
        while True:
            a, b = fr.readline(), fc.readline()
            if not a and not b: break
            n += 1
            if a != b:
                bad += 1
                if first is None: first = (n, a, b)
    print(f'records {n}  mismatching {bad}')
    if first:
        ln, a, b = first
        f = (a or b).split(b'\t')
        print(f'first difference at line {ln}: address a={f[0].decode(errors="replace")} token t={f[1].decode(errors="replace") if len(f) > 1 else "?"}')
        print(f'  reference: {a[:300]!r}')
        print(f'  candidate: {b[:300]!r}')
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2]))
