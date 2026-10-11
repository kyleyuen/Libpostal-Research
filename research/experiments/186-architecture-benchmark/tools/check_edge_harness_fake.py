#!/usr/bin/env python3
"""Fast no-model fake-library check for edge API capture and trace selection."""

import contextlib
import ctypes
import datetime as dt
import importlib.util
import io
import json
from pathlib import Path
import runpy
import sys
import tempfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
SPEC = importlib.util.spec_from_file_location("edge_api_dump", HERE / "edge_api_dump.py")
api = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(api)


class FakeFunction:
    def __init__(self, function):
        self.function = function

    def __call__(self, *args):
        return self.function(*args)


class FakeLibrary:
    def __init__(self):
        self.label = b"fixture"
        self.keepalive = []
        self.calls = 0
        self.libpostal_setup_datadir = FakeFunction(lambda path: True)
        self.libpostal_setup_parser_datadir = FakeFunction(lambda path: True)
        self.libpostal_teardown_parser = FakeFunction(lambda: None)
        self.libpostal_teardown = FakeFunction(lambda: None)
        self.libpostal_address_parser_response_destroy = FakeFunction(lambda response: None)
        self.libpostal_parse_address = FakeFunction(self.parse)
        self.diag_trace_finish = FakeFunction(self.finish_trace)

    def parse(self, address, options):
        self.calls += 1
        if address is None:
            return ctypes.POINTER(api.ParserResponse)()
        raw = address.value or b""
        count = 1 if raw else 0
        labels = (ctypes.c_char_p * count)(*([self.label] if count else []))
        components = (ctypes.c_char_p * count)(*([raw] if count else []))
        self.keepalive.extend((labels, components))
        response = api.ParserResponse(count, components, labels)
        self.keepalive.append(response)
        return ctypes.pointer(response)

    def finish_trace(self):
        expected = int(__import__("os").environ["TRACE_EXPECTED_ADDRESSES"])
        Path(__import__("os").environ["TRACE_JSONL"]).write_text(
            json.dumps({"trace_end": True, "expected_addresses": expected}) + "\n",
            encoding="utf-8",
        )
        return 0


def run_api(fake, td, args):
    fake.calls = 0
    stdout, stderr = io.StringIO(), io.StringIO()
    argv = ["edge_api_dump.py", "--library", "fake.dylib", "--datadir", str(td)] + args
    old_argv = sys.argv
    sys.argv = argv
    try:
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            code = api.main()
        return code or 0, stdout.getvalue(), stderr.getvalue()
    except Exception as exc:
        return -1, stdout.getvalue(), stderr.getvalue() + f"{type(exc).__name__}: {exc}\n"
    finally:
        sys.argv = old_argv


def main():
    started = dt.datetime.now(dt.timezone.utc).isoformat()
    report = {"started_utc": started, "mode": "fake ctypes library; no real parser/model loaded", "checks": []}
    registry = json.loads((HERE.parent / "edge-cases.json").read_text(encoding="utf-8"))
    expected_safe = api.select_safe_cases(registry["cases"])
    fake = FakeLibrary()
    original_cdll = ctypes.CDLL
    ctypes.CDLL = lambda path: fake
    try:
        with tempfile.TemporaryDirectory() as tmp:
            td = Path(tmp)
            all_out = td / "all.jsonl"
            code, _, stderr = run_api(fake, td, ["--cases", str(HERE.parent / "edge-cases.json"), "--output-jsonl", str(all_out)])
            rows = [json.loads(line) for line in all_out.read_text(encoding="utf-8").splitlines()]
            by_id = {row["case_id"]: row for row in rows}
            assert code == 0 and len(rows) == len(expected_safe)
            assert by_id["empty"]["status"] == "ok" and by_id["empty"]["ordered_pairs"] == []
            assert by_id["api-null-address-pointer"]["status"] == "null"
            assert by_id["input-embedded-nul"]["trace_safe"] is False
            assert "bytes-invalid-utf8-lead" not in by_id
            report["checks"].append({"name": "safe/full capture, null-vs-empty, unsafe omission", "exit": code, "rows": len(rows), "stderr": stderr})

            subset_out = td / "subset.jsonl"
            code, _, stderr = run_api(fake, td, ["--cases", str(HERE.parent / "edge-cases.json"), "--case-ids", "input-embedded-nul,api-null-address-pointer", "--output-jsonl", str(subset_out)])
            subset = [json.loads(line) for line in subset_out.read_text(encoding="utf-8").splitlines()]
            assert code == 0 and [row["case_id"] for row in subset] == ["input-embedded-nul", "api-null-address-pointer"]
            report["checks"].append({"name": "API-only requested order", "exit": code, "rows": len(subset), "stderr": stderr})

            ids = "city-london,postcode-uk-sw1a-1aa,empty,punctuation-only"
            trace_out, trace_jsonl = td / "trace-output.jsonl", td / "trace.jsonl"
            code, _, stderr = run_api(fake, td, ["--cases", str(HERE.parent / "edge-cases.json"), "--trace", "--trace-case-ids", ids, "--trace-jsonl", str(trace_jsonl), "--output-jsonl", str(trace_out)])
            traced = [json.loads(line) for line in trace_out.read_text(encoding="utf-8").splitlines()]
            trace_end = json.loads(trace_jsonl.read_text(encoding="utf-8"))
            assert code == 0 and len(traced) == 4 and all(row["trace_safe"] for row in traced)
            assert trace_end["expected_addresses"] == 4
            report["checks"].append({"name": "trace-safe named subset/environment/finalizer", "exit": code, "rows": len(traced), "trace_end": trace_end, "stderr": stderr})

            same_out = td / "same.jsonl"
            code, _, stderr = run_api(fake, td, ["--cases", str(HERE.parent / "edge-cases.json"), "--case-ids", "city-london,empty", "--output-jsonl", str(same_out)])
            compare_out = td / "compare.jsonl"
            code, _, stderr = run_api(fake, td, ["--cases", str(HERE.parent / "edge-cases.json"), "--case-ids", "city-london,empty", "--output-jsonl", str(compare_out), "--compare-jsonl", str(same_out)])
            assert code == 0 and "BYTE_COMPARE=IDENTICAL" in stderr
            report["checks"].append({"name": "exact-byte positive control", "exit": code, "stderr": stderr})

            fake.label = b"deliberately-mutated"
            different = td / "different.jsonl"
            code, _, stderr = run_api(fake, td, ["--cases", str(HERE.parent / "edge-cases.json"), "--case-ids", "city-london,empty", "--output-jsonl", str(different), "--compare-jsonl", str(same_out)])
            assert code == 1 and "BYTE_COMPARE=DIFFERENT" in stderr
            report["checks"].append({"name": "exact-byte negative control", "exit": code, "stderr": stderr})

            try:
                api.validate_cases([registry["cases"][0], registry["cases"][0]])
            except ValueError:
                pass
            else:
                raise AssertionError("duplicate IDs accepted")
            report["checks"].append({"name": "duplicate-ID validation", "exit": 0})
    finally:
        ctypes.CDLL = original_cdll
    report["exit"] = 0
    report["finished_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
    output = HERE.parent / "raw" / "edge-harness-fakecheck.json"
    output.write_text(json.dumps(report, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")
    print(f"FAKE_HARNESS_CHECK=PASS checks={len(report['checks'])} record={output}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception:
        raise
