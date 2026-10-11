#!/usr/bin/env python3
"""Collect operation diagnostics from prepared dumpers; never build or time speed."""
import argparse
import datetime as dt
import hashlib
import json
import os
import platform
from pathlib import Path
import re
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
DEFAULT = ROOT / 'research/experiments/184-operation-measurements/config.json'
MAX_COUNT = (1 << 64) - 1


def stamp():
    return dt.datetime.now().astimezone().isoformat()


def save(path, data):
    path.write_text(json.dumps(data, indent=2) + '\n')


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def resolve(path):
    p = Path(path)
    return p if p.is_absolute() else ROOT / p


def rows(path):
    with path.open('rb') as stream:
        return sum(bool(line.rstrip(b'\r\n')) for line in stream)


def equal_files(a, b):
    with a.open('rb') as left, b.open('rb') as right:
        while True:
            x, y = left.read(1024 * 1024), right.read(1024 * 1024)
            if x != y:
                return False
            if not x:
                return True


def compare_frozen_dump(dump_path, reference_path):
    """Compare ordered components using frozen IDs or legacy frozen addresses."""
    reference = {}
    with reference_path.open(encoding='utf-8') as stream:
        for line in stream:
            if not line.strip():
                continue
            item = json.loads(line)
            key = item.get('id')
            if key is None:
                if not isinstance(item.get('address'), str):
                    raise ValueError('Frozen row needs id or address')
                key = hashlib.sha256(item['address'].encode('utf-8')).hexdigest()
            if not isinstance(key, str) or not isinstance(item.get('components'), list):
                raise ValueError('Malformed frozen reference row')
            reference[key] = item['components']
    output = {}
    with dump_path.open(encoding='utf-8') as stream:
        for line in stream:
            if not line.strip():
                continue
            item = json.loads(line)
            if not isinstance(item.get('address'), str) or not isinstance(item.get('components'), list):
                raise ValueError('Malformed dump row for frozen comparison')
            key = hashlib.sha256(item['address'].encode('utf-8')).hexdigest()
            output[key] = item['components']
    missing = sum(key not in output for key in reference)
    mismatched = sum(key in output and output[key] != components
                     for key, components in reference.items())
    result = {'reference_rows': rows(reference_path),
              'dump_rows': rows(dump_path),
              'reference_unique_ids': len(reference), 'dump_unique_addresses': len(output),
              'missing': missing, 'mismatched_ordered_components': mismatched}
    result['identical'] = (missing == 0 and mismatched == 0 and
                           len(output) == len(reference))
    result['comparison'] = 'ordered components keyed by SHA-256(address); byte representation may differ'
    return result


def write_negative_component_control(source_path, target_path):
    """Copy a JSONL dump while changing one component, preserving all other rows."""
    changed = False
    with source_path.open('rb') as source, target_path.open('wb') as target:
        for raw in source:
            if not changed and raw.strip():
                item = json.loads(raw)
                components = item['components']
                if components and isinstance(components[0], list) and len(components[0]) == 2:
                    components[0] = [components[0][0], components[0][1] + '__negative_control__']
                else:
                    components = [['__negative_control__', '__negative_control__']]
                    item['components'] = components
                target.write((json.dumps(item, ensure_ascii=False, separators=(',', ':')) + '\n').encode('utf-8'))
                changed = True
            else:
                target.write(raw)
    if not changed:
        raise ValueError('Cannot create negative component control from empty dump')


def _metric_item(item, name, definition, expected_addresses):
    if not isinstance(item, dict) or 'value' not in item:
        raise ValueError('Malformed metric item: ' + name)
    value = item['value']
    if value is not None and (type(value) is not int or value < 0 or value > MAX_COUNT):
        raise ValueError('Invalid count: ' + name)
    if value is None:
        if not isinstance(item.get('reason'), str) or not item['reason'].strip():
            raise ValueError('Unavailable metric needs a reason: ' + name)
    elif item.get('definition') != definition:
        raise ValueError('Counter semantics differ: ' + name)
    return {**item, 'per_address': value / expected_addresses if value is not None else None}


