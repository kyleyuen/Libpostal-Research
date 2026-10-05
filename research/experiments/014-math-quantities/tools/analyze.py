#!/usr/bin/env python3
# Experiment 014 analysis (no timing): Q3 line statistics, Q4 vocabulary Zipf/coverage, Q5 empirical top-N coverage, Q6 integer-key reach.
# Inputs are the files written by tools/trace.c (data/*.tsv.gz, raw/trace_dev.stdout, raw/model.stdout) and, for the Q3 formula, data/q2_chase.json.
# usage: analyze.py q3|q4|q5|q6
import gzip, json, math, os, statistics as st, sys
from collections import defaultdict

E = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
D = lambda *p: os.path.join(E, *p)
L = 20  # labels

def vocab_ranked():
    """(word bytes, count) sorted by count desc, ties by word bytes asc (deterministic top-N boundary)."""
    rows = []
    with gzip.open(D("data", "vocab_counts.tsv.gz"), "rb") as f:
        for line in f:
            w, c = line.rstrip(b"\n").rsplit(b"\t", 1)
            rows.append((w, int(c)))
    rows.sort(key=lambda r: (-r[1], r[0]))
    return rows

def fit(xs, ys):
    n = len(xs); mx, my = sum(xs) / n, sum(ys) / n
    sxx = sum((x - mx) ** 2 for x in xs); sxy = sum((x - mx) * (y - my) for x, y in zip(xs, ys)); syy = sum((y - my) ** 2 for y in ys)
    b = sxy / sxx; return -b, 1 - (syy - b * sxy) / syy, n

def q4():
    rows = vocab_ranked(); n = len(rows); counts = [c for _, c in rows]; total = sum(counts)
    cum = [0] * (n + 1)
    for i, c in enumerate(counts): cum[i + 1] = cum[i] + c
    over50 = sum(1 for c in counts if c > 50)
    out = ["## Q4. Vocabulary counts: coverage C(N), dense memory M(N), Zipf fit\n",
           f"Vocabulary words: {n:,}; sum of counts: {total:,}; max count {counts[0]:,} ({rows[0][0].decode()!r}); words with count > 50 (not n-gram words): {over50:,}; min count {counts[-1]}.",
           "Ranking: count descending, ties broken by word bytes ascending.\n",
           "| N | C(N) = top-N count share | C(N) among count>50 words only | M(N) dense = N*20*8 B | count of N-th word |", "|---:|---:|---:|---:|---:|"]
    tot50 = sum(c for c in counts if c > 50)
    for k in range(10, 23):
        N = 1 << k; m = min(N, n); m50 = min(N, over50)
        out.append(f"| 2^{k} = {N:,} | {cum[m] / total:.4f} | {cum[m50] / tot50:.4f} | {N * L * 8 / 1e6:.1f} MB ({N * L * 8 / 2**20:.1f} MiB) | {counts[m - 1]:,} |")
    out.append(f"\nLargest N with M(N) <= 40 MB (decimal, 40e6 B): {40_000_000 // (L * 8):,} (C = {cum[min(n, 250000)] / total:.4f}); "
               f"<= 40 MiB: {40 * 2**20 // (L * 8):,} (C = {cum[min(n, 262144)] / total:.4f}). Headroom basis: 2.613 - 2.557 GB (decimal) = 56 MB.")
    out.append("M(N) assumes dense 20-double vectors; a sparse layout would be smaller but its size depends on the rows' non-zeros (not computed).\n")
    out.append("Zipf fit: least squares of ln(count) on ln(rank) (rank from 1).\n\n| Fit range (ranks) | s | R^2 | points |\n|---|---:|---:|---:|")
    lr = [math.log(r) for r in range(1, n + 1)]; lc = [math.log(c) for c in counts]
    for a, b, lab in [(1, n, "all"), (1, over50, "count > 50 (1..%d)" % over50), (10, 100_000, "10..1e5"), (100, 1_000_000, "100..1e6"), (1000, n, "1000..all")]:
        s, r2, k = fit(lr[a - 1:b], lc[a - 1:b]); out.append(f"| {lab} | {s:.3f} | {r2:.4f} | {k:,} |")
    print("\n".join(out))

