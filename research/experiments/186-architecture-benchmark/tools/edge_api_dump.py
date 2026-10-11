#!/usr/bin/env python3
"""Capture pinned-library outputs for safe records in edge-cases.json.

This is a correctness harness, not a timer. Each emitted JSONL record contains
the case identity, input/options, a status, and ordered (label, component)
pairs. Records marked unsafe or lacking input_utf8 are skipped by default.
"""

import argparse
import base64
import ctypes
import json
import os
from pathlib import Path
import sys


class ParserOptions(ctypes.Structure):
    _fields_ = [("language", ctypes.c_char_p), ("country", ctypes.c_char_p)]


class ParserResponse(ctypes.Structure):
    _fields_ = [
        ("num_components", ctypes.c_size_t),
        ("components", ctypes.POINTER(ctypes.c_char_p)),
        ("labels", ctypes.POINTER(ctypes.c_char_p)),
    ]


def c_string(value):
    return None if value is None else value.encode("utf-8")


def validate_cases(cases):
    if not isinstance(cases, list):
        raise ValueError("cases must be an array")
    seen = set()
    for case in cases:
        if not isinstance(case, dict) or not isinstance(case.get("id"), str):
            raise ValueError("every case must be an object with a string id")
        if case["id"] in seen:
            raise ValueError(f"duplicate case id: {case['id']}")
        seen.add(case["id"])
        options = case.get("options")
        if (not isinstance(options, dict) or set(options) != {"language", "country"} or
                any(v is not None and not isinstance(v, str) for v in options.values())):
            raise ValueError(f"{case['id']}: options must contain nullable language/country strings")
        if case.get("scope") == "valid-utf8":
            if not isinstance(case.get("input_utf8"), str):
                raise ValueError(f"{case['id']}: valid-utf8 case needs input_utf8")
            case["input_utf8"].encode("utf-8", errors="strict")
            if case.get("safe_to_call") is not True:
                raise ValueError(f"{case['id']}: valid-utf8 scope must be marked safe_to_call")
        elif case.get("scope") == "invalid-api":
            try:
                base64.b64decode(case["input_base64"], validate=True)
            except (KeyError, ValueError) as exc:
                raise ValueError(f"{case['id']}: invalid-api needs valid input_base64") from exc
            if case.get("safe_to_call") is not False:
                raise ValueError(f"{case['id']}: invalid-api scope must be marked unsafe")
        elif case.get("scope") == "safe-api-null":
            if case.get("input_kind") != "null-pointer" or case.get("safe_to_call") is not True:
                raise ValueError(f"{case['id']}: safe-api-null must explicitly identify a null pointer")
        else:
            raise ValueError(f"{case['id']}: unsupported scope {case.get('scope')!r}")


