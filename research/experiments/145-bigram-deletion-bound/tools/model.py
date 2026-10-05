#!/usr/bin/env python3
# 145 cost model for the integer bigram rewrite. Every input is a measured number (source in comments); outputs are UNVERIFIED projections.
# DeltaT = c_build_saved + lookup_saving - key_compose - unit_cost   (per address, split-harness cold us)
M = {   # dataset: values
 'dev': dict(total=5.534, d_feat=0.171, d_look=0.220,   # 145: nbs vs css (features, lookups deltas, cold)
             int_look_saving=0.032, canon_cost=0.171 + 0.079,  # 122: ipr lookups -0.032; features +0.079 while skipping the string build (0.171) => canonicalization 0.250
             n_b=9.6, n_tok=75656 / 12863,                    # 123 bigram keys/address; 142 oracle predicted tokens/address (golden-v1)
             cached=0.38,                                     # 131: share of bigram parts already canonical in the memo (37-39%)
             base_speedup=6.077),                             # 121 golden-v1 all (proportional transfer, UNVERIFIED)
 'osm': dict(total=3.640, d_feat=0.160, d_look=0.170, int_look_saving=0.035, canon_cost=0.160 + 0.015,
             n_b=8.47, n_tok=61157 / 11405, cached=0.38, base_speedup=5.775),
}
KEY_COMPOSE_NS = 1.0   # assumption: forming a 64-bit pair key from two u32 + pushing an id, ~1 ns/key warm (UNVERIFIED; bounded below by 0)
for ds, m in M.items():
    c_canon = m['canon_cost'] / (2 * m['n_b'])                 # us per canonicalization lookup in situ (122)
    unit_hi = m['n_tok'] * c_canon                             # one fresh lookup per token, nothing cached
    unit_mid = unit_hi * (1 - m['cached'])
    compose = m['n_b'] * KEY_COMPOSE_NS / 1000
    gain = lambda unit: m['d_feat'] + m['int_look_saving'] - compose - unit
    dmax = m['d_feat'] + m['d_look']
    print(f"{ds}: c_canon {c_canon*1000:.1f} ns; ceiling (deletion) {dmax:.3f} us = {100*dmax/m['total']:.1f}%")
    for lab, u in (('pessimistic (unit ids cost one fresh lookup per token)', unit_hi), ('middle (131: 38% already cached)', unit_mid), ('optimistic (unit ids free)', 0.0)):
        g = gain(u); f = g / m['total']
        print(f"   {lab:58} gain {g:.3f} us = {100*f:.1f}%  -> projected {m['base_speedup']:.2f}x -> {m['base_speedup']/(1-f):.2f}x")
    f = dmax / m['total']; print(f"   {'ceiling: bigram path deleted outright (not exact)':58} gain {dmax:.3f} us = {100*f:.1f}%  -> {m['base_speedup']/(1-f):.2f}x")
    need = 1 - m['base_speedup'] / 7.8; print(f"   7.8x needs -{100*need:.1f}% of total = {need*m['total']:.3f} us (split units)")