def q5():
    rows = vocab_ranked(); rank = {w: i + 1 for i, (w, _) in enumerate(rows)}
    toks = []
    with gzip.open(D("data", "dev_tokens.tsv.gz"), "rb") as f:
        next(f)
        for line in f:
            a, t, ty, isw, w, freq, bias, nbb, fs, pre, suf, ns, npp = line.rstrip(b"\n").split(b"\t")
            toks.append((w, int(freq), int(isw), int(bias), int(nbb), int(pre), int(suf)))
    n_all = len(toks); plain = [x for x in toks if x[3]]
    first = [x for x in plain if x[4] == 0]
    clean = [x for x in first if not x[5] and not x[6]]
    out = ["## Q5. Development split: share of plain tokens whose word is in the model's top-N words\n",
           f"Tokens scored by the CRF (3 bypassed addresses excluded): {n_all:,}. Plain tokens (`bias` emitted, i.e. not inside an address-dictionary or component phrase): {len(plain):,} ({len(plain) / n_all:.1%}).",
           f"Plain tokens where `bias` is the FIRST state feature (the U block is a prefix, Lemma 2 applies): {len(first):,} ({len(first) / len(plain):.1%} of plain). "
           f"The rest have postcode features (or `commonly X` with zero component types) before `bias`: {len(plain) - len(first):,}.",
           f"Of those, without a `prefix`/`suffix` feature (so the key does not need the pre-normalization token): {len(clean):,}.",
           f"Plain tokens with word count > 50 (not n-gram words): {sum(1 for x in plain if x[1] > 50):,}; count 1..50: {sum(1 for x in plain if 0 < x[1] <= 50):,}; unknown (count 0): {sum(1 for x in plain if x[1] == 0):,}.\n",
           "| N | plain tokens with word in top N | ... and count > 50 | bias-first plain tokens, word in top N, count > 50 | same, no prefix/suffix |", "|---:|---:|---:|---:|---:|"]
    def share(sub, N, need50):
        return sum(1 for x in sub if rank.get(x[0], 1 << 30) <= N and (not need50 or x[1] > 50))
    for N in (1 << 10, 1 << 12, 1 << 14, 1 << 16, 250_000, 1 << 18, 1 << 20, 1 << 22):
        a = share(plain, N, False); b = share(plain, N, True); c = share(first, N, True); d = share(clean, N, True)
        out.append(f"| {N:,} | {a / len(plain):.4f} ({a:,}) | {b / len(plain):.4f} | {c / len(plain):.4f} ({c:,}) | {d / len(plain):.4f} |")
    # model-predicted vs empirical: C(N) is a count share over the model's training text; the empirical share is over dev plain tokens
    counts = [c for _, c in rows]; total = sum(counts); cum = 0; pred = {}
    marks = {1 << 10, 1 << 12, 1 << 14, 1 << 16, 250_000, 1 << 18, 1 << 20}
    for i, c in enumerate(counts):
        cum += c
        if i + 1 in marks: pred[i + 1] = cum / total
    out.append("\n| N | model C(N) | empirical share of plain dev tokens in top N | difference |\n|---:|---:|---:|---:|")
    for N in sorted(pred):
        e = share(plain, N, False) / len(plain); out.append(f"| {N:,} | {pred[N]:.4f} | {e:.4f} | {e - pred[N]:+.4f} |")
    bar = sum(1 for x in toks if b"|" in x[0])
    out.append(f"\nDev tokens whose normalized word contains '|': {bar}.")
    print("\n".join(out))