def read_metrics(path, definitions, expected_addresses, breakdown_dimensions=None,
                 require_repeat_summary=False, breakdown_complete=None):
    data = json.loads(path.read_text())
    if type(data.get('schema_version')) is not int or data['schema_version'] != 1 or data.get('scope') != 'parsing':
        raise ValueError('Metrics must use schema 1 and parsing-only scope')
    if (type(data.get('addresses')) is not int or data['addresses'] != expected_addresses
            or expected_addresses <= 0 or expected_addresses > MAX_COUNT):
        raise ValueError('Counter address denominator differs from input')
    tokens = data.get('tokens')
    if tokens is not None and (type(tokens) is not int or tokens < 0 or tokens > MAX_COUNT):
        raise ValueError('tokens must be a nonnegative integer or null')
    counters = data.get('counters')
    if not isinstance(counters, dict):
        raise ValueError('counters must be an object')
    if set(counters) - set(definitions):
        raise ValueError('Undefined counter names in metrics')
    result = {}
    for name in definitions:
        item = counters.get(name, {'value': None, 'reason': 'not emitted'})
        result[name] = _metric_item(item, name, definitions[name]['definition'], expected_addresses)
        value = item['value']
        result[name]['per_token'] = value / tokens if value is not None and tokens else None
        if value is not None and tokens == 0:
            result[name]['per_token_reason'] = 'token denominator is zero'
        elif value is not None and tokens is None:
            result[name]['per_token_reason'] = 'token denominator unavailable'
    if all(k in result and result[k]['value'] is not None
           for k in ('lookup_calls', 'lookup_hits', 'lookup_misses')):
        if result['lookup_hits']['value'] + result['lookup_misses']['value'] != result['lookup_calls']['value']:
            raise ValueError('Lookup calls do not reconcile to hits + misses')

    breakdowns = data.get('breakdowns', {})
    if not isinstance(breakdowns, dict):
        raise ValueError('breakdowns must be an object')
    if set(breakdowns) - {'by_stage', 'by_family'}:
        raise ValueError('Unknown breakdown axes')
    validated_breakdowns = {}
    expected = breakdown_dimensions or {}
    complete = breakdown_complete or {}
    for axis in ('by_stage', 'by_family'):
        allowed = set(expected.get(axis, []))
        if allowed and axis not in breakdowns:
            raise ValueError('Missing breakdown section: ' + axis)
        values = breakdowns.get(axis, {})
        if not isinstance(values, dict):
            raise ValueError(axis + ' must be an object')
        if set(values) - set(definitions):
            raise ValueError('Undefined counter in ' + axis)
        axis_result = {}
        for metric, dims in values.items():
            if not isinstance(dims, dict):
                raise ValueError('Malformed breakdown: ' + axis + '/' + metric)
            if allowed and set(dims) - allowed:
                raise ValueError('Unknown dimension in ' + axis + '/' + metric)
            dimension_values = {
                dim: _metric_item(item, axis + '/' + metric + '/' + dim,
                                  definitions[metric]['definition'], expected_addresses)
                for dim, item in dims.items()
            }
            missing_dimensions = sorted(allowed - set(dimension_values))
            axis_result[metric] = {'values': dimension_values,
                'unavailable_dimensions': {d: 'not emitted; consult experiment coverage record'
                                           for d in missing_dimensions}}
        if allowed:
            for metric in set(definitions) - set(axis_result):
                axis_result[metric] = {'values': {},
                    'unavailable_dimensions': {d: 'metric breakdown not emitted; consult experiment coverage record'
                                               for d in sorted(allowed)}}
        # A configured complete partition is an explicit contract: all named
        # dimensions must be emitted and sum to the pipeline total.
        required_metrics = complete.get(axis, [])
        if not isinstance(required_metrics, list) or set(required_metrics) - set(definitions):
            raise ValueError('Invalid complete-partition metric list: ' + axis)
        for metric in required_metrics:
            if not allowed or metric not in values or metric not in result:
                raise ValueError('Missing complete partition: ' + axis + '/' + metric)
            cells = values[metric]
            if set(cells) != allowed or result[metric]['value'] is None:
                raise ValueError('Incomplete complete partition: ' + axis + '/' + metric)
            if any(cells[d].get('value') is None for d in allowed):
                raise ValueError('Unavailable cell in complete partition: ' + axis + '/' + metric)
            if sum(cells[d]['value'] for d in allowed) != result[metric]['value']:
                raise ValueError('Breakdown totals do not reconcile: ' + axis + '/' + metric)
        validated_breakdowns[axis] = axis_result

    patterns = data.get('repeated_patterns', [])
    if not isinstance(patterns, list):
        raise ValueError('repeated_patterns must be an array')
    seen_patterns = set()
    validated_patterns = []
    for pattern in patterns:
        if not isinstance(pattern, dict):
            raise ValueError('Malformed repeated pattern')
        pattern_id = pattern.get('pattern_id')
        if not isinstance(pattern_id, str) or not re.fullmatch(r'[0-9a-f]{64}', pattern_id):
            raise ValueError('pattern_id must be a SHA-256 hex digest')
        if pattern_id in seen_patterns:
            raise ValueError('Duplicate repeated pattern id: ' + pattern_id)
        seen_patterns.add(pattern_id)
        for field in ('dictionary_id', 'operation'):
            if not isinstance(pattern.get(field), str) or not pattern[field].strip():
                raise ValueError('Repeated pattern needs ' + field)
        for axis in ('stages', 'families'):
            vals = pattern.get(axis)
            allowed = set(expected.get('by_stage' if axis == 'stages' else 'by_family', []))
            if not isinstance(vals, list) or any(not isinstance(v, str) for v in vals):
                raise ValueError('Repeated pattern ' + axis + ' must be a string array')
            if len(vals) != len(set(vals)) or (allowed and set(vals) - allowed):
                raise ValueError('Invalid repeated pattern ' + axis)
        for field in ('queries', 'executed', 'reused'):
            if type(pattern.get(field)) is not int or pattern[field] < 0 or pattern[field] > MAX_COUNT:
                raise ValueError('Invalid repeated pattern count: ' + field)
        if pattern['executed'] + pattern['reused'] != pattern['queries']:
            raise ValueError('Repeated pattern queries must equal executed + reused')
        if pattern['queries'] < 2:
            raise ValueError('Repeated pattern must have at least two queries')
        address_groups = pattern.get('address_groups')
        if type(address_groups) is not int or address_groups < 1 or address_groups > MAX_COUNT:
            raise ValueError('Invalid repeated pattern address_groups')
        stage_occurrences = pattern.get('stage_occurrences')
        allowed_stages = set(expected.get('by_stage', []))
        if not isinstance(stage_occurrences, dict) or set(stage_occurrences) != set(pattern['stages']):
            raise ValueError('Repeated pattern stage_occurrences must match stages')
        if allowed_stages and set(stage_occurrences) - allowed_stages:
            raise ValueError('Unknown repeated pattern stage occurrence')
        if any(type(v) is not int or v < 1 or v > MAX_COUNT for v in stage_occurrences.values()):
            raise ValueError('Invalid repeated pattern stage occurrence count')
        if not isinstance(pattern.get('definition'), str) or not pattern['definition'].strip():
            raise ValueError('Repeated pattern needs a definition')
        validated_patterns.append(pattern)
    repeat_summary = data.get('repeated_patterns_summary')
    if require_repeat_summary and not isinstance(repeat_summary, dict):
        raise ValueError('Missing repeated_patterns_summary')
    if repeat_summary is not None:
        count_fields = ('retained_patterns', 'omitted_patterns', 'omitted_observations',
                        'query_table_overflow_addresses', 'untracked_lookup_calls',
                        'untracked_query_events')
        for field in count_fields:
            if (type(repeat_summary.get(field)) is not int or repeat_summary[field] < 0
                    or repeat_summary[field] > MAX_COUNT):
                raise ValueError('Invalid repeated pattern summary count: ' + field)
        if type(repeat_summary.get('truncated')) is not bool:
            raise ValueError('repeated pattern summary truncated must be boolean')
        if repeat_summary.get('coverage') != 'first_128_encountered_repeat_groups':
            raise ValueError('Unknown repeated pattern summary coverage')
        if not isinstance(repeat_summary.get('summary_definition'), str) or not repeat_summary['summary_definition'].strip():
            raise ValueError('Repeated pattern summary needs a definition')
        for field in ('distinct_patterns_total', 'observations_total'):
            value = repeat_summary.get(field)
            if value is not None and (type(value) is not int or value < 0 or value > MAX_COUNT):
                raise ValueError('Invalid repeated pattern summary count: ' + field)
            if value is None and repeat_summary['query_table_overflow_addresses'] == 0:
                raise ValueError('Null repeated pattern total requires query table overflow: ' + field)
        if repeat_summary['retained_patterns'] != len(validated_patterns):
            raise ValueError('Repeated pattern summary retained count differs')
        retained_observations = sum(p['queries'] for p in validated_patterns)
        if (repeat_summary['observations_total'] is not None and
                repeat_summary['observations_total'] != retained_observations + repeat_summary['omitted_observations']):
            raise ValueError('Repeated pattern summary observations do not reconcile')
        expected_truncated = (repeat_summary['omitted_patterns'] > 0 or
                              repeat_summary['omitted_observations'] > 0 or
                              repeat_summary['untracked_query_events'] > 0)
        if repeat_summary['truncated'] != expected_truncated:
            raise ValueError('Repeated pattern summary truncation flag differs')
    result['_breakdowns'] = validated_breakdowns
    result['_repeated_patterns'] = validated_patterns
    result['_repeated_patterns_summary'] = repeat_summary
    result['_tokens'] = tokens
    return result


