#!/usr/bin/env python3
"""Preflight, collect gated small-workload traces, and analyze architecture traces."""
import argparse
import datetime as dt
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import platform
import re
import sys

ROOT = Path(__file__).resolve().parents[4]
EXPERIMENT = ROOT / 'research/experiments/186-architecture-benchmark'
DEFAULT_CONFIG = EXPERIMENT / 'config.example.json'
MAX_COUNT = (1 << 64) - 1


def stamp():
    return dt.datetime.now().astimezone().isoformat()


def resolve(path):
    p = Path(path)
    return p if p.is_absolute() else ROOT / p


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def save(path, value):
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def require_byte_exact(oracle_path, candidate_path):
    """Compare complete response byte streams and return SHA-256 provenance."""
    with Path(oracle_path).open('rb') as oracle, Path(candidate_path).open('rb') as candidate:
        while True:
            left = oracle.read(1024 * 1024)
            right = candidate.read(1024 * 1024)
            if left != right:
                raise ValueError('output mismatch: %s != %s' % (candidate_path, oracle_path))
            if not left:
                break
    return {'status': 'byte-identical', 'oracle_sha256': digest(oracle_path),
            'candidate_sha256': digest(candidate_path)}


def require_single_thread(samples):
    """Require observed thread samples; a too-short run is incomplete evidence."""
    if not samples:
        raise ValueError('single-thread evidence unavailable: no process samples')
    if any(type(n) is not int or n != 1 for n in samples):
        raise ValueError('single-thread check failed: %r' % (samples,))


def percentile(values, p):
    """Nearest-rank percentile, explicitly deterministic for small samples."""
    if not values:
        return None
    ordered = sorted(values)
    return ordered[max(0, math.ceil(p * len(ordered)) - 1)]


def _count(value, label):
    if type(value) is not int or value < 0 or value > MAX_COUNT:
        raise ValueError(label + ' must be a nonnegative integer')
    return value


