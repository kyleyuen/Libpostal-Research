"""Compare a dump_outputs JSONL (address, components) against a frozen baseline-outputs.jsonl (keyed by "id" = sha256(address) or by "address").
usage: compare_frozen.py dump.jsonl reference.jsonl   exit 0 iff every reference entry has an identical ordered component list and counts match."""
import hashlib, json, sys
dump = [json.loads(l) for l in open(sys.argv[1], encoding="utf-8")]
ref = {}
for l in open(sys.argv[2], encoding="utf-8"):
    r = json.loads(l)
    ref[r["id"] if "id" in r else hashlib.sha256(r["address"].encode()).hexdigest()] = r["components"]
got = {hashlib.sha256(d["address"].encode()).hexdigest(): d["components"] for d in dump}
missing = [k for k in ref if k not in got]
diff = [k for k in ref if k in got and got[k] != ref[k]]
print(f"reference={len(ref)} dump={len(dump)} unique_dump={len(got)} missing={len(missing)} mismatched={len(diff)}")
ok = not missing and not diff and len(got) == len(ref)
print("RESULT:", "IDENTICAL" if ok else "DIFFERENT")
sys.exit(0 if ok else 1)