def q6():
    V = set()
    with gzip.open(D("data", "vocab_counts.tsv.gz"), "rb") as f:
        for line in f: V.add(line.rstrip(b"\n").rsplit(b"\t", 1)[0])
    SYM = {b"UNKNOWN", b"UNKNOWN_NUMERIC"}
    by = defaultdict(lambda: [0, 0, 0, 0])  # lookups, novalue, inD (literal), inD with UNKNOWN symbols interned
    with gzip.open(D("data", "dev_features_014.tsv.gz"), "rb") as f:
        for line in f:
            kind, a, t, key = line.rstrip(b"\n").split(b"\t", 3)
            parts = key.split(b"|"); name, vals = parts[0], parts[1:]
            k = (kind.decode(), name.decode(errors="replace"), len(vals)); s = by[k]; s[0] += 1
            if not vals: s[1] += 1; continue
            if all(v in V for v in vals): s[2] += 1
            if all(v in V or v in SYM for v in vals): s[3] += 1
    tot = [0, 0, 0, 0]
    out = ["## Q6. Integer-key reach on the development split (all lookups, hits and misses)\n",
           "Lemma 5 predicate: key = name|p1|...|pk with k >= 1 and every p_i a vocabulary word. `bias` (no value) is a separate class. "
           "Column 'with UNKNOWN symbols' also accepts the two substitution strings UNKNOWN / UNKNOWN_NUMERIC, which are NOT vocabulary words but could be interned as extra ids.\n",
           "| Kind | Name | value parts | lookups | no value | R (literal) | R (with UNKNOWN symbols) |", "|---|---|---:|---:|---:|---:|---:|"]
    for k in sorted(by, key=lambda k: -by[k][0]):
        s = by[k]; tot = [x + y for x, y in zip(tot, s)]
        out.append(f"| {k[0]} | {k[1]} | {k[2]} | {s[0]:,} | {s[1]:,} | {s[2] / s[0]:.3f} | {s[3] / s[0]:.3f} |")
    out.append(f"| all | all | | {tot[0]:,} | {tot[1]:,} | {tot[2] / tot[0]:.3f} | {tot[3] / tot[0]:.3f} |")
    out.append(f"\nOverall R literal = {tot[2]:,}/{tot[0]:,} = {tot[2] / tot[0]:.4f}; with UNKNOWN symbols = {tot[3] / tot[0]:.4f}; constant no-value keys = {tot[1] / tot[0]:.4f} (F3 territory).")
    # word-role subset: features whose values are token words (ids available from the per-token vocabulary lookup), as report 3.2 assumes;
    # phrase-valued features only count as 'in D' when the phrase string happens to be a vocabulary word, and need their own lookup.
    WR = {("S", "word", 1), ("S", "prev word", 1), ("S", "next word", 1), ("S", "prev word+word", 2), ("S", "word+next word", 2), ("S", "first word", 1),
          ("S", "sub_word", 1), ("P", "word", 1), ("P", "prev word", 1)}
    w = [sum(by[k][i] for k in WR if k in by) for i in range(4)]
    out.append(f"Word-role features only ({', '.join(f'{k[0]}:{k[1]}' for k in sorted(WR))}): {w[0]:,} lookups = {w[0] / tot[0]:.3f} of all; "
               f"R literal {w[2] / w[0]:.4f}, with UNKNOWN symbols {w[3] / w[0]:.4f}; reach over ALL lookups: {w[2] / tot[0]:.4f} / {w[3] / tot[0]:.4f}.")
    # hits-only view and model-key counts from the C output
    tr = open(D("raw", "trace_dev.stdout")).read().splitlines(); i = next(j for j, l in enumerate(tr) if l.startswith("kind\tname\tlookups"))
    h = [l.split("\t") for l in tr[i + 1:] if l.strip()]
    hits = sum(int(r[3]) for r in h); hD = sum(int(r[8]) for r in h); h0 = sum(int(r[7]) for r in h)
    out.append(f"Hits only (C harness, literal predicate, names not split by arity): {hD:,}/{hits:,} = {hD / hits:.4f} of hit lookups are in D; no-value hits (bias, postcode have context, 1 first-word key) {h0:,}.")
    mo = open(D("raw", "model.stdout")).read().splitlines(); j = next(k for k, l in enumerate(mo) if l.startswith("kind\tname\tkeys"))
    m = [l.split("\t") for l in mo[j + 1:] if l.strip()]
    for kind in ("S", "P"):
        ks = [r for r in m if r[0] == kind]
        out.append(f"Model {kind} keys: {sum(int(r[2]) for r in ks):,}; in D: {sum(int(r[4]) for r in ks):,}; no value: {sum(int(r[3]) for r in ks):,}; "
                   f"keys with an empty value part: {sum(int(r[6]) for r in ks):,}.")
    print("\n".join(out))

