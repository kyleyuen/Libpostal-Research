#!/usr/bin/env python3
import json
import sys

with open(sys.argv[1], encoding="utf-8") as f:
    rows = [json.loads(line) for line in f if line.strip()]
if len(sys.argv) > 2 and sys.argv[2] == "overflow":
    assert rows[0]["record_type"] == "trace_header"
    assert any(r["record_type"] == "trace_overflow" for r in rows)
    end = rows[-1]
    assert end["record_type"] == "trace_end"
    assert end["overflow"] is True and end["complete"] is False
    assert len([r for r in rows if r["record_type"] == "query"]) == 2
    print("Trace overflow calibration passed: cap stops query records and marks the run incomplete.")
    sys.exit(0)
assert rows[0]["record_type"] == "trace_header"
assert rows[0]["schema_version"] == 1 and rows[0]["expected_addresses"] == 1
queries = [r for r in rows if r["record_type"] == "query"]
assert len(queries) == 3
assert [r["query_kind"] for r in queries] == ["physical_search", "physical_search", "answer_reuse"]
assert [r["physical_search_index"] for r in queries] == [1, 2, None]
canonical_key = "070000000000000000000000000000006d61696e007374"
assert queries[0]["key_hex"] == canonical_key and queries[1]["key_hex"] == queries[0]["key_hex"]
assert queries[1]["producer_query_index"] == 1 and queries[1]["producer_stage"] == "fixture_producer"
assert queries[1]["request_stage"] == "fixture_consumer"
assert queries[2]["producer_query_index"] == 1 and queries[2]["safe_reuse_proof"] == "unknown"
assert queries[2]["hit"] is True and all(r["dictionary_generation"] is None for r in queries)
summary = next(r for r in rows if r["record_type"] == "address_summary")
assert summary["logical_requests"] == 3 and summary["physical_searches"] == 2 and summary["answer_reuses"] == 1
assert summary["hits"] == 1 and summary["misses"] == 1
assert summary["feature_keys_materialized"] == 2 and summary["feature_key_bytes_materialized"] == 19
assert summary["model_rows"] == 1 and summary["weights_accumulated"] == 3
assert summary["scoped_allocation_calls"] == 1 and summary["scoped_allocation_requested_bytes"] == 64
end = rows[-1]
assert end["record_type"] == "trace_end" and end["addresses"] == end["expected_addresses"] == 1
assert end["overflow"] is False and end["complete"] is True
print("Trace runtime calibration passed: 3 exact-key events, stage linkage, counts, summary, and completeness.")
