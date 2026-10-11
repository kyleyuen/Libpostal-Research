#!/usr/bin/env python3
"""Gated, serial CPI trace collection using 185 capture and 186 checks."""
import argparse
import datetime as dt
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import shutil
import sys

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = HERE / 'config.json'
RUNNER_186 = ROOT / 'research/experiments/186-architecture-benchmark/tools/run_architecture_benchmark.py'
MAX_COUNT = (1 << 64) - 1


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load ' + str(path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def stamp():
    return dt.datetime.now().astimezone().isoformat()


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def archive_trace_losslessly(trace_path):
    """Create deterministic gzip, verify exact expansion, then remove this new raw trace."""
    trace_path = Path(trace_path)
    archive_path = trace_path.with_suffix(trace_path.suffix + '.gz')
    raw_sha256 = digest(trace_path)
    raw_bytes = trace_path.stat().st_size
    with trace_path.open('rb') as source, archive_path.open('xb') as target:
        with gzip.GzipFile(filename='', mode='wb', fileobj=target, mtime=0) as compressed:
            shutil.copyfileobj(source, compressed, length=1024 * 1024)
    archive_sha256 = digest(archive_path)
    expanded = hashlib.sha256()
    expanded_bytes = 0
    with gzip.open(archive_path, 'rb') as source:
        for block in iter(lambda: source.read(1024 * 1024), b''):
            expanded.update(block)
            expanded_bytes += len(block)
    if expanded_bytes != raw_bytes or expanded.hexdigest() != raw_sha256:
        raise ValueError('compressed trace failed byte-for-byte decompression verification')
    trace_path.unlink()
    return {'raw_sha256': raw_sha256, 'raw_bytes': raw_bytes,
            'archive_path': str(archive_path), 'archive_sha256': archive_sha256,
            'archive_bytes': archive_path.stat().st_size,
            'compression': 'gzip; mtime=0; verified exact decompression'}


def save(path, value):
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def resolve(path):
    value = Path(path)
    return value if value.is_absolute() else ROOT / value


def input_hashes(workload):
    if workload.get('input'):
        result = {}
        with resolve(workload['input']).open('rb') as stream:
            for line in stream:
                exact = line.rstrip(b'\r\n')
                if exact:
                    result[str(len(result) + 1)] = hashlib.sha256(exact).hexdigest()
        return result
    selector = load_module('edge_api_dump_186', ROOT / 'research/experiments/186-architecture-benchmark/tools/edge_api_dump.py')
    cases = json.loads(resolve(workload['case_manifest']).read_text(encoding='utf-8'))['cases']
    selected = selector.select_safe_cases(cases)
    ids = workload.get('case_ids')
    if ids:
        by_id = {case['id']: (case, trace_safe) for case, trace_safe in selected}
        if any(case_id not in by_id for case_id in ids):
            raise ValueError('unknown/unsafe smoke case id')
        selected = [by_id[case_id] for case_id in ids]
    selected = [pair for pair in selected if pair[1]]
    return {str(i): hashlib.sha256(case['input_utf8'].encode('utf-8').split(b'\0', 1)[0]).hexdigest()
            for i, (case, _trace_safe) in enumerate(selected, 1)}


def _api_command(role, workload, cfg, result_path, trace_path=None):
    command = [sys.executable, str(resolve(cfg['api_harness']))]
    library_key = {'oracle': 'baseline_library', 'release_control': 'release_library',
                   'diagnostic': 'diagnostic_library'}[role]
    command += ['--library', str(resolve(cfg['pinned_oracle'][library_key])),
                '--datadir', str(resolve(cfg['pinned_oracle']['model_dir']))]
    if workload.get('input'):
        command += ['--input-txt', str(resolve(workload['input']))]
    else:
        command += ['--cases', str(resolve(workload['case_manifest']))]
        if workload.get('case_ids'):
            flag = '--trace-case-ids' if role == 'diagnostic' else '--case-ids'
            command += [flag, ','.join(workload['case_ids'])]
    command += ['--output-jsonl', str(result_path)]
    if role == 'diagnostic':
        command += ['--trace', '--trace-jsonl', str(trace_path), '--trace-case-id', workload['name']]
    return command


def _count(workload):
    return len(input_hashes(workload))


