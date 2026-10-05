#!/usr/bin/env python3
"""Capture every 026 Windows source change relative to the frozen 021 package."""
from difflib import unified_diff
from pathlib import Path

base = Path(__file__).resolve().parent
repo = base.parents[3]
pairs = [
    (repo / "research/final/021-l4/sources/feature_dict.h", base / "sources/feature_dict.h"),
    (base / "upstream/src/Makefile.am", base / "src.Makefile.am"),
]
new = [
    base / "sources/win_vm.h",
    base / "sources/win_vm.c",
    base / "bench_ext_win.c",
    base / "dump_outputs_win.c",
    base / "configure_candidate.sh",
    base / "inspect_binaries.sh",
    base / "hash_artifacts.sh",
    base / "make_port_patch.py",
]
out = []
for a, b in pairs:
    out.extend(unified_diff(a.read_text().splitlines(True), b.read_text().splitlines(True), fromfile=str(a.relative_to(repo)), tofile=str(b.relative_to(repo))))
for b in new:
    out.extend(unified_diff([], b.read_text().splitlines(True), fromfile="/dev/null", tofile=str(b.relative_to(repo))))
(base / "port.patch").write_text("".join(out))
