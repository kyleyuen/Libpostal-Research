"""Median over runs of each stage (µs per address) for stage_bench outputs <variant>-<set>-r<n>.txt."""
import glob, re, sys, statistics as st, collections
D = sys.argv[1]; T = collections.defaultdict(lambda: collections.defaultdict(list))
for f in glob.glob(f"{D}/*.txt"):
    m = re.search(r"/(\w+)-(\w+)-r\d+\.txt$", f)
    if not m: continue
    for k, v in re.findall(r"^(\w+)_us=([\d.]+)", open(f).read(), re.M): T[(m.group(1), m.group(2))][k].append(float(v))
keys = ["normalize", "tokenize", "context_fill", "features", "lookups", "accumulate", "viterbi_and_rest_of_predict", "response_build", "response_destroy", "unattributed", "total"]
cols = sorted(T); print("| stage | " + " | ".join(f"{v} {s}" for v, s in cols) + " |"); print("|---|" + "---:|" * len(cols))
for k in keys: print(f"| {k} | " + " | ".join(f"{st.median(T[c][k]):.3f}" if T[c][k] else "" for c in cols) + " |")
