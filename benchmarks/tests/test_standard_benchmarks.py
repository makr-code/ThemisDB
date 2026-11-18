"""Regression tests for the database-backed YCSB measurement contract."""

import asyncio
import contextlib
import io
import json
import os
import socket
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import httpx
import psutil
import requests

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from complete_benchmark_suite import CompleteBenchmarkSuite, main as complete_main
from scientific_benchmark_runner import ScientificBenchmarkRunner, ScientificConfig
from run_complete_benchmarks import BenchmarkExecutor, main as executor_main
from standard_benchmarks import (
    TPCCBenchmark, TPCHBenchmark, SysbenchBenchmark, SysbenchWorkload,
    YCSBBenchmark, YCSBWorkload,
)
from standard_benchmarks_integration import StandardBenchmarkRunner, main, _process_snapshot, _server_resource_report
from ycsb.themisdb_adapter import ThemisDBYCSBAdapter
from unified_benchmark_suite import UnifiedBenchmark


class YCSBAdapterContractTests(unittest.IsolatedAsyncioTestCase):
    async def test_missing_adapter_is_rejected(self):
        runner = YCSBBenchmark("themisdb")
        for adapter in (None, False, "not-an-adapter"):
            with self.subTest(adapter=adapter):
                with self.assertRaisesRegex(ValueError, "explicit async database"):
                    await runner.run_workload(YCSBWorkload.WORKLOAD_C, 1, adapter)
        self.assertEqual(runner.results, {})

    async def test_invalid_operation_count_is_rejected_before_execution(self):
        calls = []

        async def operation(operation_type):
            calls.append(operation_type)

        runner = YCSBBenchmark("themisdb")
        for count in (0, -1, True, 1.5, "10"):
            with self.subTest(count=count):
                with self.assertRaisesRegex(ValueError, "positive integer"):
                    await runner.run_workload(YCSBWorkload.WORKLOAD_C, count, operation)
        self.assertEqual(calls, [])
        self.assertEqual(runner.results, {})

    async def test_explicit_adapter_receives_each_operation(self):
        calls = []

        async def operation(operation_type):
            calls.append(operation_type)

        runner = YCSBBenchmark("themisdb")
        with contextlib.redirect_stdout(io.StringIO()):
            result = await runner.run_workload(YCSBWorkload.WORKLOAD_C, 5, operation)
        self.assertEqual(calls, ["read"] * 5)
        self.assertEqual(result.total_operations, 5)
        self.assertEqual(result.read_count, 5)

    async def test_adapter_exception_is_not_stored_as_success(self):
        async def operation(operation_type):
            raise ConnectionError("database is unavailable")

        runner = YCSBBenchmark("themisdb")
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(ConnectionError, "database is unavailable"):
                await runner.run_workload(YCSBWorkload.WORKLOAD_C, 1, operation)
        self.assertEqual(runner.results, {})

    async def test_workload_f_counts_read_modify_write_without_inserts(self):
        calls = []

        async def operation(operation_type):
            calls.append(operation_type)

        runner = YCSBBenchmark("themisdb")
        with contextlib.redirect_stdout(io.StringIO()):
            result = await runner.run_workload(YCSBWorkload.WORKLOAD_F, 100, operation)
        self.assertEqual(YCSBBenchmark.WORKLOAD_DEFINITIONS["F"], {"read": 50, "read_modify_write": 50})
        self.assertEqual(set(calls), {"read", "read_modify_write"})
        self.assertEqual(result.read_count + result.read_modify_write_count, 100)
        self.assertEqual(result.insert_count, 0)
        self.assertEqual(result.write_count, 0)

    async def test_operation_stream_is_reproducible_per_run(self):
        calls = []

        async def operation(operation_type):
            calls.append(operation_type)

        runner = YCSBBenchmark("themisdb")
        with contextlib.redirect_stdout(io.StringIO()):
            await runner.run_workload(YCSBWorkload.WORKLOAD_A, 50, operation, seed=17)
            first = calls[:]
            calls.clear()
            await runner.run_workload(YCSBWorkload.WORKLOAD_A, 50, operation, seed=17)
        self.assertEqual(calls, first)

    async def test_result_has_no_unverified_industry_ranking(self):
        async def operation(operation_type):
            return None

        with contextlib.redirect_stdout(io.StringIO()):
            result = await YCSBBenchmark("themisdb").run_workload(YCSBWorkload.WORKLOAD_C, 5, operation)
        self.assertIsNone(result.expected_throughput_ops_sec)
        self.assertIsNone(result.expected_latency_p95_ms)
        self.assertIsNone(result.throughput_ratio)
        self.assertIsNone(result.latency_ratio)
        self.assertFalse(result.external_comparison_ready)
        self.assertEqual(result.benchmark_kind, "ycsb_inspired")
        self.assertLessEqual(result.latency_p50_ms, result.latency_p95_ms)
        self.assertLessEqual(result.latency_p95_ms, result.latency_p99_ms)