def analyze_trace(path, expected_addresses=None, expected_input_hashes=None,
                  strata_by_address=None):
    """Validate a bounded JSONL trace and report per-address architecture metrics.

    Unknown event families and unavailable counters are retained as unavailable;
    incomplete traces, undeclared addresses, or overflow fail closed.
    """
    header = None
    end = None
    addresses = {}
    queries = {}
    errors = []
    with Path(path).open(encoding='utf-8') as stream:
        for line_no, line in enumerate(stream, 1):
            if not line.strip():
                raise ValueError('blank trace row at line %d' % line_no)
            try:
                row = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError('invalid JSON at line %d: %s' % (line_no, exc)) from exc
            if not isinstance(row, dict):
                raise ValueError('trace row must be an object at line %d' % line_no)
            kind = row.get('record_type')
            if kind == 'trace_header':
                if header is not None or addresses or end is not None:
                    raise ValueError('trace_header must be the first and only header')
                if row.get('schema_version') != 1:
                    raise ValueError('unsupported trace schema')
                if not isinstance(row.get('source_variant'), str) or not row['source_variant']:
                    raise ValueError('trace header needs source_variant')
                if row.get('key_encoding') != 'hex':
                    raise ValueError('trace must encode exact key bytes as hex')
                if row.get('overflow_policy') != 'fail_closed':
                    raise ValueError('trace overflow policy must fail closed')
                cap = _count(row.get('event_cap'), 'event_cap')
                if cap <= 0:
                    raise ValueError('event_cap must be positive')
                header = row
            elif kind == 'trace_overflow':
                raise ValueError('trace overflow reported; all exact metrics rejected')
            elif kind == 'trace_end':
                if header is None or end is not None:
                    raise ValueError('trace_end without header or duplicate end')
                end = row
            elif kind == 'address_summary':
                if header is None or end is not None:
                    raise ValueError('address_summary outside trace body')
                idx = _count(row.get('address_index'), 'address_index')
                key = row.get('address_key')
                if not isinstance(key, str) or not re.fullmatch(r'[0-9a-f]{64}', key):
                    raise ValueError('address_key must be SHA-256 hex')
                if idx in addresses:
                    raise ValueError('duplicate address_index %d' % idx)
                addresses[idx] = row
            elif kind == 'query':
                if header is None or end is not None:
                    raise ValueError('query outside trace body')
                idx = _count(row.get('address_index'), 'address_index')
                qidx = _count(row.get('query_index'), 'query_index')
                if row.get('query_kind') not in ('physical_search', 'answer_reuse'):
                    raise ValueError('unknown query_kind')
                if not isinstance(row.get('operation'), str) or not row['operation']:
                    raise ValueError('query needs operation')
                if not isinstance(row.get('family'), str) or not row['family']:
                    raise ValueError('query needs family')
                if not isinstance(row.get('dictionary_id'), str) or not row['dictionary_id']:
                    raise ValueError('query needs dictionary identity')
                if row.get('key_hex') is not None:
                    if not isinstance(row['key_hex'], str) or not re.fullmatch(r'(?:[0-9a-f]{2})*', row['key_hex']):
                        raise ValueError('key_hex must represent exact bytes')
                if row.get('dictionary_generation') is None and not isinstance(row.get('generation_reason'), str):
                    raise ValueError('unknown generation requires a reason')
                if row['query_kind'] == 'physical_search':
                    _count(row.get('physical_search_index'), 'physical_search_index')
                    if type(row.get('hit')) is not bool:
                        raise ValueError('physical search needs boolean hit')
                elif row.get('physical_search_index') is not None:
                    raise ValueError('answer reuse cannot claim a physical search')
                group = queries.setdefault(idx, [])
                if any(existing['query_index'] == qidx for existing in group):
                    raise ValueError('duplicate query index within address')
                group.append(row)
            else:
                raise ValueError('unknown record_type at line %d: %r' % (line_no, kind))
    if header is None or end is None:
        raise ValueError('trace is missing header or trace_end')
    if header.get('generation_semantics') in (None, 'null unless observed'):
        # Per-query generation may still be available, but do not silently assert it.
        generation_coverage = 'partial; consult per-query generation_reason'
    else:
        generation_coverage = 'declared by instrumentation'
    declared = _count(end.get('addresses'), 'trace_end.addresses')
    if (declared != len(addresses) or (expected_addresses is not None and declared != expected_addresses)
            or end.get('expected_addresses') != declared):
        raise ValueError('address count incomplete: expected %s, trace has %d summaries' %
                         (expected_addresses, len(addresses)))
    if end.get('complete') is not True:
        raise ValueError('trace_end does not declare complete=true')
    if end.get('overflow') is not False or end.get('address_count_mismatch') is not False:
        raise ValueError('trace_end reports overflow')
    if _count(end.get('query_events'), 'trace_end.query_events') != sum(map(len, queries.values())):
        raise ValueError('trace_end query count differs from retained query records')
    if set(queries) - set(addresses):
        raise ValueError('query references an address without a summary')
    if expected_input_hashes is not None:
        actual = {str(row['address_index']): row['address_key'] for row in addresses.values()}
        if actual != {str(k): v for k, v in expected_input_hashes.items()}:
            raise ValueError('input identity hashes do not match declared workload')

    per_address = []
    by_stratum = {}
    for idx, summary in sorted(addresses.items()):
        events = sorted(queries.get(idx, []), key=lambda q: q['query_index'])
        if [e['query_index'] for e in events] != list(range(1, len(events) + 1)):
            raise ValueError('query indices must be contiguous from one per address')
        physical = [e for e in events if e['query_kind'] == 'physical_search']
        reuses = [e for e in events if e['query_kind'] == 'answer_reuse']
        for n, event in enumerate(physical):
            if event['physical_search_index'] != n + 1:
                raise ValueError('physical search indices must be contiguous from one per address')
        identities = {}
        repeats = 0
        same_generation_repeats = 0
        lifetime_unproven_repeats = 0
        for event in physical:
            identity = (event['dictionary_id'], event.get('dictionary_generation'),
                        event.get('operation'), event.get('family'), event.get('key_hex'))
            if identity in identities:
                repeats += 1
                prev = identities[identity]
                if (identity[1] is not None and prev.get('dictionary_generation') == identity[1]
                        and event.get('safe_reuse_proof') == 'valid'):
                    same_generation_repeats += 1
                else:
                    lifetime_unproven_repeats += 1
            else:
                identities[identity] = event
        def metric(name, source=None):
            source = source or name
            val = summary.get(source)
            if val is None:
                reason = summary.get(source + '_reason') or summary.get('metric_reasons', {}).get(source)
                return {'value': None, 'reason': reason or 'not emitted'}
            return {'value': _count(val, name), 'reason': None}
        metrics = {
            'logical_requests': len(events),
            'physical_searches': len(physical),
            'answer_reuses': len(reuses),
            'executed_repeat_searches': repeats,
            'generation_valid_repeat_searches': same_generation_repeats,
            'lifetime_unproven_repeat_searches': lifetime_unproven_repeats,
            'hits': sum(e['hit'] for e in physical),
            'misses': sum(not e['hit'] for e in physical),
            'text_keys_created': metric('feature_keys_materialized'),
            'text_bytes_created': metric('feature_key_bytes_materialized'),
            'allocation_calls': metric('scoped_allocation_calls'),
            'allocation_requested_bytes': metric('scoped_allocation_requested_bytes'),
            'retained_bookkeeping_bytes': metric('diagnostic_tracker_bytes_current'),
            'diagnostic_tracker_peak_bytes_run': metric('diagnostic_tracker_peak_bytes_run'),
            'replacement_cost': {'value': None, 'reason': 'not instrumented'},
            'input_bytes': metric('input_bytes'),
            'output_bytes': metric('output_bytes'),
            'model_rows': metric('model_rows'),
            'weights_accumulated': metric('weights_accumulated'),
        }
        if summary.get('physical_searches') is not None and summary['physical_searches'] != len(physical):
            raise ValueError('query stream and address summary physical search totals differ')
        if summary.get('logical_requests') is not None and summary['logical_requests'] != len(events):
            raise ValueError('query stream and address summary logical request totals differ')
        strata = (strata_by_address or {}).get(idx, summary.get('strata', {}))
        if not isinstance(strata, dict):
            raise ValueError('stratum metadata must be an object')
        distribution_strata = {k: v for k, v in strata.items() if k != 'case_id'}
        stratum_key = json.dumps(distribution_strata, sort_keys=True) if distribution_strata else 'unknown'
        item = {'address_index': idx, 'address_key': summary['address_key'],
                'strata': strata or {'label': 'unknown'}, 'metrics': metrics,
                'generation_coverage': generation_coverage,
                'output_sha256': summary.get('output_sha256')}
        per_address.append(item)
        bucket = by_stratum.setdefault(stratum_key, {'strata': distribution_strata or {'label': 'unknown'}, 'rows': []})
        bucket['rows'].append(metrics)

    distributions = {}
    for stratum_key, bucket in by_stratum.items():
        names = sorted(set(k for row in bucket['rows'] for k in row))
        for name in names:
            vals = [row[name] if isinstance(row.get(name), int) else row[name].get('value')
                    for row in bucket['rows'] if isinstance(row.get(name), int) or isinstance(row.get(name), dict)]
            vals = [v for v in vals if isinstance(v, int) and not isinstance(v, bool)]
            distributions.setdefault(name, {})[stratum_key] = {
                'strata': bucket['strata'], 'n': len(vals), 'median': percentile(vals, .5),
                'p95': percentile(vals, .95), 'max': max(vals) if vals else None,
                'reason': None if vals else 'no available per-address observations'}
    return {'schema_version': 1, 'status': 'complete trace; diagnostic guidance only',
            'trace_sha256': digest(path), 'source_variant': header['source_variant'],
            'event_cap': header['event_cap'], 'address_count': len(addresses),
            'query_event_count': sum(map(len, queries.values())),
            'overflow': False, 'generation_coverage': generation_coverage,
            'per_address': per_address, 'distributions': distributions,
            'acceptance': {'byte_identical': None, 'single_thread': None,
                           'speed_memory_eligible': False,
                           'reason': 'analysis does not substitute for the uninstrumented acceptance benchmark'}}


