#!/usr/bin/env python3
"""Streaming equality checks for parse outputs and frozen stage oracles."""
import argparse
import gzip
import hashlib
import pathlib
import tempfile

ROOT = pathlib.Path("/Users/kyle/Desktop/HW/CSCI 201 Reserach")
RAW = ROOT / "research/experiments/179-phrase-recognition-flow/raw"


def digest_stream(f):
    h = hashlib.sha256()
    size = 0
    while True:
        b = f.read(1 << 20)
        if not b:
            return h.hexdigest(), size
        h.update(b)
        size += len(b)


def compare_streams(left, right):
    offset = 0
    hl, hr = hashlib.sha256(), hashlib.sha256()
    while True:
        a, b = left.read(1 << 20), right.read(1 << 20)
        hl.update(a)
        hr.update(b)
        if a != b:
            return False, offset, hl.hexdigest(), hr.hexdigest()
        offset += len(a)
        if not a:
            return True, offset, hl.hexdigest(), hr.hexdigest()


def regular_pair(label, a_path, b_path):
    with open(a_path, "rb") as a, open(b_path, "rb") as b:
        ok, n, ah, bh = compare_streams(a, b)
    print(f"{label}: {'IDENTICAL' if ok else 'MISMATCH'} bytes={n} sha256_left={ah} sha256_right={bh}")
    return ok


def negative_control():
    with tempfile.TemporaryDirectory(dir=RAW) as td:
        a = pathlib.Path(td) / "a"
        b = pathlib.Path(td) / "b"
        a.write_bytes(b"ordered labels\n")
        b.write_bytes(b"ordered labelx\n")
        with open(a, "rb") as af, open(b, "rb") as bf:
            ok, _, _, _ = compare_streams(af, bf)
        print(f"negative_control: {'FAILED' if ok else 'detected one-byte mismatch'}")
        return not ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--baseline", required=True)
    ap.add_argument("--candidate", required=True)
    ap.add_argument("--oracle-dir", required=True)
    ap.add_argument("--reference-dir", required=True)
    args = ap.parse_args()
    ok = regular_pair("final_parse", args.baseline, args.candidate)
    for name in ("O1", "O2", "O3k", "O3i", "O4"):
        candidate = pathlib.Path(args.oracle_dir) / f"{name}.txt"
        reference = pathlib.Path(args.reference_dir) / f"{name}.txt.gz"
        with open(candidate, "rb") as c, gzip.open(reference, "rb") as r:
            equal, n, ch, rh = compare_streams(r, c)
        print(f"{name}: {'IDENTICAL' if equal else 'MISMATCH'} bytes={n} sha256_reference={rh} sha256_candidate={ch}")
        ok &= equal
    ok &= negative_control()
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