def preflight(cfg, mode):
    problems = []
    if mode not in cfg.get('workload_modes', {}):
        problems.append('workload mode is not configured: ' + str(mode))
    for key in ('baseline_library', 'release_library', 'diagnostic_library'):
        value = cfg.get('pinned_oracle', {}).get(key)
        if not value or not resolve(value).is_file():
            problems.append('missing ' + key + ': ' + str(value))
    for key in ('source_tree', 'diagnostic_source_tree', 'source_provenance', 'diagnostic_provenance',
                'source_manifest', 'flags_release', 'flags_diagnostic'):
        value = cfg.get('artifacts', {}).get(key)
        if not value or not resolve(value).exists():
            problems.append('missing artifact ' + key + ': ' + str(value))
    source_tree = resolve(cfg.get('artifacts', {}).get('source_tree', '.'))
    required_sources = {'libpostal.c', 'address_parser.c', 'crf.c', 'feature_dict.h', 'trie.c', 'trie_search.c'}
    if source_tree.is_dir():
        present = {path.name for path in source_tree.rglob('*') if path.is_file()}
        missing = sorted(required_sources - present)
        if missing:
            problems.append('source proof bundle misses: ' + ', '.join(missing))
    proof = cfg.get('within_parse_stability_proof')
    if not isinstance(proof, dict) or proof.get('status') != 'address_scoped_readonly':
        problems.append('within-parse stability proof is not declared against a source/flags manifest')
    elif (not proof.get('source_sha256') or not proof.get('source_manifest_path')
          or digest(resolve(proof['source_manifest_path'])) != proof['source_sha256']):
        problems.append('source-stability manifest digest does not match config')
    if not cfg.get('api_harness') or not resolve(cfg['api_harness']).is_file():
        problems.append('missing API harness')
    model = cfg.get('pinned_oracle', {}).get('model_dir')
    if not model or not resolve(model).is_dir():
        problems.append('missing model directory')
    if mode in cfg.get('workload_modes', {}):
        names = []
        for workload in cfg['workload_modes'][mode]:
            name = workload.get('name')
            if not name or name in names or '/' in name or name in ('.', '..'):
                problems.append('invalid or duplicate workload name: ' + str(name))
            names.append(name)
            for field in ('input', 'case_manifest', 'frozen_reference'):
                if workload.get(field) and not resolve(workload[field]).is_file():
                    problems.append(f'missing {field} for {name}: {workload[field]}')
            try:
                if _count(workload) <= 0:
                    problems.append('empty workload: ' + str(name))
            except (OSError, ValueError, KeyError) as exc:
                problems.append(f'invalid workload {name}: {exc}')
    return problems


