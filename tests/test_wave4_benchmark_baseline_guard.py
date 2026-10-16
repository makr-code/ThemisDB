import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'benchmarks' / 'wave4'))

import report_variance


def test_missing_baseline_entry_fails_gate(tmp_path):
    current = {
        'benchmarks': [
            {
                'name': 'StorageBenchFixture/W4A_01_WriteThroughput/500',
                'run_type': 'iteration',
                'items_per_second': 1_000_000.0,
            }
        ]
    }
    current_path = tmp_path / 'current.json'
    current_path.write_text(json.dumps(current), encoding='utf-8')

    baseline = {
        'benchmarks': [
            {
                'name': 'VectorBenchFixture/W4A_03_VectorInsertThroughput/200',
                'run_type': 'iteration',
                'items_per_second': 600_000.0,
            }
        ]
    }
    baseline_path = tmp_path / 'baseline.json'
    baseline_path.write_text(json.dumps(baseline), encoding='utf-8')

    current_lookup = report_variance.build_lookup(report_variance.load_benchmark_json(current_path))
    baseline_lookup = report_variance.build_lookup(report_variance.load_benchmark_json(baseline_path))

    gate = {
        'id': 'W4A-01',
        'name': 'Storage Write Throughput',
        'benchmark': 'bench_w4a_release_gates',
        'benchmark_filter': 'StorageBenchFixture/W4A_01_WriteThroughput/500',
        'metric': 'items_per_second',
        'direction': 'higher_is_better',
        'regression_threshold_pct': 10,
        'criticality': 'release-blocking',
    }

    result = report_variance.evaluate_gate(gate, current_lookup, baseline_lookup)

    assert result.passed is False
    assert 'missing or stale baseline' in result.message.lower()
