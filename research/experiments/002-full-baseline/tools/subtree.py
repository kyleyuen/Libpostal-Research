"""Attribute a macOS `sample` call tree beneath chosen ancestor functions (inclusive, then exclusive with source lines).
usage: subtree.py sample.txt ancestor [ancestor ...]"""
import collections, re, sys
text = open(sys.argv[1]).read().split("Call graph:\n", 1)[1].split("Total number in stack", 1)[0]
stack, nodes = [], []
for line in text.splitlines():
    m = re.match(r"^([ +!:|`-]*)(\d+) (.+?)(?:  \(in |$)", line)
    if not m:
        continue
    srcm = re.search(r"(\S+\.[ch]:\d+)\s*$", line)
    depth, count, name, src = len(m[1]), int(m[2]), m[3], (srcm[1] if srcm else None)
    while stack and stack[-1]["depth"] >= depth:
        stack.pop()
    node = dict(depth=depth, count=count, name=name, src=src, children=0, anc=[n["name"] for n in stack])
    if stack:
        stack[-1]["children"] += count
    stack.append(node); nodes.append(node)
total = sum(n["count"] for n in nodes if not n["anc"])
print("total samples", total)
for target in sys.argv[2:]:
    sub = [n for n in nodes if n["name"] == target or target in n["anc"]]
    roots = [n for n in nodes if n["name"] == target]
    tsum = sum(n["count"] for n in roots)
    print(f"\n== {target}: {tsum} samples = {100*tsum/total:.1f}% of total")
    incl, excl, lines = collections.Counter(), collections.Counter(), collections.Counter()
    for n in sub:
        own = n["count"] - n["children"]
        idx = n["anc"].index(target) if target in n["anc"] else len(n["anc"])
        chain = set(n["anc"][idx:] + [n["name"]])
        for c in chain:
            incl[c] += own
        excl[n["name"]] += own
        if own and n["src"]:
            lines[(n["name"], n["src"])] += own
    print("-- inclusive within subtree (overlapping; % of the subtree)")
    for k, v in incl.most_common(14):
        print(f"  {100*v/tsum:5.1f}% {k}")
    print("-- exclusive within subtree (non-overlapping; % of the subtree)")
    for k, v in excl.most_common(14):
        print(f"  {100*v/tsum:5.1f}% {k}")
    print("-- exclusive by source line")
    for (k, s), v in lines.most_common(12):
        print(f"  {100*v/tsum:5.1f}% {k} {s}")
