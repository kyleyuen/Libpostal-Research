#!/usr/bin/env python3
# Q1 (ALGORITHM_RESEARCH.md 6.4): Amdahl ceilings p*T_now per audit item and per stage. Pure arithmetic on documented inputs; nothing is timed.
# Inputs: AUDIT_REPORT.md section 2 stage shares of build h and the per-function profile shares quoted in section 3 item text;
# T_now = 13.22 us (full set) and 14.67 us (floor set), T_base = 34.88 / 34.16 us (experiment 013).
# Rule for p: the narrowest *measured* share the audit quotes for the code the item touches; if none is quoted, the share of the stage(s) it touches
# (an item cannot remove more than the time of the code it changes). Load-time-only items get p = 0 for steady-state parse time.
T_NOW = {"full": 13.22, "floor": 14.67}
T_BASE = {"full": 34.88, "floor": 34.16}

STAGE = {  # audit section 2, build h, share of samples (upper value where a range is given)
    "ctx": (24.0, "context_fill"), "feat": (19.0, "feature generation"), "dict": (15.0, "dictionary lookups (12-15%)"),
    "acc": (8.7, "weight accumulation + zeroing"), "vit": (5.2, "Viterbi"), "alloc": (5.7, "allocation and freeing"),
    "norm": (10.0, "whole-string normalization (~10%)"), "tok": (2.4, "tokenizing"), "resp": (3.0, "response build/free (~3%)"),
    "load": (0.0, "load time (not in parse time)"), "all": (100.0, "whole program"),
}
S = lambda *k: sum(STAGE[x][0] for x in k)

# (id, primary stage, p %, basis quoted from the audit)
ITEMS = [
    ("N1", "norm", S("norm"), "stage: normalization (no function share quoted)"),
    ("N2", "norm", S("norm"), "stage: normalization"),
    ("N3", "norm", 1.4, "trie_get_prefix_from_index 1.2-1.4%"),
    ("N4", "norm", S("norm"), "stage: normalization"),
    ("N5", "norm", S("norm"), "stage: normalization"),
    ("N6", "norm", S("norm"), "part of N5 (not additive)"),
    ("N7", "norm", S("norm"), "stage: normalization"),
    ("N8", "norm", S("norm"), "stage: normalization"),
    ("N9", "norm", S("norm"), "stage: normalization"),
    ("N10", "tok", S("tok", "alloc"), "stages: tokenizing + allocation"),
    ("N11", "tok", S("tok"), "stage: tokenizing loop (prev2 clear is in feature gen)"),
    ("N12", "tok", S("tok"), "tokenizing 2.4% inclusive"),
    ("N13", "tok", 3.7, "_platform_strlen 3.7% (all callers)"),
    ("P1", "ctx", 12.6, "trie_search_tokens_from_index 12.6% own"),
    ("P2", "ctx", 3.0, "prefix/suffix searches ~3% inclusive"),
    ("P3", "ctx", 4.3, "normalize_token 4.3% inclusive"),
    ("P4", "ctx", 4.3, "normalize_token 4.3% inclusive"),
    ("P5", "ctx", S("ctx"), "stage: context_fill"),
    ("P6", "ctx", S("ctx"), "stage: context_fill"),
    ("P7", "ctx", S("ctx"), "is_valid_dictionary_phrase 0.3% + unquoted callee; stage cap"),
    ("P8", "ctx", 2.1, "graph_has_edge 2.1% own"),
    ("P9", "ctx", 12.6, "same searches as P1"),
    ("P10", "ctx", 15.0, "phrase search ~15% inclusive"),
    ("P11", "ctx", S("ctx"), "stage: context_fill"),
    ("P12", "ctx", 1.1, "_platform_strncmp 1.0-1.1%"),
    ("F1", "feat", S("feat"), "stage: feature generation (vocab lookup 3.7% is part)"),
    ("F2", "feat", S("feat", "dict"), "stages: feature gen + duplicate lookups"),
    ("F3", "dict", S("dict", "acc"), "stages: lookup + accumulate of 'bias'"),
    ("F4", "dict", S("dict"), "stage: dictionary lookups"),
    ("F5", "dict", 7.3, "fd_split 2.5% + memchr 1.4% + memcmp 3.4%"),
    ("F6", "dict", 2.5, "fd_split 2.5% (contains fd_find_name)"),
    ("F7", "feat", S("feat"), "stage: feature generation"),
    ("F8", "feat", S("feat"), "stage: feature generation"),
    ("F9", "feat", S("feat"), "covered by F1/F2"),
    ("F10", "feat", S("feat"), "stage: feature generation"),
    ("F11", "dict", S("dict"), "stage: dictionary lookups"),
    ("F12", "feat", 3.7, "_platform_strlen 3.7% (all callers)"),
    ("C1", "dict", S("dict", "acc"), "stages: lookup + accumulate"),
    ("C2", "acc", 1.8, "memset 1.8% own"),
    ("C3", "acc", S("acc"), "stage: accumulate/zeroing"),
    ("C4", "vit", S("vit"), "Viterbi 5.2%"),
    ("C5", "vit", S("vit"), "Viterbi 5.2%"),
    ("C6", "resp", S("resp"), "stage: response"),
    ("C7", "dict", S("dict"), "stage: dictionary lookups"),
    ("C8", "acc", S("acc"), "stage: accumulate"),
    ("C9", "acc", S("acc"), "stage: accumulate"),
    ("C10", "acc", S("acc"), "reads weights; audit expects speed-neutral"),
    ("C11", "acc", S("acc"), "stage: accumulate"),
    ("R1", "resp", 4.4, "cstring_array_to_strings 1.8 + destroy 0.6 + allocations ~2"),
    ("R2", "alloc", 8.0, "allocator 5.7% inclusive + zeroing 2.3%"),
    ("R3", "alloc", 1.2, "xzm_realloc + memmove 1.2% own"),
    ("R4", "resp", S("resp"), "covered by C6"),
    ("R5", "alloc", 8.0, "covered by R2"),
    ("R6", "resp", S("resp"), "stage: response"),
    ("R7", "resp", S("resp"), "stage: response"),
    ("L1", "load", 0.0, "memory item; lookup cost covered by P8"),
    ("L2", "load", 0.0, "load time only"),
    ("L3", "load", 0.0, "load time only"),
    ("L4", "dict", S("dict", "acc"), "layout of lookup + weight tables"),
    ("L5", "load", 0.0, "cold pass only"),
    ("L6", "load", 0.0, "load time only"),
    ("X1", "all", 100.0, "whole program"),
    ("X2", "all", 12.9, "stubs 0.8+1.1 + memmove 7.3 + strlen 3.7"),
    ("X3", "all", 100.0, "whole program (subsumed by X1)"),
    ("X4", "all", 100.0, "whole program"),
    ("X5", "feat", 8.0, "feature_array_add 8.0% inclusive"),
    ("3.1", "acc", S("feat", "dict", "acc"), "U-block keys: generation + lookup + accumulate"),
    ("3.2", "dict", S("feat", "dict"), "key building + lookup"),
]