class YCSBSuiteContractTests(unittest.IsolatedAsyncioTestCase):
    async def test_suite_validates_all_adapters_before_execution(self):
        calls = []

        async def operation(operation_type):
            calls.append(operation_type)

        with tempfile.TemporaryDirectory() as output_directory:
            runner = StandardBenchmarkRunner(output_directory)
            for adapters in (None, {}, {"themisdb": operation}):
                with self.subTest(adapters=adapters):
                    with self.assertRaisesRegex(ValueError, "Every database requires"):
                        await runner.run_ycsb_suite(
                            ["themisdb", "postgres"], operation_adapters=adapters
                        )
            self.assertEqual(list(Path(output_directory).iterdir()), [])
        self.assertEqual(calls, [])

    async def test_suite_uses_adapters_without_industry_grades(self):
        calls = []

        async def operation(operation_type):
            calls.append(operation_type)

        with tempfile.TemporaryDirectory() as output_directory:
            runner = StandardBenchmarkRunner(output_directory)
            with contextlib.redirect_stdout(io.StringIO()):
                report = await runner.run_ycsb_suite(
                    ["themisdb"],
                    [YCSBWorkload.WORKLOAD_C],
                    operation_adapters={"themisdb": operation},
                    num_operations=3,
                    seed=17,
                )
        self.assertEqual(calls, ["read"] * 3)
        self.assertEqual(report["results"][0]["seed"], 17)
        self.assertEqual(report["comparisons"], [])
        self.assertEqual(report["comparison_status"], "not_established")
        self.assertFalse(report["external_comparison_ready"])

    def test_cli_rejects_unsupported_or_invalid_runs_without_output(self):
        script = Path(__file__).resolve().parents[1] / "standard_benchmarks_integration.py"
        with tempfile.TemporaryDirectory() as output_directory:
            output = Path(output_directory) / "result.json"
            for extra in (["--workload", "E"], ["--operations", "0"], ["--timeout", "nan"], ["--server-pid", "0"]):
                with self.subTest(extra=extra):
                    result = subprocess.run(
                        [sys.executable, str(script), "--endpoint", "http://127.0.0.1:1",
                         "--workload", "C", "--output", str(output), *extra],
                        capture_output=True, text=True, timeout=30,
                    )
                    self.assertEqual(result.returncode, 2, result.stderr)
                    self.assertFalse(output.exists())

    def test_cli_connection_failure_produces_no_report(self):
        script = Path(__file__).resolve().parents[1] / "standard_benchmarks_integration.py"
        with tempfile.TemporaryDirectory() as output_directory, socket.socket() as reserved:
            reserved.bind(("127.0.0.1", 0))
            output = Path(output_directory) / "result.json"
            result = subprocess.run(
                [sys.executable, str(script), "--endpoint", f"http://127.0.0.1:{reserved.getsockname()[1]}",
                 "--workload", "C", "--record-count", "1", "--operations", "1", "--timeout", "0.1",
                 "--output", str(output)], capture_output=True, text=True, timeout=30,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("httpx.Connect", result.stderr)
            self.assertFalse(output.exists())


class CentralBenchmarkContractTests(unittest.IsolatedAsyncioTestCase):
    def test_existing_themis_adapter_rejects_http_errors(self):
        response = requests.Response()
        response.status_code = 503
        response.url = "http://benchmark.test"
        adapter = UnifiedBenchmark.ThemisAdapter("http://benchmark.test")
        for method, operation in (
            ("post", lambda: adapter.insert("key", {})),
            ("get", lambda: adapter.read("key")),
            ("put", lambda: adapter.update("key", {})),
            ("get", lambda: adapter.query({})),
            ("post", lambda: adapter.aggregate_count({})),
        ):
            with self.subTest(method=method), patch(
                f"unified_benchmark_suite.requests.{method}", return_value=response
            ):
                with self.assertRaises(requests.HTTPError):
                    operation()

    async def test_unbacked_standard_workloads_are_rejected(self):
        cases = (
            TPCCBenchmark("ThemisDB").run_benchmark(duration_seconds=1),
            TPCHBenchmark("ThemisDB").run_benchmark(),
            SysbenchBenchmark("ThemisDB").run_workload(
                SysbenchWorkload.OLTP_READ_ONLY, duration_seconds=1
            ),
        )
        for operation in cases:
            with self.assertRaisesRegex(ValueError, "explicit .* adapter"):
                await operation

    async def test_scientific_failure_does_not_store_partial_samples(self):
        config = ScientificConfig(warmup_runs=0, repetitions=1, iterations_per_run=2)
        runner = ScientificBenchmarkRunner(config)
        calls = 0

        async def operation():
            nonlocal calls
            calls += 1
            if calls == 2:
                raise ConnectionError("database failed")

        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(ConnectionError, "database failed"):
                await runner.run_benchmark("ThemisDB", "read", operation)
        self.assertEqual(runner.results, {})
        self.assertEqual(runner.analyses, {})

    async def test_scientific_timeout_does_not_store_samples(self):
        config = ScientificConfig(
            warmup_runs=0, repetitions=1, iterations_per_run=2, operation_timeout_ms=0
        )
        runner = ScientificBenchmarkRunner(config)

        async def operation():
            return None

        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(TimeoutError, "exceeded timeout"):
                await runner.run_benchmark("ThemisDB", "read", operation)
        self.assertEqual(runner.results, {})

    async def test_scientific_output_supports_windows_legacy_encoding(self):
        config = ScientificConfig(warmup_runs=1, repetitions=1, iterations_per_run=2)
        runner = ScientificBenchmarkRunner(config)

        async def operation():
            return None

        with io.TextIOWrapper(io.BytesIO(), encoding="cp1252") as output:
            with contextlib.redirect_stdout(output):
                result = await runner.run_benchmark("ThemisDB", "read", operation)
        self.assertEqual(result.sample_count, 2)

    async def test_complete_cli_rejects_unavailable_mode(self):
        with tempfile.TemporaryDirectory() as output_directory:
            arguments = ["complete_benchmark_suite.py", "--mode", "tpcc",
                         "--output-dir", output_directory]
            with patch.object(sys, "argv", arguments), contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(SystemExit) as failure:
                    await complete_main()
            self.assertEqual(failure.exception.code, 1)
            self.assertEqual(list(Path(output_directory).iterdir()), [])


class ServerResourceContractTests(unittest.TestCase):
    def test_real_process_snapshots_and_phase_deltas(self):
        process = psutil.Process(os.getpid())
        samples = [_process_snapshot(process, phase) for phase in
                   ("before_load", "after_load", "after_measurement", "after_cleanup")]
        report = _server_resource_report(process, samples)
        self.assertEqual(report["pid"], os.getpid())
        self.assertEqual(len(report["samples"]), 4)
        self.assertEqual([phase["phase"] for phase in report["phases"]], ["load", "measurement", "cleanup"])
        for sample in samples:
            self.assertGreater(sample["rss_bytes"], 0)
            self.assertGreaterEqual(sample["cpu_seconds"], 0)
        for phase in report["phases"]:
            self.assertGreaterEqual(phase["wall_seconds"], 0)
            self.assertGreaterEqual(phase["cpu_seconds"], 0)

    def test_cli_rejects_nonlistening_process_before_writes(self):
        script = Path(__file__).resolve().parents[1] / "standard_benchmarks_integration.py"
        with tempfile.TemporaryDirectory() as output_directory:
            output = Path(output_directory) / "result.json"
            result = subprocess.run(
                [sys.executable, str(script), "--endpoint", "http://127.0.0.1:1",
                 "--workload", "C", "--server-pid", str(os.getpid()), "--output", str(output)],
                capture_output=True, text=True, timeout=30,
            )
            self.assertEqual(result.returncode, 2, result.stderr)
            self.assertIn("not listening", result.stderr)
            self.assertFalse(output.exists())


class ThemisDBAdapterContractTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.entities = {}
        self.requests = []
        self.reject_after_write = False

        def handle(request):
            if not request.url.path.startswith("/entities/"):
                return httpx.Response(404)
            self.requests.append(request)
            key = request.url.path.removeprefix("/entities/")
            if "." in key or ":" not in key:
                return httpx.Response(400, json={"error": True, "message": "invalid path segment"})
            if request.method == "PUT":
                payload = json.loads(request.content)
                self.assertIsInstance(payload["blob"], str)
                self.entities[key] = json.loads(payload["blob"])
                if self.reject_after_write:
                    return httpx.Response(503)
                return httpx.Response(201, json={"success": True, "key": key})
            if request.method == "DELETE":
                self.entities.pop(key, None)
                return httpx.Response(200, json={"success": True})
            if key not in self.entities:
                return httpx.Response(404)
            return httpx.Response(200, json={"key": key, "blob": json.dumps(self.entities[key])})

        self.client = httpx.Client(
            base_url="http://benchmark.test", transport=httpx.MockTransport(handle)
        )
        self.addCleanup(self.client.close)

    async def test_canonical_entity_contract_and_owned_cleanup(self):
        self.entities["unrelated:user"] = {"keep": True}
        adapter = ThemisDBYCSBAdapter(self.client, "C", record_count=2)
        await adapter.load()
        with contextlib.redirect_stdout(io.StringIO()):
            result = await YCSBBenchmark("themisdb").run_workload(
                YCSBWorkload.WORKLOAD_C, 3, adapter
            )
        self.assertEqual(result.read_count, 3)
        self.assertEqual(sum(request.method == "PUT" for request in self.requests), 2)
        self.assertEqual(sum(request.method == "GET" for request in self.requests), 5)
        await adapter.cleanup()
        self.assertEqual(self.entities, {"unrelated:user": {"keep": True}})

    async def test_read_modify_write_gets_then_updates_existing_record(self):
        adapter = ThemisDBYCSBAdapter(self.client, "F", record_count=1)
        await adapter.load()
        before = dict(next(iter(self.entities.values())))
        self.requests.clear()
        await adapter("read_modify_write")
        self.assertEqual([request.method for request in self.requests], ["GET", "PUT"])
        after = next(iter(self.entities.values()))
        self.assertEqual(sum(before[field] != after[field] for field in before), 1)
        await adapter("read")
        await adapter.cleanup()

    async def test_corrupted_read_is_rejected(self):
        adapter = ThemisDBYCSBAdapter(self.client, "C", record_count=1)
        await adapter.load()
        next(iter(self.entities.values()))["field0"] = "corrupt"
        with self.assertRaisesRegex(RuntimeError, "read validation failed"):
            await adapter("read")
        await adapter.cleanup()

    async def test_cleanup_includes_writes_with_uncertain_commit(self):
        self.reject_after_write = True
        adapter = ThemisDBYCSBAdapter(self.client, "A", record_count=1)
        with self.assertRaises(httpx.HTTPStatusError):
            await adapter.load()
        self.assertEqual(len(self.entities), 1)
        await adapter.cleanup()
        self.assertEqual(self.entities, {})

    async def test_load_and_supported_workloads_are_required(self):
        for workload in ("D", "E"):
            with self.assertRaisesRegex(ValueError, "only A/B/C/F"):
                ThemisDBYCSBAdapter(self.client, workload)
        adapter = ThemisDBYCSBAdapter(self.client, "C", record_count=1)
        with self.assertRaisesRegex(RuntimeError, "Load and validate"):
            await adapter("read")
        self.assertEqual(self.requests, [])

    async def test_transport_failure_is_not_a_measurement(self):
        def unavailable(request):
            return httpx.Response(503)

        with httpx.Client(base_url="http://benchmark.test",
                  transport=httpx.MockTransport(unavailable)) as client:
            adapter = ThemisDBYCSBAdapter(client, "A", record_count=1)
            with self.assertRaises(httpx.HTTPStatusError):
                await adapter.load()
            with self.assertRaisesRegex(RuntimeError, "Load and validate"):
                await adapter("read")

    async def test_cli_exports_only_after_success_and_owned_cleanup(self):
        with tempfile.TemporaryDirectory() as output_directory:
            output = Path(output_directory) / "result.json"
            arguments = ["standard_benchmarks_integration.py", "--endpoint", "http://benchmark.test",
                         "--workload", "C", "--record-count", "2", "--operations", "3",
                         "--output", str(output)]
            with patch.object(sys, "argv", arguments), patch("httpx.Client", return_value=self.client):
                with contextlib.redirect_stdout(io.StringIO()):
                    await main()
            report = json.loads(output.read_text(encoding="utf-8"))
        self.assertEqual(report["result"]["read_count"], 3)
        self.assertEqual(report["dataset"]["record_count"], 2)
        self.assertEqual(report["protocol"]["concurrency"], 1)
        self.assertFalse(report["external_comparison_ready"])
        self.assertIsNone(report["server_resources"])
        self.assertEqual(self.entities, {})

    async def test_complete_suite_runs_real_http_workloads_and_cleans_up(self):
        with tempfile.TemporaryDirectory() as output_directory:
            suite = CompleteBenchmarkSuite(
                output_directory, endpoint="http://benchmark.test",
                record_count=2, operations=3, seed=17,
            )
            with patch("httpx.Client", return_value=self.client), contextlib.redirect_stdout(io.StringIO()):
                report = await suite.run_ycsb_benchmarks(["ThemisDB"], ["C", "A"])
            self.assertEqual([result["total_operations"] for result in report["results"]], [3, 3])
            self.assertFalse(report["external_comparison_ready"])
            self.assertEqual(sum(request.method == "DELETE" for request in self.requests), 4)
            self.assertEqual(self.entities, {})

    async def test_complete_scientific_measures_http_reads(self):
        with tempfile.TemporaryDirectory() as output_directory:
            suite = CompleteBenchmarkSuite(
                output_directory, endpoint="http://benchmark.test", record_count=1,
            )
            with patch("httpx.Client", return_value=self.client), contextlib.redirect_stdout(io.StringIO()):
                report = await suite.run_scientific_benchmark(
                    "ThemisDB", operation="read", repetitions=1,
                    warmup_runs=0, iterations_per_run=2,
                )
            self.assertEqual(report["analysis"]["sample_count"], 2)
            self.assertFalse(report["protocol"]["outliers_removed"])
            self.assertNotIn("network_latency_ms", report["hardware"])
            self.assertEqual(report["hardware"]["scope"], "benchmark client host")
            self.assertEqual([request.method for request in self.requests], ["PUT", "GET", "GET", "GET", "DELETE"])
            self.assertEqual(self.entities, {})

    async def test_complete_cli_failure_does_not_export(self):
        self.reject_after_write = True
        with tempfile.TemporaryDirectory() as output_directory:
            arguments = ["complete_benchmark_suite.py", "--mode", "ycsb",
                         "--endpoint", "http://benchmark.test", "--workloads", "C",
                         "--record-count", "1", "--operations", "1",
                         "--output-dir", output_directory]
            with patch.object(sys, "argv", arguments), patch("httpx.Client", return_value=self.client):
                with contextlib.redirect_stdout(io.StringIO()):
                    with self.assertRaises(SystemExit) as failure:
                        await complete_main()
            self.assertEqual(failure.exception.code, 1)
            self.assertEqual(list(Path(output_directory).iterdir()), [])
            self.assertEqual(self.entities, {})

    async def test_unified_entry_point_runs_only_validated_http_workloads(self):
        with tempfile.TemporaryDirectory() as output_directory:
            with patch("unified_benchmark_suite.os.makedirs"):
                suite = UnifiedBenchmark(iterations=1, warmup=0)
            suite.report_dir = output_directory
            suite.dataset_size = 1
            suite.operations = 2
            suite.endpoints["themis"] = "http://benchmark.test"
            with patch("httpx.Client", return_value=self.client), contextlib.redirect_stdout(io.StringIO()):
                await asyncio.to_thread(suite.run_all_benchmarks)
            report = json.loads((Path(output_directory) / "results.json").read_text(encoding="utf-8"))
            self.assertFalse(report["external_comparison_ready"])
            self.assertEqual(
                [item["workload"] for item in report["runs"][0]["results"]],
                ["A", "B", "C", "F"],
            )
            self.assertEqual(self.entities, {})

    async def test_unified_entry_point_does_not_report_failed_http_load(self):
        self.reject_after_write = True
        with tempfile.TemporaryDirectory() as output_directory:
            with patch("unified_benchmark_suite.os.makedirs"):
                suite = UnifiedBenchmark(iterations=1, warmup=0)
            suite.report_dir = output_directory
            suite.dataset_size = 1
            suite.operations = 2
            suite.endpoints["themis"] = "http://benchmark.test"
            with patch("httpx.Client", return_value=self.client), contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(httpx.HTTPStatusError):
                    await asyncio.to_thread(suite.run_all_benchmarks)
            self.assertFalse((Path(output_directory) / "results.json").exists())
            self.assertEqual(self.entities, {})

    async def test_complete_executor_reports_only_database_backed_results(self):
        client_type = type(self.client)
        transport = self.client._transport

        def client_factory(**kwargs):
            return client_type(transport=transport, **kwargs)

        with tempfile.TemporaryDirectory() as output_directory:
            executor = BenchmarkExecutor(
                output_directory, endpoint="http://benchmark.test",
                record_count=1, operations=2, repetitions=1,
                warmup_runs=0, iterations_per_run=2,
            )
            with patch("httpx.Client", side_effect=client_factory), contextlib.redirect_stdout(io.StringIO()):
                report = await executor.run_all_benchmarks()
            self.assertEqual(report["status"], "complete")
            self.assertEqual(report["scientific"]["read"]["analysis"]["sample_count"], 2)
            self.assertEqual(report["scientific"]["write"]["analysis"]["sample_count"], 2)
            self.assertEqual(len(report["ycsb_inspired"]["results"]), 4)
            self.assertNotIn("compliance", report)
            self.assertEqual(self.entities, {})

    async def test_complete_executor_failure_does_not_publish_report(self):
        self.reject_after_write = True
        with tempfile.TemporaryDirectory() as output_directory:
            output = Path(output_directory) / "result.json"
            arguments = ["run_complete_benchmarks.py", "--endpoint", "http://benchmark.test",
                         "--output", str(output), "--record-count", "1",
                         "--operations", "2", "--repetitions", "1",
                         "--warmup-runs", "0", "--iterations-per-run", "2"]
            with patch.object(sys, "argv", arguments), patch("httpx.Client", return_value=self.client):
                with contextlib.redirect_stdout(io.StringIO()):
                    with self.assertRaises(httpx.HTTPStatusError):
                        await executor_main()
            self.assertFalse(output.exists())
            self.assertEqual(self.entities, {})


if __name__ == "__main__":
    unittest.main()