def load_measurement_runner():
    path = ROOT / 'research/tools/run_measurements.py'
    spec = importlib.util.spec_from_file_location('measurement_runner_185', path)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot import 185 measurement helpers')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run_preflight(cfg, mode):
    problems = []
    workload_modes = cfg.get('workload_modes', {})
    if mode not in workload_modes:
        problems.append('workload mode not configured: ' + mode)
    for name in ('oracle', 'release_control', 'diagnostic'):
        command = cfg.get('commands', {}).get(name)
        if not isinstance(command, list) or not command:
            problems.append('command not prepared: ' + name)
        elif any(not isinstance(token, str) or not token for token in command):
            problems.append('invalid command token: ' + name)
        elif not any('{' in token for token in command):
            executable = resolve(command[0])
            if not executable.is_file():
                problems.append('missing executable: ' + str(executable))
    oracle = cfg.get('pinned_oracle', {})
    if oracle.get('baseline_library') != 'research/experiments/000-baseline/artifacts/libpostal.dylib':
        problems.append('pinned oracle must identify the preserved experiment 000 dynamic library')
    for name, path in [('oracle library', oracle.get('baseline_library')),
                       ('oracle dump', oracle.get('baseline_dump')),
                       ('model directory', oracle.get('model_dir'))]:
        if not path:
            problems.append('missing configured ' + name)
        elif not resolve(path).exists():
            problems.append('missing ' + name + ': ' + str(resolve(path)))
    for field in ('release_library', 'diagnostic_library', 'release_dump', 'diagnostic_dump',
                  'source_provenance', 'diagnostic_provenance'):
        value = cfg.get('artifacts', {}).get(field)
        if not value:
            problems.append('artifact not prepared: ' + field)
        elif not resolve(value).is_file():
            problems.append('missing artifact %s: %s' % (field, resolve(value)))
    for name, rows in workload_modes.items():
        if not isinstance(rows, list) or not rows:
            problems.append('empty workload mode: ' + name)
            continue
        for workload in rows:
            if not isinstance(workload, dict) or not workload.get('name'):
                problems.append('malformed workload in mode ' + name)
                continue
            for field in ('input', 'case_manifest', 'address_metadata', 'frozen_reference'):
                if workload.get(field):
                    if not resolve(workload[field]).is_file():
                        problems.append('missing %s for %s: %s' % (field, workload['name'], workload[field]))
    return problems


