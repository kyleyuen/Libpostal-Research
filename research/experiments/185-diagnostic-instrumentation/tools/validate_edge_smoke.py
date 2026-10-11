#!/usr/bin/env python3
"""Validate one-address diagnostic sidecars with the production runner parser."""
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
EXP = ROOT / "research/experiments/185-diagnostic-instrumentation"
SMOKE = EXP / "raw/smoke-20261007-1540-stable-labels"
CONFIG = json.loads((EXP / "config.json").read_text())
RUNNER_PATH = EXP / "runner-audit/source/run_measurements.py"
spec = importlib.util.spec_from_file_location("measurement_runner", RUNNER_PATH)
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)

reference = (SMOKE / "original-000.stdout").read_bytes()
for name in ("original", "cpi", "prototype181"):
    diagnostic_stdout = (SMOKE / f"{name}-diagnostic.stdout").read_bytes()
    release_stdout = (SMOKE / f"{name}-release.stdout").read_bytes()
    assert diagnostic_stdout == reference, f"{name} diagnostic stdout differs from original-000"
    assert release_stdout == reference, f"{name} release stdout differs from original-000"

    metrics_path = SMOKE / f"{name}-diagnostic.metrics.json"
    metrics = runner.read_metrics(
        metrics_path,
        CONFIG["metrics"],
        expected_addresses=1,
        breakdown_dimensions=CONFIG["breakdown_dimensions"],
        require_repeat_summary=CONFIG.get("require_repeat_pattern_summary", False),
        breakdown_complete=CONFIG.get("breakdown_complete"),
    )
    assert metrics["lookup_calls"]["value"] == metrics["lookup_hits"]["value"] + metrics["lookup_misses"]["value"]
    if name == "prototype181":
        stage_calls = metrics["_breakdowns"]["by_stage"]["lookup_calls"]["values"]
        assert stage_calls["context_fill"]["value"] > 0, "known-word context_fill lookups must be attributed"
        assert any(p["dictionary_id"] == "word_vocab" and "context_fill" in p["stages"]
                   for p in metrics["_repeated_patterns"]), "word-vocabulary repeats must retain context_fill stage"
    print(json.dumps({
        "variant": name,
        "output_bytes": len(diagnostic_stdout),
        "tokens": metrics["_tokens"],
        "lookup_calls": metrics["lookup_calls"]["value"],
        "lookup_hits": metrics["lookup_hits"]["value"],
        "lookup_misses": metrics["lookup_misses"]["value"],
        "repeated_queries": metrics["repeated_queries"]["value"],
        "answers_reused": metrics["answers_reused"]["value"],
        "repeated_patterns": len(metrics["_repeated_patterns"]),
        "pattern_summary": metrics["_repeated_patterns_summary"],
    }, sort_keys=True))

print("edge-address outputs match original-000; all three metric sidecars pass run_measurements.read_metrics")
