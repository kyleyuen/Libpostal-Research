#!/usr/bin/env python3
"""Assertions for the standalone diagnostic runtime calibration outputs."""
import json
import sys
from pathlib import Path


def value(obj, key):
    item = obj["counters"][key]
    return item["value"] if item["value"] is not None else None


def check_basic(path):
    data = json.loads(Path(path).read_text())
    assert data["addresses"] == 2
    assert value(data, "lookup_calls") == 5
    assert value(data, "lookup_hits") == 4
    assert value(data, "lookup_misses") == 1
    assert value(data, "repeated_queries") == 2
    assert value(data, "answers_reused") == 1
    calls = data["breakdowns"]["by_stage"]["lookup_calls"]
    assert all(v["value"] is not None for v in calls.values())
    assert sum(v["value"] for v in calls.values()) == 5
    assert calls["crf_state_lookup"]["value"] == 2
    assert calls["feature_generation"]["value"] == 1
    assert calls["crf_transition_lookup"]["value"] == 2
    patterns = data["repeated_patterns"]
    assert len(patterns) == 1  # exported stable semantic pattern aggregates same-family dictionary instances
    assert patterns[0]["address_groups"] == 2
    assert patterns[0]["queries"] == patterns[0]["executed"] + patterns[0]["reused"] == 5
    assert patterns[0]["executed"] == 4 and patterns[0]["reused"] == 1
    assert patterns[0]["stage_occurrences"] == {"crf_state_lookup": 1, "feature_generation": 1, "context_fill": 1, "crf_transition_lookup": 2}


def check_nested_labels(path):
    data = json.loads(Path(path).read_text())
    assert value(data, "lookup_calls") == 1
    assert data["breakdowns"]["by_stage"]["lookup_calls"]["context_fill"]["value"] == 1
    assert data["breakdowns"]["by_stage"]["lookup_calls"]["unclassified"]["value"] == 0
    assert data["breakdowns"]["by_family"]["lookup_calls"]["state_feature"]["value"] == 1
    assert data["breakdowns"]["by_family"]["lookup_calls"]["address_phrase"]["value"] is None


def check_label_failure(path, expected):
    text = Path(path).read_text()
    assert expected in text


def check_query_overflow(path):
    data = json.loads(Path(path).read_text())
    assert value(data, "lookup_calls") == 4098
    assert value(data, "lookup_hits") == 4098
    assert value(data, "lookup_misses") == 0
    assert value(data, "repeated_queries") is None
    assert value(data, "tracked_repeated_queries") == 0
    assert value(data, "untracked_lookup_calls") == 2
    summary = data["repeated_patterns_summary"]
    assert summary["query_table_overflow_addresses"] == 1
    assert summary["distinct_patterns_total"] is None and summary["observations_total"] is None
    assert summary["truncated"] is True


def check_pattern_overflow(path):
    data = json.loads(Path(path).read_text())
    summary = data["repeated_patterns_summary"]
    assert len(data["repeated_patterns"]) == summary["retained_patterns"] == 128
    assert summary["distinct_patterns_total"] == 129
    assert summary["observations_total"] == 260
    assert summary["omitted_patterns"] == 1 and summary["omitted_observations"] == 4
    assert summary["coverage"] == "first_128_encountered_repeat_groups"
    assert summary["truncated"] is True
    assert sum(p["queries"] for p in data["repeated_patterns"]) + summary["omitted_observations"] == summary["observations_total"]


if __name__ == "__main__":
    if len(sys.argv) not in (4, 5, 7):
        raise SystemExit("usage: validate_diag_calibration.py basic.json query-overflow.json pattern-overflow.json [nested-labels.json [long-label.stderr pool-full.stderr]]")
    check_basic(sys.argv[1])
    check_query_overflow(sys.argv[2])
    check_pattern_overflow(sys.argv[3])
    if len(sys.argv) == 5:
        check_nested_labels(sys.argv[4])
    if len(sys.argv) == 7:
        check_nested_labels(sys.argv[4])
        check_label_failure(sys.argv[5], "label length exceeds 63 bytes")
        check_label_failure(sys.argv[6], "intern pool capacity of 512 distinct labels exceeded")
    print("diagnostic calibration assertions passed (basic, query cap, pattern cap, nested labels)")