def collect(cfg, mode, output):
    from_runner = load_module('architecture_runner_186', RUNNER_186)
    capture = from_runner.load_measurement_runner().capture
    output.mkdir(parents=True, exist_ok=False)
    (output / 'runner.py').write_bytes(Path(__file__).read_bytes())
    (output / 'config.json').write_bytes(config_path.read_bytes())
    result = {'schema_version': 1, 'experiment': '187-cpi-reuse-flow', 'mode': mode,
              'status': 'running', 'start': stamp(), 'end': None,
              'host': {'platform': platform.platform(), 'python': sys.version},
              'acceptance': {'final_speed_memory_benchmark': 'not run'},
              'records': []}
    save(output / 'results.json', result)
    inputs = [resolve(row['input']) if row.get('input') else resolve(row['case_manifest'])
              for row in cfg['workload_modes'][mode]]
    sources = []
    for key in ('source_tree', 'diagnostic_source_tree'):
        root = resolve(cfg['artifacts'][key])
        sources.extend(file for file in root.rglob('*') if file.is_file())
    files = [Path(__file__), config_path, RUNNER_186,
             ROOT / 'research/tools/run_measurements.py', ROOT / 'research/tools/env_gate.sh',
             resolve(cfg['api_harness']), resolve(cfg['artifacts']['source_provenance']),
             resolve(cfg['artifacts']['diagnostic_provenance']),
             resolve(cfg['artifacts']['source_manifest']), resolve(cfg['artifacts']['flags_release']),
             resolve(cfg['artifacts']['flags_diagnostic']),
             resolve(cfg['within_parse_stability_proof']['source_manifest_path']),
             resolve(cfg['within_parse_stability_proof']['flags_release_path']),
             resolve(cfg['within_parse_stability_proof']['flags_diagnostic_path']),
             resolve(cfg['pinned_oracle']['baseline_library']),
             resolve(cfg['pinned_oracle']['release_library']),
             resolve(cfg['pinned_oracle']['diagnostic_library']),
             *inputs, *sources]
    model_dir = resolve(cfg['pinned_oracle']['model_dir'])
    files.extend(path for path in model_dir.rglob('*') if path.is_file())
    unique_files = list(dict.fromkeys(files))
    hashes = {str(path): digest(path) for path in unique_files if path.is_file()}
    if len(hashes) != len(unique_files):
        raise FileNotFoundError('an identity file is missing')
    save(output / 'manifest.json', {'created': stamp(), 'hash_algorithm': 'SHA-256', 'files': hashes})
    gate_number = 0

    def gate():
        nonlocal gate_number
        gate_number += 1
        capture(['/bin/bash', str(ROOT / 'research/tools/env_gate.sh')],
                output / f'gate-{gate_number:03d}')

    try:
        for workload in cfg['workload_modes'][mode]:
            count = _count(workload)
            folder = output / workload['name']
            folder.mkdir()
            produced = {}
            for role in ('oracle', 'release_control', 'diagnostic'):
                gate()
                output_file = folder / f'{role}-output.jsonl'
                trace_file = folder / 'diagnostic.trace.jsonl' if role == 'diagnostic' else None
                cmd = _api_command(role, workload, cfg, output_file, trace_file)
                env = ({'TRACE_EVENT_CAP': str(cfg['trace_contract']['maximum_events']),
                        'TRACE_SOURCE_MANIFEST_SHA256': cfg['within_parse_stability_proof']['source_sha256']}
                       if role == 'diagnostic' else None)
                capture(cmd, folder / role, env=env, sample_threads=True)
                process_record = json.loads((folder / role / 'command.json').read_text())
                from_runner.require_single_thread(process_record.get('thread_counts_observed', []))
                if not output_file.is_file() or output_file.stat().st_size == 0:
                    raise ValueError(role + ' emitted no serialized output')
                produced[role] = output_file
            checks = {'release_control': from_runner.require_byte_exact(produced['oracle'], produced['release_control']),
                      'diagnostic': from_runner.require_byte_exact(produced['oracle'], produced['diagnostic'])}
            frozen_check = negative_check = None
            if workload.get('frozen_reference'):
                standard_dump = folder / 'oracle-frozen-format.jsonl'
                with produced['oracle'].open(encoding='utf-8') as source, standard_dump.open('x', encoding='utf-8') as target:
                    for line in source:
                        row = json.loads(line)
                        target.write(json.dumps({'address': row['input_utf8'], 'components': row['ordered_pairs'] or []},
                                                ensure_ascii=False, separators=(',', ':')) + '\n')
                measurement = from_runner.load_measurement_runner()
                frozen_check = measurement.compare_frozen_dump(standard_dump, resolve(workload['frozen_reference']))
                if not frozen_check['identical']:
                    raise ValueError('oracle differs from frozen golden-v1 components')
                neg_path = folder / 'oracle-negative-control.jsonl'
                measurement.write_negative_component_control(standard_dump, neg_path)
                negative_check = measurement.compare_frozen_dump(neg_path, resolve(workload['frozen_reference']))
                if negative_check['identical']:
                    raise ValueError('golden-v1 negative control was not detected')
                save(folder / 'frozen-reference-check.json', frozen_check)
                save(folder / 'negative-frozen-reference-check.json', negative_check)
            expected_hashes = input_hashes(workload)
            address_rows = folder / 'address-metrics.jsonl'
            analyzer_module = load_module('analyze_cpi_reuse_flow_187', HERE / 'tools/analyze_cpi_reuse_flow.py')
            analysis = analyzer_module.analyze(folder / 'diagnostic.trace.jsonl', address_rows,
                                               expected_addresses=count, expected_input_hashes=expected_hashes,
                                               expected_stability_proof=cfg.get('within_parse_stability_proof'))
            save(folder / 'analysis.json', analysis)
            raw_trace_sha256 = digest(folder / 'diagnostic.trace.jsonl')
            raw_trace_bytes = (folder / 'diagnostic.trace.jsonl').stat().st_size
            trace_archive = None
            if mode == 'remaining12':
                trace_archive = archive_trace_losslessly(folder / 'diagnostic.trace.jsonl')
            result['records'].append({
                'workload': workload['name'], 'input_count': count,
                'input_sha256': digest(inputs[cfg['workload_modes'][mode].index(workload)]),
                'byte_checks': checks, 'trace_sha256': raw_trace_sha256,
                'trace_bytes': raw_trace_bytes, 'trace_archive': trace_archive,
                'threads': {role: json.loads((folder / role / 'command.json').read_text())['thread_counts_observed']
                            for role in ('oracle', 'release_control', 'diagnostic')},
                'frozen_reference_check': frozen_check,
                'frozen_reference_negative_control': negative_check,
                'analysis': str(folder / 'analysis.json')})
            save(output / 'results.json', result)
            # Recheck immutable inputs/source/artifact hashes after each workload.
            for path, expected in hashes.items():
                if digest(path) != expected:
                    raise ValueError('source/input/artifact changed during collection: ' + path)
        result['status'] = 'complete; analysis is diagnostic guidance only'
    except BaseException as exc:
        result['status'] = 'failed or interrupted'
        result['error'] = repr(exc)
        raise
    finally:
        result['end'] = stamp()
        save(output / 'results.json', result)
    return output


config_path = DEFAULT_CONFIG


def main(argv=None):
    global config_path
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--config', type=Path, default=DEFAULT_CONFIG)
    ap.add_argument('--mode', choices=('smoke', 'development', 'golden-preservation', 'remaining12', 'all13'))
    ap.add_argument('--run', action='store_true', help='run serial captures; requires explicit mode')
    ap.add_argument('--output', type=Path)
    args = ap.parse_args(argv)
    config_path = args.config.resolve()
    try:
        cfg = json.loads(config_path.read_text())
    except (OSError, json.JSONDecodeError) as exc:
        ap.error('cannot read config: ' + str(exc))
    if args.run and (not args.mode or not args.output):
        ap.error('--run requires --mode and a new --output directory')
    mode = args.mode or 'development'
    problems = preflight(cfg, mode)
    if not args.run:
        print(json.dumps({'mode': 'preflight only; no process launched', 'workload_mode': mode,
                          'ready': not problems, 'problems': problems}, indent=2))
        return 0 if not problems else 2
    if problems:
        ap.error('preflight failed: ' + '; '.join(problems))
    destination = args.output.resolve()
    collect(cfg, mode, destination)
    print(str(destination))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
