#!/usr/bin/env python3
"""Regression tests for Wave 5 benchmark CTest coverage."""

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT / "tools" / "ci"))

import audit_benchmark_coverage


WAVE5_TARGETS = {
    "bench_w5a_production_workloads": "wave5/bench_w5a_production_workloads.cpp",
    "bench_w5b_degradation": "wave5/bench_w5b_degradation.cpp",
    "bench_w5c_determinism": "wave5/bench_w5c_determinism.cpp",
    "bench_w5d_governance": "wave5/bench_w5d_governance.cpp",
}


def test_wave5_benchmarks_are_registered_in_ctest():
    targets = audit_benchmark_coverage.collect_cmake_targets()

    for target_name, source_file in WAVE5_TARGETS.items():
        target = targets[target_name]
        assert target.source_file == source_file
        assert target.ctest_entries == [target_name]
