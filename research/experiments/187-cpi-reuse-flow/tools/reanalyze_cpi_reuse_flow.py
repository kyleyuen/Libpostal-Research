#!/usr/bin/env python3
"""Rebuild reviewed 187 analysis from retained traces without invoking parsers."""
import argparse
import importlib.util
import json
import math
from pathlib import Path
import statistics

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def percentile(values, q):
    if not values:
        return None
    ordered = sorted(values)
    return ordered[max(0, math.ceil(q * len(ordered)) - 1)]


def summarize_addresses(paths):
    names = ('max_first_to_last_consumer_event_span',
             'max_simultaneously_live_repeated_answers',
             'max_live_key_bytes_lower_bound',
             'max_live_numeric_payload_bytes_lower_bound')
    values = {name: [] for name in names}
    count = 0
    for path in paths:
        with path.open(encoding='utf-8') as stream:
            for line in stream:
                row = json.loads(line)
                count += 1
                for name in names:
                    value = row.get(name)
                    # No eligible repeated interval means a zero lower bound.
                    values[name].append(0 if value is None else value)
    return {'addresses': count, 'statistics': {
        name: {'p50': statistics.median(rows), 'p95_nearest_rank': percentile(rows, .95),
               'max': max(rows) if rows else None}
        for name, rows in values.items()}}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True,
                    help='new output directory; must not already exist')
    args = ap.parse_args()
    cfg_path = ROOT / 'research/experiments/187-cpi-reuse-flow/config.json'
    cfg = json.loads(cfg_path.read_text(encoding='utf-8'))
    run = load_module('run_cpi_reuse_flow_for_reanalysis', HERE / 'run_cpi_reuse_flow.py')
    analyzer = load_module('analyze_cpi_reuse_flow_reviewed', HERE / 'analyze_cpi_reuse_flow.py')
    output = args.output if args.output.is_absolute() else ROOT / args.output
    output.mkdir(parents=True, exist_ok=False)
    records = {}
    for result_name in ('golden-preservation-build-187-final4', 'remaining12-build-187-final4'):
        result_path = ROOT / 'research/experiments/187-cpi-reuse-flow/results' / result_name / 'results.json'
        result = json.loads(result_path.read_text(encoding='utf-8'))
        if not result.get('status', '').startswith('complete'):
            raise ValueError('collection result is incomplete: ' + str(result_path))
        records.update({row['workload']: row for row in result['records']})
    workloads = cfg['workload_modes']['all13']
    if set(records) != {row['name'] for row in workloads}:
        raise ValueError('collected workload registry differs from configured all13 mode')
    corpus_summaries = []
    address_paths = []
    for workload in workloads:
        name = workload['name']
        record = records[name]
        hashes = run.input_hashes(workload)
        expected_count = len(hashes)
        if expected_count != record['input_count']:
            raise ValueError('input count changed since collection: ' + name)
        trace_archive = record.get('trace_archive')
        trace = (Path(trace_archive['archive_path']) if trace_archive else
                 ROOT / 'research/experiments/187-cpi-reuse-flow/results/golden-preservation-build-187-final4/golden-v1/diagnostic.trace.jsonl')
        corpus_dir = output / name
        corpus_dir.mkdir(exist_ok=False)
        address_path = corpus_dir / 'address-metrics.jsonl'
        analysis = analyzer.analyze(trace, address_path,
            expected_addresses=expected_count, expected_input_hashes=hashes,
            expected_stability_proof=cfg['within_parse_stability_proof'])
        analysis_path = corpus_dir / 'analysis.json'
        with analysis_path.open('x', encoding='utf-8') as stream:
            json.dump(analysis, stream, indent=2, sort_keys=True)
            stream.write('\n')
        address_paths.append(address_path)
        corpus_summaries.append({'workload': name, 'input_count': expected_count,
            'source': workload.get('source', 'unknown'), 'language': workload.get('language', 'unknown'),
            'country': workload.get('country', 'unknown'), 'split': workload.get('split', 'observed'),
            'trace_path': str(trace), 'analysis_path': str(analysis_path),
            'address_metrics_path': str(address_path), 'analysis': analysis})
    combined = {
        'schema_version': 1, 'analysis_source': 'reviewed reanalysis of retained final4 traces; no parser execution',
        'workloads': corpus_summaries,
        'address_liveness': summarize_addresses(address_paths),
    }
    with (output / 'overall-analysis.json').open('x', encoding='utf-8') as stream:
        json.dump(combined, stream, indent=2, sort_keys=True)
        stream.write('\n')
    print(json.dumps({'output': str(output), 'workloads': len(corpus_summaries),
                      'addresses': combined['address_liveness']['addresses'],
                      'overall_analysis': str(output / 'overall-analysis.json')}, indent=2))


if __name__ == '__main__':
    main()
