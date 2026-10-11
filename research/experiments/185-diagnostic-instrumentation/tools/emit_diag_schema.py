#!/usr/bin/env python3
"""Emit runner schema from the C runtime registry (no duplicated definitions)."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
text = (root / "source" / "diag.c").read_text(encoding="utf-8")

def cstrings(body):
    return [json.loads('"' + x + '"') for x in re.findall(r'"((?:\\.|[^"\\])*)"', body)]

defs_body = re.search(r"static const char \*defs\[\]\[2\] = \{(.*?)\n\};", text, re.S).group(1)
metrics = [
    {"name": json.loads('"' + name + '"'), "definition": json.loads('"' + desc + '"')}
    for name, desc in re.findall(r'\{"((?:\\.|[^"\\])*)",\s*"((?:\\.|[^"\\])*)"\}', defs_body)
]
stages = cstrings(re.search(r'const char \*stages\[\]\s*=\s*\{(.*?)\};', text, re.S).group(1))
families = cstrings(re.search(r'const char \*families\[\]\s*=\s*\{(.*?)\};', text, re.S).group(1))
print(json.dumps({
    "schema_version": 1,
    "metrics": metrics,
    "stages": stages,
    "families": families,
    "repeated_pattern_fields": ["pattern_id", "dictionary_id", "operation", "stages", "stage_occurrences", "families", "address_groups", "queries", "executed", "reused", "definition"],
    "repeated_patterns_summary_fields": ["distinct_patterns_total", "observations_total", "retained_patterns", "omitted_patterns", "omitted_observations", "coverage", "truncated", "query_table_overflow_addresses", "untracked_lookup_calls", "untracked_query_events", "summary_definition"],
}, indent=2, sort_keys=True))
