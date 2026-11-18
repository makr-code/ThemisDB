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

from compare_expectations import (
    _evaluate_expectation,
    _load_all_results,
    _parse_performance_expectations,
    _split_benchmark_cases,
    _write_run_manifest,
    main,
)


class BenchmarkResultAggregationTests(unittest.TestCase):
    def test_benchmark_registration_macros_keep_internal_commas(self):
        cases = _split_benchmark_cases(
            "BENCHMARK_REGISTER_F(Fixture, First), "
            "BENCHMARK_REGISTER_F(Fixture, Second)"
        )

        self.assertEqual(cases, ["Fixture/First", "Fixture/Second"])

    def test_case_parser_ignores_source_paths_and_prose(self):
        cases = _split_benchmark_cases(
            "`benchmarks/gpu/bench_cuda.cpp`; hardware-gated; `Fixture/Run`, BM_Distance"
        )

        self.assertEqual(cases, ["Fixture/Run", "BM_Distance"])

    def test_inline_benchmark_macro_is_normalized(self):
        self.assertEqual(_split_benchmark_cases("`BENCHMARK(FuncName)`"), ["FuncName"])

    def test_table_without_case_column_is_not_reported_as_not_run(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            expectation_file = Path(temporary_directory) / "PERFORMANCE_EXPECTATIONS.md"
            expectation_file.write_text(
                "## Specific Expectations\n\n"
                "| Target ID | Benchmark-source workload | Declared target |\n"
                "|---|---|---|\n"
                "| FRG-01 | Mock heartbeat message work | p99 <= 500 us |\n",
                encoding="utf-8",
            )

            expectations = _parse_performance_expectations(expectation_file, "failover")

        self.assertEqual(len(expectations), 1)
        self.assertEqual(expectations[0].benchmark_cases, [])
        self.assertEqual(expectations[0].constraint_op, "<=")
        self.assertEqual(expectations[0].constraint_value, 500.0)
        verdict = _evaluate_expectation(expectations[0], {})
        self.assertEqual(verdict.status, "NO_MAPPING")

    def test_german_benchmark_case_header_is_recognized(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            expectation_file = Path(temporary_directory) / "PERFORMANCE_EXPECTATIONS.md"
            expectation_file.write_text(
                "## Spezifische Erwartungswerte\n\n"
                "| Ziel-ID | Erwartungswert | Benchmark-Fall |\n"
                "|---|---|---|\n"
                "| TX-1 | <= 100 us | `TransactionBenchmarkFixture/CommitLatency/1` |\n",
                encoding="utf-8",
            )

            expectations = _parse_performance_expectations(expectation_file, "transaction")

        self.assertEqual(expectations[0].benchmark_cases,
                         ["TransactionBenchmarkFixture/CommitLatency/1"])

    def test_run_manifest_normalizes_empty_invalid_and_non_finite_results(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            results_directory = Path(temporary_directory)
            (results_directory / "complete.json").write_text(json.dumps({
                "context": {"library_version": "v1.9.0"},
                "benchmarks": [{"name": "BM_Read", "real_time": float("inf"),
                                "cpu_time": 1.0, "time_unit": "us"}],
            }), encoding="utf-8")
            (results_directory / "empty.json").write_text("", encoding="utf-8")
            (results_directory / "invalid.json").write_text("{", encoding="utf-8")

            manifest_path = _write_run_manifest(results_directory, [])
            manifest_text = manifest_path.read_text(encoding="utf-8")
            manifest = json.loads(manifest_text)

        self.assertEqual(manifest["schema"], "themisdb.benchmark_run")
        self.assertEqual(manifest["schema_version"], 1)
        self.assertEqual(manifest["summary"]["file_status_counts"], {
            "complete": 1, "empty": 1, "invalid_json": 1,
        })
        self.assertIsNone(manifest["benchmarks"][0]["real_time"])
        self.assertNotIn("Infinity", manifest_text)

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

    def test_synthetic_cases_are_excluded_from_performance_results(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            results_directory = Path(temporary_directory)
            (results_directory / "mixed.json").write_text(json.dumps({
                "benchmarks": [
                    {"name": "BM_Synthetic", "label": "synthetic",
                     "iterations": 100, "real_time": 1.0, "time_unit": "us"},
                    {"name": "BM_Production", "label": "component",
                     "iterations": 100, "real_time": 1.0, "time_unit": "us"},
                ]
            }), encoding="utf-8")

            results = _load_all_results(results_directory)

        self.assertNotIn("BM_Synthetic", results)
        self.assertIn("BM_Production", results)

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