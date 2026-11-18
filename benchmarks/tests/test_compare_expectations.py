"""Tests for deterministic aggregation of CTest Google Benchmark JSON files."""

import json
import contextlib
import io
import shutil
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from compare_expectations import (
    BenchmarkResult, Expectation, _evaluate_expectation, _load_all_results,
    _parse_constraint, _parse_performance_expectations, _split_benchmark_cases, main,
)


@contextlib.contextmanager
def workspace():
    with tempfile.TemporaryDirectory(prefix="themis-benchmark-comparator-") as root:
        yield Path(root)


class BenchmarkResultAggregationTests(unittest.TestCase):
    def test_benchmark_registration_macros_keep_internal_commas(self):
        cases = _split_benchmark_cases(
            "BENCHMARK_REGISTER_F(Fixture, First), "
            "BENCHMARK_REGISTER_F(Fixture, Second)"
        )

        self.assertEqual(cases, ["Fixture/First", "Fixture/Second"])

    def test_duplicate_cases_keep_the_result_with_more_iterations(self):
        with workspace() as temporary_directory:
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
        with workspace() as temporary_directory:
            results_directory = Path(temporary_directory)
            (results_directory / "skipped.json").write_text(json.dumps({
                "benchmarks": [{"name": "BM_ModelTraining", "iterations": 0,
                                "error_occurred": True,
                                "error_message": "GGUF model is missing"}]
            }), encoding="utf-8")

            results = _load_all_results(results_directory)

        self.assertNotIn("BM_ModelTraining", results)

    def test_synthetic_cases_are_excluded_from_performance_results(self):
        with workspace() as temporary_directory:
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
        with workspace() as temporary_directory:
            root = Path(temporary_directory)
            source_directory = root / "src" / "network"
            results_directory = root / "bench_results"
            source_directory.mkdir(parents=True)
            results_directory.mkdir()
            (source_directory / "PERFORMANCE_EXPECTATIONS.md").write_text(
                "## Specific Expectations\n\n"
                "| Target ID | Expectation | Benchmark case |\n"
                "|---|---|---|\n"
                "| SP-1 | > 50 M ops/s (items_per_second = ops/s) | BM_StreamProtocol_FrameHeaderBuild |\n",
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
        with workspace() as temporary_directory:
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


class ComparatorHardeningTests(unittest.TestCase):
    def verdict(self, text, results=None, cases=None):
        op, value, unit = _parse_constraint(text)
        exp = Expectation("module", "T-1", text, cases or ["BM_Read"],
                          op, value, unit)
        if results is None:
            results = {"BM_Read": BenchmarkResult(
                "BM_Read", real_time_ns=1_000, iterations=10,
                items_per_second=100_000)}
        return _evaluate_expectation(exp, results)

    def test_numeric_formats_and_units(self):
        for text, expected in (
            ("≤ 1.2 s", ("<=", 1.2, "s")),
            ("≥ 1,000 ops/sec", (">=", 1000, "ops/s")),
            ("< 1 000 ns", ("<", 1000, "ns")),
            ("≤ 0.25 μs", ("<=", .25, "µs")),
            ("> 1.5 M ops/s", (">", 1_500_000, "ops/s")),
            ("<= 5%", ("<=", 5, "%")),
            ("<= 3 MiB", ("<=", 3, "mib")),
        ):
            with self.subTest(text=text):
                self.assertEqual(_parse_constraint(text), expected)

    def test_separate_threshold_column_keeps_context_and_case(self):
        with workspace() as root:
            path = root / "PERFORMANCE_EXPECTATIONS.md"
            path.write_text(
                "## Specific Expectations\n"
                "| Gate ID | Expectation | Threshold | Measurement | Benchmark |\n"
                "|---|---|---|---|---|\n"
                "| IMRG-1 | parsing latency | ≤ 1.2 ms | p95 | BM_Read |\n"
                "| IMRG-2 | p99 parsing latency | ≤ 1.2 ms | p99 | BM_Read |\n"
                "## Other\n| IMRG-3 | other | ≤ 1 ms | p95 | BM_Read |\n",
                encoding="utf-8")
            targets = _parse_performance_expectations(path, "importers")
        self.assertEqual(len(targets), 2)
        self.assertEqual(targets[0].expectation_text,
                         "parsing latency; Threshold: ≤ 1.2 ms; Measurement: p95")
        self.assertEqual(targets[0].benchmark_cases, ["BM_Read"])
        self.assertEqual((targets[0].constraint_value, targets[0].constraint_unit), (1.2, "ms"))
        self.assertEqual(_evaluate_expectation(targets[0], {
            "BM_Read": BenchmarkResult("BM_Read", real_time_ns=1000)
        }).status, "UNCHECKED")
        self.assertEqual(_evaluate_expectation(targets[1], {
            "BM_Read": BenchmarkResult("BM_Read", real_time_ns=1000)
        }).status, "UNCHECKED")

    def test_header_percentile_and_multiple_thresholds(self):
        with workspace() as root:
            path = root / "PERFORMANCE_EXPECTATIONS.md"
            path.write_text(
                "## Specific Expectations\n"
                "| Target ID | Benchmark | p95 Target | p99 Target |\n"
                "|---|---|---|---|\n"
                "| T-1 | BM_Read | ≤ 1 ms | ≤ 2 ms |\n", encoding="utf-8")
            target = _parse_performance_expectations(path, "m")[0]
        self.assertIn("p95 Target", target.expectation_text)
        self.assertEqual(_evaluate_expectation(target, {
            "BM_Read": BenchmarkResult("BM_Read", real_time_ns=1)
        }).status, "UNCHECKED")

    def test_unverifiable_expectations_are_never_pass(self):
        for text in (
            "bounded", "≤ 10% overhead", "≤ 2 ms vs baseline", "≥ 2×",
            "≤ 2 MB", "≤ 10", "p95 ≤ 2 ms", "p99 ≤ 2 ms",
            "p50 ≤ 2 ms", "P90 ≤ 2 ms", "p99.9 ≤ 2 ms",
            "p99_9 ≤ 2 ms", "95th percentile ≤ 2 ms",
            "percentile 95 ≤ 2 ms",
            "≤ 2 ms and ≥ 1 ms", "!= 2 ms", "≈ 2 ms",
            "≤ 1e309 ms", "≥ 10 ops/s", "≥ 10 k/s",
        ):
            with self.subTest(text=text):
                self.assertEqual(self.verdict(text).status, "UNCHECKED")

    def test_percentile_must_be_explicit_and_unambiguous(self):
        result = BenchmarkResult("BM_Read", real_time_ns=1,
                                 percentile_time_ns={"p95": 3_000_000})
        self.assertEqual(self.verdict("p95 ≤ 2 ms", {"BM_Read": result}).status, "FAIL")
        result.percentile_time_ns["p95"] = 1_000_000
        self.assertEqual(self.verdict("p95 ≤ 2 ms", {"BM_Read": result}).status, "PASS")
        result.ambiguous_percentiles.add("p95")
        self.assertEqual(self.verdict("p95 ≤ 2 ms", {"BM_Read": result}).status, "UNCHECKED")
        result.repeated_sample = True
        self.assertEqual(self.verdict("≤ 2 ms", {"BM_Read": result}).status, "UNCHECKED")

    def test_missing_cases_and_measurements_do_not_pass(self):
        result = BenchmarkResult("BM_Read", real_time_ns=1000)
        verdict = self.verdict("≤ 2 ms", {"BM_Read": result}, ["BM_Read", "BM_Missing"])
        self.assertEqual(verdict.status, "UNCHECKED")
        self.assertIn("BM_Missing", verdict.detail)
        result.real_time_ns = None
        self.assertEqual(self.verdict("≤ 2 ms", {"BM_Read": result}).status, "UNCHECKED")
        result.real_time_ns = 3_000_000
        verdict = self.verdict("≤ 2 ms", {"BM_Read": result}, ["BM_Read", "BM_Missing"])
        self.assertEqual(verdict.status, "FAIL")
        self.assertIn("BM_Missing", verdict.detail)
        self.assertEqual(self.verdict("≤ 2 ms", {}).status, "NOT_RUN")

    def test_throughput_requires_mapping_and_counter(self):
        result = BenchmarkResult("BM_Read", real_time_ns=10, iterations=1000,
                                 items_per_second=100_000)
        self.assertEqual(self.verdict("≥ 10 k/s", {"BM_Read": result}).status, "UNCHECKED")
        text = "≥ 10 k/s (items_per_second = ops/s)"
        self.assertEqual(self.verdict(text, {"BM_Read": result}).status, "PASS")
        result.items_per_second = None
        self.assertEqual(self.verdict(text, {"BM_Read": result}).status, "UNCHECKED")

    def test_transaction_write_throughput_is_measured_in_committed_transactions(self):
        repository_root = Path(__file__).resolve().parents[2]
        expectations = _parse_performance_expectations(
            repository_root / "src" / "transaction" / "PERFORMANCE_EXPECTATIONS.md",
            "transaction",
        )
        tx3 = next(expectation for expectation in expectations if expectation.target_id == "TX-3")

        self.assertIn("one item is one committed transaction", tx3.expectation_text)
        self.assertEqual(tx3.constraint_unit, "k/s")
        self.assertEqual(
            self.verdict(tx3.expectation_text, {
                tx3.benchmark_cases[0]: BenchmarkResult(
                    tx3.benchmark_cases[0], items_per_second=7_000,
                )
            }, tx3.benchmark_cases).status,
            "PASS",
        )
        self.assertEqual(
            self.verdict(tx3.expectation_text, {
                tx3.benchmark_cases[0]: BenchmarkResult(
                    tx3.benchmark_cases[0], items_per_second=5_000,
                )
            }, tx3.benchmark_cases).status,
            "FAIL",
        )

    def test_invalid_empty_failed_files_and_unsupported_units_are_diagnosed(self):
        with workspace() as root:
            for name, content in (
                ("bad.json", "{"), ("empty.json", '{"benchmarks": []}'),
                ("wrong.json", "[]"),
                ("failed.json", json.dumps({"benchmarks": [
                    {"name": "BM_Partial", "real_time": 1, "time_unit": "ns"},
                    {"name": "BM_Error", "error_occurred": True, "error_message": "failed"},
                ]})),
                ("units.json", json.dumps({"benchmarks": [
                    {"name": "BM_Units", "real_time": 1, "time_unit": "fortnights"},
                    {"name": "BM_NaN", "real_time": float("nan"), "time_unit": "ns"},
                    {"name": "BM_Inf", "real_time": float("inf"), "time_unit": "ns"},
                ]})),
            ):
                (root / name).write_text(content, encoding="utf-8")
            diagnostics = []
            with contextlib.redirect_stderr(io.StringIO()):
                results = _load_all_results(root, diagnostics)
            self.assertNotIn("BM_Partial", results)
            self.assertIsNone(results["BM_Units"].real_time_ns)
            self.assertIsNone(results["BM_NaN"].real_time_ns)
            self.assertIsNone(results["BM_Inf"].real_time_ns)
            joined = "\n".join(diagnostics)
            for term in ("bad.json", "empty.json", "wrong.json", "BM_Error",
                         "discarded", "unsupported time unit", "invalid real_time"):
                self.assertIn(term, joined)

    def test_duplicate_percentiles_are_not_chosen_by_iteration_count(self):
        with workspace() as root:
            for name, iterations, p95 in (("a.json", 10, 20), ("b.json", 100, 1)):
                (root / name).write_text(json.dumps({"benchmarks": [{
                    "name": "BM_Read", "iterations": iterations, "time_unit": "ms",
                    "real_time": 1, "p95_real_time": p95}]}), encoding="utf-8")
            results = _load_all_results(root)
        self.assertEqual(self.verdict("p95 ≤ 2 ms", results).status, "UNCHECKED")

    def test_only_explicit_real_time_percentile_fields_are_used(self):
        with workspace() as root:
            path = root / "stats.json"
            row = {"name": "BM_Read", "real_time": 1, "time_unit": "ms", "p95": .5}
            path.write_text(json.dumps({"benchmarks": [row]}), encoding="utf-8")
            self.assertEqual(self.verdict("p95 ≤ 2 ms", _load_all_results(root)).status,
                             "UNCHECKED")
            row["p95_real_time"] = .5
            path.write_text(json.dumps({"benchmarks": [row]}), encoding="utf-8")
            self.assertEqual(self.verdict("p95 ≤ 2 ms", _load_all_results(root)).status, "PASS")

    def test_report_contains_diagnostics_and_scope(self):
        with workspace() as root:
            src = root / "src"
            src.mkdir()
            results = root / "results"
            results.mkdir()
            (results / "broken.json").write_text("{", encoding="utf-8")
            report_path = root / "report.md"
            args = ["compare_expectations.py", "--src-dir", str(src), "--results-dir",
                    str(results), "--output", str(report_path)]
            with patch.object(sys, "argv", args), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main(), 0)
            report = report_path.read_text(encoding="utf-8")
        self.assertIn("Result Loading Diagnostics", report)
        self.assertIn("broken.json", report)
        self.assertIn("other tables are not evaluated", report)
        self.assertNotIn("All covered targets pass", report)

    def test_manifest_admits_only_completed_valid_tests_and_reports_mappings(self):
        with workspace() as root:
            src = root / "src"
            module = src / "transaction"
            module.mkdir(parents=True)
            (module / "PERFORMANCE_EXPECTATIONS.md").write_text(
                "## Specific Expectations\n| Target ID | Expectation | Benchmark case |\n"
                "|---|---|---|\n| TX-3 | ≥ 6 k/s | BM_Transaction |\n"
                "| TX-4 | ≤ 2 ms | BM_Timeout |\n", encoding="utf-8")
            results = root / "results"
            results.mkdir()
            tests = []
            for name, execution, json_status in (
                ("Transaction", "completed", "valid"),
                ("Timeout", "timeout", "valid"),
                ("Failed", "process_failure", "valid"),
                ("Invalid", "completed", "invalid"),
                ("NotBuilt", "not_built", "missing"),
            ):
                path = results / (name + ".json")
                path.write_text(json.dumps({"benchmarks": [{
                    "name": "BM_" + name, "real_time": 1, "time_unit": "us",
                    "items_per_second": 100000}]}), encoding="utf-8")
                tests.append({"test": name, "result": path.name,
                              "result_path_at_run": str(path),
                              "execution_status": execution, "json_status": json_status,
                              "cases": [{"name": "BM_" + name, "status": "observed"}]})
            (results / "unlisted.json").write_text(json.dumps({"benchmarks": [
                {"name": "BM_Unlisted", "real_time": 1, "time_unit": "ns"}]}),
                encoding="utf-8")
            manifest = {
                "status": "finished", "tests": tests,
                "expectations": [
                    {"module": "transaction", "target_id": "TX-3",
                     "case": "BM_Transaction", "mapping_status": "observed",
                     "observations": [{"test": "Transaction", "case_status": "observed"}]},
                    {"module": "transaction", "target_id": "TX-4",
                     "case": "BM_Timeout", "mapping_status": "unknown", "observations": []},
                ]}
            (results / "manifest").write_text(json.dumps(manifest), encoding="utf-8")
            diagnostics = []
            info = {}
            with contextlib.redirect_stderr(io.StringIO()):
                loaded = _load_all_results(results, diagnostics, info)
            self.assertEqual(list(loaded), ["BM_Transaction"])
            self.assertEqual(self.verdict("≥ 6 k/s", loaded, ["BM_Transaction"]).status, "UNCHECKED")
            self.assertTrue(any("process_failure" in item for item in diagnostics))
            report_path = root / "report.md"
            args = ["compare_expectations.py", "--src-dir", str(src), "--results-dir",
                    str(results), "--output", str(report_path)]
            with patch.object(sys, "argv", args), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main(), 0)
            report = report_path.read_text(encoding="utf-8")
            for text in ("Manifest Test and Case Outcomes", "process_failure", "timeout",
                         "not_built", "TX-3", "BM_Transaction", "Mapping",
                         "no matching usable result", "UNCHECKED"):
                self.assertIn(text, report)

    def test_manifest_fails_closed_for_invalid_unfinished_and_changed_cases(self):
        with workspace() as root:
            file = root / "valid.json"
            file.write_text(json.dumps({"benchmarks": [
                {"name": "BM_Read", "real_time": 1, "time_unit": "ns"}]}),
                encoding="utf-8")
            test = {"test": "read", "result": file.name, "execution_status": "completed",
                    "json_status": "valid", "cases": [{"name": "BM_Other", "status": "observed"}]}
            for manifest in ("{", json.dumps({"status": "in_progress", "tests": [test]}),
                             json.dumps({"status": "finished", "tests": [test]})):
                with self.subTest(manifest=manifest):
                    (root / "manifest").write_text(manifest, encoding="utf-8")
                    diagnostics = []
                    with contextlib.redirect_stderr(io.StringIO()):
                        loaded = _load_all_results(root, diagnostics)
                    self.assertEqual(loaded, {})
                    self.assertTrue(diagnostics)

    def test_manifest_case_error_and_duplicate_path_fail_closed(self):
        with workspace() as root:
            file = root / "results.json"
            file.write_text(json.dumps({"benchmarks": [
                {"name": "BM_Read", "real_time": 1, "time_unit": "ns"}]}), encoding="utf-8")
            test = {"test": "read", "result": file.name, "execution_status": "completed",
                    "json_status": "valid", "cases": [{"name": "BM_Read", "status": "observed"}]}
            for entries in ([test, {**test, "execution_status": "process_failure"}],
                            [{**test, "cases": [{"name": "BM_Read", "status": "error",
                                                "error_message": "failed"}]}]):
                (root / "manifest").write_text(
                    json.dumps({"status": "finished", "tests": entries}), encoding="utf-8")
                diagnostics = []
                with contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(_load_all_results(root, diagnostics), {})
                self.assertTrue(diagnostics)

    def test_archived_manifest_paths_and_untrusted_paths(self):
        with workspace() as root:
            current = root / "current"
            current.mkdir()
            path = current / "bench.json"
            path.write_text(json.dumps({"benchmarks": [
                {"name": "BM_Read", "real_time": 1, "time_unit": "ns"}]}), encoding="utf-8")
            test = {"test": "read", "result": "bench.json", "result_path_at_run": str(path),
                    "execution_status": "completed", "json_status": "valid",
                    "cases": [{"name": "BM_Read", "status": "observed"}]}
            (current / "manifest").write_text(json.dumps({
                "status": "finished", "tests": [test]}), encoding="utf-8")
            archived = root / "archive"
            shutil.copytree(current, archived)
            shutil.rmtree(current)
            self.assertIn("BM_Read", _load_all_results(archived, []))
            for invalid in ("../archive/bench.json", str(archived / "bench.json")):
                test["result"] = invalid
                (archived / "manifest").write_text(json.dumps({
                    "status": "finished", "tests": [test]}), encoding="utf-8")
                diagnostics = []
                self.assertEqual(_load_all_results(archived, diagnostics), {})
                self.assertTrue(diagnostics)
            (archived / "escape.json").symlink_to(root / "outside.json")
            test["result"] = "escape.json"
            (archived / "manifest").write_text(json.dumps({
                "status": "finished", "tests": [test]}), encoding="utf-8")
            diagnostics = []
            self.assertEqual(_load_all_results(archived, diagnostics), {})
            self.assertTrue(diagnostics)

    def test_strict_completeness_exit_codes(self):
        with workspace() as root:
            src = root / "src"
            module = src / "test"
            module.mkdir(parents=True)
            results = root / "results"
            results.mkdir()
            path = results / "bench.json"
            expectations = module / "PERFORMANCE_EXPECTATIONS.md"
            args = ["compare_expectations.py", "--src-dir", str(src), "--results-dir",
                    str(results), "--output", str(root / "report.md"), "--strict-completeness"]

            def run():
                with patch.object(sys, "argv", args), contextlib.redirect_stderr(io.StringIO()):
                    return main()

            self.assertEqual(run(), 2)  # No expectations and no measurements.
            expectations.write_text(
                "## Specific Expectations\n| Target ID | Expectation | Benchmark case |\n"
                "|---|---|---|\n| T-1 | ≤ 2 ms | BM_Read |\n", encoding="utf-8")
            self.assertEqual(run(), 2)  # No result.
            path.write_text(json.dumps({"benchmarks": [
                {"name": "BM_Read", "real_time": 1, "time_unit": "ms"}]}), encoding="utf-8")
            self.assertEqual(run(), 0)
            (results / "broken.json").write_text("{", encoding="utf-8")
            self.assertEqual(run(), 2)  # Loading diagnostics despite a passing target.
            (results / "broken.json").unlink()
            expectations.write_text(expectations.read_text().replace("≤ 2 ms", "bounded"),
                                    encoding="utf-8")
            self.assertEqual(run(), 2)  # Unchecked.
            args.remove("--strict-completeness")
            self.assertEqual(run(), 0)  # Backward-compatible default.
            args.append("--strict-completeness")
            expectations.write_text(expectations.read_text().replace("bounded", "≤ 0.1 ms"),
                                    encoding="utf-8")
            (results / "broken.json").write_text("{", encoding="utf-8")
            self.assertEqual(run(), 1)  # FAIL takes precedence over diagnostics.


if __name__ == "__main__":
    unittest.main()