def _case_selection(workload, trace=False):
    data = json.loads(resolve(workload['case_manifest']).read_text())
    cases = data.get('cases')
    if not isinstance(cases, list):
        raise ValueError('case manifest needs a cases array')
    api_tool_path = EXPERIMENT / 'tools/edge_api_dump.py'
    spec = importlib.util.spec_from_file_location('edge_api_dump_186', api_tool_path)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load edge case selector')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    selected = module.select_safe_cases(cases)
    ids = workload.get('trace_case_ids') if trace else workload.get('case_ids')
    if ids:
        by_id = {case['id']: (case, safe) for case, safe in selected}
        missing = [case_id for case_id in ids if case_id not in by_id]
        if missing:
            raise ValueError('case ids are absent or unsafe: ' + ', '.join(missing))
        selected = [by_id[case_id] for case_id in ids]
    if trace:
        selected = [(case, safe) for case, safe in selected if safe]
    return selected


def _workload_count(workload, trace=False):
    if workload.get('input'):
        with resolve(workload['input']).open('rb') as stream:
            return sum(1 for line in stream if line.rstrip(b'\r\n'))
    trace = trace or workload.get('protocol') == 'api-trace'
    return len(_case_selection(workload, trace=trace))


def _expected_input_hashes(workload):
    if workload.get('input'):
        values = []
        with resolve(workload['input']).open('rb') as stream:
            for line in stream:
                address = line.rstrip(b'\r\n')
                if address:
                    values.append(hashlib.sha256(address).hexdigest())
        return {str(index): value for index, value in enumerate(values, 1)}
    selected = _case_selection(workload, trace=workload.get('protocol') == 'api-trace')
    hashes = {}
    for case, _trace_safe in selected:
        raw = case.get('input_utf8', '').encode('utf-8')
        # The parser's public C API stops at the first NUL byte.
        effective = raw.split(b'\0', 1)[0]
        hashes[str(len(hashes) + 1)] = hashlib.sha256(effective).hexdigest()
    return hashes


