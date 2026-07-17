#!/usr/bin/env python3
"""Unittests for benchmarks/scripts/scientific_evaluation_framework.py."""

import unittest
import json
import random
from pathlib import Path
import sys

_SCRIPTS = Path(__file__).resolve().parent.parent / "scripts"
_WAVE6_INPUT = Path(__file__).resolve().parent.parent / "ci_wave6_release_candidate_experiments.json"
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
                "wave6_track": "B6-A",
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

    def test_wave6_percentiles_and_track_are_reported(self):
        report = sef.run_pipeline(_base_payload())
        result = report["results"][0]
        self.assertEqual(result["wave6_track"], "B6-A")
        self.assertIn("baseline_percentiles", result["statistics"])
        self.assertIn("treatment_percentiles", result["statistics"])
        self.assertIn("p99", result["statistics"]["treatment_percentiles"])
        self.assertIn("B6-A", report["summary"]["wave6_tracks"])

    def test_wave6_guardrail_violation_sets_gate(self):
        payload = _base_payload()
        payload["experiments"][0]["wave6_guardrails"] = {
            "max_p99_latency_ms": 10.5,
        }
        report = sef.run_pipeline(payload)
        self.assertEqual(report["summary"]["gate_violations"], 1)
        self.assertEqual(report["summary"]["wave6_gate_violations"], 1)
        self.assertIn(
            "max_p99_latency_ms_exceeded",
            report["results"][0]["governance"]["wave6_gate_failures"],
        )

    def test_wave6_drift_and_recovery_analysis(self):
        payload = _base_payload()
        payload["experiments"][0]["wave6_track"] = "B6-B"
        payload["experiments"][0]["time_window_samples"] = [10.0, 10.8, 11.6, 12.4]
        payload["experiments"][0]["recovery_time_seconds_samples"] = [6.5, 7.1, 5.9, 6.8, 7.4]
        payload["experiments"][0]["wave6_guardrails"] = {
            "max_regression_drift_percent": 5.0,
            "max_recovery_time_seconds_p95": 8.0,
        }
        report = sef.run_pipeline(payload)
        result = report["results"][0]
        drift = result["wave6_analysis"]["drift"]
        recovery = result["wave6_analysis"]["recovery_time_seconds"]
        self.assertTrue(drift["degrading_over_time"])
        self.assertGreater(drift["regression_drift_percent"], 0.0)
        self.assertGreater(recovery["p95"], 0.0)
        self.assertIn("max_regression_drift_percent_exceeded", result["governance"]["wave6_gate_failures"])

    def test_wave6_reference_suite_executes_without_gate_violation(self):
        payload = json.loads(_WAVE6_INPUT.read_text(encoding="utf-8"))
        report = sef.run_pipeline(payload)
        self.assertEqual(report["summary"]["total_experiments"], 5)
        self.assertEqual(report["summary"]["gate_violations"], 0)
        self.assertEqual(report["summary"]["wave6_gate_violations"], 0)


if __name__ == "__main__":
    unittest.main()
