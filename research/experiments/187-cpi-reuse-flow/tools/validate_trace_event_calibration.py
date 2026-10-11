#!/usr/bin/env python3
import json
import sys

rows = [json.loads(line) for line in open(sys.argv[1], encoding="utf-8")]
assert rows[0]["record_type"] == "trace_header"
assert rows[0]["within_parse_stability_proof"]["proof_id"] == "cpi-within-parse-readonly-v1"
assert rows[0]["event_cap_scope"] == "query_and_consumer_rows"
assert rows[0]["overflow_policy"] == "fail_closed_data_rows"
queries = [r for r in rows if r["record_type"] == "query"]
consumers = [r for r in rows if r["record_type"] == "query_consumer"]
assert len(queries) == 6 and len(consumers) == 3
assert queries[0]["result_value"] == 17 and queries[1]["source_query_index"] == queries[0]["query_index"]
assert queries[0]["dictionary_id"] != queries[2]["dictionary_id"]
assert queries[0]["operation"] != queries[3]["operation"]
assert all(r["consumer_dictionary_id"] == queries[0]["dictionary_id"] for r in consumers)
miss = queries[-1]
assert miss["hit"] is False and miss["result_kind"] == "vocab_frequency" and miss["result_value"] == 0
assert miss["site_id"] == "calibration.physical"
precomputed = next(r for r in queries if r["query_kind"] == "precomputed_id")
assert precomputed["site_id"] == "calibration.precomputed_id"
summary = next(r for r in rows if r["record_type"] == "address_summary")
assert summary["query_events"] == len(queries)
assert summary["query_consumer_events"] == len(consumers)
assert summary["answer_reuses"] == 1 and summary["precomputed_id_events"] == 1
assert summary["consumer_complete"] is False
assert rows[-1]["record_type"] == "trace_end" and rows[-1]["complete"] is True
assert rows[-1]["query_events"] == len(queries)
assert rows[-1]["consumer_events"] == len(consumers)
assert rows[-1]["total_event_rows"] == len(queries) + len(consumers)
print("runtime trace calibration passed")