def select_safe_cases(cases):
    """Return manifest-order ``(case, trace_safe)`` pairs after full validation.

    Trace-safe means the parser receives a non-NULL ordinary C string whose
    UTF-8 payload contains no embedded NUL. API-only cases still belong in
    correctness captures, but a NULL pointer cannot begin an address trace.
    """
    validate_cases(cases)
    selected = []
    for case in cases:
        if case.get("safe_to_call") is not True:
            continue
        trace_safe = (case.get("scope") == "valid-utf8" and
                      "\0" not in case["input_utf8"])
        selected.append((case, trace_safe))
    return selected


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--library", required=True, help="preserved libpostal shared library")
    ap.add_argument("--datadir", required=True, help="directory containing libpostal model data")
    inputs = ap.add_mutually_exclusive_group(required=True)
    inputs.add_argument("--cases", help="edge-cases.json registry")
    inputs.add_argument("--input-txt", help="one address per line, legacy benchmark convention")
    ap.add_argument("--output-jsonl", required=True, help="destination for raw ordered outputs")
    ap.add_argument("--compare-jsonl", help="optional original capture; exit 1 unless output bytes match")
    ap.add_argument("--case-ids", help="comma-separated safe API case IDs for correctness-only capture")
    ap.add_argument("--trace", action="store_true", help="trace-safe cases only; require diagnostic library and trace output")
    ap.add_argument("--trace-case-ids", help="comma-separated case IDs to select for --trace")
    ap.add_argument("--trace-jsonl", help="trace output path (also accepts TRACE_JSONL)")
    ap.add_argument("--trace-case-id", default="edge-cases-trace-smoke", help="TRACE_CASE_ID header value")
    args = ap.parse_args()

    if args.cases:
        registry = json.loads(Path(args.cases).read_text(encoding="utf-8"))
        if registry.get("schema_version") != 1:
            raise ValueError("unsupported edge-cases.json schema_version")
        cases = registry.get("cases")
    else:
        lines = Path(args.input_txt).read_text(encoding="utf-8").splitlines()
        cases = [
            {"id": f"txt-{i:08d}", "category": "text-workload",
             "input_utf8": line, "options": {"language": None, "country": None},
             "scope": "valid-utf8", "safe_to_call": True}
            for i, line in enumerate(lines) if line != ""
        ]
    safe_cases = select_safe_cases(cases)
    if args.trace:
        if args.case_ids:
            raise ValueError("--case-ids is correctness-only; use --trace-case-ids with --trace")
        selected = [(case, trace_safe) for case, trace_safe in safe_cases if trace_safe]
        if args.trace_case_ids:
            requested = [value for value in args.trace_case_ids.split(",") if value]
            if not requested or len(requested) != len(set(requested)):
                raise ValueError("--trace-case-ids must contain unique, nonempty IDs")
            by_id = {case["id"]: (case, trace_safe) for case, trace_safe in safe_cases}
            missing = [case_id for case_id in requested if case_id not in by_id]
            if missing:
                raise ValueError(f"unknown or unsafe trace case IDs: {missing}")
            selected = [by_id[case_id] for case_id in requested]
        if not selected:
            raise ValueError("--trace selected no trace-safe addresses")
        trace_path = args.trace_jsonl or os.environ.get("TRACE_JSONL")
        if not trace_path:
            raise ValueError("--trace requires --trace-jsonl or TRACE_JSONL")
        trace_path = Path(trace_path)
        if trace_path.exists():
            raise FileExistsError(f"trace output already exists: {trace_path}")
        trace_path.parent.mkdir(parents=True, exist_ok=True)
        os.environ["TRACE_JSONL"] = str(trace_path.resolve())
        os.environ["TRACE_CASE_ID"] = args.trace_case_id
        os.environ["TRACE_EXPECTED_ADDRESSES"] = str(len(selected))
    else:
        selected = safe_cases
        if args.trace_case_ids or args.trace_jsonl:
            raise ValueError("--trace-case-ids and --trace-jsonl require --trace")
        if args.case_ids:
            requested = [value for value in args.case_ids.split(",") if value]
            if not requested or len(requested) != len(set(requested)):
                raise ValueError("--case-ids must contain unique, nonempty IDs")
            by_id = {case["id"]: (case, trace_safe) for case, trace_safe in safe_cases}
            missing = [case_id for case_id in requested if case_id not in by_id]
            if missing:
                raise ValueError(f"unknown or unsafe API case IDs: {missing}")
            selected = [by_id[case_id] for case_id in requested]
    lib = ctypes.CDLL(str(Path(args.library).resolve()))
    lib.libpostal_setup_datadir.argtypes = [ctypes.c_char_p]
    lib.libpostal_setup_datadir.restype = ctypes.c_bool
    lib.libpostal_setup_parser_datadir.argtypes = [ctypes.c_char_p]
    lib.libpostal_setup_parser_datadir.restype = ctypes.c_bool
    lib.libpostal_teardown_parser.argtypes = []
    lib.libpostal_teardown_parser.restype = None
    lib.libpostal_teardown.argtypes = []
    lib.libpostal_teardown.restype = None
    lib.libpostal_parse_address.argtypes = [ctypes.c_char_p, ParserOptions]
    lib.libpostal_parse_address.restype = ctypes.POINTER(ParserResponse)
    lib.libpostal_address_parser_response_destroy.argtypes = [ctypes.POINTER(ParserResponse)]
    lib.libpostal_address_parser_response_destroy.restype = None
    if args.trace:
        lib.diag_trace_finish.argtypes = []
        lib.diag_trace_finish.restype = ctypes.c_int

    data_dir = str(Path(args.datadir).resolve()).encode("utf-8")
    if not lib.libpostal_setup_datadir(data_dir):
        raise SystemExit("libpostal_setup_datadir failed")
    if not lib.libpostal_setup_parser_datadir(data_dir):
        lib.libpostal_teardown()
        raise SystemExit("libpostal_setup_parser_datadir failed")

    output_path = Path(args.output_jsonl)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    count = 0
    skipped = len(cases) - len(selected)
    trace_status = 0
    try:
        # Exclusive creation protects preserved oracle captures from accidental replacement.
        with output_path.open("x", encoding="utf-8", newline="\n") as out:
            for case, trace_safe in selected:
                address = (None if case.get("scope") == "safe-api-null" else
                           case["input_utf8"].encode("utf-8"))
                options = case["options"]
                response = lib.libpostal_parse_address(
                    ctypes.c_char_p(address) if address is not None else None,
                    ParserOptions(c_string(options["language"]), c_string(options["country"])),
                )
                input_fields = {"input_kind": "null-pointer"} if address is None else {"input_utf8": case["input_utf8"]}
                if not response:
                    record = {"case_id": case["id"], "category": case["category"],
                              **input_fields, "options": options, "trace_safe": trace_safe,
                              "status": "null", "ordered_pairs": None}
                else:
                    result = response.contents
                    pairs = []
                    for i in range(result.num_components):
                        label = result.labels[i].decode("utf-8", errors="surrogateescape")
                        component = result.components[i].decode("utf-8", errors="surrogateescape")
                        pairs.append([label, component])
                    record = {"case_id": case["id"], "category": case["category"],
                              **input_fields, "options": options, "trace_safe": trace_safe,
                              "status": "ok", "ordered_pairs": pairs}
                    lib.libpostal_address_parser_response_destroy(response)
                out.write(json.dumps(record, ensure_ascii=True, separators=(",", ":")) + "\n")
                count += 1
        if args.trace:
            trace_status = lib.diag_trace_finish()
    finally:
        lib.libpostal_teardown_parser()
        lib.libpostal_teardown()
    print(f"captured={count} skipped_unsafe_or_non_utf8={skipped} output={output_path}", file=sys.stderr)
    if args.trace:
        print(f"TRACE_FINISH={trace_status} trace={trace_path}", file=sys.stderr)
        if trace_status != 0:
            return 1
    if args.compare_jsonl:
        expected = Path(args.compare_jsonl).read_bytes()
        actual = output_path.read_bytes()
        if actual != expected:
            print(f"BYTE_COMPARE=DIFFERENT expected={args.compare_jsonl}", file=sys.stderr)
            return 1
        print(f"BYTE_COMPARE=IDENTICAL expected={args.compare_jsonl}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