def _strata_for(workload, count):
    shared = {k: workload.get(k) or 'unknown' for k in ('source', 'language', 'country', 'split')}
    if workload.get('case_manifest'):
        selected = _case_selection(workload, trace=workload.get('protocol') == 'api-trace')
        if len(selected) != count:
            raise ValueError('selected case count differs from workload count')
        return {i: {**shared, 'source': case.get('provenance') or shared['source'],
                    'case_id': case['id'], 'category': case['category']}
                for i, (case, _safe) in enumerate(selected, 1)}
    metadata_path = workload.get('address_metadata')
    if not metadata_path:
        return {i: shared for i in range(1, count + 1)}
    metadata = json.loads(resolve(metadata_path).read_text())
    records = metadata.get('records')
    if not isinstance(records, list) or len(records) != count:
        raise ValueError('address metadata count differs from input workload')
    result = {}
    for i, record in enumerate(records, 1):
        if not isinstance(record.get('case_id'), str) or not isinstance(record.get('category'), str):
            raise ValueError('address metadata needs case_id and category')
        result[i] = {**shared, 'case_id': record['case_id'], 'category': record['category']}
    return result


def _command_for(template, role, workload, cfg, output_path):
    oracle = cfg['pinned_oracle']
    artifacts = cfg['artifacts']
    selected_case_ids = (workload.get('case_ids') or workload.get('trace_case_ids') or
                         ([case['id'] for case, _ in _case_selection(workload)] if workload.get('case_manifest') else []))
    values = {'oracle_library': str(resolve(oracle['baseline_library'])),
              'model_dir': str(resolve(oracle['model_dir'])),
              'release_library': str(resolve(artifacts['release_library'])),
              'diagnostic_library': str(resolve(artifacts['diagnostic_library'])),
              'oracle_dump': str(resolve(oracle['baseline_dump'])),
              'release_dump': str(resolve(artifacts['release_dump'])),
              'diagnostic_dump': str(resolve(artifacts['diagnostic_dump'])),
              'output_path': str(output_path),
              'input_flag': '--input-txt' if workload.get('input') else '--cases',
              'input_value': str(resolve(workload['input'])) if workload.get('input') else str(resolve(workload['case_manifest'])),
              'input_path': str(resolve(workload['input'])) if workload.get('input') else '',
              'case_manifest': str(resolve(workload['case_manifest'])) if workload.get('case_manifest') else '',
              'case_ids': ','.join(selected_case_ids),
              'trace_case_ids': ','.join(workload.get('trace_case_ids', [])),
              'trace_case_id': workload['name'],
              'trace_path': str(workload.get('_trace_path', ''))}
    command = []
    for token in template:
        try:
            command.append(token.format_map(values))
        except KeyError as exc:
            raise ValueError('unknown command placeholder: ' + str(exc)) from exc
    if any('{' in token or '}' in token for token in command):
        raise ValueError('unresolved command template value')
    return command