def parse_ps_m(stdout, pid):
    """Count ps -M rows for pid by the PID header column, not a fixed field index."""
    lines = stdout.splitlines()
    if not lines:
        return 0
    header = lines[0].split()
    try:
        pid_index = header.index('PID')
    except ValueError:
        return 0
    count = 0
    for line in lines[1:]:
        fields = line.split()
        if len(fields) > pid_index and fields[pid_index] == str(pid):
            count += 1
    return count


def parse_time_l(path):
    try:
        raw = path.read_text(errors='replace')
    except OSError as exc:
        return {'peak_rss_bytes': None, 'peak_rss_reason': 'resource log unavailable: ' + str(exc),
                'peak_footprint_bytes': None,
                'peak_footprint_reason': 'not reported by /usr/bin/time -l'}
    rss = re.search(r'(\d+)\s+maximum resident set size', raw, re.IGNORECASE)
    footprint = re.search(r'(\d+)\s+peak memory footprint', raw, re.IGNORECASE)
    return {'peak_rss_bytes': int(rss.group(1)) if rss else None,
            'peak_rss_reason': None if rss else 'maximum resident set size not parsed from raw time log',
            'peak_footprint_bytes': int(footprint.group(1)) if footprint else None,
            'peak_footprint_reason': None if footprint else 'peak memory footprint not parsed from raw time log',
            'raw_time_log': str(path)}