def q3():
    rows = []
    with gzip.open(D("data", "addr_lines.tsv.gz"), "rt") as f:
        hdr = next(f).rstrip("\n").split("\t")
        for line in f: rows.append(dict(zip(hdr, map(int, line.rstrip("\n").split("\t")))))
    sc = [r for r in rows if not r["bypass"]]
    def dist(key):
        v = sorted(r[key] for r in sc); n = len(v)
        return f"| {key} | {st.mean(v):.1f} | {v[n // 2]} | {v[n // 10]} | {v[9 * n // 10]} | {v[-1]} |"
    out = ["## Q3. Cache lines (128 B) touched by the feature-weight lookups of build h's layout, per scored address (development split)\n",
           f"Scored addresses: {len(sc):,} (bypassed: {len(rows) - len(sc)}). Scope: state + transition dictionary probes (slots, arena keys) and CSR rows (indptr, indices, data). "
           "Excluded: vocabulary dictionary, phrase/postal tries, normalization, the per-address state matrices (so this is a floor for the lookup path only).\n",
           "| Quantity per address | mean | median | P10 | P90 | max |", "|---|---:|---:|---:|---:|---:|"]
    for k in ["nS", "nP", "touches", "distinct", "d_slot", "d_arena", "d_indptr", "d_indices", "d_data", "compulsory", "miss_128K", "miss_16M", "miss_32M", "miss_64M"]:
        out.append(dist(k))
    tot = lambda k: sum(r[k] for r in sc)
    out.append(f"\nTotals: lookups {tot('nS') + tot('nP'):,}; line touches {tot('touches'):,}; sum of per-address distinct lines {tot('distinct'):,}; "
               f"distinct lines over the whole split (compulsory) {tot('compulsory'):,}; LRU misses at 16 MiB {tot('miss_16M'):,} ({tot('miss_16M') / tot('distinct'):.3f} of per-address distinct).")
    out.append("miss_X = touches whose exact LRU stack distance over this stream is >= X (or first touch); the stream is ONLY these lookups, so real caches (shared with all other parser data) would miss more.")
    q2 = D("data", "q2_chase.json")
    if os.path.exists(q2):
        c = json.load(open(q2)); lam, M = c["lambda_ns"], c["M_max"]
        out.append(f"\nLittle's law floor, T_mem_min = N * lambda / M_max with lambda = {lam:.1f} ns and M_max = {M:.1f} (Q2):\n")
        out.append("| N used | N (mean per address) | T_mem_min us | share of 13.22 us |\n|---|---:|---:|---:|")
        for lab, k in [("distinct lines per address (every address starts cold)", "distinct"), ("LRU misses at 16 MiB (lookup stream only)", "miss_16M"),
                       ("LRU misses at 64 MiB", "miss_64M")]:
            N = tot(k) / len(sc); t = N * lam / M / 1000; out.append(f"| {lab} | {N:.1f} | {t:.3f} | {t / 13.22:.1%} |")
        out.append(f"\nWith no overlap at all (M = 1): distinct lines x lambda = {tot('distinct') / len(sc) * lam / 1000:.2f} us per address.")
    else:
        out.append("\n(q2_chase.json not present yet: formula result pending Q2.)")
    print("\n".join(out))

if __name__ == "__main__":
    {"q3": q3, "q4": q4, "q5": q5, "q6": q6}[sys.argv[1]]()