def collect(cfg, mode, output):
    """Serially run oracle, release control and trace build with 185's AC/thread monitor."""
    runner = load_measurement_runner()
    output.mkdir(parents=True, exist_ok=False)
    mode_rows = cfg['workload_modes'][mode]
    (output / 'runner.py').write_bytes(Path(__file__).read_bytes())
    config_bytes = Path(args_config_path).read_bytes() if args_config_path else b''
    (output / 'config.json').write_bytes(config_bytes)
    result = {'schema_version': 1, 'status': 'running', 'kind': 'architecture operation trace',
              'mode': mode, 'start': stamp(), 'end': None,
              'host': {'platform': platform.platform(), 'python': sys.version},
              'expected_workloads': len(mode_rows), 'expected_records': len(mode_rows),
              'expected_processes': len(mode_rows) * 3,
              'records': [], 'acceptance': {'final_speed_memory_benchmark': 'not run'}}
    save(output / 'results.json', result)
    path_fields = [cfg['pinned_oracle']['baseline_library'], cfg['pinned_oracle']['baseline_dump'],
                   cfg['artifacts']['release_library'], cfg['artifacts']['diagnostic_library'],
                   cfg['artifacts']['release_dump'], cfg['artifacts']['diagnostic_dump'],
                   cfg['artifacts']['source_provenance'],
                   cfg['artifacts']['diagnostic_provenance']]
    workload_paths = []
    for row in mode_rows:
        field = 'input' if row.get('input') else 'case_manifest'
        workload_paths.append(row[field])
        for extra in ('address_metadata', 'frozen_reference'):
            if row.get(extra):
                workload_paths.append(row[extra])
        if row.get('address_metadata'):
            workload_paths.append(row['address_metadata'])
    hashed_paths = [Path(__file__), Path(args_config_path), resolve('research/tools/run_measurements.py'),
                    resolve('research/tools/env_gate.sh'), EXPERIMENT / 'tools/edge_api_dump.py',
                    EXPERIMENT / 'tools/materialize_trace_inputs.py', EXPERIMENT / 'trace-schema.json']
    hashed_paths.extend(resolve(p) for p in path_fields)
    hashed_paths.extend(resolve(p) for p in workload_paths)
    source_dir = EXPERIMENT / 'source'
    if source_dir.is_dir():
        hashed_paths.extend(p for p in source_dir.rglob('*') if p.is_file())
    model_dir = resolve(cfg['pinned_oracle']['model_dir'])
    hashed_paths.extend(p for p in model_dir.rglob('*') if p.is_file())
    hashed_paths = list(dict.fromkeys(hashed_paths))
    file_hashes = {str(p): digest(p) if p.is_file() else None for p in hashed_paths}
    if any(value is None for value in file_hashes.values()):
        raise FileNotFoundError('a configured identity path disappeared before execution')
    save(output / 'manifest.json', {'created': stamp(), 'hash_algorithm': 'SHA-256',
         'files': file_hashes, 'roles': {'inputs': [str(resolve(p)) for p in workload_paths],
         'pinned_oracle': [str(resolve(p)) for p in path_fields[:2]],
         'model_files': [str(p) for p in model_dir.rglob('*') if p.is_file()],
         'release_control': [str(resolve(cfg['artifacts'][k])) for k in ('release_library', 'source_provenance')],
         'diagnostic': [str(resolve(cfg['artifacts'][k])) for k in ('diagnostic_library', 'diagnostic_provenance')]}})
    gate_count = 0

    def gate():
        nonlocal gate_count
        gate_count += 1
        runner.capture(['/bin/bash', str(resolve('research/tools/env_gate.sh'))],
                       output / ('gate-%03d' % gate_count))

    def launch(role, workload, workdir, trace_path=None):
        protocol = workload.get('protocol', 'trace-txt')
        command_key = 'api_trace_commands' if protocol == 'api-trace' else (
            'api_commands' if protocol == 'api' else 'commands')
        template = cfg[command_key][role]
        result_path = workdir / (role + '-output.jsonl') if protocol.startswith('api') else workdir / (role + '/stdout')
        launch_workload = {**workload, '_trace_path': str(trace_path) if trace_path else ''}
        command = _command_for(template, role, launch_workload, cfg, result_path)
        env = {}
        if role == 'diagnostic' and protocol == 'trace-txt' and workload.get('collect_trace', True):
            env[cfg.get('trace_environment', 'ARCHITECTURE_TRACE_PATH')] = str(trace_path)
            env['TRACE_EXPECTED_ADDRESSES'] = str(_workload_count(workload))
            env['TRACE_EVENT_CAP'] = str(cfg['trace_contract']['maximum_events'])
            env['TRACE_CASE_ID'] = workload['name']
        if role == 'diagnostic' and protocol.startswith('api') and protocol != 'api':
            env['TRACE_EVENT_CAP'] = str(cfg['trace_contract']['maximum_events'])
        gate()
        runner.capture(command, workdir / role, env=env, sample_threads=True)
        command_record = json.loads((workdir / role / 'command.json').read_text())
        require_single_thread(command_record.get('thread_counts_observed', []))
        if not result_path.is_file() or result_path.stat().st_size == 0:
            raise ValueError(role + ' emitted no output JSONL')
        return result_path, command_record

    try:
        gate()
        for workload in mode_rows:
            name = workload['name']
            count = _workload_count(workload)
            if count <= 0:
                raise ValueError('empty workload is not a valid trace run: ' + name)
            folder = output / name
            folder.mkdir()
            oracle, oracle_command = launch('oracle', workload, folder)
            release, release_command = launch('release_control', workload, folder)
            diagnostic_trace = folder / 'diagnostic.trace.jsonl'
            diagnostic, diagnostic_command = launch('diagnostic', workload, folder, diagnostic_trace)
            byte_checks = {'release_control': require_byte_exact(oracle, release),
                           'diagnostic': require_byte_exact(oracle, diagnostic)}
            expected_hashes = _expected_input_hashes(workload)
            frozen_check = None
            negative_frozen_check = None
            if workload.get('frozen_reference'):
                frozen_check = runner.compare_frozen_dump(oracle, resolve(workload['frozen_reference']))
                save(folder / 'frozen-reference-check.json', frozen_check)
                if not frozen_check['identical']:
                    raise ValueError('pinned 000 output differs from frozen golden reference')
                negative = folder / 'negative-frozen-reference-control.jsonl'
                runner.write_negative_component_control(oracle, negative)
                negative_frozen_check = runner.compare_frozen_dump(negative, resolve(workload['frozen_reference']))
                save(folder / 'negative-frozen-reference-check.json', negative_frozen_check)
                if negative_frozen_check['identical']:
                    raise ValueError('frozen-reference negative control was not detected')
            trace_report = None
            trace_enabled = workload.get('protocol') != 'api' and workload.get('collect_trace', True)
            if trace_enabled:
                trace_count = _workload_count(workload, trace=workload.get('protocol') == 'api-trace')
                expected_hashes = _expected_input_hashes(workload)
                trace_report = analyze_trace(diagnostic_trace, expected_addresses=trace_count,
                    expected_input_hashes=expected_hashes,
                    strata_by_address=_strata_for(workload, trace_count))
                save(folder / 'trace-analysis.json', trace_report)
            result['records'].append({'workload': name, 'input_count': count,
                'input_sha256': digest(resolve(workload.get('input') or workload['case_manifest'])),
                'byte_checks': byte_checks,
                'threads': {'oracle': oracle_command['thread_counts_observed'],
                            'release_control': release_command['thread_counts_observed'],
                            'diagnostic': diagnostic_command['thread_counts_observed']},
                'trace_sha256': digest(diagnostic_trace) if diagnostic_trace.is_file() else None,
                'trace_analysis': str(folder / 'trace-analysis.json') if trace_report else None,
                'trace_reason': None if trace_report else ('API correctness workload; no per-address trace requested'
                    if workload.get('protocol') == 'api' else 'trace disabled for output-preservation run'),
                'frozen_reference_check': frozen_check,
                'frozen_reference_negative_control': negative_frozen_check,
                'duration_seconds_provenance_only': {'oracle': oracle_command['duration_seconds'],
                    'release_control': release_command['duration_seconds'],
                    'diagnostic': diagnostic_command['duration_seconds']}})
            save(output / 'results.json', result)
        names = [r['workload'] for r in result['records']]
        expected_names = [w['name'] for w in mode_rows]
        if len(names) != len(mode_rows) or len(set(names)) != len(names) or set(names) != set(expected_names):
            raise ValueError('workload records are missing, duplicated, or unexpected')
        for path, recorded in file_hashes.items():
            if digest(path) != recorded:
                raise ValueError('input/config/artifact changed during run: ' + path)
        result['status'] = 'trace collection complete; diagnostic guidance only'
    except BaseException as exc:
        result['status'] = 'failed or interrupted'
        result['error'] = repr(exc)
        raise
    finally:
        result['end'] = stamp()
        save(output / 'results.json', result)
    return output


