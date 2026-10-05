"""Build deterministic benchmark inputs from the preserved Senzing CSV."""
import collections
import csv
import hashlib
import json
from pathlib import Path

root = Path(__file__).parent / "golden-v1"
raw = root / "raw" / "senzing-v1.2.0.csv"
rows = list(csv.DictReader(raw.open(newline="", encoding="utf-8")))
groups = collections.defaultdict(list)
for row in rows:
    address = row["full_address"]
    assert address.strip() and not any(c in address for c in "\r\n\0")
    groups[address].append(row)

records = []
for address, source_rows in groups.items():
    records.append(dict(id=hashlib.sha256(address.encode()).hexdigest(),
                        address=address,
                        country_codes=sorted({r["country_code"] for r in source_rows}),
                        source_rows=source_rows))

# Hash ordering is deterministic across Python versions and does not group countries.
records.sort(key=lambda r: r["id"])
countries = collections.defaultdict(list)
for record in records:
    countries[record["country_codes"][0]].append(record)
for country_records in countries.values():
    validation_count = max(1, len(country_records) // 5)
    for index, record in enumerate(country_records):
        record["split"] = "validation" if index < validation_count else "development"

for name in ("all", "development", "validation"):
    selected = [r for r in records if name == "all" or r["split"] == name]
    (root / f"{name}.txt").write_text("".join(r["address"] + "\n" for r in selected))
(root / "records.jsonl").write_text("".join(json.dumps(r, ensure_ascii=False) + "\n" for r in records))
assert len({r["id"] for r in records}) == len(records)
dev = {r["id"] for r in records if r["split"] == "development"}
val = {r["id"] for r in records if r["split"] == "validation"}
assert not dev & val and len(dev | val) == len(records)
stats = dict(raw_records=len(rows), unique_addresses=len(records),
             duplicate_records=len(rows)-len(records), countries=len(countries),
             development=len(dev), validation=len(val),
             non_ascii_addresses=sum(not r["address"].isascii() for r in records),
             max_utf8_bytes=max(len(r["address"].encode()) for r in records),
             country_counts={c: len(v) for c, v in sorted(countries.items())},
             raw_source_counts=dict(collections.Counter(r["source"] for r in rows)))
(root / "statistics.json").write_text(json.dumps(stats, indent=2) + "\n")
print(json.dumps({k: v for k, v in stats.items() if not isinstance(v, dict)}, indent=2))
