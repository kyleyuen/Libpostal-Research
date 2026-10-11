#!/usr/bin/env python3
"""Small deterministic tests for trace validation and gated collection wiring."""
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('architecture_benchmark_186', HERE / 'run_architecture_benchmark.py')
benchmark = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(benchmark)


def trace_fixture(counts, generations=None, include_events=True):
    rows = [{'record_type': 'trace_header', 'schema_version': 1,
             'source_variant': 'fixture', 'case_id': 'fixture',
             'expected_addresses': len(counts), 'event_cap': 1000,
             'overflow_policy': 'fail_closed', 'key_encoding': 'hex',
             'dictionary_generation': None,
             'generation_reason': 'fixture'}]
    qtotal = 0
    for address_index, value in enumerate(counts, 1):
        key = hashlib.sha256(('address-%d' % address_index).encode()).hexdigest()
        physical = 0
        if include_events:
            for query_index in range(1, value + 1):
                physical += 1
                qtotal += 1
                generation = generations[address_index - 1] if generations else None
                rows.append({'record_type': 'query', 'address_index': address_index,
                    'address_key': key, 'query_index': query_index,
                    'physical_search_index': query_index, 'query_kind': 'physical_search',
                    'dictionary_id': 'ptr:1', 'dictionary_generation': generation,
                    'generation_reason': 'fixture' if generation is None else None,
                    'operation': 'lookup', 'family': 'address', 'key_hex': '6162',
                    'hit': True, 'request_stage': 'fixture', 'safe_reuse_proof': 'unknown'})
        rows.append({'record_type': 'address_summary', 'address_index': address_index,
            'address_key': key, 'logical_requests': physical, 'physical_searches': physical,
            'answer_reuses': 0, 'hits': physical, 'misses': 0,
            'feature_keys_materialized': 1, 'feature_key_bytes_materialized': 2,
            'scoped_allocation_calls': 3, 'scoped_allocation_requested_bytes': 8,
            'diagnostic_tracker_bytes_current': 0, 'diagnostic_tracker_peak_bytes_run': 0,
            'input_bytes': 8, 'output_bytes': 4, 'model_rows': 1,
            'weights_accumulated': 2, 'dictionary_generation_complete': False,
            'safe_reuse_proof_complete': False})
    rows.append({'record_type': 'trace_end', 'addresses': len(counts),
        'expected_addresses': len(counts), 'query_events': qtotal,
        'trace_key_bytes_written': qtotal * 2, 'overflow': False,
        'address_count_mismatch': False, 'complete': True})
    return '\n'.join(json.dumps(row) for row in rows) + '\n'


class FakeRunner:
    def capture(self, argv, directory, env=None, sample_threads=False):
        directory = Path(directory)
        directory.mkdir()
        start = benchmark.stamp()
        if argv[0] == '/bin/bash':
            completed = subprocess.CompletedProcess(argv, 0, b'', b'fake AC gate pass\n')
        else:
            completed = subprocess.run(argv, cwd=benchmark.ROOT,
                env={**os.environ, **(env or {})}, capture_output=True)
        (directory / 'stdout').write_bytes(completed.stdout)
        (directory / 'stderr').write_bytes(completed.stderr)
        observed = [1] if sample_threads and argv[0] != '/bin/bash' else []
        record = {'argv': argv, 'start': start, 'end': benchmark.stamp(),
                  'exit': completed.returncode, 'thread_sample_count': len(observed),
                  'thread_counts_observed': observed, 'duration_seconds': 0.01,
                  'resource_summary': {'peak_rss_bytes': None, 'peak_rss_reason': 'fixture'}}
        benchmark.save(directory / 'command.json', record)
        benchmark.save(directory / 'threads.json', [{'threads': n} for n in observed])
        if completed.returncode:
            raise RuntimeError('fake runner subprocess failed: ' + str(argv) + '\n' + completed.stderr.decode(errors='replace'))
        return observed


