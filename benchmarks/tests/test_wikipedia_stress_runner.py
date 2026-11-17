"""Contract tests for the ThemisDB-backed Wikipedia entity workload."""

import asyncio
import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import httpx

from benchmarks.wikipedia_stress_runner import WikipediaStressTest


class WikipediaStressRunnerTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.entities = {}
        self.requests = []
        self.fail_cleanup = False
        self.active_puts = 0
        self.max_active_puts = 0

        async def handle(request):
            self.requests.append(request)
            key = request.url.path.removeprefix("/entities/")
            if request.method == "PUT":
                body = json.loads(request.content)
                self.active_puts += 1
                self.max_active_puts = max(self.max_active_puts, self.active_puts)
                try:
                    await asyncio.sleep(0.005)
                    self.entities[key] = body["blob"]
                    return httpx.Response(201, json={"success": True, "key": key})
                finally:
                    self.active_puts -= 1
            if request.method == "GET":
                if key not in self.entities:
                    return httpx.Response(404)
                return httpx.Response(200, json={"key": key, "blob": self.entities[key]})
            if request.method == "DELETE":
                if self.fail_cleanup:
                    return httpx.Response(503)
                self.entities.pop(key, None)
                return httpx.Response(200, json={"success": True})
            return httpx.Response(405)

        self.client = httpx.AsyncClient(
            base_url="http://benchmark.test",
            transport=httpx.MockTransport(handle),
        )
        self.addAsyncCleanup(self.client.aclose)

    def _dataset(self, directory):
        path = Path(directory) / "articles.jsonl"
        articles = [
            {"id": 1, "title": "Article One", "text": "first content"},
            {"id": 2, "title": "Article Two", "text": "second content"},
        ]
        path.write_text(
            "".join(json.dumps(article) + "\n" for article in articles),
            encoding="utf-8",
        )
        return path

    async def test_writes_reads_validates_and_cleans_only_owned_entities(self):
        with tempfile.TemporaryDirectory() as directory:
            data_file = self._dataset(directory)
            report_file = Path(directory) / "report.json"
            self.entities["unrelated:record"] = '{"keep":true}'
            runner = WikipediaStressTest(
                str(data_file), "http://benchmark.test", dataset_kind="wikipedia-dump",
                max_articles=2, concurrency=2,
            )
            with patch("benchmarks.wikipedia_stress_runner.httpx.AsyncClient", return_value=self.client):
                report = await runner.run_stress_test(str(report_file))

            self.assertEqual(report["dataset"]["articles_measured"], 2)
            self.assertEqual(report["measurements"]["write_operations"], 2)
            self.assertEqual(report["measurements"]["read_operations"], 2)
            self.assertFalse(report["protocol"]["full_text_search_measured"])
            self.assertFalse(report["external_comparison_ready"])
            self.assertEqual(report["protocol"]["concurrency"], 2)
            self.assertEqual(self.max_active_puts, 2)
            self.assertTrue(report_file.is_file())
            self.assertEqual(self.entities, {"unrelated:record": '{"keep":true}'})
            self.assertEqual([request.method for request in self.requests].count("PUT"), 2)
            self.assertEqual([request.method for request in self.requests].count("GET"), 2)
            self.assertEqual([request.method for request in self.requests].count("DELETE"), 2)

    async def test_default_concurrency_remains_serial(self):
        with tempfile.TemporaryDirectory() as directory:
            data_file = self._dataset(directory)
            report_file = Path(directory) / "report.json"
            runner = WikipediaStressTest(
                str(data_file), "http://benchmark.test", dataset_kind="synthetic",
                max_articles=2,
            )
            with patch("benchmarks.wikipedia_stress_runner.httpx.AsyncClient", return_value=self.client):
                report = await runner.run_stress_test(str(report_file))

            self.assertEqual(report["protocol"]["concurrency"], 1)
            self.assertEqual(self.max_active_puts, 1)

    async def test_http_failure_prevents_report_and_still_cleans_attempted_keys(self):
        with tempfile.TemporaryDirectory() as directory:
            data_file = self._dataset(directory)
            report_file = Path(directory) / "report.json"

            async def unavailable(request):
                self.requests.append(request)
                if request.method == "PUT":
                    return httpx.Response(503)
                return httpx.Response(404)

            failing_client = httpx.AsyncClient(
                base_url="http://benchmark.test",
                transport=httpx.MockTransport(unavailable),
            )
            with patch("benchmarks.wikipedia_stress_runner.httpx.AsyncClient", return_value=failing_client):
                runner = WikipediaStressTest(
                    str(data_file), "http://benchmark.test", dataset_kind="synthetic",
                )
                with self.assertRaises(httpx.HTTPStatusError):
                    await runner.run_stress_test(str(report_file))
            self.assertFalse(report_file.exists())
            self.assertEqual([request.method for request in self.requests], ["PUT", "DELETE"])
            await failing_client.aclose()

    async def test_cleanup_failure_prevents_success_report_and_attempts_all_deletes(self):
        with tempfile.TemporaryDirectory() as directory:
            data_file = self._dataset(directory)
            report_file = Path(directory) / "report.json"
            self.fail_cleanup = True
            runner = WikipediaStressTest(
                str(data_file), "http://benchmark.test", dataset_kind="synthetic",
                max_articles=2,
            )
            with patch("benchmarks.wikipedia_stress_runner.httpx.AsyncClient", return_value=self.client):
                with self.assertRaisesRegex(RuntimeError, "clean up"):
                    await runner.run_stress_test(str(report_file))
            self.assertFalse(report_file.exists())
            self.assertEqual([request.method for request in self.requests].count("DELETE"), 2)

    async def test_invalid_input_and_preexisting_report_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            data_file = self._dataset(directory)
            report_file = Path(directory) / "report.json"
            report_file.write_text("existing", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "HTTP\\(S\\) origin"):
                WikipediaStressTest(
                    str(data_file), "http://benchmark.test/path", dataset_kind="synthetic"
                )
            runner = WikipediaStressTest(
                str(data_file), "http://benchmark.test", dataset_kind="synthetic"
            )
            with self.assertRaises(FileExistsError):
                await runner.run_stress_test(str(report_file))
            self.assertEqual(self.requests, [])

    def test_invalid_concurrency_is_rejected(self):
        for concurrency in (0, -1, True, 1.5, "2"):
            with self.subTest(concurrency=concurrency):
                with self.assertRaisesRegex(ValueError, "concurrency must be a positive integer"):
                    WikipediaStressTest(
                        "articles.jsonl",
                        "http://benchmark.test",
                        dataset_kind="synthetic",
                        concurrency=concurrency,
                    )


if __name__ == "__main__":
    unittest.main()
