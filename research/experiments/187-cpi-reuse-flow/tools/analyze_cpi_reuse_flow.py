#!/usr/bin/env python3
"""Stream a 187 CPI trace into per-address reuse/lifetime evidence."""
import argparse
from collections import Counter, defaultdict
import gzip
import hashlib
import json
from pathlib import Path
import sys


# Only typed scalar/ID resolver misses are structurally comparable. Phrase
# booleans and mutation-oriented wrappers are deliberately absent.
NEGATIVE_SCALAR_DOMAINS = {
    ('word_vocab', 'word_vocab_lookup', 'vocab_frequency', 'address_parser.ap_vm_get.vocab_entry'),
    ('state_feature', 'feature_hash', 'feature_id', 'address_parser.ap_vm_get.vocab_entry'),
    ('transition_feature', 'feature_hash', 'feature_id', 'address_parser.ap_vm_get.vocab_entry'),
}


def _json_value_relation(query, consumer):
    consumer_kind = consumer.get('value_kind', consumer.get('result_kind'))
    consumer_value = consumer.get('value', consumer.get('result_value'))
    result_kind = query.get('result_kind')
    result_value = query.get('result_value')
    if result_kind is None or result_value is None or consumer_kind is None or consumer_value is None:
        return 'unobserved', None
    if result_kind != consumer_kind:
        return 'typed-domain-projection-unmapped', None
    if type(result_value) is not type(consumer_value) or result_value != consumer_value:
        return 'typed-value-disagreement', False
    return 'exact-typed-match', True


def _json_value_matches(query, consumer):
    return _json_value_relation(query, consumer)[1]


def _query_result_agreement(events):
    hits = [event.get('hit') for event in events]
    if not hits or any(type(hit) is not bool for hit in hits):
        return 'hit-state-unobserved', None
    if any(hit != hits[0] for hit in hits):
        return 'hit-state-disagreement', False
    answers = [(event.get('result_kind'), event.get('result_value')) for event in events]
    if any(kind is None or value is None for kind, value in answers):
        return 'typed-answer-unobserved', None
    if len({kind for kind, _ in answers}) != 1:
        return 'typed-domain-projection-unmapped', None
    if any(type(value) is not type(answers[0][1]) for _, value in answers):
        return 'typed-value-representation-disagreement', False
    if any(value != answers[0][1] for _, value in answers):
        return 'typed-value-disagreement', False
    return 'exact-typed-answer-match', True


def _identity(query):
    key = query.get('key_hex')
    return (query.get('dictionary_id'), query.get('dictionary_generation'),
            query.get('model_generation'), query.get('operation'), query.get('family'), key)


def _payload_bytes(query):
    declared = query.get('result_width_bytes')
    if type(declared) is int and declared >= 0:
        return declared
    kind = query.get('result_kind')
    return {'feature_id': 4, 'vocab_frequency': 4, 'u32': 4, 'i32': 4,
            'u64': 8, 'i64': 8, 'f64': 8}.get(kind)


