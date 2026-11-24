"""Contract tests for the owned AQL/index workload; not performance evidence."""

import json
import contextlib
import io
import os
import re
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import httpx

from benchmarks.core_http_aql_index import AqlIndexBenchmark, _row, main
from benchmarks.core_http_mix import _ProcessResources, psutil
from benchmarks.tests.test_core_http_mix import listener


class AqlIndexBenchmarkTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.output = Path(self.directory.name) / "report.json"
        self.entities = {"unrelated:record": '{"keep":true}'}
        self.indexes = set()
        self.requests = []
        self.mode = None
        self.explain_seen = False

        def handle(request):
            self.requests.append(request)
            path = request.url.path
            if path in {"/index/create", "/index/drop"}:
                body = json.loads(request.content)
                index = (body["table"], body["column"])
                if path.endswith("create"):
                    self.assertEqual(body["field"], "lookup")
                    self.indexes.add(index)
                    if self.mode == "index_timeout":
                        raise httpx.ReadTimeout("index timeout", request=request)
                    return httpx.Response(200, json={"success": True, **index_dict(index),
                                                     "unique": False})
                if self.mode == "drop_failure":
                    return httpx.Response(503)
                self.indexes.discard(index)
                return httpx.Response(200, json={"success": True, **index_dict(index)})
            if path == "/index/stats":
                table = request.url.params["table"]
                entries = sum(1 for key in self.entities if key.startswith(table + ":"))
                if self.mode == "stale_index" and entries == 0:
                    entries = 1
                return httpx.Response(200, json=[{**index_dict(i), "entry_count": entries}
                                                for i in self.indexes if i[0] == table])
            if path == "/entities/batch":
                ops = json.loads(request.content)["operations"]
                if ops[0]["op"] == "delete" and self.mode == "cleanup_failure":
                    return httpx.Response(503)
                is_warmup = ops[0]["key"].split(":")[1].startswith("c0w")
                if (ops[0]["op"] == "put" and
                        ((self.mode == "partial_load" and not is_warmup)
                         or (self.mode == "partial_warmup" and is_warmup))):
                    self.entities[ops[0]["key"]] = ops[0]["blob"]
                    raise httpx.ReadTimeout("load timeout", request=request)
                for op in ops:
                    if op["op"] == "put":
                        self.entities[op["key"]] = op["blob"]
                    else:
                        self.entities.pop(op["key"], None)
                return httpx.Response(200, json={"success": True, "total": len(ops),
                                                "succeeded": len(ops), "failed": 0})
            if path.startswith("/entities/") and request.method == "GET":
                key = path.removeprefix("/entities/")
                if key not in self.entities:
                    return httpx.Response(404)
                return httpx.Response(200, json={"key": key, "blob": self.entities[key]})
            if path == "/query/aql":
                body = json.loads(request.content)
                self.assertFalse(body["allow_full_scan"])
                match = re.fullmatch(
                    r'FOR doc IN (aql_index_[a-f0-9]+) FILTER doc.lookup == "([^"]+)" RETURN doc',
                    body["query"])
                self.assertIsNotNone(match)
                table, lookup = match.groups()
                blobs = [blob for key, blob in self.entities.items()
                         if key.startswith(table + ":") and json.loads(blob).get("lookup") == lookup]
                result = {"table": table, "count": len(blobs), "entities": blobs}
                if body["explain"]:
                    self.explain_seen = True
                    result["plan"] = {"mode": "full_scan" if self.mode == "bad_plan"
                                      else "index_optimized",
                                      "order": [{"column": "lookup", "value": lookup}]}
                elif self.explain_seen and self.mode == "bad_result":
                    result["entities"] = ['{"id":true}']
                elif self.explain_seen and self.mode == "duplicate":
                    result["entities"] = blobs * 2
                    result["count"] = 2
                elif self.explain_seen and self.mode == "query_error":
                    return httpx.Response(503)
                elif self.explain_seen and self.mode == "query_unauthorized":
                    return httpx.Response(401)
                elif self.explain_seen and self.mode == "bad_count_type":
                    result["count"] = True
                elif self.explain_seen and self.mode == "invalid_json":
                    return httpx.Response(200, text="not JSON")
                return httpx.Response(200, json=result)
            return httpx.Response(405)

        self.transport = httpx.MockTransport(handle)
        self.client_type = httpx.Client

    def run_benchmark(self, records=2, queries=4, endpoint="http://benchmark.test", server_pid=None):
        def client(*args, **kwargs):
            return self.client_type(*args, transport=self.transport, **kwargs)
        with patch("benchmarks.core_http_aql_index.httpx.Client", side_effect=client):
            return AqlIndexBenchmark(endpoint, records=records, queries=queries,
                                     token="test-token", server_pid=server_pid).run(self.output)

    def test_verified_queries_and_index_cleanup_order(self):
        report = self.run_benchmark()
        self.assertEqual(json.loads(self.output.read_text(encoding="utf-8")), report)
        self.assertEqual(report["workload_id"], "themisdb:e2e/core-http-aql-index-equality@1")
        self.assertEqual(report["measurement"]["requests"], 4)
        self.assertAlmostEqual(report["measurement"]["requests_per_second"],
                               4 / report["phase_seconds"]["measurement"])
        self.assertIsNone(report["measurement"]["p95_request_latency_ms"])
        self.assertIsNone(report["measurement"]["p99_request_latency_ms"])
        self.assertIsNone(report["resources"]["server"])
        if psutil is not None:
            self.assertEqual(report["resources"]["client"]["pid"], os.getpid())
        else:
            self.assertIsNone(report["resources"]["client"])
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        self.assertFalse(self.indexes)
        self.assertTrue(all(r.headers["Authorization"] == "Bearer test-token"
                            for r in self.requests))
        drop = next(i for i, r in enumerate(self.requests) if r.url.path == "/index/drop")
        delete = next(i for i, r in enumerate(self.requests) if r.url.path == "/entities/batch"
                      and json.loads(r.content)["operations"][0]["op"] == "delete")
        self.assertLess(delete, drop)

    def test_bad_queries_and_plan_reject_report_and_cleanup(self):
        for mode in ("bad_plan", "bad_result", "duplicate", "query_error",
                     "query_unauthorized", "bad_count_type", "invalid_json"):
            with self.subTest(mode=mode):
                self.mode = mode
                self.explain_seen = False
                with self.assertRaises((RuntimeError, httpx.HTTPError)):
                    self.run_benchmark()
                self.assertFalse(self.output.exists())
                self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
                self.assertFalse(self.indexes)

    def test_setup_timeouts_clean_attempted_index_and_entities(self):
        for mode in ("index_timeout", "partial_load", "partial_warmup"):
            with self.subTest(mode=mode):
                self.mode = mode
                with self.assertRaises(httpx.ReadTimeout):
                    self.run_benchmark()
                self.assertFalse(self.output.exists())
                self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
                self.assertFalse(self.indexes)

    def test_failed_entity_cleanup_keeps_index_for_recovery(self):
        self.mode = "cleanup_failure"
        with self.assertRaisesRegex(RuntimeError, "Failed to clean up collection"):
            self.run_benchmark()
        self.assertFalse(self.output.exists())
        self.assertEqual(len(self.indexes), 1)
        self.assertFalse(any(r.url.path == "/index/drop" for r in self.requests))

    def test_failed_index_drop_prevents_report(self):
        self.mode = "drop_failure"
        with self.assertRaises(httpx.HTTPStatusError):
            self.run_benchmark()
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        self.assertEqual(len(self.indexes), 1)

    def test_stale_index_entries_prevent_drop_and_publication(self):
        self.mode = "stale_index"
        with self.assertRaisesRegex(RuntimeError, "index entries remain"):
            self.run_benchmark()
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        self.assertEqual(len(self.indexes), 1)

    def test_invalid_inputs_and_existing_output_rejected_before_io(self):
        self.output.write_text("existing", encoding="utf-8")
        with self.assertRaises(FileExistsError):
            self.run_benchmark()
        self.assertEqual(self.output.read_text(encoding="utf-8"), "existing")
        self.assertEqual(self.requests, [])
        for records, queries in ((0, 1), (100001, 1), (1, 0), (1, True), (1, 100001)):
            with self.assertRaises(ValueError):
                AqlIndexBenchmark("http://benchmark.test", records=records, queries=queries)

    def test_synthetic_lookup_tokens_are_unique_and_alphabetic(self):
        tokens = [_row(i)["lookup"] for i in range(101000)]
        self.assertEqual(len(set(tokens)), len(tokens))
        self.assertTrue(all(token.isascii() and token.isalpha() for token in tokens))

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_server_resources_cover_all_phases(self):
        process, port = listener(self)
        report = self.run_benchmark(endpoint=f"http://127.0.0.1:{port}", server_pid=process.pid)
        self.assertEqual(report["resources"]["server"]["pid"], process.pid)
        self.assertEqual(report["resources"]["client"]["pid"], os.getpid())
        for resource in report["resources"].values():
            self.assertEqual([phase["phase"] for phase in resource["phases"]],
                             ["load_and_index_setup", "warmup_and_plan_check", "measurement",
                              "validation", "cleanup"])
            self.assertTrue(all(phase["wall_seconds"] >= 0 for phase in resource["phases"]))

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_cli_accepts_server_pid(self):
        process, port = listener(self)

        def client(*args, **kwargs):
            return self.client_type(*args, transport=self.transport, **kwargs)

        with patch("benchmarks.core_http_aql_index.httpx.Client", side_effect=client), contextlib.redirect_stdout(io.StringIO()):
            code = main(["--endpoint", f"http://127.0.0.1:{port}", "--records", "2",
                         "--queries", "4", "--server-pid", str(process.pid), "--output", str(self.output)])
        self.assertEqual(code, 0)
        self.assertEqual(json.loads(self.output.read_text(encoding="utf-8"))["resources"]["server"]["pid"],
                         process.pid)

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_wrong_listener_rejected_before_index_creation(self):
        process, port = listener(self)
        with self.assertRaisesRegex(RuntimeError, "not listening"):
            self.run_benchmark(endpoint=f"http://127.0.0.2:{port}", server_pid=process.pid)
        self.assertEqual(self.requests, [])
        self.assertFalse(self.output.exists())
        self.assertFalse(self.indexes)

    @unittest.skipIf(psutil is None, "psutil not installed")
    def test_cleanup_still_runs_when_observation_fails(self):
        original = _ProcessResources.begin

        def fail_cleanup(observer, phase):
            if phase == "cleanup":
                raise RuntimeError("observer exited")
            return original(observer, phase)

        with patch.object(_ProcessResources, "begin", fail_cleanup):
            with self.assertRaisesRegex(RuntimeError, "observer exited"):
                self.run_benchmark()
        self.assertFalse(self.output.exists())
        self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
        self.assertFalse(self.indexes)


def index_dict(index):
    return {"table": index[0], "column": index[1]}


if __name__ == "__main__":
    unittest.main()
