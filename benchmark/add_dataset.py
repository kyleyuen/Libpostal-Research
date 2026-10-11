#!/usr/bin/env python3
"""Register a reviewer-supplied address file as a suite dataset (called by benchmark/run.sh --dataset).

usage: add_dataset.py CONFIG FILE NAME WORK_DIR

FILE is either plain text (one address per line) or TSV (two-letter country code, TAB, address).
The parser never receives the country column; the harness uses it only to group timings by country,
so plain text gets the placeholder country "xx". Blank lines are dropped, Windows line endings are
removed, and invalid UTF-8 is rejected with the offending line numbers. The prepared copies and a
provenance record go to WORK_DIR/datasets/NAME/; the repository is not modified. CONFIG (the
generated suite.json in WORK_DIR) gains the dataset and is set to time it for at least ~12 s of
optimized parsing per run, and to take memory and thread samples on it.
"""
import hashlib, json, math, os, re, sys, time

TARGET_OPT_SECONDS = 12.0      # per timed run of the optimized build (the suite requires >= 10 s)
EST_OPT_US = 3.0               # optimized warm µs/address measured on 14 datasets: about 2.5-3.3


def main():
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    config, src, name, work = sys.argv[1:]
    if not re.fullmatch(r"[A-Za-z0-9._-]{1,40}", name):
        sys.exit(f"--name must be 1-40 letters, digits, '.', '_' or '-': {name!r}")
    raw = open(src, "rb").read()
    lines, bad = [], []
    for i, b in enumerate(raw.split(b"\n"), 1):
        b = b.rstrip(b"\r")
        if not b.strip():
            continue
        try:
            b.decode("utf-8", errors="strict")
        except UnicodeDecodeError:
            bad.append(i); continue
        if b"\x00" in b:
            bad.append(i); continue
        lines.append(b)
    if bad:
        sys.exit(f"{src}: {len(bad)} line(s) are not valid UTF-8 or contain NUL bytes, e.g. lines {bad[:10]}; fix or remove them")
    if not lines:
        sys.exit(f"{src}: no addresses found")
    is_tsv = all(re.match(rb"[A-Za-z]{2}\t.", l) for l in lines)
    if is_tsv:
        tsv = [l[:2].lower() + l[2:] for l in lines]
        txt = [l.split(b"\t", 1)[1] for l in lines]
    else:
        txt = lines
        tsv = [b"xx\t" + l for l in lines]
    d = os.path.join(work, "datasets", name); os.makedirs(d, exist_ok=True)
    ptxt, ptsv = os.path.join(d, name + ".txt"), os.path.join(d, name + ".tsv")
    open(ptxt, "wb").write(b"\n".join(txt) + b"\n")
    open(ptsv, "wb").write(b"\n".join(tsv) + b"\n")
    n = len(txt)
    passes = max(3, min(100000, math.ceil(TARGET_OPT_SECONDS / (n * EST_OPT_US * 1e-6))))
    prov = {"name": name, "source_file": os.path.abspath(src), "source_sha256": hashlib.sha256(raw).hexdigest(),
            "format": "tsv" if is_tsv else "txt (country placeholder xx)", "addresses": n,
            "unique_addresses": len(set(txt)), "passes_per_timed_run": passes, "prepared": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
            "txt_sha256": hashlib.sha256(open(ptxt, "rb").read()).hexdigest(), "tsv_sha256": hashlib.sha256(open(ptsv, "rb").read()).hexdigest()}
    json.dump(prov, open(os.path.join(d, "provenance.json"), "w"), indent=1)
    c = json.load(open(config))
    c["datasets"] = [x for x in c["datasets"] if x["name"] != name] + [
        {"name": name, "source": "reviewer", "kind": "reviewer-supplied (never used for tuning or PGO)", "reserved": False, "txt": ptxt, "tsv": ptsv}]
    s = c["settings"]
    s["passes"], s["rounds"] = passes, 3
    s["settle_max_wait_s"] = min(s.get("settle_max_wait_s", 300), 300)   # reviewer runs: wait at most 5 min for a quiet machine; load is recorded per run
    s["steady_dataset"] = s["thread_check_dataset"] = name
    json.dump(c, open(config, "w"), indent=1)
    note = "" if n >= 100 else "  (fewer than 100 addresses: correctness still counts, but per-address timing is noisy)"
    print(f"dataset {name}: {n:,} addresses ({len(set(txt)):,} unique), {prov['format']}; {passes} passes per timed run{note}")


if __name__ == "__main__":
    main()
