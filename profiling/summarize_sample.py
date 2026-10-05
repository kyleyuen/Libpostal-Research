"""Aggregate macOS sample call-tree counts, accounting for inclusive overlap."""
import collections
import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent
graph = (root / "parser.sample.txt").read_text().split("Call graph:\n", 1)[1]
graph = graph.split("Total number in stack", 1)[0]
stack, nodes = [], []
for line in graph.splitlines():
    match = re.match(r"^([ +!:|`-]*)(\d+) (.+?)(?:  \(in |$)", line)
    if not match:
        continue
    depth, count, name = len(match[1]), int(match[2]), match[3]
    while stack and stack[-1]["depth"] >= depth:
        stack.pop()
    node = dict(depth=depth, count=count, name=name, children=0,
                ancestors=[parent["name"] for parent in stack])
    if stack:
        stack[-1]["children"] += count
    stack.append(node)
    nodes.append(node)

total = sum(node["count"] for node in nodes if not node["ancestors"])
inclusive, exclusive = collections.Counter(), collections.Counter()
for node in nodes:
    own = node["count"] - node["children"]
    assert own >= 0, node
    exclusive[node["name"]] += own
    for name in set(node["ancestors"] + [node["name"]]):
        inclusive[name] += own
assert total > 0 and sum(exclusive.values()) == total
result = dict(total_samples=total, inclusive=inclusive.most_common(),
              exclusive=exclusive.most_common())
(root / "profile-summary.json").write_text(json.dumps(result, indent=2) + "\n")
for label, counts in (("INCLUSIVE (overlapping)", inclusive), ("EXCLUSIVE", exclusive)):
    print(label)
    for name, count in counts.most_common(30):
        print(f"{100 * count / total:5.1f}% {name}")
