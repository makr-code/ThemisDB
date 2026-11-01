"""Tests for deterministic aggregation of CTest Google Benchmark JSON files."""

import json
import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from compare_expectations import _load_all_results, main


class BenchmarkResultAggregationTests(unittest.TestCase):
    def test_duplicate_cases_keep_the_result_with_more_iterations(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            results_directory = Path(temporary_directory)
            (results_directory / "short.json").write_text(json.dumps({
                "benchmarks": [{"name": "BM_Read", "iterations": 10,
                                "real_time": 5.0, "cpu_time": 4.0, "time_unit": "us"}]
            }), encoding="utf-8")
            (results_directory / "long.json").write_text(json.dumps({
                "benchmarks": [{"name": "BM_Read", "iterations": 100,
                                "real_time": 3.0, "cpu_time": 2.0, "time_unit": "us"}]
            }), encoding="utf-8")

            result = _load_all_results(results_directory)["BM_Read"]

        self.assertEqual(result.iterations, 100)
        self.assertEqual(result.real_time_ns, 3000.0)
        self.assertEqual(result.cpu_time_ns, 2000.0)

    def test_benchmark_preflight_errors_are_not_counted_as_measurements(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            results_directory = Path(temporary_directory)
            (results_directory / "skipped.json").write_text(json.dumps({
                "benchmarks": [{"name": "BM_ModelTraining", "iterations": 0,
                                "error_occurred": True,
                                "error_message": "GGUF model is missing"}]
            }), encoding="utf-8")

            results = _load_all_results(results_directory)

        self.assertNotIn("BM_ModelTraining", results)

    def test_megascale_ops_per_second_uses_items_per_second(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_directory = root / "src" / "network"
            results_directory = root / "bench_results"
            source_directory.mkdir(parents=True)
            results_directory.mkdir()
            (source_directory / "PERFORMANCE_EXPECTATIONS.md").write_text(
                "## Specific Expectations\n\n"
                "| Target ID | Expectation | Benchmark case |\n"
                "|---|---|---|\n"
                "| SP-1 | > 50 M ops/s | BM_StreamProtocol_FrameHeaderBuild |\n",
                encoding="utf-8",
            )
            (results_directory / "bench_stream_protocol.json").write_text(json.dumps({
                "benchmarks": [{
                    "name": "BM_StreamProtocol_FrameHeaderBuild/128",
                    "iterations": 17920000,
                    "real_time": 6.749,
                    "cpu_time": 6.104,
                    "time_unit": "ns",
                    "items_per_second": 327680000.0,
                }]
            }), encoding="utf-8")
            report_path = results_directory / "report.md"
            args = ["compare_expectations.py", "--src-dir", str(root / "src"),
                    "--results-dir", str(results_directory), "--output", str(report_path)]

            with patch.object(sys, "argv", args), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main(), 0)

            report = report_path.read_text(encoding="utf-8")

        self.assertIn("`SP-1` | ✅ PASS", report)

    def test_cli_writes_markdown_report_from_results_directory(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_directory = root / "src"
            results_directory = root / "bench_results" / "current"
            source_directory.mkdir()
            results_directory.mkdir(parents=True)
            (results_directory / "bench_read.json").write_text(json.dumps({
                "benchmarks": [{"name": "BM_Read", "iterations": 10,
                                "real_time": 5.0, "cpu_time": 4.0, "time_unit": "us"}]
            }), encoding="utf-8")
            report_path = results_directory / "benchmark_comparison_report.md"
            args = ["compare_expectations.py", "--src-dir", str(source_directory),
                    "--results-dir", str(results_directory), "--output", str(report_path)]
            with patch.object(sys, "argv", args), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main(), 0)
            report = report_path.read_text(encoding="utf-8")
            self.assertIn("ThemisDB Benchmark Performance Gate", report)


if __name__ == "__main__":
    unittest.main()