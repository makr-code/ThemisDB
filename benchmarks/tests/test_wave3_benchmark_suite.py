#!/usr/bin/env python3
"""Tests for benchmarks/wave3_benchmark_suite.py."""

from __future__ import annotations

import json
import sys
from pathlib import Path


_BENCHMARKS = Path(__file__).resolve().parent.parent
if str(_BENCHMARKS) not in sys.path:
    sys.path.insert(0, str(_BENCHMARKS))

from wave3_benchmark_suite import (  # noqa: E402
    _extract_metrics,
    _load_profiles,
    compare_runs,
    main,
)


def test_extract_metrics_with_time_unit_conversion():
    payload = {
        "benchmarks": [
            {"real_time": 1000.0, "time_unit": "us", "items_per_second": 10000.0},
            {"real_time": 2.0, "time_unit": "ms", "items_per_second": 8000.0},
            {"real_time": 3_000_000.0, "time_unit": "ns", "items_per_second": 12000.0},
        ]
    }
    m = _extract_metrics(payload)
    assert m["throughput_ops_per_sec"] == 10000.0
    assert m["p50_latency_ms"] > 0.0
    assert m["p95_latency_ms"] >= m["p50_latency_ms"]
    assert m["p99_latency_ms"] >= m["p95_latency_ms"]


def test_load_profiles_from_json_file():
    profiles = _load_profiles(_BENCHMARKS / "wave3_workload_profiles.json")
    assert len(profiles) == 3
    assert {p.profile for p in profiles} == {"read-heavy", "write-heavy", "mixed"}
    assert all(len(p.dimensions) == 3 for p in profiles)


def test_compare_runs_reports_blocking_breach():
    baseline = {
        "results": [
            {
                "workload_key": "critical_read_path:small:p1",
                "profile": "read-heavy",
                "metrics": {
                    "throughput_ops_per_sec": 1000.0,
                    "p95_latency_ms": 10.0,
                    "p99_latency_ms": 20.0,
                },
                "guardrails": {
                    "throughput_drop_percent": 8.0,
                    "p95_latency_increase_percent": 10.0,
                    "p99_latency_increase_percent": 15.0,
                },
            }
        ]
    }
    current = {
        "results": [
            {
                "workload_key": "critical_read_path:small:p1",
                "profile": "read-heavy",
                "metrics": {
                    "throughput_ops_per_sec": 850.0,
                    "p95_latency_ms": 12.0,
                    "p99_latency_ms": 25.0,
                },
                "guardrails": {
                    "throughput_drop_percent": 8.0,
                    "p95_latency_increase_percent": 10.0,
                    "p99_latency_increase_percent": 15.0,
                },
            }
        ]
    }

    report = compare_runs(baseline, current)
    assert report["summary"]["status"] == "failed"
    assert report["summary"]["blocking"] == 1
    assert report["comparisons"][0]["blocking"] is True


def test_compare_runs_pass_when_within_guardrails():
    baseline = {
        "results": [
            {
                "workload_key": "critical_mixed_path:medium:p4",
                "profile": "mixed",
                "metrics": {
                    "throughput_ops_per_sec": 1000.0,
                    "p95_latency_ms": 10.0,
                    "p99_latency_ms": 15.0,
                },
                "guardrails": {
                    "throughput_drop_percent": 15.0,
                    "p95_latency_increase_percent": 20.0,
                    "p99_latency_increase_percent": 20.0,
                },
            }
        ]
    }
    current = {
        "results": [
            {
                "workload_key": "critical_mixed_path:medium:p4",
                "profile": "mixed",
                "metrics": {
                    "throughput_ops_per_sec": 920.0,
                    "p95_latency_ms": 11.0,
                    "p99_latency_ms": 16.0,
                },
                "guardrails": {
                    "throughput_drop_percent": 15.0,
                    "p95_latency_increase_percent": 20.0,
                    "p99_latency_increase_percent": 20.0,
                },
            }
        ]
    }

    report = compare_runs(baseline, current)
    assert report["summary"]["status"] == "passed"
    assert report["summary"]["blocking"] == 0


def test_cli_run_dry_mode_writes_report(tmp_path):
    out = tmp_path / "wave3_run.json"
    rc = main(
        [
            "run",
            "--benchmark-bin-dir",
            str(tmp_path),
            "--output",
            str(out),
            "--profiles-file",
            str(_BENCHMARKS / "wave3_workload_profiles.json"),
            "--dry-run",
        ]
    )
    assert rc == 0
    payload = json.loads(out.read_text(encoding="utf-8"))
    assert payload["suite"] == "wave3_full_function"
    assert payload["summary"]["workloads_total"] == 9