def _address_report(summary, queries, consumers, source_stability_proof=None):
    groups = defaultdict(list)
    family = Counter()
    answer_sources = Counter()
    consumer_links = Counter()
    consumer_evidence = Counter()
    answer_source_flows = Counter()
    query_kinds = Counter()
    physical_by_family = Counter()
    reuse_by_family = Counter()
    precomputed_by_family = Counter()
    untyped_hit_by_family = Counter()
    for query in queries.values():
        family[query['family']] += 1
        if query.get('query_kind') == 'physical_search' and query.get('hit') is True and (
                query.get('result_kind') is None or query.get('result_value') is None):
            untyped_hit_by_family[query['family']] += 1
        answer_sources[query.get('answer_source') or 'unknown'] += 1
        query_kinds[query.get('query_kind')] += 1
        if query.get('query_kind') == 'physical_search':
            physical_by_family[query['family']] += 1
        elif query.get('query_kind') == 'answer_reuse':
            reuse_by_family[query['family']] += 1
        elif query.get('query_kind') == 'precomputed_id':
            precomputed_by_family[query['family']] += 1
        if query.get('query_kind') == 'physical_search' and query.get('key_hex') is not None:
            groups[_identity(query)].append(query)
        source_index = query.get('source_query_index')
        if source_index is not None:
            source = queries[source_index]
            answer_source_flows[(query['family'], source.get('site_id'), query.get('site_id'),
                                 query.get('answer_source') or 'unknown',
                                 _json_value_relation(source, query)[0],
                                 _json_value_matches(source, query),
                                 (None if source.get('model_generation') is None or query.get('model_generation') is None
                                  else source.get('model_generation') == query.get('model_generation')))] += 1
    for qindex, linked_consumers in consumers.items():
        query = queries[qindex]
        for consumer in linked_consumers:
            answer_matches = _json_value_matches(query, consumer)
            generation_matches = (None if query.get('model_generation') is None or consumer.get('model_generation') is None
                                  else query.get('model_generation') == consumer.get('model_generation'))
            consumer_evidence[(query['family'], query.get('site_id'), consumer.get('consumer_site_id'),
                               consumer.get('consumer_role'), _json_value_relation(query, consumer)[0], answer_matches,
                               generation_matches, query.get('dictionary_id'),
                               consumer.get('consumer_dictionary_id'))] += 1
    repeat_groups = []
    repeats_by_family = Counter()
    flow_aggregates = Counter()
    per_address_groups = []
    for identity, events in groups.items():
        if len(events) < 2:
            continue
        events.sort(key=lambda item: item['query_index'])
        first = events[0]
        known_dict_gen = first.get('dictionary_generation') is not None and all(
            event.get('dictionary_generation') == first.get('dictionary_generation') for event in events)
        known_model_gen = first.get('model_generation') is not None and all(
            event.get('model_generation') == first.get('model_generation') for event in events)
        dict_generations = [event.get('dictionary_generation') for event in events]
        model_generations = [event.get('model_generation') for event in events]
        known_generation_change = (
            all(value is not None for value in dict_generations) and len(set(dict_generations)) > 1 or
            all(value is not None for value in model_generations) and len(set(model_generations)) > 1)
        source_stable = bool(source_stability_proof and
                             source_stability_proof.get('status') == 'address_scoped_readonly' and
                             isinstance(source_stability_proof.get('source_sha256'), str))
        identity_stable = (known_dict_gen and known_model_gen) or source_stable
        hits = [event.get('hit') for event in events]
        answer_relation, answer_matches = _query_result_agreement(events)
        hit_agrees = answer_relation not in ('hit-state-unobserved', 'hit-state-disagreement')
        # A boolean miss does not describe phrase-array mutations or other
        # wrapper effects. Repeated negatives are equal only when an explicit
        # typed negative answer was captured for every request.
        answer_agrees = answer_matches is True
        linked = {event['query_index']: consumers.get(event['query_index'], []) for event in events}
        all_linked = all(linked.values())
        consumer_matches = all(_json_value_matches(event, consumer)
                               for event in events for consumer in linked[event['query_index']])
        consumer_rows = [row for rows in linked.values() for row in rows]
        consumer_dicts_agree = all(row.get('consumer_dictionary_id') == event['dictionary_id']
                                   for event in events for row in linked[event['query_index']])
        roles = sorted({row.get('consumer_role') or 'unknown' for row in consumer_rows})
        sites = sorted({row.get('consumer_site_id') or 'unknown' for row in consumer_rows})
        key_hex = first.get('key_hex')
        key_bytes = len(key_hex) // 2 if isinstance(key_hex, str) else None
        payload = _payload_bytes(first)
        negative_domain = all((event.get('family'), event.get('operation'),
                               event.get('result_kind'), event.get('site_id')) in NEGATIVE_SCALAR_DOMAINS
                              for event in events)
        if identity_stable and hit_agrees and not hits[0]:
            negative_consumers_complete = (all_linked and consumer_matches and consumer_dicts_agree
                                           and summary.get('consumer_complete') is True)
            if not (answer_agrees and negative_domain and negative_consumers_complete):
                evidence = 'negative-answer-or-consumer-proof-incomplete'
            else:
                evidence = 'source-reviewed-scalar-negative-consumer-linked'
        elif identity_stable and answer_agrees and all_linked and consumer_matches and consumer_dicts_agree:
            if summary.get('consumer_complete') is not True:
                evidence = 'observed-match-covered-sites-coverage-partial'
            else:
                evidence = 'source-proven-stable-answer-to-consumer' if source_stable and not (known_dict_gen and known_model_gen) else 'observed-stable-answer-to-consumer'
        elif not all_linked:
            evidence = 'consumer-link-incomplete'
        elif not identity_stable:
            evidence = 'generation-unknown'
        elif not answer_agrees or not consumer_matches:
            evidence = 'answer-disagreement-or-unobserved'
        else:
            evidence = 'unproven'
        live_end = max((row['_sequence'] for row in consumer_rows), default=None)
        item = {
            'family': first['family'], 'operation': first['operation'],
            'dictionary_id': first['dictionary_id'],
            'dictionary_generation': first.get('dictionary_generation'),
            'model_generation': first.get('model_generation'),
            'query_key_sha256': hashlib.sha256(bytes.fromhex(key_hex)).hexdigest(),
            'query_key_bytes': key_bytes,
            'query_count': len(events),
            'potentially_avoided_physical_searches_upper_bound': len(events) - 1,
            'query_indices': [event['query_index'] for event in events],
            'first_physical_request_site_id': first.get('site_id'),
            'query_answer_relation': answer_relation,
            'request_site_ids': sorted({event.get('site_id') or 'unknown' for event in events}),
            'consumer_site_ids': sites, 'consumer_roles': roles,
            'consumer_events': len(consumer_rows),
            'public_generation_stable': bool(known_dict_gen and known_model_gen),
            'public_generation_status': ('known-stable' if known_dict_gen and known_model_gen else
                                         'known-different' if known_generation_change else 'unknown'),
            'within_parse_source_stability_proof': source_stability_proof if source_stable else None,
            'identity_stable_within_parse': bool(identity_stable),
            'resolved_answer_agrees': bool(answer_agrees),
            'all_queries_have_consumer': bool(all_linked),
            'consumer_values_agree': bool(consumer_matches),
            'consumer_dictionary_identity_agrees': bool(consumer_dicts_agree),
            'consumer_coverage_complete': summary.get('consumer_complete'),
            'consumer_scope': summary.get('consumer_scope'),
            'evidence_status': evidence,
            'live_query_span': (max(event['query_index'] for event in events) - first['query_index']),
            'live_event_span': (live_end - first['_sequence']) if live_end is not None else None,
            'key_bytes_lower_bound': key_bytes,
            'answer_payload_bytes_lower_bound': payload,
            'additional_probe_insert_invalidation_cost': {
                'probe_per_later_request': len(events) - 1,
                'insert_once': 1,
                'generation_validation': 'required if model/dictionary generation is not immutable',
                'timing': 'not measured; design cost estimate only'},
        }
        repeat_groups.append(item)
        per_address_groups.append(item)
        repeats_by_family[first['family']] += len(events) - 1
        flow_aggregates[(first['family'], first['operation'],
                         str(first.get('site_id')), evidence)] += len(events) - 1
        for row in consumer_rows:
            consumer_links[(first['family'], str(first.get('site_id')),
                            str(row.get('consumer_site_id')), str(row.get('consumer_role')))] += 1

    intervals = []
    for item in per_address_groups:
        if item['live_event_span'] is None:
            continue
        first_index = item['query_indices'][0]
        first_seq = queries[first_index]['_sequence']
        end_seq = first_seq + item['live_event_span']
        if item['key_bytes_lower_bound'] is None or item['answer_payload_bytes_lower_bound'] is None:
            continue
        intervals.append((first_seq, end_seq, item['key_bytes_lower_bound'],
                          item['answer_payload_bytes_lower_bound']))
    live_slots = live_key_bytes = live_payload_bytes = max_slots = max_key_bytes = max_payload_bytes = 0
    for event in sorted([(s, 1, k, p) for s, e, k, p in intervals] +
                        [(e + 1, -1, k, p) for s, e, k, p in intervals]):
        _, delta, k, p = event
        live_slots += delta
        live_key_bytes += delta * k
        live_payload_bytes += delta * p
        max_slots, max_key_bytes, max_payload_bytes = (max(max_slots, live_slots),
                                                       max(max_key_bytes, live_key_bytes),
                                                       max(max_payload_bytes, live_payload_bytes))
    return {
        'address_index': summary['address_index'], 'address_key': summary['address_key'],
        'input_bytes': summary.get('input_bytes'), 'output_bytes': summary.get('output_bytes'),
        'query_events': len(queries), 'consumer_events': sum(map(len, consumers.values())),
        'query_counts_by_family': dict(family), 'answer_sources': dict(answer_sources),
        'query_kinds': dict(query_kinds),
        'physical_searches_by_family': dict(physical_by_family),
        'observed_answer_reuses_by_family': dict(reuse_by_family),
        'precomputed_id_events_by_family': dict(precomputed_by_family),
        'hit_rows_without_typed_result_by_family': dict(untyped_hit_by_family),
        'answer_source_flows': [{'family': k[0], 'source_site_id': k[1], 'request_site_id': k[2],
                                 'answer_source': k[3], 'answer_relation': k[4],
                                 'answer_matches_source': k[5],
                                 'model_generation_matches': k[6], 'count': v}
                                for k, v in sorted(answer_source_flows.items(), key=lambda x: str(x[0]))],
            'consumer_link_evidence': [{'family': k[0], 'query_site_id': k[1], 'consumer_site_id': k[2],
                                    'consumer_role': k[3], 'answer_relation': k[4],
                                    'answer_matches_resolved_value': k[5],
                                    'model_generation_matches': k[6],
                                    'source_dictionary_id': k[7], 'consumer_dictionary_id': k[8], 'count': v}
                                   for k, v in sorted(consumer_evidence.items(), key=lambda x: str(x[0]))],
        'repeat_search_groups': len(repeat_groups),
        'max_first_to_last_consumer_event_span': max(
            (item['live_event_span'] for item in repeat_groups if item['live_event_span'] is not None),
            default=None),
        'potential_repeat_searches_upper_bound_by_family': dict(repeats_by_family),
        'generic_cache_probe_count_if_checked_before_every_physical_search': dict(physical_by_family),
        'repeat_groups': repeat_groups,
        'max_simultaneously_live_repeated_answers': max_slots,
        'max_live_key_bytes_lower_bound': max_key_bytes,
        'max_live_numeric_payload_bytes_lower_bound': max_payload_bytes,
        'liveness_scope': 'observed query-to-consumer intervals; excludes hash slots, pointers, allocator and alignment overhead',
    }


