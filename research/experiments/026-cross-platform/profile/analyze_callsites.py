"""Group macOS sample's top-level parser frames by source call site.

With whole-library LTO, sample attributes inlined work to the caller line.
These groups distinguish broad stages but cannot identify the inlined routine.
"""
import collections
import re
import sys

counts = collections.Counter()
for line in open(sys.argv[1]):
    match = re.match(r"^        \+ (\d+) address_parser_parse\b.*address_parser\.c:(\d+)$", line)
    if match:
        counts[int(match.group(2))] += int(match.group(1))

total = sum(counts.values())
print(f"parser samples={total}")
for source_line, count in counts.most_common():
    print(f"address_parser.c:{source_line} count={count} fraction_of_parser={count / total:.4f}")
