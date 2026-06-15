#!/usr/bin/env python3
"""CLI integration tests for benchmarks/scripts/scientific_evaluation_framework.py.

Covers the full round-trip through main():
- valid input JSON → output report JSON written to disk (exit 0)
- valid input JSON with --tickets-output → regression tickets JSON written to disk
- missing --input / --output flags → parser exits non-zero
- malformed JSON input → IOError / JSONDecodeError propagates, non-zero exit
- classification result embedded in output report
- gate_violations count reflected in output summary

These tests close finding BENCH-A01 from benchmarks/AUDIT.md.
"""

import json
import random
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parent.parent.parent
_FRAMEWORK = _REPO_ROOT / "benchmarks" / "scripts" / "scientific_evaluation_framework.py"
_PYTHON = sys.executable


def _seeded_samples(mean: float, stddev: float, n: int, seed: int) -> list:
    rng = random.Random(seed)
    return [mean + rng.gauss(0.0, stddev) for _ in range(n)]


def _base_payload() -> dict:
    """Minimal valid payload sufficient for a successful pipeline run."""
    return {
        "seed": 42,
        "confidence_level": 0.95,
        "alpha": 0.05,
        "bootstrap_iterations": 100,
        "permutation_iterations": 100,
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
                "id": "cli-p95-latency",
                "subsystem": "query",
                "hypothesis": {
                    "h0": "No improvement.",
                    "h1": "Treatment lowers p95 latency.",
                    "expected_effect_direction": "improve",
                    "risks": ["cache pollution"],
                    "stop_criteria": ["error_rate > 1%"],
                },
                "scenario": {
                    "workload_family": "oltp",
                    "dataset_size": "10M rows",
                    "query_mix": "80r/20w",
                    "concurrency_profile": "16 clients",
                    "warmup_runs": 2,
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


def _run_cli(*extra_args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        [_PYTHON, str(_FRAMEWORK)] + list(extra_args),
        capture_output=True,
        text=True,
    )


class ScientificEvaluationFrameworkCLITests(unittest.TestCase):
    """End-to-end CLI tests exercising main() via subprocess."""

    def test_happy_path_produces_valid_json_report(self):
        """Valid input → output JSON written, exit code 0, summary present."""
        with tempfile.TemporaryDirectory() as tmpdir:
            input_path = Path(tmpdir) / "input.json"
            output_path = Path(tmpdir) / "report.json"
            input_path.write_text(json.dumps(_base_payload()), encoding="utf-8")

            result = _run_cli("--input", str(input_path), "--output", str(output_path))

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(output_path.exists(), "Output report not written")

            report = json.loads(output_path.read_text(encoding="utf-8"))
            self.assertIn("summary", report)
            self.assertIn("total_experiments", report["summary"])
            self.assertEqual(report["summary"]["total_experiments"], 1)

    def test_classification_in_output_report(self):
        """Positive treatment → classification is signifikant_positiv in report."""
        with tempfile.TemporaryDirectory() as tmpdir:
            input_path = Path(tmpdir) / "input.json"
            output_path = Path(tmpdir) / "report.json"
            input_path.write_text(json.dumps(_base_payload()), encoding="utf-8")

            _run_cli("--input", str(input_path), "--output", str(output_path))

            report = json.loads(output_path.read_text(encoding="utf-8"))
            self.assertEqual(report["summary"]["gate_violations"], 0)
            self.assertEqual(report["summary"]["signifikant_positiv"], 1)

    def test_tickets_output_written_on_regression(self):
        """Regressive treatment → tickets JSON contains one entry."""
        payload = _base_payload()
        # Make treatment worse (regression)
        payload["experiments"][0]["treatment_samples"] = _seeded_samples(14.0, 0.25, 40, 9999)
        with tempfile.TemporaryDirectory() as tmpdir:
            input_path = Path(tmpdir) / "input.json"
            output_path = Path(tmpdir) / "report.json"
            tickets_path = Path(tmpdir) / "tickets.json"
            input_path.write_text(json.dumps(payload), encoding="utf-8")

            result = _run_cli(
                "--input", str(input_path),
                "--output", str(output_path),
                "--tickets-output", str(tickets_path),
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(tickets_path.exists(), "Tickets file not written")
            tickets = json.loads(tickets_path.read_text(encoding="utf-8"))
            self.assertIsInstance(tickets, list)
            self.assertGreaterEqual(len(tickets), 1)

    def test_tickets_output_empty_when_no_regression(self):
        """Positive treatment → tickets JSON is an empty list."""
        with tempfile.TemporaryDirectory() as tmpdir:
            input_path = Path(tmpdir) / "input.json"
            output_path = Path(tmpdir) / "report.json"
            tickets_path = Path(tmpdir) / "tickets.json"
            input_path.write_text(json.dumps(_base_payload()), encoding="utf-8")

            _run_cli(
                "--input", str(input_path),
                "--output", str(output_path),
                "--tickets-output", str(tickets_path),
            )

            tickets = json.loads(tickets_path.read_text(encoding="utf-8"))
            self.assertEqual(tickets, [])

    def test_missing_input_flag_exits_nonzero(self):
        """Omitting --input causes argparse error, non-zero exit."""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "report.json"
            result = _run_cli("--output", str(output_path))
            self.assertNotEqual(result.returncode, 0)

    def test_missing_output_flag_exits_nonzero(self):
        """Omitting --output causes argparse error, non-zero exit."""
        with tempfile.TemporaryDirectory() as tmpdir:
            input_path = Path(tmpdir) / "input.json"
            input_path.write_text("{}", encoding="utf-8")
            result = _run_cli("--input", str(input_path))
            self.assertNotEqual(result.returncode, 0)

    def test_nonexistent_input_file_exits_nonzero(self):
        """Non-existent --input path causes non-zero exit."""
        with tempfile.TemporaryDirectory() as tmpdir:
            result = _run_cli(
                "--input", str(Path(tmpdir) / "no_such_file.json"),
                "--output", str(Path(tmpdir) / "out.json"),
            )
            self.assertNotEqual(result.returncode, 0)

    def test_malformed_json_input_exits_nonzero(self):
        """Malformed JSON in --input causes non-zero exit."""
        with tempfile.TemporaryDirectory() as tmpdir:
            input_path = Path(tmpdir) / "bad.json"
            input_path.write_text("{not valid json", encoding="utf-8")
            result = _run_cli(
                "--input", str(input_path),
                "--output", str(Path(tmpdir) / "out.json"),
            )
            self.assertNotEqual(result.returncode, 0)

    def test_output_parent_dirs_created(self):
        """Output path with non-existent parent directories is created."""
        with tempfile.TemporaryDirectory() as tmpdir:
            input_path = Path(tmpdir) / "input.json"
            output_path = Path(tmpdir) / "nested" / "deep" / "report.json"
            input_path.write_text(json.dumps(_base_payload()), encoding="utf-8")

            result = _run_cli("--input", str(input_path), "--output", str(output_path))

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(output_path.exists())


if __name__ == "__main__":
    unittest.main()