def thread_status(command):
    counts = command.get('thread_counts_observed', [])
    if counts:
        return 'sampled one thread' if all(n == 1 for n in counts) else 'sampled thread count: ' + repr(sorted(set(counts)))
    if command.get('thread_sample_count', 0) == 0:
        return 'no sample; process ended before first poll'
    return 'incomplete; no dumper thread rows were observed'


def capture(argv, directory, env=None, sample_threads=False):
    directory.mkdir()
    started = time.monotonic()
    record = {'argv': argv, 'cwd': str(ROOT), 'start': stamp(), 'env_overrides': env or {},
              'timing_use': 'command provenance only; not a performance measurement'}
    save(directory / 'command.json', record)
    samples = []
    try:
        with (directory / 'stdout').open('wb') as out, (directory / 'stderr').open('wb') as err:
            resource_path = directory / 'time-l.txt'
            monitored_argv = ['/usr/bin/time', '-l', '-o', str(resource_path), *argv]
            if not Path('/usr/bin/time').is_file():
                raise RuntimeError('/usr/bin/time unavailable; resource evidence required')
            record['argv'] = argv
            record['monitored_argv'] = monitored_argv
            record['resource_log'] = str(resource_path)
            process = subprocess.Popen(monitored_argv, cwd=ROOT, env={**os.environ, **(env or {})},
                                       stdout=out, stderr=err, start_new_session=True)
            try:
                while process.poll() is None:
                    if sample_threads:
                        group = subprocess.run(['/usr/bin/pgrep', '-g', str(process.pid)],
                                               capture_output=True, text=True)
                        group_pids = [line.strip() for line in group.stdout.splitlines()
                                      if line.strip().isdigit()]
                        dumper_pids = [pid for pid in group_pids if pid != str(process.pid)]
                        target_pid = dumper_pids[0] if len(dumper_pids) == 1 else None
                        sample = (subprocess.run(['/bin/ps', '-M', '-p', target_pid],
                                                 capture_output=True, text=True)
                                  if target_pid else None)
                        text = sample.stdout if sample else ''
                        count = parse_ps_m(text, target_pid) if sample else 0
                        samples.append({'at': stamp(), 'wrapper_pid': process.pid,
                                        'dumper_pid': target_pid,
                                        'process_group_pids': group_pids,
                                        'dumper_pids': dumper_pids,
                                        'process_count': len(dumper_pids),
                                        'pgrep_exit': group.returncode,
                                        'exit': sample.returncode if sample else group.returncode,
                                        'threads': count, 'stdout': text,
                                        'stderr': sample.stderr if sample else group.stderr})
                        if len(dumper_pids) > 1:
                            raise RuntimeError('Multiple parser processes observed: ' + repr(dumper_pids))
                    time.sleep(0.2)
                record['exit'] = process.wait()
            except BaseException:
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    process.wait()
                record['exit'] = process.returncode
                raise
    except BaseException as exc:
        record['error'] = repr(exc)
        record.setdefault('exit', None)
        raise
    finally:
        record['end'] = stamp()
        record['duration_seconds'] = time.monotonic() - started
        record['thread_sample_count'] = len(samples)
        record['thread_counts_observed'] = [s['threads'] for s in samples
                                            if s['exit'] == 0 and s.get('process_count') == 1
                                            and s['threads'] > 0]
        record['resource_summary'] = parse_time_l(directory / 'time-l.txt')
        save(directory / 'command.json', record)
        save(directory / 'threads.json', samples)
    if record['exit']:
        raise RuntimeError('Command failed; retained logs: ' + str(directory))
    counts = [s['threads'] for s in samples if s['exit'] == 0
              and s.get('process_count') == 1 and s['threads']]
    if any(n != 1 for n in counts):
        raise RuntimeError('Single-thread check failed: ' + str(directory))
    return counts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, default=DEFAULT)
    parser.add_argument('--datasets', help='Comma-separated registered dataset names')
    parser.add_argument('--run', action='store_true', help='Execute serial diagnostics; requires AC')
    parser.add_argument('--output', type=Path, help='New result directory (must not exist)')
    args = parser.parse_args()
    cfg = json.loads(args.config.read_text())
    registry = json.loads(resolve(cfg['suite_registry']).read_text())
    requested = args.datasets.split(',') if args.datasets else cfg['datasets']
    if len(set(requested)) != len(requested):
        parser.error('Duplicate dataset name')
    datasets = [d for d in registry['datasets'] if d['name'] in requested]
    if set(requested) != {d['name'] for d in datasets}:
        parser.error('Unknown dataset name')
    if args.run and (len(requested) != 13 or set(requested) != set(cfg['datasets'])):
        parser.error('A diagnostic run must include all 13 configured datasets')
    if len({v['name'] for v in cfg['variants']}) != len(cfg['variants']):
        parser.error('Variant names must be unique')
    if args.run:
        if not cfg['variants']:
            parser.error('Run config must declare at least one variant')
        if not datasets:
            parser.error('Run config must include at least one dataset')
        metric_config = cfg.get('metrics')
        if not isinstance(metric_config, dict) or not metric_config:
            parser.error('Run config must declare metric definitions')
        dimensions = cfg.get('breakdown_dimensions')
        if not isinstance(dimensions, dict) or not dimensions.get('by_stage') or not dimensions.get('by_family'):
            parser.error('Run config must declare nonempty stage and family dimensions')
        for axis in ('by_stage', 'by_family'):
            values = dimensions[axis]
            if not isinstance(values, list) or any(not isinstance(v, str) or not v for v in values) \
                    or len(values) != len(set(values)):
                parser.error('Invalid ' + axis + ' dimensions')
        complete = cfg.get('breakdown_complete', {})
        if not isinstance(complete, dict):
            parser.error('breakdown_complete must be an object')
        for axis in ('by_stage', 'by_family'):
            names = complete.get(axis, [])
            if (not isinstance(names, list) or any(not isinstance(n, str) for n in names)
                    or set(names) - set(metric_config)):
                parser.error('Invalid complete-partition metrics for ' + axis)
        core = {'lookup_calls', 'lookup_hits', 'lookup_misses', 'repeated_queries'} & set(metric_config)
        if not core.issubset(set(complete.get('by_stage', []))):
            parser.error('Core lookup counters need a complete stage partition')
        if complete.get('by_stage') and 'unclassified' not in dimensions['by_stage']:
            parser.error('Complete stage partitions need an unclassified bucket')
    missing = []
    files = [Path(__file__), args.config.resolve(), resolve(cfg['suite_registry']),
             resolve('research/tools/env_gate.sh'), resolve(cfg['baseline_library']),
             resolve(cfg['baseline_dump']), Path('/usr/bin/time'),
             Path('/usr/bin/pgrep'), Path('/bin/ps')]
    for field in ('schema_source', 'config_generator'):
        if cfg.get(field):
            files.append(resolve(cfg[field]))
    build_record_dir = resolve(cfg['build_record_dir']) if cfg.get('build_record_dir') else None
    build_artifacts_dir = resolve(cfg['build_artifacts_dir']) if cfg.get('build_artifacts_dir') else None
    for label, directory in (('build record', build_record_dir), ('build artifacts', build_artifacts_dir)):
        if directory is not None:
            if not directory.is_dir():
                missing.append('Missing ' + label + ' directory: ' + str(directory))
            else:
                files.extend(p for p in directory.rglob('*') if p.is_file())
    for variant in cfg['variants']:
        if not re.fullmatch(r'[a-zA-Z0-9_-]+', variant['name']):
            parser.error('Unsafe variant name')
        for field in ('diagnostic_dump', 'release_dump', 'provenance'):
            if not variant.get(field):
                missing.append(variant['name'] + ': ' + field + ' not prepared')
            else:
                files.append(resolve(variant[field]))
    for dataset in datasets:
        files.append(resolve(dataset['txt']))
        if dataset.get('frozen_reference'):
            files.append(resolve(dataset['frozen_reference']))
    model_dir = resolve(registry['model_dir'])
    if not model_dir.is_dir():
        missing.append('Model directory absent: ' + str(model_dir))
    else:
        files.extend(p for p in model_dir.rglob('*') if p.is_file())
    missing.extend('Missing file: ' + str(p) for p in files if not p.is_file())
    if not args.run:
        print(json.dumps({'mode': 'preparation only; no processes launched', 'missing': missing,
                          'datasets': [d['name'] for d in datasets]}, indent=2))
        return 0
    if missing:
        parser.error('Cannot collect diagnostics:\n' + '\n'.join(missing))
    output = (args.output or ROOT / 'research/experiments/184-operation-measurements/results' /
              dt.datetime.now().strftime('%Y%m%d-%H%M%S-%f')).resolve()
    output.mkdir(parents=True, exist_ok=False)
    save(output / 'config.json', cfg)
    save(output / 'suite.json', registry)
    (output / 'runner.py').write_bytes(Path(__file__).read_bytes())
    results = {'status': 'running', 'start': stamp(), 'kind': 'operation diagnostics',
               'runtime_interpretation': 'Process durations are retained only for provenance. Release vs diagnostic duration is not a speed comparison; instrumentation and metric serialization add overhead.',
               'host': {'platform': platform.platform(), 'python': sys.version},
               'final_benchmark': 'not run', 'expected_record_count': len(datasets) * len(cfg['variants']),
               'records': []}
    save(output / 'results.json', results)
    try:
        # No bypass: check power before hashing/loading models and before each process.
        gate_number = 0
        def gate():
            nonlocal gate_number
            gate_number += 1
            capture(['/bin/bash', str(resolve('research/tools/env_gate.sh'))],
                    output / ('gate-%03d' % gate_number))
        gate()
        hashes = {str(p): digest(p) for p in files}
        model_files = [str(p) for p in model_dir.rglob('*') if p.is_file()]
        input_files = {d['name']: str(resolve(d['txt'])) for d in datasets}
        reference_files = {d['name']: str(resolve(d['frozen_reference']))
                           for d in datasets if d.get('frozen_reference')}
        binary_files = [str(resolve(cfg['baseline_dump']))]
        for variant in cfg['variants']:
            binary_files.extend(str(resolve(variant[k])) for k in ('diagnostic_dump', 'release_dump'))
        roles = {'binaries': binary_files, 'model': model_files,
                 'inputs': input_files, 'frozen_references': reference_files,
                 'build_artifacts': [str(p) for p in build_artifacts_dir.rglob('*') if p.is_file()] if build_artifacts_dir else [],
                 'build_records': [str(p) for p in build_record_dir.rglob('*') if p.is_file()] if build_record_dir else [],
                 'other_harness_and_provenance': [str(p) for p in files
                     if str(p) not in set(binary_files + model_files + list(input_files.values()) + list(reference_files.values()) +
                         ([str(p) for p in build_artifacts_dir.rglob('*') if p.is_file()] if build_artifacts_dir else []) +
                         ([str(p) for p in build_record_dir.rglob('*') if p.is_file()] if build_record_dir else []))]}
        role_hashes = {role: ({key: hashes[value] for key, value in paths.items()}
                               if isinstance(paths, dict) else [hashes[value] for value in paths])
                       for role, paths in roles.items()}
        save(output / 'manifest.json', {'files': hashes, 'by_role': roles,
             'sha256_by_role': role_hashes,
             'created': stamp(), 'hash_algorithm': 'SHA-256', 'model_directory': str(model_dir)})
        expected_addresses = 0
        for dataset in datasets:
            source = resolve(dataset['txt'])
            count = rows(source)
            if not count:
                raise ValueError('Empty corpus: ' + str(source))
            expected_addresses += count
            folder = output / dataset['name']
            folder.mkdir()
            gate()
            original_thread_counts = capture([str(resolve(cfg['baseline_dump'])), str(source)],
                    folder / 'original', sample_threads=True)
            original = folder / 'original/stdout'
            if rows(original) != count:
                raise ValueError('Original output row count differs')
            frozen_check = None
            frozen_negative_check = None
            if dataset.get('frozen_reference'):
                frozen_path = resolve(dataset['frozen_reference'])
                frozen_check = compare_frozen_dump(original, frozen_path)
                save(folder / 'frozen-reference-check.json', frozen_check)
                if not frozen_check['identical']:
                    raise ValueError('Original differs from frozen reference: ' + json.dumps(frozen_check, sort_keys=True))
                frozen_negative = folder / 'negative-frozen-reference-control.jsonl'
                write_negative_component_control(original, frozen_negative)
                frozen_negative_check = compare_frozen_dump(frozen_negative, frozen_path)
                save(folder / 'negative-frozen-reference-check.json', frozen_negative_check)
                if frozen_negative_check['identical']:
                    raise ValueError('Frozen-reference negative control failed')
            negative = folder / 'negative-control'
            negative.write_bytes(b'deliberate mismatch\n')
            if equal_files(original, negative):
                raise ValueError('Negative control failed')
            for variant in cfg['variants']:
                name = variant['name']
                gate()
                release_thread_counts = capture([str(resolve(variant['release_dump'])), str(source)],
                    folder / (name + '-release'), sample_threads=True)
                metric_file = folder / (name + '-metrics.json')
                gate()
                thread_counts = capture([str(resolve(variant['diagnostic_dump'])), str(source)],
                                        folder / (name + '-diagnostic'),
                                        {'MEASUREMENTS_JSON': str(metric_file)}, True)
                for suffix in ('release', 'diagnostic'):
                    if not equal_files(original, folder / (name + '-' + suffix) / 'stdout'):
                        raise ValueError('Output mismatch: ' + name + '-' + suffix)
                metrics = read_metrics(metric_file, cfg['metrics'], count,
                    cfg.get('breakdown_dimensions'), cfg.get('require_repeat_pattern_summary', False),
                    cfg.get('breakdown_complete'))
                if not any(item.get('value') is not None for key, item in metrics.items()
                           if not key.startswith('_')):
                    raise ValueError('No available counters: ' + name)
                release_command = json.loads((folder / (name + '-release/command.json')).read_text())
                diagnostic_command = json.loads((folder / (name + '-diagnostic/command.json')).read_text())
                original_command = json.loads((folder / 'original/command.json').read_text())
                results['records'].append({'dataset': dataset['name'], 'variant': name,
                    'addresses': count, 'output': 'byte-identical',
                    'thread_evidence': {'original': thread_status(original_command),
                        'release': thread_status(release_command),
                        'diagnostic': thread_status(diagnostic_command)},
                    'original_thread_counts': original_thread_counts,
                    'release_thread_counts': release_thread_counts,
                    'thread_sample_count': diagnostic_command['thread_sample_count'],
                    'thread_counts_observed': diagnostic_command['thread_counts_observed'],
                    'input_sha256': hashes[str(source)],
                    'baseline_output_sha256': digest(original),
                    'release_output_sha256': digest(folder / (name + '-release/stdout')),
                    'diagnostic_output_sha256': digest(folder / (name + '-diagnostic/stdout')),
                    'frozen_reference_check': frozen_check,
                    'frozen_reference_negative_control': frozen_negative_check,
                    'release_process_seconds_provenance_only': release_command['duration_seconds'],
                    'diagnostic_process_seconds_provenance_only': diagnostic_command['duration_seconds'],
                    'resource_evidence': {'original': original_command['resource_summary'],
                        'release': release_command['resource_summary'],
                        'diagnostic': diagnostic_command['resource_summary']},
                    'runtime_warning': 'Diagnostic process duration includes instrumentation overhead and is not a performance result.',
                    'metrics': metrics})
                save(output / 'results.json', results)
        expected_pairs = {(d['name'], v['name']) for d in datasets for v in cfg['variants']}
        observed_pairs = [(r.get('dataset'), r.get('variant')) for r in results['records']]
        if (len(observed_pairs) != len(expected_pairs) or len(set(observed_pairs)) != len(observed_pairs)
                or set(observed_pairs) != expected_pairs):
            raise ValueError('Incomplete diagnostic run: expected %d dataset/variant records, got %d' %
                             (len(expected_pairs), len(observed_pairs)))
        if any(r.get('output') != 'byte-identical' for r in results['records']):
            raise ValueError('Incomplete diagnostic run: a candidate output was not verified byte-identical')
        for variant in cfg['variants']:
            variant_addresses = sum(r['addresses'] for r in results['records'] if r['variant'] == variant['name'])
            if variant_addresses != expected_addresses:
                raise ValueError('Incomplete diagnostic run: %s covers %d of %d expected addresses' %
                                 (variant['name'], variant_addresses, expected_addresses))
        changed = [str(p) for p in files if digest(p) != hashes[str(p)]]
        if changed:
            raise ValueError('Files changed during collection: ' + ', '.join(changed))
        results['status'] = 'diagnostics collected; final acceptance not evaluated'
    except BaseException as exc:
        results['status'] = 'failed or interrupted'
        results['error'] = repr(exc)
        raise
    finally:
        results['end'] = stamp()
        save(output / 'results.json', results)
    print(output)
    return 0


if __name__ == '__main__':
    sys.exit(main())
