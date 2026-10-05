"""102 summary: per run, median µs/address of passes 2-20; per binary, median over the 5 rounds; speedup vs original and per-round ratio vs vmx; max peak footprint."""
import re, glob, statistics as st, os
D = os.path.join(os.path.dirname(__file__), "../raw/timing"); names = ["orig", "rt8", "cpi"]
def run(r, n):
    out = open(f"{D}/u_r{r}_{n}.stdout").read(); err = open(f"{D}/u_r{r}_{n}.stderr").read()
    p = [float(u) for k, u in re.findall(r"^pass=(\d+) us_per_addr=([\d.]+)", out, re.M) if int(k) >= 2]; assert len(p) == 19
    return st.median(p), int(re.search(r"(\d+)\s+peak memory footprint", err)[1])
R = {n: [run(r, n) for r in range(1, 6)] for n in names}
print("| binary | unseen-v10 µs/address | speedup vs original (range) | vs rt8 (per-round range) | max peak footprint |\n|---|---:|---:|---:|---:|")
for n in names:
    t = [x[0] for x in R[n]]; so = [o[0] / x for o, x in zip(R["orig"], t)]; sv = [v[0] / x for v, x in zip(R["rt8"], t)]
    print(f"| {n} | {st.median(t):.3f} | {st.median(so):.3f}× ({min(so):.2f}–{max(so):.2f}) | {st.median(sv):.4f} ({min(sv):.3f}–{max(sv):.3f}) | {max(x[1] for x in R[n]) / 1e9:.3f} GB |")
for a, b in (("cpi", "rt8"),):
    q = [y[0] / x[0] for x, y in zip(R[a], R[b])]; print(f"{a} vs {b} per round: {[round(v, 4) for v in q]} median {st.median(q):.4f}")
