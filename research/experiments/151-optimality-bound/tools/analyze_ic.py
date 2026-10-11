#!/usr/bin/env python3
"""151: instructions/cycles per address by the difference method ((P=12) - (P=2)) / (10 * N), per rep; median of reps."""
import re, statistics, sys, pathlib
D = pathlib.Path(__file__).resolve().parent.parent / "raw" / "ic"
N = {"dev": 10308, "osm": 11405}
def read(tag):
    t = (D / f"{tag}.stderr").read_text()
    ins = int(re.search(r"(\d+)\s+instructions retired", t).group(1)); cyc = int(re.search(r"(\d+)\s+cycles elapsed", t).group(1))
    return ins, cyc
for ds in ("dev", "osm"):
    for b in ("orig", "cpi"):
        I, C = [], []
        for r in (1, 2, 3):
            i12, c12 = read(f"{b}_{ds}_p12_r{r}"); i2, c2 = read(f"{b}_{ds}_p2_r{r}")
            I.append((i12 - i2) / (10 * N[ds])); C.append((c12 - c2) / (10 * N[ds]))
        mi, mc = statistics.median(I), statistics.median(C)
        print(f"{ds} {b:4s} instr/addr {mi:9.0f} {[round(x) for x in I]}  cycles/addr {mc:9.0f} {[round(x) for x in C]}  IPC {mi/mc:.2f}")
