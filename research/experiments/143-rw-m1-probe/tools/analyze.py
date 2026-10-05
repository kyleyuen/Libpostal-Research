#!/usr/bin/env python3
# 143 M1 analysis. usage: analyze.py KEYDIR ORACLEDIR GOLDEN_ALL DEV_TXT
# Units U (model-derived only): vocab words (count > 0), component-phrase keys, address-dictionary phrases (key after the first '|'
# when the key is "<lang>|phrase", else the key), "UNKNOWN", "UNKNOWN_NUMERIC".
# Model side: for each word-family key, count the ways its remainder splits into parts that are all units (0 / 1 / >=2), and size integer tables.
# Runtime side (142 oracles, golden-v1 development subset and OSM): the share of emitted keys per family whose parts split into units.
import sys, gzip, collections
KD, OD, ALL, DEV = sys.argv[1:5]

def unesc(s):
    if '\\' not in s: return s
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            n = s[i + 1]; out.append({'\\': '\\', 't': '\t', 'n': '\n', 'r': '\r', '0': '\0'}.get(n, n)); i += 2
        else: out.append(c); i += 1
    return ''.join(out)

def keys(name):
    with open(f'{KD}/{name}.tsv', encoding='utf-8', errors='surrogateescape') as f:
        for line in f:
            k, d = line.rstrip('\n').rsplit('\t', 1)
            yield unesc(k), int(d)

U = set(); src = collections.Counter()
for k, d in keys('vocab'):
    if d > 0: U.add(k); src['vocab'] += 1
for k, _ in keys('component_phrases'): U.add(k); src['component'] += 1
for k, _ in keys('address_dictionary'):
    U.add(k.split('|', 1)[1] if '|' in k else k); src['dictionary'] += 1
U.update(['UNKNOWN', 'UNKNOWN_NUMERIC'])
print(f'units: {len(U):,} distinct strings (vocab {src["vocab"]:,}, component {src["component"]:,}, dictionary {src["dictionary"]:,}, +2 unknowns)')

def splits(rest, nparts):
    """number of ways rest splits at '|' into nparts parts that are all units (capped at 2)"""
    if nparts == 1: return 1 if rest in U else 0
    n = 0; i = rest.find('|')
    while i >= 0:
        if rest[:i] in U and splits(rest[i + 1:], nparts - 1): n += 1
        if n >= 2: return 2
        i = rest.find('|', i + 1)
    return n

# families handled as units (number of unit parts); everything else stays on the string path
FAM_S = {'prev word+word': 2, 'word+next word': 2, 'word': 1, 'prev word': 1, 'next word': 1, 'first word': 1, 'sub_word': 1}
FAM_P = {'word': 1, 'prev word': 1}

def model_side(name, fams):
    c = collections.defaultdict(collections.Counter)
    for k, _ in keys(name):
        fam, _, rest = k.partition('|')
        if fam not in fams: c['(other)']['keys'] += 1; continue
        if fam == 'word' and rest.startswith('ngrams|'): c['word|ngrams (string path)']['keys'] += 1; continue
        c[fam]['keys'] += 1; c[fam][('split0', 'split1', 'split2+')[splits(rest, fams[fam])]] += 1
    return c

BYTES_PER_ENTRY = {1: 8 / 0.75, 2: 12 / 0.75}   # open addressing at load 0.75: unigram (unit u32 + id u32), pair (2 x u32 + id u32)
total_bytes = 0
for name, fams, tag in (('state_features', FAM_S, 'S'), ('trans_features', FAM_P, 'P')):
    print(f'\nmodel {name}:')
    for fam, cnt in sorted(model_side(name, fams).items(), key=lambda x: -x[1]['keys']):
        if 'split1' not in cnt and 'split0' not in cnt and 'split2+' not in cnt:
            print(f'  {fam:28} keys {cnt["keys"]:>10,}'); continue
        n = cnt['keys']; bad = cnt['split0'] + cnt['split2+']
        mb = cnt['split1'] * BYTES_PER_ENTRY[fams[fam]] / 1e6; total_bytes += mb
        print(f'  {fam:28} keys {n:>10,}  1-split {cnt["split1"]:>10,}  0-split {cnt["split0"]:>8,}  2+-split {cnt["split2+"]:>6,}'
              f'  not unambiguous {100 * bad / n:5.2f}%  table {mb:6.1f} MB')
unit_map_mb = (src['component'] + src['dictionary']) * 4 / 1e6
print(f'\nunit-id maps (component and dictionary data id -> unit id, u32): {unit_map_mb:.1f} MB')
print(f'GROSS extra tables: {total_bytes + unit_map_mb:.1f} MB (before removing the same keys from the string dictionary; kill rule 1: > 50 MB)')

# runtime coverage from oracles
allv = open(ALL, encoding='utf-8', errors='surrogateescape').read().split('\n')
devset = set(open(DEV, encoding='utf-8', errors='surrogateescape').read().split('\n')) - {''}
dev_idx = {i for i, s in enumerate(l for l in allv if l) if s in devset}
print(f'\ndevelopment addresses mapped into golden-v1 oracle: {len(dev_idx):,}')

def runtime(corpus, keep):
    st = collections.defaultdict(collections.Counter); naddr = set()
    with gzip.open(f'{OD}/{corpus}/O3k.txt.gz', 'rt', encoding='utf-8', errors='surrogateescape') as fk, \
         gzip.open(f'{OD}/{corpus}/O3i.txt.gz', 'rt', encoding='utf-8', errors='surrogateescape') as fi:
        for lk, li in zip(fk, fi):
            a, t, kind, i, key = lk.rstrip('\n').split('\t', 4)
            if keep is not None and int(a) not in keep: continue
            naddr.add(a)
            found = li.rstrip('\n').rsplit('\t', 1)[1] != '-'
            fam, _, rest = unesc(key).partition('|')
            fams = FAM_S if kind == 'S' else FAM_P
            if fam not in fams or (fam == 'word' and rest.startswith('ngrams|')): continue
            s = splits(rest, fams[fam]); f = f'{kind} {fam}'
            st[f]['emitted'] += 1; st[f]['found'] += found
            st[f]['unit'] += s >= 1; st[f]['unit_found'] += (s >= 1) and found; st[f]['ambiguous'] += s >= 2
    return st, len(naddr)

for corpus, keep, label in (('golden-v1', dev_idx, 'golden-v1 development'), ('osm-v3', None, 'OSM v3')):
    st, na = runtime(corpus, keep)
    print(f'\nruntime coverage, {label} ({na:,} addresses with predicted tokens):')
    tot = collections.Counter()
    for f, c in sorted(st.items(), key=lambda x: -x[1]['emitted']):
        tot.update(c)
        print(f'  {f:22} per address {c["emitted"] / na:5.2f}  in model {100 * c["found"] / c["emitted"]:5.1f}%  unit-covered {100 * c["unit"] / c["emitted"]:5.1f}%'
              f'  of in-model keys covered {100 * c["unit_found"] / max(1, c["found"]):5.1f}%  ambiguous {c["ambiguous"]}')
    print(f'  ALL word families       per address {tot["emitted"] / na:5.2f}  unit-covered {100 * tot["unit"] / tot["emitted"]:5.1f}%  of in-model keys {100 * tot["unit_found"] / tot["found"]:5.1f}%')
