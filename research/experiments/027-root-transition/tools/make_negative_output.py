#!/usr/bin/env python3
"""Create one deliberately wrong component in a disposable output copy."""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_bytes()
wrong = source.replace(b'"road"', b'"city"', 1)
if wrong == source:
    raise SystemExit("no road component to corrupt")
Path(sys.argv[2]).write_bytes(wrong)
