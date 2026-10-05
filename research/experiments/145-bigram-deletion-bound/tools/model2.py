#!/usr/bin/env python3
# 145 cost model v2 (after review): 122's terms rescaled by its measured bigram coverage c (tools/coverage_122.py);
# phrase-type families take 130's measured whole-address result; remaining non-bigram families use the bigram per-key ratio.
# Inputs measured (sources inline); outputs UNVERIFIED. 122 deltas come from its own session (01:51-02:22) vs cpi; 145 deltas from 20:37-20:43 vs cpi:
# each delta is against cpi within its own session, sessions are not mixed inside one ratio.
M = {
 'dev': dict(total=5.534, d_feat=0.171, d_look=0.220, i122_look=0.032, i122_feat=0.079, c=0.772, n_b=9.56, n_tok=75656/12863,
             cached=0.38, phrase_pct=1.1, other_keys=17.5-9.56-2.32, base=6.077),
 'osm': dict(total=3.640, d_feat=0.160, d_look=0.170, i122_look=0.035, i122_feat=0.015, c=0.819, n_b=8.477, n_tok=61157/11405,
             cached=0.38, phrase_pct=-0.2, other_keys=14.1-8.477-1.797, base=5.775),
}
for ds, m in M.items():
    c = m['c']
    look_full = m['i122_look'] / c                                   # lookup saving at full coverage
    canon_total = c * m['d_feat'] + m['i122_feat']                   # 122 features delta = canon cost - c * build saved
    c_canon = canon_total / (2 * c * m['n_b'])                       # per canonicalization lookup
    compose = m['n_b'] * 1.0 / 1000
    ceil = m['d_feat'] + m['d_look']
    g = {k: m['d_feat'] + look_full - compose - u for k, u in
         (('pess', m['n_tok'] * c_canon), ('mid', m['n_tok'] * c_canon * (1 - m['cached'])), ('opt', 0.0))}
    pk = ceil / m['n_b']; share_lo, share_hi = g['pess'] / ceil, g['opt'] / ceil
    other_lo, other_hi = m['other_keys'] * pk * share_lo, m['other_keys'] * pk * share_hi
    ph = m['phrase_pct'] / 100 * m['total']
    tot_lo, tot_hi = g['pess'] + other_lo + ph, g['opt'] + other_hi + ph
    tot_ceil = ceil + m['other_keys'] * pk + max(ph, 0)
    sp = lambda x: m['base'] / (1 - x / m['total'])
    print(f"{ds}: c={c}; lookup saving at full coverage {look_full:.3f} us; c_canon {1000*c_canon:.1f} ns")
    print(f"   bigram card: pess {g['pess']:.3f} ({100*g['pess']/m['total']:.1f}%, {sp(g['pess']):.2f}x)  mid {g['mid']:.3f} ({100*g['mid']/m['total']:.1f}%)  opt {g['opt']:.3f} ({100*g['opt']/m['total']:.1f}%, {sp(g['opt']):.2f}x)  ceiling {ceil:.3f} ({100*ceil/m['total']:.1f}%, {sp(ceil):.2f}x)")
    print(f"   + other families {other_lo:.3f}-{other_hi:.3f} us (extrapolated) + phrase-type slice {ph:+.3f} us (130 measured)")
    print(f"   WHOLE PIPELINE realistic {tot_lo:.3f}-{tot_hi:.3f} us = {100*tot_lo/m['total']:.1f}-{100*tot_hi/m['total']:.1f}% -> {sp(tot_lo):.2f}-{sp(tot_hi):.2f}x ; outer ceiling {tot_ceil:.3f} us = {100*tot_ceil/m['total']:.1f}% -> {sp(tot_ceil):.2f}x")
    need = (1 - m['base'] / 7.8) * m['total']
    print(f"   7.8x needs {need:.3f} us; left after realistic pipeline {need-tot_hi:.3f}-{need-tot_lo:.3f} us; after ceiling {need-tot_ceil:.3f} us")
