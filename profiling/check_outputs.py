"""Record or verify exact ordered parser output; never used for speed timing."""
import argparse
import ctypes as C
import json
from pathlib import Path

args = argparse.ArgumentParser()
args.add_argument("mode", choices=["record", "check"])
args.add_argument("--library", type=Path, default=Path(__file__).resolve().parents[1] / "libpostal/src/.libs/libpostal.dylib")
args.add_argument("--reference", type=Path, default=Path(__file__).parent / "golden-v1/baseline-outputs.jsonl")
opts = args.parse_args()
root = Path(__file__).parent / "golden-v1"

class Options(C.Structure):
    _fields_ = [("language", C.c_char_p), ("country", C.c_char_p)]

class Response(C.Structure):
    _fields_ = [("num_components", C.c_size_t), ("components", C.POINTER(C.c_char_p)),
                ("labels", C.POINTER(C.c_char_p))]

lib = C.CDLL(str(opts.library.resolve()))
for name in ("libpostal_setup", "libpostal_setup_parser"):
    getattr(lib, name).argtypes = []
    getattr(lib, name).restype = C.c_bool
lib.libpostal_get_address_parser_default_options.argtypes = []
lib.libpostal_get_address_parser_default_options.restype = Options
lib.libpostal_parse_address.argtypes = [C.c_char_p, Options]
lib.libpostal_parse_address.restype = C.POINTER(Response)
lib.libpostal_address_parser_response_destroy.argtypes = [C.POINTER(Response)]
lib.libpostal_address_parser_response_destroy.restype = None
for name in ("libpostal_teardown_parser", "libpostal_teardown"):
    getattr(lib, name).argtypes = []
    getattr(lib, name).restype = None
assert lib.libpostal_setup() and lib.libpostal_setup_parser()
options = lib.libpostal_get_address_parser_default_options()
outputs = []
try:
    for line in (root / "records.jsonl").read_text().splitlines():
        row = json.loads(line)
        response = lib.libpostal_parse_address(row["address"].encode(), options)
        assert response and response.contents.num_components, row["id"]
        try:
            r = response.contents
            components = [[r.labels[i].decode(), r.components[i].decode()]
                          for i in range(r.num_components)]
            outputs.append(dict(id=row["id"], components=components))
        finally:
            lib.libpostal_address_parser_response_destroy(response)
finally:
    lib.libpostal_teardown_parser()
    lib.libpostal_teardown()

if opts.mode == "record":
    with opts.reference.open("x") as f:
        for row in outputs:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")
    print(f"Recorded {len(outputs)} baseline outputs; these are not ground truth.")
else:
    expected = [json.loads(line) for line in opts.reference.read_text().splitlines()]
    assert len(outputs) == len(expected), "Reference length mismatch"
    differences = [actual["id"] for actual, old in zip(outputs, expected) if actual != old]
    print(json.dumps(dict(checked=len(outputs), mismatches=len(differences), first_ids=differences[:10])))
    raise SystemExit(bool(differences))
