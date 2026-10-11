#!/usr/bin/env python3
"""Generate the 185 runner config from the emitted runtime schema and one build ID."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
EXP = ROOT / 'research/experiments/185-diagnostic-instrumentation'
BYTE_METRICS = {'feature_key_bytes_materialized', 'input_bytes', 'normalized_bytes',
                'output_bytes', 'allocation_requested_bytes', 'diagnostic_tracker_peak_bytes'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-id', required=True)
    parser.add_argument('--output', type=Path, default=EXP / 'config.json')
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        parser.error('Refusing to overwrite existing config: ' + str(output))

    schema = json.loads((EXP / 'diag-schema.json').read_text())
    registry_path = ROOT / 'research/suite/suite.json'
    registry = json.loads(registry_path.read_text())
    build_id = args.build_id
    relative_build = Path('research/experiments/185-diagnostic-instrumentation')
    artifact_root = relative_build / 'artifacts' / ('build-' + build_id)
    record_root = relative_build / 'raw' / ('build-' + build_id)
    variants = []
    for name in ('original', 'cpi', 'prototype181'):
        variants.append({
            'name': name,
            'diagnostic_dump': str(artifact_root / name / 'diagnostic/dump_outputs'),
            'release_dump': str(artifact_root / name / 'release/dump_outputs'),
            'diagnostic_library': str(artifact_root / name / 'diagnostic/libpostal.a'),
            'release_library': str(artifact_root / name / 'release/libpostal.a'),
            'provenance': str(record_root / 'provenance' / (name + '.txt')),
        })
    metrics = {}
    for metric in schema['metrics']:
        name = metric['name']
        metrics[name] = {'definition': metric['definition'],
                         'unit': 'bytes' if name in BYTE_METRICS else 'count'}
    config = {
        'suite_registry': 'research/suite/suite.json',
        'baseline_library': 'research/experiments/000-baseline/artifacts/libpostal.a',
        'baseline_dump': 'research/experiments/182-benchmark-suite-prep/artifacts/original-000/dump_outputs',
        'datasets': [d['name'] for d in registry['datasets']],
        'variants': variants,
        'metrics': metrics,
        'breakdown_dimensions': {'by_stage': schema['stages'], 'by_family': schema['families']},
        'breakdown_complete': {'by_stage': ['lookup_calls', 'lookup_hits', 'lookup_misses', 'repeated_queries'],
                               'by_family': []},
        'require_repeat_pattern_summary': True,
        'build_id': build_id,
        'build_record_dir': str(record_root),
        'build_artifacts_dir': str(artifact_root),
        'schema_source': 'research/experiments/185-diagnostic-instrumentation/diag-schema.json',
        'config_generator': 'research/experiments/185-diagnostic-instrumentation/tools/generate_config.py',
        'repeated_pattern_aggregation_note': 'Pattern rows aggregate semantic family, operation, and exact key bytes across addresses; per-address repeated-query identity remains dictionary-pointer exact. Distinct same-family dictionary instances can therefore share an exported row.',
        'runtime_interpretation': 'Diagnostic and release process durations are provenance and diagnostic-overhead evidence only; they are not product-speed measurements.',
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(config, indent=2) + '\n')
    print(output)


if __name__ == '__main__':
    main()
