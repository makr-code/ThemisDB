/*
 * ThemisDB | File: test_scientific_evaluation_framework_unittest.py | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#!/usr/bin/env python3
"""Unittests for benchmarks/scripts/scientific_evaluation_framework.py."""

import unittest
import random
from pathlib import Path
import sys

_SCRIPTS = Path(__file__).resolve().parent.parent / "scripts"
if str(_SCRIPTS) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS))

import scientific_evaluation_framework as sef  # noqa: E402


def _seeded_samples(mean, stddev, n, seed):
    rng = random.Random(seed)
    return [mean + rng.gauss(0.0, stddev) for _ in range(n)]


def _base_payload():
    return {
        "seed": 42,
        "confidence_level": 0.95,
        "alpha": 0.05,
        "bootstrap_iterations": 300,
        "permutation_iterations": 300,
        "baseline_freeze": {
            "compiler": "g++-14",
            "compiler_flags": "-O3 -DNDEBUG",
            "preset": "linux-ninja-perf",
            "hardware_profile": "ci-x64-32c-128g",
            "os_image": "ubuntu-24.04",
        },
        "ticket_defaults": {
            "project": "PERF",
            "owner": "team-perf",
        },
        "experiments": [
            {
                "id": "oltp-p95-latency",
                "subsystem": "query",
                "hypothesis": {
                    "h0": "No latency improvement.",
                    "h1": "Treatment lowers p95 latency.",
                    "expected_effect_direction": "improve",
                    "risks": ["cache pollution"],
                    "stop_criteria": ["error_rate > 1%"],
                },
                "scenario": {
                    "workload_family": "oltp",
                    "dataset_size": "100M rows",
                    "query_mix": "70r/30w",
                    "concurrency_profile": "32 clients",
                    "warmup_runs": 5,
                    "cache_mode": "warm",
                    "numa_mode": "interleaved",
                    "io_profile": "nvme",
                    "gpu_allocation": "none",
                },
                "metric": {
                    "name": "p95_latency_ms",
                    "higher_is_better": False,
                    "practical_significance_percent": 3.0,
                    "critical": True,
                },
                "baseline_samples": _seeded_samples(12.0, 0.25, 40, 1001),
                "treatment_samples": _seeded_samples(10.8, 0.20, 40, 2002),
                "performance_budget_percent": 2.0,
            }
        ],
    }


class ScientificEvaluationFrameworkTests(unittest.TestCase):
    def test_pipeline_classifies_significant_positive(self):
        report = sef.run_pipeline(_base_payload())
        self.assertEqual(report["summary"]["signifikant_positiv"], 1)
        self.assertEqual(report["summary"]["gate_violations"], 0)

    def test_pipeline_is_seed_deterministic_for_statistics(self):
        report_a = sef.run_pipeline(_base_payload())
        report_b = sef.run_pipeline(_base_payload())
        stats_a = report_a["results"][0]["statistics"]
        stats_b = report_b["results"][0]["statistics"]
        self.assertEqual(stats_a["bootstrap_ci"], stats_b["bootstrap_ci"])
        self.assertEqual(stats_a["p_value"], stats_b["p_value"])

    def test_rejects_less_than_30_samples(self):
        payload = _base_payload()
        payload["experiments"][0]["baseline_samples"] = [1.0] * 29
        payload["experiments"][0]["treatment_samples"] = [1.0] * 29
        with self.assertRaises(ValueError):
            sef.run_pipeline(payload)

    def test_gate_violation_generates_ticket(self):
        payload = _base_payload()
        payload["experiments"][0]["treatment_samples"] = _seeded_samples(13.5, 0.25, 40, 3003)
        report = sef.run_pipeline(payload)
        self.assertEqual(report["summary"]["gate_violations"], 1)
        self.assertEqual(len(report["regression_tickets"]), 1)
        self.assertEqual(report["results"][0]["classification"], "regressiv")


if __name__ == "__main__":
    unittest.main()
