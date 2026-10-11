#!/usr/bin/env python3
"""Small fail-closed fixtures for the CPI trace analyzer."""
import importlib.util
import gzip
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('analyze_cpi_reuse_flow', HERE / 'analyze_cpi_reuse_flow.py')
analyzer = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(analyzer)


def make_trace(generation=3, answer=17, consumer_value=17, consumer=True, overflow=False):
    rows = [
        {'record_type': 'trace_header', 'schema_version': 1, 'source_variant': 'cpi-trace',
         'event_cap': 100, 'overflow_policy': 'fail_closed', 'key_encoding': 'hex',
         'event_cap_scope': 'query_and_consumer_rows',
         'consumer_coverage': ['state_feature']},
    ]
    for index in (1, 2):
        rows.append({'record_type': 'query', 'address_index': 1, 'address_key': 'a' * 64,
            'query_index': index, 'query_kind': 'physical_search', 'physical_search_index': index,
            'site_id': 'feature_dict.state', 'dictionary_id': 'dict:1',
            'dictionary_generation': generation, 'dictionary_generation_reason': None,
            'model_generation': generation, 'model_generation_reason': None,
            'operation': 'feature_hash', 'family': 'state_feature', 'key_hex': '6162',
            'hit': True, 'result_kind': 'feature_id', 'result_value': answer,
            'result_width_bytes': 4, 'answer_source': 'dictionary_search', 'source_query_index': None})
        if consumer:
            rows.append({'record_type': 'query_consumer', 'address_index': 1,
                'consumer_index': index, 'source_query_index': index,
                'consumer_dictionary_id': 'dict:1',
                'consumer_site_id': 'crf.state_row', 'consumer_stage': 'crf_state_lookup',
                'consumer_role': 'state_model_row', 'value_kind': 'feature_id',
                'value': consumer_value, 'value_width_bytes': 4,
                'model_generation': generation, 'model_generation_reason': None})
    rows.extend([
        {'record_type': 'address_summary', 'address_index': 1, 'address_key': 'a' * 64,
         'complete': True, 'query_events': 2, 'query_consumer_events': 2 if consumer else 0,
         'consumer_complete': True, 'consumer_scope': ['state_feature_id_score_occurrence'],
         'input_bytes': 2, 'output_bytes': 10},
        {'record_type': 'trace_end', 'addresses': 1, 'expected_addresses': 1,
         'query_events': 2, 'consumer_events': 2 if consumer else 0,
         'complete': True, 'overflow': False},
    ])
    if overflow:
        rows.insert(-2, {'record_type': 'trace_overflow', 'overflow': True})
    return rows


