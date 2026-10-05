#!/bin/bash
# 142 negative controls: perturb one record of the golden-v1 oracles at address 5000 (O3i: one id +1; O4: lowest bit of first state score flipped;
# O1: token 1 offset +1). The comparator and cmp must both fail and point at a=5000. Positive control: oracle vs itself must pass.
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/142-stage-oracles"; O="$W/research/oracles/v1/golden-v1"; N="$E/raw/negctl"
python3 - "$O" "$N" <<'PY'
import sys
O, N = sys.argv[1], sys.argv[2]
def perturb(name, pred, edit):
    done = False
    with open(f'{O}/{name}.txt', 'rb') as fi, open(f'{N}/{name}_perturbed.txt', 'wb') as fo:
        for line in fi:
            f = line.rstrip(b'\n').split(b'\t')
            if not done and pred(f): f = edit(f); done = True; print(name, 'perturbed:', b'\t'.join(f)[:120])
            fo.write(b'\t'.join(f) + b'\n')
perturb('O3i', lambda f: f[0] == b'5000' and f[2] == b'S' and f[4] != b'-', lambda f: f[:4] + [str(int(f[4]) + 1).encode()])
perturb('O4', lambda f: f[0] == b'5000' and f[2] == b'S', lambda f: f[:3] + [b'%016x' % (int(f[3], 16) ^ 1)] + f[4:])
perturb('O1', lambda f: f[0] == b'5000' and f[1] == b'1', lambda f: f[:3] + [str(int(f[3]) + 1).encode()] + f[4:])
PY
for k in O3i O4 O1; do
  python3 "$E/tools/oracle_cmp.py" "$O/$k.txt" "$N/${k}_perturbed.txt" > "$N/$k.cmp.out"; echo "$k comparator exit=$?" >> "$N/summary.txt"
  cmp -s "$O/$k.txt" "$N/${k}_perturbed.txt"; echo "$k cmp exit=$?" >> "$N/summary.txt"; gzip -f "$N/${k}_perturbed.txt"; done
python3 "$E/tools/oracle_cmp.py" "$O/O3i.txt" "$O/O3i.txt" > "$N/positive.cmp.out"; echo "positive (O3i vs itself) comparator exit=$?" >> "$N/summary.txt"