def main():
    out = []
    out.append("## Q1a. Savings needed (6.4), recomputed: saved = T_now - T_base/S\n")
    out.append("| Target S | Full: saved us | Full % of T_now | Floor: saved us | Floor % of T_now |\n|---:|---:|---:|---:|---:|")
    for s in (3.5, 4, 5, 7):
        row = [f"{s}x"]
        for k in ("full", "floor"):
            need = T_NOW[k] - T_BASE[k] / s
            row += [f"{need:.2f}", f"{100 * need / T_NOW[k]:.1f}%"]
        out.append("| " + " | ".join(row) + " |")
    out.append("\n## Q1b. Per-item ceiling p*T_now (upper bound, not an estimate of the gain)\n")
    out.append("| Item | Stage | p % | Ceiling full us | Ceiling floor us* | Speedup if ceiling fully removed (full) | Basis for p |\n|---|---|---:|---:|---:|---:|---|")
    for iid, st, p, basis in ITEMS:
        cf, cl = p / 100 * T_NOW["full"], p / 100 * T_NOW["floor"]
        rest = T_NOW["full"] - cf
        sp = f"{T_BASE['full'] / rest:.2f}x" if rest > 1e-9 else "unbounded"
        out.append(f"| {iid} | {STAGE[st][1]} | {p:.1f} | {cf:.2f} | {cl:.2f} | {sp} | {basis} |")
    out.append("\n## Q1c. Per stage: stage ceiling and naive sum of item ceilings (overlapping; the stage ceiling is the binding bound)\n")
    out.append("| Stage | Stage share % | Stage ceiling full us | Stage ceiling floor us* | Items | Sum of item ceilings full us (overlaps) |\n|---|---:|---:|---:|---|---:|")
    tot = 0.0
    for st, (share, name) in STAGE.items():
        its = [i for i in ITEMS if i[1] == st]
        if not its: continue
        ssum = sum(i[2] for i in its) / 100 * T_NOW["full"]
        out.append(f"| {name} | {share:.1f} | {share / 100 * T_NOW['full']:.2f} | {share / 100 * T_NOW['floor']:.2f} | {', '.join(i[0] for i in its)} | {ssum:.2f} |")
        if st not in ("all", "load"): tot += share
    out.append(f"\nSum of the nine parse-path stage shares: {tot:.1f}% (shares are approximate and inclusive; the gap to 100% is profile overlap/omission).")
    for s in (3.5, 4, 5, 7):
        out.append(f"- {s}x on the full set needs {100 * (1 - T_BASE['full'] / s / T_NOW['full']):.1f}% of T_now removed; floor set {100 * (1 - T_BASE['floor'] / s / T_NOW['floor']):.1f}%.")
    out.append("\n*Floor-set column applies the full-set profile shares to 14.67 us: ASSUMPTION (no floor-set profile exists).")
    print("\n".join(out))
    # self-check: reproduce the 6.4 table as printed in ALGORITHM_RESEARCH.md
    exp = {3.5: (3.25, 4.91), 4: (4.50, 6.13), 5: (6.24, 7.84), 7: (8.24, 9.79)}
    for s, (a, b) in exp.items():
        assert abs(T_NOW["full"] - T_BASE["full"] / s - a) < 0.006 and abs(T_NOW["floor"] - T_BASE["floor"] / s - b) < 0.006, s

if __name__ == "__main__":
    main()