args_config_path = None


def main(argv=None):
    global args_config_path
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, default=DEFAULT_CONFIG)
    parser.add_argument('--mode', choices=('smoke', 'development', 'golden-v1-acceptance', 'full', 'newedge'))
    parser.add_argument('--run', action='store_true', help='Run configured serial workloads; requires AC')
    parser.add_argument('--analyze', type=Path, help='Analyze a completed v1 JSONL trace (no workloads launched)')
    parser.add_argument('--expected-addresses', type=int)
    parser.add_argument('--strata', type=Path, help='Optional JSON mapping address_index to provenance strata')
    parser.add_argument('--output', type=Path, help='New result directory; existing paths are rejected')
    args = parser.parse_args(argv)
    try:
        args_config_path = args.config.resolve()
        cfg = json.loads(args.config.read_text())
    except (OSError, json.JSONDecodeError) as exc:
        parser.error('cannot read config: ' + str(exc))
    if args.analyze:
        if args.run:
            parser.error('--analyze and --run are mutually exclusive')
        strata = json.loads(args.strata.read_text()) if args.strata else None
        report = analyze_trace(args.analyze, args.expected_addresses, strata_by_address=strata)
        if args.output:
            out = args.output.resolve()
            out.parent.mkdir(parents=True, exist_ok=True)
            if out.exists():
                parser.error('output already exists: ' + str(out))
            save(out, report)
        print(json.dumps(report, indent=2, sort_keys=True))
        return 0
    if args.run and not args.mode:
        parser.error('--run requires an explicit --mode development|full|newedge')
    mode = args.mode or 'development'
    problems = run_preflight(cfg, mode)
    if not args.run:
        print(json.dumps({'mode': 'preflight only; no process launched', 'workload_mode': mode,
                          'ready': not problems, 'problems': problems,
                          'legacy_185': 'summary totals only; no per-address distributions available'}, indent=2))
        return 0 if not problems else 2
    if problems:
        parser.error('preflight failed:\n' + '\n'.join(problems))
    output = (args.output or EXPERIMENT / 'results' /
              (dt.datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '-' + mode)).resolve()
    if output.exists():
        parser.error('output already exists: ' + str(output))
    print(collect(cfg, mode, output))
    return 0


if __name__ == '__main__':
    sys.exit(main())
