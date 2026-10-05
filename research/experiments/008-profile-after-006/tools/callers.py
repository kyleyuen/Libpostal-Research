"""Who calls the hot libc/alloc functions (immediate caller and caller-of-caller), and line-level exclusive time inside chosen functions.
usage: callers.py sample.txt func1,func2,... [linefunc1,linefunc2]"""
import re, sys, collections
text = open(sys.argv[1]).read().split("Call graph:\n", 1)[1].split("Total number in stack", 1)[0]
stack, nodes = [], []
for line in text.splitlines():
    m = re.match(r"^([ +!:|`-]*)(\d+) (.+?)(?:  \(in |$)", line)
    if not m: continue
    srcm = re.search(r"(\S+\.[ch]:\d+)\s*$", line)
    depth, count, name = len(m[1]), int(m[2]), re.sub(r"\s*\+\s*[\d,]+.*$", "", m[3].strip())
    while stack and stack[-1]["depth"] >= depth: stack.pop()
    n = dict(depth=depth, count=count, name=name, children=0, chain=[x["name"] for x in stack], src=(srcm[1] if srcm else None))
    if stack: stack[-1]["children"] += count
    stack.append(n); nodes.append(n)
total = sum(n["count"] for n in nodes if not n["chain"])
for f in sys.argv[2].split(","):
    own_total = 0; by = collections.Counter(); by2 = collections.Counter()
    for n in nodes:
        if n["name"] == f:
            own = n["count"] - n["children"]; own_total += own
            if n["chain"]: by[n["chain"][-1]] += own
            if len(n["chain"]) > 1: by2[n["chain"][-2] + " > " + n["chain"][-1]] += own
    print(f"\n== {f}: exclusive {100*own_total/total:.1f}% of samples; by caller:")
    for k, v in by.most_common(8): print(f"   {100*v/total:5.2f}%  {k}")
if len(sys.argv) > 3:
    for f in sys.argv[3].split(","):
        lines = collections.Counter()
        for n in nodes:
            if n["name"] == f and n["src"]: lines[n["src"]] += n["count"] - n["children"]
        print(f"\n== line-level exclusive inside {f}:")
        for k, v in lines.most_common(10): print(f"   {100*v/total:5.2f}%  {k}")
