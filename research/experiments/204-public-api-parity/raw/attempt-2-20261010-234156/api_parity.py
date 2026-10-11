#!/usr/bin/env python3
"""Compare the public libpostal API of two builds (experiment 204). Standard library only.

usage: api_parity.py --original BIN --optimized BIN --input addresses.txt --out DIR

Runs every api_suite.c mode once per build, sequentially (one process at a time, one thread each),
and compares stdout, stderr and exit status byte-for-byte. libpostal's log lines contain
"(file.c:LINE)"; the optimized build changes line numbers in the files it modifies, so stderr is also
compared with those line numbers masked. A mode passes when stdout and exit status are identical and
stderr is identical after masking. Writes DIR/api_parity.json and the raw outputs.
"""
import argparse, hashlib, json, os, re, subprocess, sys, time

MODES = ["parse_options", "expand", "classify", "dedupe", "toponym", "normalize", "print_features", "lifecycle"]
LINE_NO = re.compile(rb"(\.[ch]):\d+\)")


def sha(b):
    return hashlib.sha256(b).hexdigest()


def run(binary, mode, inp, out_dir, tag):
    t0 = time.time()
    p = subprocess.run([binary, mode, inp], capture_output=True)
    for ext, data in (("stdout", p.stdout), ("stderr", p.stderr)):
        open(os.path.join(out_dir, f"{mode}.{tag}.{ext}"), "wb").write(data)
    return {"exit": p.returncode, "seconds": round(time.time() - t0, 3), "stdout_sha256": sha(p.stdout),
            "stdout_lines": p.stdout.count(b"\n"), "stderr_sha256": sha(p.stderr), "_out": p.stdout, "_err": p.stderr}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--original", required=True); ap.add_argument("--optimized", required=True)
    ap.add_argument("--input", required=True); ap.add_argument("--out", required=True)
    ap.add_argument("--modes", default=",".join(MODES))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    inp = open(a.input, "rb").read()
    res = {"started": time.strftime("%Y-%m-%dT%H:%M:%S%z"), "input": os.path.abspath(a.input), "input_sha256": sha(inp),
           "addresses": sum(1 for l in inp.splitlines() if l.strip()),
           "original": {"path": a.original, "sha256": sha(open(a.original, "rb").read())},
           "optimized": {"path": a.optimized, "sha256": sha(open(a.optimized, "rb").read())}, "modes": {}}
    for mode in a.modes.split(","):
        o = run(a.original, mode, a.input, a.out, "original")
        c = run(a.optimized, mode, a.input, a.out, "optimized")
        m = {"original": {k: v for k, v in o.items() if not k.startswith("_")},
             "optimized": {k: v for k, v in c.items() if not k.startswith("_")},
             "stdout_identical": o["_out"] == c["_out"], "exit_identical": o["exit"] == c["exit"],
             "stderr_identical": o["_err"] == c["_err"],
             "stderr_identical_ignoring_line_numbers": LINE_NO.sub(rb"\1:N)", o["_err"]) == LINE_NO.sub(rb"\1:N)", c["_err"])}
        m["pass"] = m["stdout_identical"] and m["exit_identical"] and m["stderr_identical_ignoring_line_numbers"]
        m["note"] = (f"both builds exit {o['exit']} identically after {o['stdout_lines']} lines (upstream behavior, reproduced)"
                     if m["pass"] and o["exit"] != 0 else "")
        res["modes"][mode] = m
        print(f"{mode:15s} {'PASS' if m['pass'] else 'FAIL'}  exit {o['exit']}/{c['exit']}  stdout lines {o['stdout_lines']}/{c['stdout_lines']}"
              f"{'' if m['stdout_identical'] else '  STDOUT DIFFERS'}{'' if m['stderr_identical_ignoring_line_numbers'] else '  STDERR DIFFERS'}"
              f"{'  NOTE: ' + m['note'] if m['note'] else ''}", flush=True)
    res["pass"] = all(m["pass"] for m in res["modes"].values())
    res["finished"] = time.strftime("%Y-%m-%dT%H:%M:%S%z")
    json.dump(res, open(os.path.join(a.out, "api_parity.json"), "w"), indent=1)
    print(f"api parity: {'PASS' if res['pass'] else 'FAIL'} ({sum(m['pass'] for m in res['modes'].values())}/{len(res['modes'])} modes) -> {a.out}/api_parity.json")
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
