"""HTTP contract checks for the entity batch benchmark."""

import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import httpx

from benchmarks.benchmark_batch_operations import BatchOperationsBenchmark


class BatchOperationsBenchmarkTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.output = Path(self.directory.name) / "report.json"
        self.entities = {"unrelated:record": '{"keep":true}'}
        self.requests = []
        self.mode = None
        self.failed_write = False
        self.cleanup_batches = []

        def handle(request):
            self.requests.append(request)
            if request.method == "POST" and request.url.path == "/entities/batch":
                operations = json.loads(request.content)["operations"]
                if operations[0]["op"] == "delete":
                    self.cleanup_batches.append(operations)
                    if self.mode == "partial_and_cleanup" and len(self.cleanup_batches) == 1:
                        return httpx.Response(503)
                    if self.mode == "rate_limit_cleanup" and len(self.cleanup_batches) == 1:
                        return httpx.Response(429, headers={"Retry-After": "0"})
                    for op in operations:
                        self.entities.pop(op["key"], None)
                else:
                    measured = "m" in operations[0]["key"].split(":", 1)[1]
                    if measured and not self.failed_write and self.mode in {
                        "partial", "partial_and_cleanup", "timeout"
                    }:
                        self.failed_write = True
                        self.entities[operations[0]["key"]] = operations[0]["blob"]
                        if self.mode == "timeout":
                            raise httpx.ReadTimeout("write timed out", request=request)
                        return httpx.Response(
                            200,
                            json={
                                "success": True,
                                "total": len(operations),
                                "succeeded": 1,
                                "failed": len(operations) - 1,
                                "errors": [{"index": 1, "error": "failed"}],
                            },
                        )
                    for op in operations:
                        self.entities[op["key"]] = op["blob"]
                return httpx.Response(
                    200,
                    json={
                        "success": True,
                        "total": len(operations),
                        "succeeded": len(operations),
                        "failed": 0,
                    },
                )
            if request.method == "GET" and request.url.path.startswith("/entities/"):
                key = request.url.path.removeprefix("/entities/")
                blob = self.entities.get(key)
                if blob is None:
                    return httpx.Response(404)
                if self.mode == "mismatch" and "m" in key.split(":", 1)[1]:
                    entity = json.loads(blob)
                    if entity["id"] == 1:
                        entity["id"] = True
                        blob = json.dumps(entity)
                return httpx.Response(200, json={"key": key, "blob": blob})
            return httpx.Response(405)

        self.transport = httpx.MockTransport(handle)
        self.client_type = httpx.Client

    def run_benchmark(self, *, items=5, batch_sizes=(2, 3), token=None):
        benchmark = BatchOperationsBenchmark(
            "http://benchmark.test", items=items, batch_sizes=batch_sizes, token=token
        )

        def make_client(*args, **kwargs):
            return self.client_type(*args, transport=self.transport, **kwargs)

        with patch(
            "benchmarks.benchmark_batch_operations.httpx.Client", side_effect=make_client
        ):
            return benchmark.run(self.output)

    def test_writes_distinct_batches_verifies_readback_and_cleans_owned_keys(self):
        report = self.run_benchmark(token="test-token")

        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        self.assertEqual(json.loads(self.output.read_text(encoding="utf-8")), report)
        self.assertEqual([case["batch_requests"] for case in report["results"]], [3, 2])
        self.assertEqual([case["items_verified"] for case in report["results"]], [5, 5])
        self.assertTrue(all(case["items_per_second"] > 0 for case in report["results"]))
        self.assertTrue(all(case["p95_batch_latency_ms"] is None for case in report["results"]))
        self.assertFalse(report["protocol"]["raid_replication_measured"])
        self.assertTrue(all(request.headers["Authorization"] == "Bearer test-token"
                            for request in self.requests))

        measured_keys = []
        for request in self.requests:
            if request.method != "POST":
                continue
            for operation in json.loads(request.content)["operations"]:
                if operation["op"] == "put" and "m" in operation["key"].split(":", 1)[1]:
                    measured_keys.append(operation["key"])
        self.assertEqual(len(measured_keys), 10)
        self.assertEqual(len(set(measured_keys)), 10)
        read_keys = {
            request.url.path.removeprefix("/entities/")
            for request in self.requests if request.method == "GET"
        }
        self.assertTrue(set(measured_keys).issubset(read_keys))

    def test_partial_success_is_rejected_and_all_attempted_keys_are_cleaned(self):
        self.mode = "partial"
        with self.assertRaisesRegex(RuntimeError, "did not confirm every batch operation"):
            self.run_benchmark(batch_sizes=(3,))
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        deleted = {op["key"] for batch in self.cleanup_batches for op in batch}
        measured = {
            op["key"]
            for request in self.requests if request.method == "POST"
            for op in json.loads(request.content)["operations"]
            if op["op"] == "put" and "m" in op["key"].split(":", 1)[1]
        }
        self.assertEqual(len(measured), 3)
        self.assertTrue(measured.issubset(deleted))

    def test_timeout_after_partial_commit_is_not_reported_as_success(self):
        self.mode = "timeout"
        with self.assertRaisesRegex(RuntimeError, "timed.out request may still commit"):
            self.run_benchmark(batch_sizes=(3,))
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_wrong_readback_type_prevents_report(self):
        self.mode = "mismatch"
        with self.assertRaisesRegex(RuntimeError, "Readback payload mismatch"):
            self.run_benchmark(batch_sizes=(3,))
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_cleanup_failure_preserves_original_error_and_attempts_other_chunks(self):
        self.mode = "partial_and_cleanup"
        with patch("benchmarks.benchmark_batch_operations.MAX_BATCH_SIZE", 10):
            with self.assertRaisesRegex(RuntimeError, "Benchmark error:.*cleanup error:"):
                self.run_benchmark(batch_sizes=(5,))
        self.assertFalse(self.output.exists())
        self.assertEqual(len(self.cleanup_batches), 6)

    def test_rate_limited_cleanup_retries_but_does_not_publish(self):
        self.mode = "rate_limit_cleanup"
        with self.assertRaisesRegex(RuntimeError, "retry succeeded but the run is rejected"):
            self.run_benchmark(batch_sizes=(5,))
        self.assertFalse(self.output.exists())
        self.assertEqual(len(self.cleanup_batches), 2)
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_existing_report_and_invalid_inputs_are_rejected_before_requests(self):
        self.output.write_text("existing", encoding="utf-8")
        benchmark = BatchOperationsBenchmark("http://benchmark.test", items=1, batch_sizes=(1,))
        with self.assertRaises(FileExistsError):
            benchmark.run(self.output)
        self.assertEqual(self.output.read_text(encoding="utf-8"), "existing")
        self.assertEqual(self.requests, [])
        for kwargs in (
            {"endpoint": "http://benchmark.test/path"},
            {"endpoint": "http://benchmark.test", "items": 0},
            {"endpoint": "http://benchmark.test", "batch_sizes": (1, 1)},
            {"endpoint": "http://benchmark.test", "batch_sizes": (10_001,)},
            {"endpoint": "http://benchmark.test", "timeout": float("nan")},
        ):
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError):
                BatchOperationsBenchmark(**kwargs)

    def test_repeated_runs_use_distinct_collections(self):
        benchmark = BatchOperationsBenchmark("http://benchmark.test", items=1, batch_sizes=(1,))

        def make_client(*args, **kwargs):
            return self.client_type(*args, transport=self.transport, **kwargs)

        with patch("benchmarks.benchmark_batch_operations.httpx.Client", side_effect=make_client):
            first = benchmark.run(self.output)
            second = benchmark.run(Path(self.directory.name) / "second.json")
        self.assertNotEqual(first["collection"], second["collection"])
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_interrupted_report_write_removes_partial_file(self):
        open_file = Path.open

        class InterruptingFile:
            def __init__(self, path, *args, **kwargs):
                self.handle = open_file(path, *args, **kwargs)

            def __enter__(self):
                return self

            def __exit__(self, *args):
                self.handle.close()

            def write(self, text):
                self.handle.write(text[:10])
                raise KeyboardInterrupt

        def interrupt_open(path, *args, **kwargs):
            return InterruptingFile(path, *args, **kwargs)

        with patch.object(Path, "open", interrupt_open):
            with self.assertRaises(KeyboardInterrupt):
                self.run_benchmark(items=1, batch_sizes=(1,))
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})


if __name__ == "__main__":
    unittest.main()