def analyze(path, address_rows_path=None, expected_addresses=None, expected_input_hashes=None,
            expected_stability_proof=None):
    """Validate schema/order while holding only the active address's events."""
    header = None
    end = None
    last_summary_query_events = last_summary_consumer_events = None
    current_index = None
    current_queries = {}
    current_consumers = defaultdict(list)
    query_total = consumer_total = address_total = sequence = 0
    aggregates = Counter()
    flows = Counter()
    answer_flows = Counter()
    repeat_flows = Counter()
    family_totals = Counter()
    physical_totals = Counter()
    reuse_totals = Counter()
    precomputed_totals = Counter()
    untyped_hit_totals = Counter()
    repeat_upper_totals = Counter()
    source_totals = Counter()
    evidence_totals = Counter()
    address_metrics = []
    out = address_rows_path.open('x', encoding='utf-8') if address_rows_path else None

    def close_address(summary):
        nonlocal current_index, current_queries, current_consumers, address_total
        nonlocal last_summary_query_events, last_summary_consumer_events
        if expected_input_hashes is not None:
            expected_hash = expected_input_hashes.get(str(summary['address_index']))
            if summary['address_key'] != expected_hash:
                raise ValueError('trace input hash mismatch at address %s' % summary['address_index'])
        header_proof = header.get('within_parse_stability_proof')
        proof = ({**expected_stability_proof, **header_proof}
                 if header_proof is not None and expected_stability_proof is not None else None)
        report = _address_report(summary, current_queries, current_consumers, proof)
        last_summary_query_events = len(current_queries)
        last_summary_consumer_events = sum(map(len, current_consumers.values()))
        address_total += 1
        for name, count in report['query_counts_by_family'].items():
            family_totals[name] += count
        for name, count in report['physical_searches_by_family'].items():
            physical_totals[name] += count
        for name, count in report['observed_answer_reuses_by_family'].items():
            reuse_totals[name] += count
        for name, count in report['precomputed_id_events_by_family'].items():
            precomputed_totals[name] += count
        for name, count in report['hit_rows_without_typed_result_by_family'].items():
            untyped_hit_totals[name] += count
        for name, count in report['potential_repeat_searches_upper_bound_by_family'].items():
            repeat_upper_totals[name] += count
        for name, count in report['answer_sources'].items():
            source_totals[name] += count
        for flow in report['answer_source_flows']:
            answer_flows[(flow['family'], flow['source_site_id'], flow['request_site_id'], flow['answer_source'], flow['answer_relation'], flow['answer_matches_source'], flow['model_generation_matches'])] += flow['count']
        for flow in report['consumer_link_evidence']:
            flows[(flow['family'], flow['query_site_id'], flow['consumer_site_id'], flow['consumer_role'], flow['answer_relation'], flow['answer_matches_resolved_value'], flow['model_generation_matches'], flow['source_dictionary_id'], flow['consumer_dictionary_id'])] += flow['count']
        for group in report['repeat_groups']:
            aggregates[(group['family'], group['operation'], group['first_physical_request_site_id'], group['evidence_status'])] += group['potentially_avoided_physical_searches_upper_bound']
            evidence_totals[group['evidence_status']] += group['potentially_avoided_physical_searches_upper_bound']
            for site in group['consumer_site_ids']:
                repeat_flows[(group['family'], group['operation'],
                              group['first_physical_request_site_id'], site)] += group['consumer_events']
        if out:
            out.write(json.dumps(report, ensure_ascii=True, separators=(',', ':')) + '\n')
        else:
            address_metrics.append({k: report[k] for k in ('address_index', 'address_key',
                'query_events', 'consumer_events', 'repeat_search_groups',
                'max_first_to_last_consumer_event_span',
                'max_simultaneously_live_repeated_answers', 'max_live_key_bytes_lower_bound',
                'max_live_numeric_payload_bytes_lower_bound')})
        current_index = None
        current_queries = {}
        current_consumers = defaultdict(list)

    try:
        trace_path = Path(path)
        opener = gzip.open if trace_path.suffix == '.gz' else open
        with opener(trace_path, 'rt', encoding='utf-8') as trace_stream:
          for line_no, line in enumerate(trace_stream, 1):
            if not line.strip():
                raise ValueError(f'blank trace row at line {line_no}')
            try:
                row = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(f'invalid JSON at line {line_no}: {exc}') from exc
            if not isinstance(row, dict):
                raise ValueError(f'trace row {line_no} is not an object')
            kind = row.get('record_type')
            if kind == 'trace_header':
                if header is not None or address_total or current_index is not None:
                    raise ValueError('trace_header must be first and unique')
                if row.get('schema_version') != 1 or row.get('overflow_policy') not in (
                        'fail_closed', 'fail_closed_all_rows', 'fail_closed_data_rows'):
                    raise ValueError('unsupported or non-fail-closed trace header')
                if row.get('key_encoding') != 'hex':
                    raise ValueError('exact query identity must be hex encoded')
                if row.get('event_cap_scope') != 'query_and_consumer_rows':
                    raise ValueError('trace event cap scope must count all query and consumer rows')
                if type(row.get('event_cap')) is not int or row['event_cap'] <= 0:
                    raise ValueError('trace event cap must be a positive integer')
                proof = row.get('within_parse_stability_proof')
                if proof is not None:
                    if (not isinstance(proof, dict) or proof.get('status') != 'address_scoped_readonly'
                            or not isinstance(proof.get('proof_id'), str)
                            or not isinstance(proof.get('scope'), str)
                            or (proof.get('source_sha256') is not None and
                                (not isinstance(proof.get('source_sha256'), str)
                                 or len(proof['source_sha256']) != 64
                                 or any(c not in '0123456789abcdef' for c in proof['source_sha256'])))):
                        raise ValueError('invalid within-parse source stability proof')
                    if expected_stability_proof is None:
                        raise ValueError('trace claims source stability without configured source proof')
                    for field in ('status', 'proof_id', 'scope'):
                        if proof.get(field) != expected_stability_proof.get(field):
                            raise ValueError('trace source proof does not match configured source/flags manifest: ' + field)
                    if (not isinstance(expected_stability_proof.get('source_sha256'), str)
                            or len(expected_stability_proof['source_sha256']) != 64
                            or row.get('source_manifest_sha256') != expected_stability_proof['source_sha256']):
                        raise ValueError('configured source bundle digest is missing or differs from trace proof')
                header = row
            elif kind == 'trace_overflow':
                raise ValueError('overflow invalidates the complete run')
            elif kind == 'trace_end':
                if header is None or end is not None or current_index is not None:
                    raise ValueError('trace_end misplaced or duplicated')
                end = row
            elif kind == 'query':
                if header is None or end is not None:
                    raise ValueError('query outside trace body')
                index = row.get('address_index')
                if type(index) is not int or index < 1:
                    raise ValueError('query address index must be positive')
                if current_index is None:
                    current_index = index
                if index != current_index:
                    raise ValueError('address changed before previous address_summary')
                qindex = row.get('query_index')
                if type(qindex) is not int or qindex != len(current_queries) + 1:
                    raise ValueError('query indices must be contiguous within address')
                if row.get('query_kind') not in ('physical_search', 'answer_reuse', 'precomputed_id'):
                    raise ValueError('unknown query_kind')
                for field in ('family', 'operation', 'site_id'):
                    if not isinstance(row.get(field), str) or not row[field]:
                        raise ValueError(f'query missing {field}')
                if row.get('query_kind') != 'precomputed_id' and (
                        not isinstance(row.get('dictionary_id'), str) or not row['dictionary_id']):
                    raise ValueError('runtime lookup query missing dictionary identity')
                key = row.get('key_hex')
                if row.get('query_kind') == 'precomputed_id' and key is None:
                    pass
                elif (not isinstance(key, str) or len(key) % 2
                      or any(c not in '0123456789abcdef' for c in key)):
                    raise ValueError('physical query key_hex must encode exact bytes')
                if row['query_kind'] == 'physical_search':
                    if type(row.get('hit')) is not bool:
                        raise ValueError('physical search requires hit/miss boolean')
                    # Untyped hits remain unobserved and cannot support answer
                    # equality or consumer-linked reuse claims.
                elif row.get('query_kind') == 'answer_reuse' and row.get('source_query_index') is None:
                    raise ValueError('answer_reuse needs a source query link')
                row['_sequence'] = sequence
                sequence += 1
                current_queries[qindex] = row
                source_index = row.get('source_query_index')
                if source_index is not None:
                    if type(source_index) is not int or source_index not in current_queries or source_index >= qindex:
                        raise ValueError('answer source must refer to an earlier query in this address')
                    if row.get('query_kind') != 'answer_reuse':
                        raise ValueError('source_query_index is only valid for answer_reuse records')
                    source_row = current_queries[source_index]
                    if source_row.get('query_kind') != 'physical_search' or _identity(source_row) != _identity(row):
                        raise ValueError('answer_reuse source must have exact dictionary/operation/family/key identity')
                    if (row.get('producer_query_index') != source_index
                            or row.get('hit') is not source_row.get('hit')
                            or row.get('model_generation') != source_row.get('model_generation')):
                        raise ValueError('answer_reuse producer link, hit state, or model scope differs from source')
                    if (row.get('result_kind') is not None and row.get('result_value') is not None
                            and source_row.get('result_kind') is not None
                            and source_row.get('result_value') is not None
                            and not _json_value_matches(source_row, row)):
                        raise ValueError('typed answer_reuse value differs from its physical producer')
                query_total += 1
                if query_total + consumer_total > header['event_cap']:
                    raise ValueError('trace event total exceeds declared cap')
            elif kind == 'query_consumer':
                if header is None or end is not None or current_index is None:
                    raise ValueError('consumer outside active address')
                if row.get('address_index') != current_index:
                    raise ValueError('consumer address does not match active address')
                source = row.get('source_query_index')
                if type(source) is not int or source not in current_queries:
                    raise ValueError('consumer source must refer to an earlier query in this address')
                if not all(isinstance(row.get(field), str) and row[field] for field in
                           ('consumer_dictionary_id', 'consumer_site_id', 'consumer_stage', 'consumer_role', 'value_kind')):
                    raise ValueError('consumer site, stage, role, and value kind are required')
                row['_sequence'] = sequence
                sequence += 1
                current_consumers[source].append(row)
                consumer_total += 1
                if query_total + consumer_total > header['event_cap']:
                    raise ValueError('trace event total exceeds declared cap')
            elif kind == 'address_summary':
                if header is None or end is not None:
                    raise ValueError('address summary outside trace body')
                index = row.get('address_index')
                if type(index) is not int or index < 1:
                    raise ValueError('address summary index must be positive')
                if current_index is None:
                    current_index = index
                if index != current_index or index != address_total + 1:
                    raise ValueError('address summaries must be ordered and match active address')
                if not isinstance(row.get('address_key'), str) or len(row['address_key']) != 64:
                    raise ValueError('address_summary needs SHA-256 address key')
                if row.get('complete') is False:
                    raise ValueError('address summary is incomplete')
                if row.get('query_events') is not None and row['query_events'] != len(current_queries):
                    raise ValueError('address query count differs from event stream')
                expected_consumers = row.get('query_consumer_events', row.get('consumer_events'))
                if expected_consumers is not None and expected_consumers != sum(map(len, current_consumers.values())):
                    raise ValueError('address consumer count differs from event stream')
                if type(row.get('consumer_complete')) is not bool or not isinstance(row.get('consumer_scope'), list):
                    raise ValueError('address summary needs explicit consumer coverage status and scope')
                close_address(row)
            else:
                raise ValueError(f'unknown trace record type at line {line_no}: {kind!r}')
        if header is None or end is None:
            raise ValueError('trace missing header or trace_end')
        if end.get('complete') is not True or end.get('overflow') is not False:
            raise ValueError('trace_end is incomplete or reports overflow')
        if (end.get('address_count_mismatch') not in (None, False)
                or end.get('addresses') != address_total or end.get('expected_addresses') != address_total):
            raise ValueError('trace address count mismatch')
        if expected_addresses is not None and address_total != expected_addresses:
            raise ValueError('trace address count differs from declared workload')
        if expected_input_hashes is not None and set(expected_input_hashes) != {str(i) for i in range(1, address_total + 1)}:
            raise ValueError('declared workload input hash index/count mismatch')
        # The frozen collector resets these two counters at each address begin;
        # trace_end therefore repeats the final address counts. Global rows are
        # independently reconciled through total_event_rows below.
        if (end.get('query_events') != last_summary_query_events
                or end.get('consumer_events') != last_summary_consumer_events):
            raise ValueError('trace_end final-address counts differ from its address_summary')
        if end.get('total_event_rows') is not None and end['total_event_rows'] != query_total + consumer_total:
            raise ValueError('trace total_event_rows differs from observed events')
        declared_coverage = header.get('consumer_coverage')
        if declared_coverage is not None and (not isinstance(declared_coverage, list) or any(not isinstance(v, str) for v in declared_coverage)):
            raise ValueError('consumer coverage must be a family list when present')
        missing_families = set(declared_coverage or ()) - set(family_totals)
        return {
            'schema_version': 1, 'header': {k: v for k, v in header.items() if k != 'record_type'},
            'complete': True, 'addresses': address_total, 'query_events': query_total,
            'expected_addresses': expected_addresses,
            'missing_declared_consumer_families_in_workload': sorted(missing_families),
            'consumer_coverage': 'per-address summary; incomplete mappings remain explicitly partial',
            'consumer_events': consumer_total,
            'trace_end_counter_scope': 'final address for query_events/consumer_events; global total_event_rows reconciled separately',
            'family_query_totals': dict(family_totals), 'answer_source_totals': dict(source_totals),
            'physical_search_totals_by_family': dict(physical_totals),
            'observed_answer_reuse_totals_by_family': dict(reuse_totals),
            'precomputed_id_totals_by_family': dict(precomputed_totals),
            'hit_rows_without_typed_result_by_family': dict(untyped_hit_totals),
            'potential_repeat_search_upper_bounds_by_family': dict(repeat_upper_totals),
            'generic_cache_probe_lower_bounds_by_family': dict(physical_totals),
            'upper_bound_search_savings_fraction_by_family': {
                family: (repeat_upper_totals[family] / count if count else None)
                for family, count in physical_totals.items()},
            'repeat_extra_searches_by_family_site_evidence': [
                {'family': k[0], 'operation': k[1], 'first_physical_request_site_id': k[2],
                 'evidence_status': k[3], 'later_physical_searches': v}
                for k, v in sorted(aggregates.items())],
            'answer_source_flows': [{'family': k[0], 'source_site_id': k[1], 'request_site_id': k[2],
                                     'answer_source': k[3], 'answer_relation': k[4],
                                     'answer_matches_source': k[5],
                                     'model_generation_matches': k[6], 'count': v}
                                    for k, v in sorted(answer_flows.items(), key=lambda x: str(x[0]))],
            'consumer_flows': [{'family': k[0], 'query_site_id': k[1], 'consumer_site_id': k[2],
                                'consumer_role': k[3], 'answer_relation': k[4],
                                'answer_matches_resolved_value': k[5],
                                'model_generation_matches': k[6],
                                'source_dictionary_id': k[7], 'consumer_dictionary_id': k[8], 'count': v}
                               for k, v in sorted(flows.items(), key=lambda x: str(x[0]))],
            'repeat_group_consumer_flows': [{'family': k[0], 'operation': k[1],
                                             'first_physical_request_site_id': k[2],
                                             'consumer_site_id': k[3], 'linked_consumer_events': v}
                                            for k, v in sorted(repeat_flows.items())],
            'repeat_evidence_totals': dict(evidence_totals),
            'per_address_metrics': address_metrics if not out else 'streamed to address_rows_path',
            'memory_scope': 'only one address query/consumer table is retained; aggregate keys contain family/site metadata, not query bytes',
        }
    finally:
        if out:
            out.close()


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('trace', type=Path)
    ap.add_argument('--address-rows', type=Path, help='new JSONL destination for per-address reports')
    ap.add_argument('--output', type=Path, help='new JSON summary destination')
    args = ap.parse_args(argv)
    report = analyze(args.trace, args.address_rows)
    if args.output:
        with args.output.open('x', encoding='utf-8') as stream:
            json.dump(report, stream, indent=2)
            stream.write('\n')
    print(json.dumps(report, indent=2))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        raise SystemExit(2)
