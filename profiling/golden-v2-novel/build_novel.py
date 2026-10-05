"""Deterministically build a NOVEL address set from golden-v1's structured source fields.
Purpose: generalization/anti-overfit checks. Strings are re-assembled from components in styles and with perturbations that
differ from the original one-line addresses, so most are strings golden-v1 never contained. Never modifies golden-v1."""
import hashlib, json, csv
from pathlib import Path
root = Path(__file__).parent; v1 = root.parent / "golden-v1"
records = [json.loads(l) for l in (v1 / "records.jsonl").read_text(encoding="utf-8").splitlines()]
v1_strings = {r["address"] for r in records}

def h(*parts):  # deterministic pseudo-random int from record id and a salt
    return int(hashlib.sha256("|".join(parts).encode()).hexdigest(), 16)

def perturb(word, seed):
    """delete or transpose one interior character (creates unseen tokens)"""
    if len(word) < 5: return word
    i = 1 + seed % (len(word) - 2)
    return word[:i] + word[i + 1:] if seed % 2 else word[:i] + word[i + 1] + word[i] + word[i + 2:]

out = []
for r in records:
    row = r["source_rows"][0]; rid = r["id"]
    hn, road, city, pc = row["house_number"], row["road"], row["city"], row["postcode"]
    suburb, state, unit, level, cc = row["suburb"], row["state"], row["unit"], row["level"], row["country_code"]
    country = row["country"] or cc.upper()
    if not (road or city):        # too little structure to rebuild; use the original tokens reversed
        toks = r["address"].split(); toks.reverse(); out.append((r["country_codes"][0], " ".join(toks))); continue
    style = h(rid, "style") % 6
    if style == 0:   parts = [f"{hn} {road}", city, f"{state} {pc}".strip(), country]                       # US-like order
    elif style == 1: parts = [f"{road} {hn}", f"{pc} {city}".strip(), country]                               # EU-like order
    elif style == 2: parts = [f"{road} {hn} {unit}".strip(), suburb, f"{city} {pc}".strip()]                # unit and suburb inserted
    elif style == 3: parts = [country, f"{pc} {city}".strip(), f"{road} {hn}"]                              # country first, reverse
    elif style == 4: parts = [f"{hn} {road} {level}".strip(), f"{city} {state}".strip(), pc]                 # level appended
    else:            parts = [f"{city}", f"{road}", f"{hn}", pc]                                             # one field per segment
    parts = [p.strip() for p in parts if p and p.strip()]
    sep = [", ", " ", ",", " - "][h(rid, "sep") % 4]
    s = sep.join(parts)
    if h(rid, "typo") % 4 == 0 and road:                    # 25%: one typo in the road name
        words = road.split(); k = h(rid, "w") % len(words)
        s = s.replace(road, " ".join(words[:k] + [perturb(words[k], h(rid, "c"))] + words[k + 1:]), 1)
    case = h(rid, "case") % 5
    if case == 0: s = s.upper()
    elif case == 1: s = s.lower()
    s = " ".join(s.split())
    assert s and "\n" not in s and "\r" not in s and "\0" not in s
    out.append((r["country_codes"][0], s))

seen = set(); uniq = []
for c, s in out:
    if s in seen or s in v1_strings: s = s + " #" + str(len(uniq) % 97)   # keep every string unique and different from v1
    seen.add(s); uniq.append((c, s))
(root / "novel.txt").write_text("".join(s + "\n" for _, s in uniq), encoding="utf-8")
(root / "novel.tsv").write_text("".join(f"{c}\t{s}\n" for c, s in uniq), encoding="utf-8")
# country-labelled copy of golden-v1 in its frozen order (derived file; golden-v1 is untouched)
(root / "v1-all.tsv").write_text("".join(f"{r['country_codes'][0]}\t{r['address']}\n" for r in sorted(records, key=lambda r: r["id"])), encoding="utf-8")
print("novel addresses:", len(uniq), "| in golden-v1:", sum(1 for _, s in uniq if s in v1_strings))
