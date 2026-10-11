#!/usr/bin/env python3
"""Validate original diagnostic golden-v1 output and sidecar using the production parser."""
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
EXP = ROOT / "research/experiments/185-diagnostic-instrumentation"
RAW = EXP / "raw/golden-v1-original-validation-20261007-pattern-summary-fix"
INPUT = ROOT / "profiling/golden-v1/all.txt"
REFERENCE = ROOT / "profiling/golden-v1/baseline-outputs.jsonl"
config = json.loads((EXP / "config.json").read_text())
runner_path = EXP / "runner-audit/source/run_measurements.py"
spec = importlib.util.spec_from_file_location("measurement_runner", runner_path)
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)

expected = sum(1 for line in INPUT.read_bytes().splitlines() if line.strip())
diag_output = (RAW / "original-diagnostic.stdout").read_bytes()
baseline_output = (RAW / "original-000.stdout").read_bytes()
assert diag_output == baseline_output, "diagnostic outputs differ from preserved original executable"
frozen_check = runner.compare_frozen_dump(RAW / "original-diagnostic.stdout", REFERENCE)
assert frozen_check["identical"], f"diagnostic outputs differ from frozen golden-v1 reference: {frozen_check}"

metrics = runner.read_metrics(
    RAW / "original-diagnostic.metrics.json",
    config["metrics"],
    expected_addresses=expected,
    breakdown_dimensions=config["breakdown_dimensions"],
    require_repeat_summary=config.get("require_repeat_pattern_summary", False),
    breakdown_complete=config.get("breakdown_complete"),
)
summary = metrics["_repeated_patterns_summary"]
context_stage = metrics["_breakdowns"]["by_stage"]["lookup_calls"]["values"]["context_fill"]["value"]
print(json.dumps({
    "addresses": expected,
    "output_bytes": len(diag_output),
    "frozen_check": frozen_check,
    "lookup_calls": metrics["lookup_calls"]["value"],
    "lookup_hits": metrics["lookup_hits"]["value"],
    "lookup_misses": metrics["lookup_misses"]["value"],
    "repeated_queries": metrics["repeated_queries"]["value"],
    "context_fill_lookup_calls": context_stage,
    "repeated_patterns_retained": len(metrics["_repeated_patterns"]),
    "repeated_patterns_summary": summary,
}, sort_keys=True))
print("golden-v1 original diagnostic output matches both protected references; production metrics validation passed")
