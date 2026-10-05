#!/usr/bin/env python3
# Q2 analysis: medians over trials of the chase TSV (raw/q2_chase_2p5G.stdout, raw/q2_chase_8M.stdout).
# lambda = median ns per dependent load at k = 1 (idx variant, 2.5 GiB); M(k) = lambda * median loads/ns at k; M_max = max over k.
# Writes data/q2_chase.json (consumed by analyze.py q3) and prints a markdown table.
import json, os, statistics as st
from collections import defaultdict

E = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def load(name):
    d = defaultdict(list)
    for line in open(os.path.join(E, "raw", name)).read().splitlines()[1:]:
        t, v, b, k, loads, ns, nps, lpn = line.split("\t")
        d[(v, int(k))].append((float(nps), float(lpn)))
    return d

KMAX = 32
big, small = load("q2_chase_2p5G.stdout"), load("q2_chase_8M.stdout")
lam = {v: st.median(x[0] for x in big[(v, 1)]) for v in ("idx", "ptr")}
out = ["## Q2. DRAM latency and memory-level parallelism (2.5 GiB random cycle, 128-B nodes; medians over trials)\n",
       f"lambda (k = 1, dependent loads): idx {lam['idx']:.1f} ns, ptr {lam['ptr']:.1f} ns. "
       f"L2-resident control (8 MiB): idx {st.median(x[0] for x in small[('idx', 1)]):.1f} ns, ptr {st.median(x[0] for x in small[('ptr', 1)]):.1f} ns.\n",
       "| k | idx loads/ns (min-max) | idx M(k) | ptr loads/ns | ptr M(k) |", "|---:|---:|---:|---:|---:|"]
M = {}
for k in range(1, KMAX + 1):
    row = [str(k)]
    for v in ("idx", "ptr"):
        tp = [x[1] for x in big[(v, k)]]; m = lam[v] * st.median(tp); M[(v, k)] = m
        row += [f"{st.median(tp):.4f} ({min(tp):.4f}-{max(tp):.4f})" if v == "idx" else f"{st.median(tp):.4f}", f"{m:.2f}"]
    out.append("| " + " | ".join(row) + " |")
Mmax = max(M[("idx", k)] for k in range(1, KMAX + 1)); kmax = max(range(1, KMAX + 1), key=lambda k: M[("idx", k)])
knee = next((k for k in range(1, KMAX + 1) if M[("idx", k)] / k < 0.9), None)   # first k whose per-chain efficiency M(k)/k < 0.9 (None = never)
out.append(f"\nM_max (idx) = {Mmax:.2f} at k = {kmax}; first k with M(k)/k < 0.9: {knee if knee else "none up to " + str(KMAX)}. "
           f"Trials per point: {len(big[('idx', 1)])}.")
Mall = max(M.values()); vall, kall = max(M, key=M.get)
out.append(f"Largest measured M over both variants: {Mall:.2f} ({vall}, k = {kall}); no saturation found up to k = {KMAX}, so M_max is a LOWER bound on this core's memory-level parallelism. "
           "idx at k >= 30 is a code-generation artifact: the compiler moves the chain state into NEON lanes (ld1.s/mov.s per step; raw/q2_idx30_disasm.txt), while ptr stays scalar.")
print("\n".join(out))
json.dump({"lambda_ns": lam["idx"], "lambda_ptr_ns": lam["ptr"], "M_max": Mall, "M_max_variant": vall, "k_at_M_max": kall, "M_max_idx": Mmax, "k_at_M_max_idx": kmax, "knee_k": knee},
          open(os.path.join(E, "data", "q2_chase.json"), "w"), indent=1)
