#!/usr/bin/env python3
# 145: share c of runtime bigram keys whose two parts are both vocabulary words (count > 0) [+ optionally UNKNOWN/UNKNOWN_NUMERIC],
# i.e. the share 122's vocabulary-canonical integer pair route could convert. Inputs: 143 key dump, 142 oracles.
import sys, gzip
KD, OD, ALL, DEV = sys.argv[1:5]
def unesc(s):
    if '\\' not in s: return s
    out, i = [], 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s): out.append({'\\':'\\','t':'\t','n':'\n','r':'\r','0':'\0'}.get(s[i+1], s[i+1])); i += 2
        else: out.append(s[i]); i += 1
    return ''.join(out)
V = set()
for line in open(f'{KD}/vocab.tsv', encoding='utf-8', errors='surrogateescape'):
    k, d = line.rstrip('\n').rsplit('\t', 1)
    if int(d) > 0: V.add(unesc(k))
VU = V | {'UNKNOWN', 'UNKNOWN_NUMERIC'}
def ok(rest, S):
    i = rest.find('|')
    while i >= 0:
        if rest[:i] in S and rest[i+1:] in S: return True
        i = rest.find('|', i + 1)
    return False
allv = [l for l in open(ALL, encoding='utf-8', errors='surrogateescape').read().split('\n') if l]
devset = set(open(DEV, encoding='utf-8', errors='surrogateescape').read().split('\n'))
keep_dev = {i for i, s in enumerate(allv) if s in devset}
for corpus, keep in (('golden-v1', keep_dev), ('osm-v3', None)):
    n = a = b = 0
    with gzip.open(f'{OD}/{corpus}/O3k.txt.gz', 'rt', encoding='utf-8', errors='surrogateescape') as f:
        for line in f:
            x = line.rstrip('\n').split('\t', 4)
            if x[2] != 'S' or (keep is not None and int(x[0]) not in keep): continue
            fam, _, rest = unesc(x[4]).partition('|')
            if fam not in ('prev word+word', 'word+next word'): continue
            n += 1; a += ok(rest, V); b += ok(rest, VU)
    print(f'{corpus}{" development" if keep else ""}: bigram keys {n:,}; both parts vocab c={a/n:.3f}; vocab or UNKNOWN* c={b/n:.3f}')