class ArchitectureBenchmarkTests(unittest.TestCase):
    def test_generation_change_is_not_a_repeat(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'trace.jsonl'
            path.write_text(trace_fixture([1, 1], generations=['g1', 'g2']))
            result = benchmark.analyze_trace(path, 2)
            values = [row['metrics'] for row in result['per_address']]
            self.assertEqual(sum(v['executed_repeat_searches'] for v in values), 0)

    def test_unknown_generation_repeat_is_unproven(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'trace.jsonl'
            path.write_text(trace_fixture([2]))
            row = benchmark.analyze_trace(path, 1)['per_address'][0]['metrics']
            self.assertEqual(row['executed_repeat_searches'], 1)
            self.assertEqual(row['lifetime_unproven_repeat_searches'], 1)
            self.assertEqual(row['generation_valid_repeat_searches'], 0)

    def test_nearest_rank_p95_distribution(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'trace.jsonl'
            path.write_text(trace_fixture(list(range(1, 21))))
            result = benchmark.analyze_trace(path, 20)
            distribution = result['distributions']['physical_searches']
            row = next(iter(distribution.values()))
            self.assertEqual((row['median'], row['p95'], row['max']), (10, 19, 20))

    def test_empty_trace_workload_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'trace.jsonl'
            path.write_text(trace_fixture([]))
            with self.assertRaisesRegex(ValueError, 'address count incomplete'):
                benchmark.analyze_trace(path, 1)

    def test_overflow_rejected_even_with_complete_footer(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'trace.jsonl'
            text = trace_fixture([1]).replace('{"record_type": "query",',
                '{"record_type": "trace_overflow", "after_event_count": 1, "event_cap": 1}\n{"record_type": "query",', 1)
            path.write_text(text)
            with self.assertRaisesRegex(ValueError, 'overflow reported'):
                benchmark.analyze_trace(path, 1)

    def test_byte_comparison_rejects_mismatch(self):
        with tempfile.TemporaryDirectory() as temp:
            left, right = Path(temp) / 'left', Path(temp) / 'right'
            left.write_bytes(b'original\n')
            right.write_bytes(b'candidate\n')
            with self.assertRaisesRegex(ValueError, 'output mismatch'):
                benchmark.require_byte_exact(left, right)

    def test_fake_end_to_end_collection_and_manifest(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            model = root / 'model'
            model.mkdir()
            (model / 'model.dat').write_bytes(b'model')
            input_path = root / 'input.txt'
            input_path.write_bytes(b'first address\nsecond address\n')
            helper = root / 'harness.py'
            helper.write_text('''
import hashlib, json, os, sys
from pathlib import Path
role, path = sys.argv[1:]
addresses = [line.rstrip(b"\\r\\n") for line in Path(path).read_bytes().splitlines() if line]
for address in addresses:
    print(json.dumps({"address": address.decode(), "components": []}, separators=(",", ":")))
if role == "diagnostic":
    trace_path = Path(os.environ["TRACE_JSONL"])
    records = [{"record_type": "trace_header", "schema_version": 1,
        "source_variant": "fixture", "expected_addresses": len(addresses),
        "event_cap": 100, "overflow_policy": "fail_closed", "key_encoding": "hex",
        "dictionary_generation": None, "generation_reason": "fixture"}]
    query_count = 0
    for index, address in enumerate(addresses, 1):
        address_key = hashlib.sha256(address).hexdigest()
        records.append({"record_type": "query", "address_index": index,
            "address_key": address_key, "query_index": 1, "physical_search_index": 1,
            "query_kind": "physical_search", "dictionary_id": "ptr:1",
            "dictionary_generation": None, "generation_reason": "fixture",
            "operation": "lookup", "family": "address", "key_hex": "61",
            "hit": True, "request_stage": "fixture", "safe_reuse_proof": "unknown"})
        query_count += 1
        records.append({"record_type": "address_summary", "address_index": index,
            "address_key": address_key, "logical_requests": 1, "physical_searches": 1,
            "answer_reuses": 0, "hits": 1, "misses": 0,
            "feature_keys_materialized": 1, "feature_key_bytes_materialized": 1,
            "scoped_allocation_calls": 1, "scoped_allocation_requested_bytes": 1,
            "diagnostic_tracker_bytes_current": 0, "diagnostic_tracker_peak_bytes_run": 0,
            "input_bytes": len(address), "output_bytes": 0, "model_rows": 0,
            "weights_accumulated": 0, "dictionary_generation_complete": False,
            "safe_reuse_proof_complete": False})
    records.append({"record_type": "trace_end", "addresses": len(addresses),
        "expected_addresses": len(addresses), "query_events": query_count,
        "trace_key_bytes_written": query_count, "overflow": False,
        "address_count_mismatch": False, "complete": True})
    trace_path.write_text("".join(json.dumps(row) + "\\n" for row in records))
''')
            artifact = root / 'artifact.bin'
            artifact.write_bytes(b'artifact')
            config_path = root / 'config.json'
            cfg = {'pinned_oracle': {'baseline_library': str(artifact), 'baseline_dump': str(artifact),
                    'model_dir': str(model)},
                'artifacts': {'release_library': str(artifact), 'diagnostic_library': str(artifact),
                    'release_dump': str(artifact), 'diagnostic_dump': str(artifact),
                    'source_provenance': str(artifact), 'diagnostic_provenance': str(artifact)},
                'commands': {role: [sys.executable, str(helper), role, '{input_path}']
                             for role in ('oracle', 'release_control', 'diagnostic')},
                'trace_environment': 'TRACE_JSONL',
                'workload_modes': {'fixture': [{'name': 'fixture', 'input': str(input_path),
                                                'source': 'fixture', 'language': 'unknown',
                                                'country': 'unknown', 'split': 'unit'}]},
                'trace_contract': {'maximum_events': 100}}
            config_path.write_text(json.dumps(cfg))
            previous_runner = benchmark.load_measurement_runner
            previous_config = benchmark.args_config_path
            benchmark.load_measurement_runner = lambda: FakeRunner()
            benchmark.args_config_path = config_path
            try:
                output = root / 'run-result'
                benchmark.collect(cfg, 'fixture', output)
            finally:
                benchmark.load_measurement_runner = previous_runner
                benchmark.args_config_path = previous_config
            result = json.loads((output / 'results.json').read_text())
            self.assertEqual(result['status'], 'trace collection complete; diagnostic guidance only')
            self.assertEqual(len(result['records']), 1)
            record = result['records'][0]
            self.assertEqual(record['input_count'], 2)
            self.assertEqual(record['byte_checks']['diagnostic']['status'], 'byte-identical')
            self.assertEqual(record['threads']['diagnostic'], [1])
            manifest = json.loads((output / 'manifest.json').read_text())
            self.assertIn(str(input_path), manifest['files'])
            self.assertTrue((output / 'fixture/diagnostic.trace.jsonl').is_file())


if __name__ == '__main__':
    unittest.main(verbosity=2)
