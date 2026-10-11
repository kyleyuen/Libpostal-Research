#!/usr/bin/env python3
"""Small exact-output and counter-reconciliation calibration for 201."""
import datetime as dt
import fcntl
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = ROOT / "research/experiments/201-cpi-work-and-memory-prep/raw/token-storage-diagnostics-v2/calibration-small-v1"
LOCK = ROOT / "research/analysis/parallel-exploration-2026-10-07/WORKLOAD.lock"
GATE = ROOT / "research/tools/env_gate.sh"
spec = importlib.util.spec_from_file_location("run_measurements", ROOT / "research/tools/run_measurements.py")
rm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rm)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_metric(path: Path, key: str):
    item = json.loads(path.read_text())["counters"][key]
    if item["value"] is None:
        raise ValueError(f"required calibration metric unavailable: {key}: {item.get('reason')}")
    return item["value"]


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=False)
    inputs = OUT / "input.txt"
    sample = [
        "London",
        "SW1A 1AA",
        "123 Main Street, Springfield, IL 62704",
        "Via Roma 5, 00184 Roma",
    ]
    inputs.write_text("\n".join(sample) + "\n", encoding="utf-8")
    (OUT / "input.json").write_text(json.dumps({
        "created_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "source": "hand-authored small calibration; no reference inputs changed",
        "rows": len(sample), "input_sha256": sha(inputs), "input_bytes": inputs.stat().st_size,
        "addresses": sample,
    }, indent=2) + "\n")
    baseline = ROOT / "research/experiments/182-benchmark-suite-prep/artifacts/original-000/dump_outputs"
    control_release = ROOT / "research/experiments/119-component-phrase-ids/artifacts/cpi_201_matched_v1/dump_outputs"
    candidate_release = ROOT / "research/experiments/199-token-span-storage/artifacts/release-mimalloc-v2/dump_outputs"
    control_diag = ROOT / "research/experiments/201-cpi-work-and-memory-prep/artifacts/token-diagnostics-v4/control/dump_outputs"
    candidate_diag = ROOT / "research/experiments/201-cpi-work-and-memory-prep/artifacts/token-diagnostics-v4/candidate/dump_outputs"
    roles = {"original": baseline, "cpi-control-release": control_release,
             "199-release": candidate_release, "cpi-control-diagnostic": control_diag,
             "199-diagnostic": candidate_diag}
    results = {"status": "running", "started_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
               "input_sha256": sha(inputs), "rows": len(sample), "roles": {}, "checks": {}}
    with LOCK.open("a+b") as lock:
        fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
        for name, binary in roles.items():
            gate_dir = OUT / (name + "-gate")
            gate_threads = rm.capture(["/bin/bash", str(GATE)], gate_dir, sample_threads=False)
            gate_command = json.loads((gate_dir / "command.json").read_text())
            if gate_command["exit"] != 0:
                raise RuntimeError(f"environment gate failed before {name}")
            role_dir = OUT / name
            env = None
            metrics_name = name.replace("-diagnostic", "")
            metrics_path = OUT / (metrics_name + "-metrics.json")
            if name.endswith("diagnostic"):
                env = {"MEASUREMENTS_JSON": str(metrics_path)}
            threads = rm.capture([str(binary), str(inputs)], role_dir, env, sample_threads=True)
            command = json.loads((role_dir / "command.json").read_text())
            output = role_dir / "stdout"
            results["roles"][name] = {
                "binary": str(binary), "binary_sha256": sha(binary),
                "output_sha256": sha(output), "output_rows": rm.rows(output),
                "exit": command["exit"], "thread_counts_observed": sorted(set(threads)),
                "time_log": str(role_dir / "time-l.txt"),
                "metrics_file": str(metrics_path) if metrics_path.exists() else None,
            }
    base = (OUT / "original/stdout").read_bytes()
    results["checks"]["all_output_bytes_identical_to_000"] = all(
        (OUT / name / "stdout").read_bytes() == base for name in roles if name != "original")
    results["checks"]["all_rows_match_input"] = all(
        results["roles"][name]["output_rows"] == len(sample) for name in roles)
    results["checks"]["all_roles_single_thread"] = all(
        results["roles"][name]["thread_counts_observed"] == [1] for name in roles)
    ctl = json.loads((OUT / "cpi-control-metrics.json").read_text())
    cand = json.loads((OUT / "199-metrics.json").read_text())
    cv, vv = lambda n: load_metric(ctl, n), lambda n: load_metric(cand, n)
    metrics_checks = {
        "candidate_view_plus_copy_records_equal_control_copy_records": vv("token_view_records") + vv("token_copy_calls") == cv("token_copy_calls"),
        "candidate_view_plus_copy_payload_equal_control_copy_payload": vv("token_view_bytes") + vv("token_copy_bytes") == cv("token_copy_bytes"),
        "candidate_view_plus_copy_nul_writes_equal_control_copy_nul_writes": vv("token_view_nul_writes") + vv("token_copy_nul_writes") == cv("token_copy_nul_writes"),
        "eligible_opportunity_equal_between_modes": cv("token_slice_opportunity_eligible_addresses") == vv("token_slice_opportunity_eligible_addresses"),
        "unsafe_opportunity_equal_between_modes": cv("token_slice_opportunity_unsafe_fallbacks") == vv("token_slice_opportunity_unsafe_fallbacks"),
        "candidate_actual_eligible_matches_opportunity": vv("token_slice_eligible_addresses") == vv("token_slice_opportunity_eligible_addresses"),
        "candidate_actual_fallback_matches_opportunity": vv("token_slice_unsafe_fallbacks") == vv("token_slice_opportunity_unsafe_fallbacks"),
        "normalized_copy_calls_equal": cv("normalized_copy_calls") == vv("normalized_copy_calls"),
        "normalized_copy_bytes_equal": cv("normalized_copy_bytes") == vv("normalized_copy_bytes"),
        "normalized_copy_nul_writes_equal": cv("normalized_copy_nul_writes") == vv("normalized_copy_nul_writes"),
    }
    results["checks"]["counter_reconciliation"] = metrics_checks
    results["metrics"] = {"control": json.loads((OUT / "cpi-control-metrics.json").read_text()),
                          "candidate": json.loads((OUT / "199-metrics.json").read_text())}
    results["ended_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
    results["status"] = "pass" if all(results["checks"][k] for k in (
        "all_output_bytes_identical_to_000", "all_rows_match_input", "all_roles_single_thread")) and all(metrics_checks.values()) else "fail"
    (OUT / "summary.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps({"status": results["status"], "checks": results["checks"], "path": str(OUT)}, indent=2))
    return 0 if results["status"] == "pass" else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except BaseException as e:
        print(repr(e), file=sys.stderr)
        raise
