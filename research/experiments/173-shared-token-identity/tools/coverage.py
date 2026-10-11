#!/usr/bin/env python3
"""Bound potential reuse of the component FTT vocabulary slot from O2 oracles."""

import gzip
import sys
from pathlib import Path


def main() -> None:
    vocab_path = Path(sys.argv[1])
    vocab = set()
    with vocab_path.open("rb") as source:
        for line in source:
            vocab.add(line.rstrip(b"\n").rsplit(b"\t", 1)[0])
    print(f"vocab_keys={len(vocab)}", flush=True)
    for name in sys.argv[2:]:
        counts = dict(addresses=0, tokens=0, same=0, same_vocab=0, eligible=0, different_vocab=0)
        with gzip.open(name, "rb") as source:
            for line in source:
                row = line.rstrip(b"\n").split(b"\t")
                if row[1] == b"N":
                    counts["addresses"] += 1
                    continue
                counts["tokens"] += 1
                regular, admin = row[2], row[3]
                if regular == admin:
                    counts["same"] += 1
                    if regular in vocab:
                        counts["same_vocab"] += 1
                        if 0 < len(regular) <= 255 and b"|" not in regular:
                            counts["eligible"] += 1
                elif regular in vocab:
                    counts["different_vocab"] += 1
        print(f"{Path(name).parent.name}: " + " ".join(f"{k}={v}" for k, v in counts.items()), flush=True)


if __name__ == "__main__":
    main()