class ReuseFlowTests(unittest.TestCase):
    def run_rows(self, rows, address_rows=False, expected_proof=None):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'trace.jsonl'
            path.write_text(''.join(json.dumps(row) + '\n' for row in rows))
            report_path = Path(tmp) / 'addresses.jsonl' if address_rows else None
            report = analyzer.analyze(path, report_path, expected_stability_proof=expected_proof)
            per_address = json.loads(report_path.read_text()) if report_path else None
            return report, per_address

    def test_repeated_answer_links_to_matching_consumers_and_liveness(self):
        report, per_address = self.run_rows(make_trace(), address_rows=True)
        group = per_address['repeat_groups'][0]
        self.assertEqual(group['evidence_status'], 'observed-stable-answer-to-consumer')
        self.assertTrue(group['public_generation_stable'])
        self.assertEqual(group['potentially_avoided_physical_searches_upper_bound'], 1)
        self.assertEqual(group['key_bytes_lower_bound'], 2)
        self.assertEqual(group['answer_payload_bytes_lower_bound'], 4)
        self.assertEqual(group['public_generation_status'], 'known-stable')
        self.assertEqual(per_address['max_simultaneously_live_repeated_answers'], 1)
        self.assertEqual(per_address['max_first_to_last_consumer_event_span'], 3)
        self.assertEqual(report['consumer_events'], 2)

    def test_unknown_generation_stays_unproven(self):
        report, per_address = self.run_rows(make_trace(generation=None), address_rows=True)
        self.assertEqual(per_address['repeat_groups'][0]['evidence_status'], 'generation-unknown')
        self.assertEqual(per_address['repeat_groups'][0]['public_generation_status'], 'unknown')
        evidence = per_address['consumer_link_evidence'][0]
        self.assertEqual(evidence['answer_relation'], 'exact-typed-match')
        self.assertIsNone(evidence['model_generation_matches'])
        self.assertEqual(evidence['source_dictionary_id'], 'dict:1')
        self.assertEqual(evidence['consumer_dictionary_id'], 'dict:1')

    def test_different_model_generations_do_not_form_a_repeat_group(self):
        rows = make_trace()
        for row in rows:
            if row.get('record_type') == 'query' and row.get('query_index') == 2:
                row['model_generation'] = 4
            if row.get('record_type') == 'query_consumer' and row.get('consumer_index') == 2:
                row['model_generation'] = 4
        _report, per_address = self.run_rows(rows, address_rows=True)
        self.assertEqual(per_address['repeat_groups'], [])
        self.assertEqual(per_address['repeat_search_groups'], 0)

    def test_bool_is_not_numeric_answer_equality(self):
        _report, per_address = self.run_rows(make_trace(answer=1, consumer_value=True), address_rows=True)
        self.assertFalse(per_address['repeat_groups'][0]['consumer_values_agree'])
        relation, matches = analyzer._query_result_agreement([
            {'hit': True, 'result_kind': 'feature_id', 'result_value': 1},
            {'hit': True, 'result_kind': 'feature_id', 'result_value': True}])
        self.assertEqual(relation, 'typed-value-representation-disagreement')
        self.assertFalse(matches)

    def test_source_proof_is_within_parse_only_and_separate_from_epoch(self):
        rows = make_trace(generation=None)
        rows[0]['within_parse_stability_proof'] = {
            'status': 'address_scoped_readonly', 'proof_id': 'parse-sync-v1',
            'scope': 'one synchronous parse', 'source_sha256': 'b' * 64}
        proof = {'status': 'address_scoped_readonly', 'proof_id': 'parse-sync-v1',
                 'scope': 'one synchronous parse', 'source_sha256': 'b' * 64,
                 'source_manifest_path': 'fixture', 'flags_manifest_path': 'fixture'}
        rows[0]['source_manifest_sha256'] = 'b' * 64
        report, per_address = self.run_rows(rows, address_rows=True, expected_proof=proof)
        group = per_address['repeat_groups'][0]
        self.assertTrue(group['identity_stable_within_parse'])
        self.assertFalse(group['public_generation_stable'])
        self.assertEqual(group['evidence_status'], 'source-proven-stable-answer-to-consumer')

    def test_answer_disagreement_does_not_become_reuse_candidate(self):
        report, per_address = self.run_rows(make_trace(answer=17, consumer_value=18), address_rows=True)
        group = per_address['repeat_groups'][0]
        self.assertFalse(group['consumer_values_agree'])
        self.assertEqual(group['evidence_status'], 'answer-disagreement-or-unobserved')

    def test_missing_consumer_is_explicit(self):
        report, per_address = self.run_rows(make_trace(consumer=False), address_rows=True)
        self.assertEqual(per_address['repeat_groups'][0]['evidence_status'], 'consumer-link-incomplete')

    def test_phrase_boolean_miss_has_unknown_result_shape(self):
        rows = make_trace(consumer=False)
        for row in rows:
            if row.get('record_type') == 'query':
                row.update({'family': 'component_phrase', 'operation': 'phrase_tokens',
                            'hit': False, 'result_kind': None, 'result_value': None})
        report, per_address = self.run_rows(rows, address_rows=True)
        self.assertEqual(per_address['repeat_groups'][0]['evidence_status'],
                         'negative-answer-or-consumer-proof-incomplete')

    def test_answer_reuse_requires_exact_producer_identity(self):
        rows = make_trace()
        reuse = next(row for row in rows if row.get('record_type') == 'query' and row.get('query_index') == 2)
        reuse.update({'query_kind': 'answer_reuse', 'source_query_index': 1,
                      'producer_query_index': 1, 'physical_search_index': None})
        reuse['key_hex'] = '6364'
        with self.assertRaisesRegex(ValueError, 'exact dictionary/operation/family/key identity'):
            self.run_rows(rows)

    def test_gzip_trace_is_stream_analyzable(self):
        rows = make_trace()
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'trace.jsonl.gz'
            with gzip.open(path, 'wt', encoding='utf-8') as stream:
                stream.write(''.join(json.dumps(row) + '\n' for row in rows))
            self.assertEqual(analyzer.analyze(path)['addresses'], 1)

    def test_gzip_archive_is_verified_before_raw_removal(self):
        runner_path = HERE / 'run_cpi_reuse_flow.py'
        runner_spec = importlib.util.spec_from_file_location('run_cpi_reuse_flow', runner_path)
        runner = importlib.util.module_from_spec(runner_spec)
        runner_spec.loader.exec_module(runner)
        with tempfile.TemporaryDirectory() as tmp:
            raw = Path(tmp) / 'trace.jsonl'
            payload = (b'{"trace_header":true}\n' * 10000)
            raw.write_bytes(payload)
            raw_sha = hashlib.sha256(payload).hexdigest()
            archived = runner.archive_trace_losslessly(raw)
            self.assertFalse(raw.exists())
            self.assertEqual(archived['raw_sha256'], raw_sha)
            self.assertLess(archived['archive_bytes'], archived['raw_bytes'])

    def test_overflow_rejected(self):
        with self.assertRaisesRegex(ValueError, 'overflow'):
            self.run_rows(make_trace(overflow=True))

    def test_unknown_record_rejected(self):
        rows = make_trace()
        rows.insert(-2, {'record_type': 'made_up'})
        with self.assertRaisesRegex(ValueError, 'unknown trace record'):
            self.run_rows(rows)


if __name__ == '__main__':
    unittest.main(verbosity=2)
