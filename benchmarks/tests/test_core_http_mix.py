"""Contract tests, not performance or live-server evidence."""

import json
import contextlib
import io
import os
import subprocess
import sys
import tempfile
import threading
import unittest
from concurrent.futures import wait
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

import httpx

from benchmarks.core_http_mix import CoreHttpMix, _ProcessResources, _percentile, _snapshot, main, psutil


def listener(test):
    """Own a separate local listening process; no requests or benchmark run."""
    process = subprocess.Popen(
        [sys.executable, "-c",
         "import socket,sys; s=socket.socket(); s.bind(('127.0.0.1',0)); "
         "s.listen(); print(s.getsockname()[1],flush=True); sys.stdin.readline(); s.close()"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )

    def close():
        process.communicate(input="\n", timeout=10)

    test.addCleanup(close)
    return process, int(process.stdout.readline())


class CoreHttpMixTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.output = Path(self.directory.name) / "report.json"
        self.entities = {"unrelated:record": '{"keep":true}'}
        self.requests = []
        self.mode = None
        self.injected = False
        self.read_counts = {}

        def handle(request):
            self.requests.append(request)
            if request.url.path == "/entities/batch":
                operations = json.loads(request.content)["operations"]
                if self.mode in {"cleanup_failure", "timeout_and_cleanup"} and operations[0]["op"] == "delete":
                    return httpx.Response(503)
                phase = operations[0]["key"].split(":")[1]
                if (not self.injected and operations[0]["op"] == "put"
                        and ((self.mode == "load_partial" and phase.startswith("c0l"))
                             or (self.mode == "warmup_partial" and phase.startswith("c0w"))
                             or (self.mode == "load_partial_response" and phase.startswith("c0l")))):
                    self.injected = True
                    self.entities[operations[0]["key"]] = operations[0]["blob"]
                    if self.mode == "load_partial_response":
                        return httpx.Response(200, json={
                            "success": True, "total": len(operations), "succeeded": 1,
                            "failed": len(operations) - 1, "errors": [{"index": 1}],
                        })
                    raise httpx.ReadTimeout("setup timeout", request=request)
                for op in operations:
                    if op["op"] == "put":
                        self.entities[op["key"]] = op["blob"]
                    elif self.mode != "cleanup_lying":
                        self.entities.pop(op["key"], None)
                return httpx.Response(200, json={"success": True, "total": len(operations),
                                                "succeeded": len(operations), "failed": 0})
            key = request.url.path.removeprefix("/entities/")
            if request.method == "PUT":
                self.entities[key] = json.loads(request.content)["blob"]
                if self.mode in {"timeout", "timeout_and_cleanup"} and not self.injected and "m" in key.split(":")[1]:
                    self.injected = True
                    raise httpx.ReadTimeout("timeout", request=request)
                if self.mode == "bad_write" and "m" in key.split(":")[1]:
                    return httpx.Response(201, json={"success": False, "key": key})
                return httpx.Response(201, json={"success": True, "key": key})
            if request.method == "DELETE":
                if self.mode != "ignore_delete":
                    self.entities.pop(key, None)
                return httpx.Response(200, json={"success": True, "key": key})
            if request.method == "GET":
                self.read_counts[key] = self.read_counts.get(key, 0) + 1
                blob = self.entities.get(key)
                if blob is None:
                    return httpx.Response(404)
                if self.mode == "corrupt" and "m" in key.split(":")[1]:
                    blob = '{"id":true}'
                if self.mode == "bad_read" and "l" in key.split(":")[1] and self.read_counts[key] > 1:
                    blob = '{"id":true}'
                return httpx.Response(200, json={"key": key, "blob": blob})
            return httpx.Response(405)

        handler_lock = threading.RLock()

        def synchronized_handle(request):
            with handler_lock:
                return handle(request)

        self.transport = httpx.MockTransport(synchronized_handle)
        self.client_type = httpx.Client

    def run_mix(self, records=2, endpoint="http://benchmark.test", server_pid=None, concurrency=1):
        def client(*args, **kwargs):
            return self.client_type(*args, transport=self.transport, **kwargs)
        with patch("benchmarks.core_http_mix.httpx.Client", side_effect=client):
            return CoreHttpMix(endpoint, records=records, token="test-token",
                               server_pid=server_pid, concurrency=concurrency).run(self.output)

    def test_mix_checks_all_final_states_and_cleans_only_owned_keys(self):
        report = self.run_mix()
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        self.assertEqual(json.loads(self.output.read_text(encoding="utf-8")), report)
        self.assertEqual(report["workload_id"], "themisdb:e2e/core-http-crud-mix@1")
        self.assertEqual(report["measurement"]["requests"], 8)
        self.assertAlmostEqual(report["measurement"]["requests_per_second"],
                               8 / report["phase_seconds"]["measurement"])
        self.assertEqual(set(report["phase_seconds"]),
                         {"load", "warmup", "measurement", "validation", "cleanup"})
        for result in report["measurement"]["operations"].values():
            self.assertEqual(result["requests"], 2)
            self.assertIsNotNone(result["p50_request_latency_ms"])
            self.assertIsNone(result["p95_request_latency_ms"])
            self.assertIsNone(result["p99_request_latency_ms"])
        self.assertIsNone(report["resources"]["server"])
        if psutil is not None:
            self.assertEqual(report["resources"]["client"]["pid"], os.getpid())
            self.assertEqual([p["phase"] for p in report["resources"]["client"]["phases"]],
                             ["load", "warmup", "measurement", "validation", "cleanup"])
        else:
            self.assertIsNone(report["resources"]["client"])
        self.assertTrue(all(r.headers["Authorization"] == "Bearer test-token"
                            for r in self.requests))
        deleted = [r for r in self.requests if r.method == "DELETE"]
        self.assertEqual(len(deleted), 2)
        for request in deleted:
            self.assertTrue(any(r.method == "GET" and r.url.path == request.url.path
                                for r in self.requests))

    def test_delete_which_does_not_remove_record_rejects_report(self):
        self.mode = "ignore_delete"
        with self.assertRaisesRegex(RuntimeError, "Expected absent entity"):
            self.run_mix()
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_bad_readback_rejects_report_and_cleans(self):
        self.mode = "corrupt"
        with self.assertRaisesRegex(RuntimeError, "Readback payload mismatch"):
            self.run_mix()
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_timeout_after_write_cleans_attempted_key_without_publishing(self):
        self.mode = "timeout"
        with self.assertRaises(httpx.ReadTimeout):
            self.run_mix()
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_partial_load_and_warmup_timeouts_clean_all_attempted_keys(self):
        for mode, phase in (("load_partial", "c0l"), ("warmup_partial", "c0w")):
            with self.subTest(mode=mode):
                self.mode = mode
                self.injected = False
                self.requests.clear()
                with self.assertRaisesRegex(httpx.ReadTimeout, "setup timeout"):
                    self.run_mix()
                self.assertFalse(self.output.exists())
                self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
                put_batches = [
                    json.loads(request.content)["operations"] for request in self.requests
                    if request.method == "POST"
                    and json.loads(request.content)["operations"][0]["op"] == "put"
                ]
                cleanup = [
                    op["key"] for request in self.requests if request.method == "POST"
                    for op in json.loads(request.content)["operations"] if op["op"] == "delete"
                ]
                attempted = [op["key"] for batch in put_batches for op in batch]
                self.assertTrue(any(key.split(":")[1].startswith(phase) for key in attempted))
                self.assertCountEqual(attempted, cleanup)

    def test_cleanup_success_response_without_deletion_is_rejected(self):
        self.mode = "cleanup_lying"
        with self.assertRaisesRegex(RuntimeError, "Expected absent entity"):
            self.run_mix()
        self.assertFalse(self.output.exists())

    def test_partial_success_setup_response_cleans_every_attempted_key(self):
        self.mode = "load_partial_response"
        with self.assertRaisesRegex(RuntimeError, "did not confirm every batch operation"):
            self.run_mix()
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        batches = [json.loads(r.content)["operations"] for r in self.requests
                   if r.method == "POST"]
        attempted = [op["key"] for batch in batches for op in batch if op["op"] == "put"]
        cleaned = [op["key"] for batch in batches for op in batch if op["op"] == "delete"]
        self.assertEqual(len(attempted), 6)
        self.assertCountEqual(attempted, cleaned)

    def test_cleanup_failure_rejects_report_and_names_collection(self):
        self.mode = "cleanup_failure"
        with self.assertRaisesRegex(RuntimeError, r"Failed to clean up collection core_mix_"):
            self.run_mix()
        self.assertFalse(self.output.exists())

    def test_cleanup_failure_preserves_measurement_error(self):
        self.mode = "timeout_and_cleanup"
        with self.assertRaisesRegex(RuntimeError, r"Benchmark error: timeout; cleanup error:"):
            self.run_mix()
        self.assertFalse(self.output.exists())

    def test_p95_is_reported_from_twenty_actual_requests_per_operation(self):
        report = self.run_mix(records=20)
        for result in report["measurement"]["operations"].values():
            self.assertEqual(result["requests"], 20)
            self.assertIsNotNone(result["p95_request_latency_ms"])
            self.assertIsNone(result["p99_request_latency_ms"])

    def test_concurrent_mix_has_exact_counts_and_uses_multiple_workers(self):
        barrier = threading.Barrier(4, timeout=5)
        lock = threading.Lock()
        threads = set()
        calls = 0
        original = CoreHttpMix._measure_operation

        def coordinated(benchmark, client, operation):
            nonlocal calls
            with lock:
                calls += 1
                first_wave = calls <= 4
                threads.add(threading.get_ident())
            if first_wave:
                barrier.wait()
            return original(benchmark, client, operation)

        with patch.object(CoreHttpMix, "_measure_operation", coordinated):
            report = self.run_mix(records=5, concurrency=4)
        self.assertEqual(len(threads), 4)
        self.assertEqual(report["protocol"]["concurrency"], 4)
        self.assertEqual(report["workload_id"], "themisdb:e2e/core-http-crud-mix@1")
        self.assertEqual(report["protocol"]["max_in_flight_requests"], 4)
        self.assertIn("closed-loop", report["protocol"]["arrival_model"])
        self.assertEqual(report["measurement"]["requests"], 20)
        self.assertTrue(all(result["requests"] == 5
                            for result in report["measurement"]["operations"].values()))
        for result in report["measurement"]["operations"].values():
            latencies = result["request_latencies_ms"]
            self.assertEqual(len(latencies), 5)
            self.assertTrue(all(isinstance(value, float) and value >= 0 for value in latencies))
            self.assertAlmostEqual(result["mean_request_latency_ms"], sum(latencies) / len(latencies))
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_concurrent_timeout_waits_for_workers_before_cleanup(self):
        self.mode = "timeout"
        blocked = threading.Event()
        timed_out = threading.Event()
        observed_failure = threading.Event()
        release = threading.Event()
        calls = []
        calls_lock = threading.Lock()
        errors = []
        original = CoreHttpMix._measure_operation

        def coordinated(benchmark, client, operation):
            kind, key, _ = operation
            with calls_lock:
                calls.append(kind)
            if kind == "delete":
                blocked.set()
                if not release.wait(5):
                    raise RuntimeError("test worker was not released")
            elif kind == "create":
                if not blocked.wait(5):
                    raise RuntimeError("test worker did not block")
                try:
                    return original(benchmark, client, operation)
                except httpx.ReadTimeout:
                    timed_out.set()
                    raise
            return original(benchmark, client, operation)

        def observed_wait(*args, **kwargs):
            done, pending = wait(*args, **kwargs)
            if any(isinstance(future.exception(), httpx.ReadTimeout) for future in done):
                observed_failure.set()
            return done, pending

        def execute():
            try:
                self.run_mix(concurrency=2)
            except BaseException as exc:
                errors.append(exc)

        with patch.object(CoreHttpMix, "_measure_operation", coordinated), \
                patch("benchmarks.core_http_mix.wait", side_effect=observed_wait):
            runner = threading.Thread(target=execute)
            runner.start()
            try:
                self.assertTrue(timed_out.wait(5))
                self.assertTrue(observed_failure.wait(5))
                cleanup_requests = [
                    r for r in self.requests if r.url.path == "/entities/batch"
                    and json.loads(r.content)["operations"][0]["op"] == "delete"
                ]
                self.assertEqual(cleanup_requests, [])
            finally:
                release.set()
                runner.join(10)
            self.assertFalse(runner.is_alive())
        self.assertEqual(len(errors), 1)
        self.assertIsInstance(errors[0], httpx.ReadTimeout)
        self.assertCountEqual(calls, ["delete", "create"])
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        cleaned = [op["key"] for r in self.requests if r.url.path == "/entities/batch"
                   for op in json.loads(r.content)["operations"] if op["op"] == "delete"]
        self.assertEqual(len([key for key in cleaned if ":c0m" in key]), 1)

    def test_concurrent_response_and_final_state_failures_prevent_report(self):
        for mode in ("bad_write", "bad_read", "corrupt", "ignore_delete"):
            with self.subTest(mode=mode):
                self.mode = mode
                self.read_counts.clear()
                with self.assertRaises(RuntimeError):
                    self.run_mix(concurrency=3)
                self.assertFalse(self.output.exists())
                self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    def test_invalid_concurrency_rejected_before_io(self):
        for value in (0, -1, 65, True, 1.5, "2"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                CoreHttpMix("http://benchmark.test", records=2, concurrency=value)
        with contextlib.redirect_stderr(io.StringIO()):
            code = main(["--endpoint", "http://benchmark.test", "--records", "2",
                         "--concurrency", "0", "--output", str(self.output)])
        self.assertEqual(code, 1)
        self.assertEqual(self.requests, [])
        self.assertFalse(self.output.exists())

    def test_percentiles_use_actual_samples_and_minimum_counts(self):
        self.assertIsNone(_percentile(list(range(19)), 95))
        self.assertEqual(_percentile(list(range(20, 0, -1)), 95), 19)
        self.assertIsNone(_percentile(list(range(99)), 99))
        self.assertEqual(_percentile(list(range(100, 0, -1)), 99), 99)

    def test_existing_report_and_invalid_records_rejected_before_io(self):
        self.output.write_text("existing", encoding="utf-8")
        with self.assertRaises(FileExistsError):
            self.run_mix()
        self.assertEqual(self.output.read_text(encoding="utf-8"), "existing")
        self.assertEqual(self.requests, [])
        for records in (0, -1, True):
            with self.assertRaises(ValueError):
                CoreHttpMix("http://benchmark.test", records=records)

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_separate_listening_server_and_client_resources(self):
        process, port = listener(self)
        report = self.run_mix(endpoint=f"http://127.0.0.1:{port}", server_pid=process.pid)
        client, server = report["resources"]["client"], report["resources"]["server"]
        self.assertEqual((client["pid"], server["pid"]), (os.getpid(), process.pid))
        for resource in (client, server):
            self.assertEqual([p["phase"] for p in resource["phases"]],
                             ["load", "warmup", "measurement", "validation", "cleanup"])
            for phase in resource["phases"]:
                self.assertGreaterEqual(phase["wall_seconds"], 0)
                self.assertIsNotNone(phase["before"]["rss_bytes"])
                self.assertIsNotNone(phase["after"]["rss_bytes"])
                self.assertIn("peak_working_set_lifetime_bytes", phase["before"])
                self.assertIn("io_read_bytes", phase)

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_cli_accepts_server_pid(self):
        process, port = listener(self)

        def client(*args, **kwargs):
            return self.client_type(*args, transport=self.transport, **kwargs)

        with patch("benchmarks.core_http_mix.httpx.Client", side_effect=client), contextlib.redirect_stdout(io.StringIO()):
            code = main(["--endpoint", f"http://127.0.0.1:{port}", "--records", "2",
                         "--server-pid", str(process.pid), "--output", str(self.output)])
        self.assertEqual(code, 0)
        self.assertEqual(json.loads(self.output.read_text(encoding="utf-8"))["resources"]["server"]["pid"],
                         process.pid)

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_wrong_listener_address_port_or_pid_rejected_before_io(self):
        process, port = listener(self)
        for endpoint, pid in ((f"http://127.0.0.2:{port}", process.pid),
                              ("http://127.0.0.1:1", process.pid),
                              (f"http://127.0.0.1:{port}", os.getpid()),
                              (f"http://localhost:{port}", process.pid),
                              (f"http://example.com:{port}", process.pid),
                              ("http://127.0.0.1:0", process.pid),
                              (f"http://127.0.0.1:{port}", 0)):
            with self.subTest(endpoint=endpoint, pid=pid):
                with self.assertRaises((ValueError, RuntimeError)):
                    self.run_mix(endpoint=endpoint, server_pid=pid)
                self.assertEqual(self.requests, [])
                self.assertFalse(self.output.exists())

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_listener_permission_failure_rejected_before_io(self):
        process, port = listener(self)
        with patch.object(psutil.Process, "net_connections", side_effect=psutil.AccessDenied(process.pid)):
            with self.assertRaisesRegex(RuntimeError, "Cannot verify listener"):
                self.run_mix(endpoint=f"http://127.0.0.1:{port}", server_pid=process.pid)
        self.assertEqual(self.requests, [])
        self.assertFalse(self.output.exists())

    def test_missing_psutil_is_null_without_pid_and_rejected_with_pid(self):
        with patch("benchmarks.core_http_mix.psutil", None):
            report = self.run_mix()
            self.assertEqual(report["resources"], {"client": None, "server": None})
            self.output.unlink()
            self.requests.clear()
            with self.assertRaisesRegex(RuntimeError, "requires psutil"):
                self.run_mix(endpoint="http://127.0.0.1:1", server_pid=123)
            self.assertEqual(self.requests, [])

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_unavailable_client_process_is_null(self):
        with patch("benchmarks.core_http_mix._observed_process", side_effect=RuntimeError("denied")):
            report = self.run_mix()
        self.assertIsNone(report["resources"]["client"])
        self.assertIsNone(report["resources"]["server"])

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_unavailable_counters_remain_null(self):
        class FakeProcess:
            pid = 123
            def is_running(self):
                return True
            def status(self):
                return psutil.STATUS_RUNNING
            def memory_info(self):
                return SimpleNamespace(rss=123, vms=456)
            def cpu_times(self):
                raise psutil.AccessDenied(self.pid)
            def io_counters(self):
                raise NotImplementedError()
        sample = _snapshot(FakeProcess())
        self.assertEqual(sample["rss_bytes"], 123)
        for key in ("private_bytes", "peak_working_set_lifetime_bytes",
                    "cpu_seconds", "io_read_bytes", "io_write_bytes"):
            self.assertIsNone(sample[key])

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_zombie_process_is_not_accepted(self):
        process = SimpleNamespace(pid=123, is_running=lambda: True,
                                  status=lambda: psutil.STATUS_ZOMBIE)
        with self.assertRaisesRegex(RuntimeError, "exited or was reused"):
            _snapshot(process)

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_unavailable_and_decreasing_deltas_are_null(self):
        observer = _ProcessResources("http://benchmark.test", None)
        first = {"monotonic_seconds": 1.0, "cpu_seconds": None,
                 "io_read_bytes": 100, "io_write_bytes": None}
        last = {"monotonic_seconds": 2.0, "cpu_seconds": 2.0,
                "io_read_bytes": 90, "io_write_bytes": 0}
        with patch("benchmarks.core_http_mix._snapshot", side_effect=[first, last]):
            observer.begin("measurement")
            observer.end("measurement")
        phase = observer.report()["client"]["phases"][0]
        self.assertEqual(phase["wall_seconds"], 1.0)
        self.assertIsNone(phase["cpu_seconds"])
        self.assertIsNone(phase["io_read_bytes"])
        self.assertIsNone(phase["io_write_bytes"])

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_observer_exit_during_run_preserves_owned_cleanup(self):
        process, port = listener(self)
        from benchmarks.core_http_mix import _snapshot as original
        server_calls = 0

        def snapshot(observed):
            nonlocal server_calls
            if observed.pid == process.pid:
                server_calls += 1
                if server_calls == 2:
                    raise RuntimeError("server exited or was reused")
            return original(observed)

        with patch("benchmarks.core_http_mix._snapshot", side_effect=snapshot):
            with self.assertRaisesRegex(RuntimeError, "server exited"):
                self.run_mix(endpoint=f"http://127.0.0.1:{port}", server_pid=process.pid)
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_observation_failure_before_cleanup_still_cleans(self):
        original = _ProcessResources.begin

        def fail_cleanup(observer, phase):
            if phase == "cleanup":
                raise RuntimeError("observer exited")
            return original(observer, phase)

        with patch.object(_ProcessResources, "begin", fail_cleanup):
            with self.assertRaisesRegex(RuntimeError, "observer exited"):
                self.run_mix()
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_cleanup_error_preserves_observation_error(self):
        self.mode = "cleanup_failure"
        original = _ProcessResources.begin

        def fail_cleanup(observer, phase):
            if phase == "cleanup":
                raise RuntimeError("observer exited")
            return original(observer, phase)

        with patch.object(_ProcessResources, "begin", fail_cleanup):
            with self.assertRaisesRegex(RuntimeError, r"Observation error: observer exited; cleanup error:"):
                self.run_mix()
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
