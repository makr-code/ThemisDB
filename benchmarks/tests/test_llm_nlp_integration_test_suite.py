"""HTTP contract tests for the ThemisDB vector API benchmark."""

import json
import unittest

import httpx

from benchmarks.llm_nlp_integration_test_suite import VectorHttpBenchmark


class VectorHttpBenchmarkTests(unittest.TestCase):
    def setUp(self):
        self.vectors = {}
        self.requests = []

        def handle(request):
            self.requests.append(request)
            if request.url.path == "/vector/index/stats" and request.method == "GET":
                return httpx.Response(
                    200,
                    json={
                        "objectName": "vectors",
                        "dimension": 3,
                        "metric": "COSINE",
                        "hnswEnabled": True,
                    },
                )
            if request.url.path == "/vector/batch_insert" and request.method == "POST":
                body = json.loads(request.content)
                inserted = 0
                for item in body["items"]:
                    self.vectors[item["pk"]] = item["vector"]
                    inserted += 1
                return httpx.Response(200, json={"inserted": inserted, "errors": 0})
            if request.url.path == "/vector/search" and request.method == "POST":
                body = json.loads(request.content)
                nearest = next(
                    (pk for pk, vector in self.vectors.items() if vector == body["vector"]),
                    None,
                )
                return httpx.Response(
                    200,
                    json={"results": [{"pk": nearest, "distance": 0.0}], "count": 1},
                )
            if request.url.path == "/vector/by-filter" and request.method == "DELETE":
                prefix = json.loads(request.content)["prefix"]
                owned = [pk for pk in self.vectors if pk.startswith(prefix)]
                for pk in owned:
                    del self.vectors[pk]
                return httpx.Response(200, json={"deleted": len(owned), "method": "prefix"})
            return httpx.Response(404)

        self.client = httpx.Client(
            base_url="http://benchmark.test",
            transport=httpx.MockTransport(handle),
        )
        self.addCleanup(self.client.close)

    def _benchmark(self, **overrides):
        options = {
            "endpoint": "http://benchmark.test",
            "vector_count": 5,
            "query_count": 7,
            "batch_size": 2,
            "k": 3,
            "seed": 17,
            "dimension": 8,
            "timeout": 1,
            "token": None,
        }
        options.update(overrides)
        benchmark = VectorHttpBenchmark(**options)
        benchmark.client.close()
        benchmark.client = self.client
        return benchmark

    def test_runs_real_vector_http_contract_and_cleans_owned_vectors(self):
        self.vectors["unrelated"] = [0.0, 0.0, 1.0]
        report = self._benchmark().run()

        self.assertEqual(report["benchmark"], "themisdb_vector_http_search")
        self.assertEqual(report["workload"]["vectors_inserted"], 5)
        self.assertEqual(report["workload"]["queries"], 7)
        self.assertEqual(report["workload"]["self_match_queries"], 7)
        self.assertEqual(report["server"]["dimension"], 3)
        self.assertEqual(report["cleanup"]["deleted"], 5)
        self.assertFalse(report["llm_or_embedding_inference"])
        self.assertFalse(report["rag_pipeline"])
        self.assertEqual(list(self.vectors), ["unrelated"])
        self.assertEqual(
            [request.url.path for request in self.requests].count("/vector/batch_insert"),
            3,
        )
        self.assertEqual(
            [request.url.path for request in self.requests].count("/vector/search"),
            7,
        )
        self.assertEqual(
            [request.method for request in self.requests].count("DELETE"),
            1,
        )

    def test_http_failure_still_attempts_owned_data_cleanup(self):
        requests = []

        def fail_search(request):
            requests.append(request)
            if request.url.path == "/vector/index/stats":
                return httpx.Response(200, json={"dimension": 3})
            if request.url.path == "/vector/batch_insert":
                body = json.loads(request.content)
                for item in body["items"]:
                    self.vectors[item["pk"]] = item["vector"]
                return httpx.Response(200, json={"inserted": len(body["items"]), "errors": 0})
            if request.url.path == "/vector/search":
                return httpx.Response(503)
            if request.url.path == "/vector/by-filter":
                prefix = json.loads(request.content)["prefix"]
                owned = [pk for pk in self.vectors if pk.startswith(prefix)]
                for pk in owned:
                    del self.vectors[pk]
                return httpx.Response(200, json={"deleted": len(owned)})
            return httpx.Response(404)

        failing_client = httpx.Client(
            base_url="http://benchmark.test",
            transport=httpx.MockTransport(fail_search),
        )
        benchmark = self._benchmark()
        benchmark.client = failing_client
        with self.assertRaises(httpx.HTTPStatusError):
            benchmark.run()
        self.assertEqual(self.vectors, {})
        self.assertTrue(any(request.url.path == "/vector/by-filter" for request in requests))

    def test_incomplete_insert_is_not_reported_as_success(self):
        def incomplete_insert(request):
            if request.url.path == "/vector/index/stats":
                return httpx.Response(200, json={"dimension": 3})
            if request.url.path == "/vector/batch_insert":
                body = json.loads(request.content)
                return httpx.Response(200, json={"inserted": 1, "errors": len(body["items"]) - 1})
            if request.url.path == "/vector/by-filter":
                return httpx.Response(200, json={"deleted": 0})
            return httpx.Response(404)

        benchmark = self._benchmark()
        benchmark.client = httpx.Client(
            base_url="http://benchmark.test",
            transport=httpx.MockTransport(incomplete_insert),
        )
        with self.assertRaisesRegex(RuntimeError, "incomplete"):
            benchmark.run()

    def test_search_without_exact_self_match_is_not_reported_as_success(self):
        requests = []

        def missing_self_match(request):
            requests.append(request)
            if request.url.path == "/vector/index/stats":
                return httpx.Response(200, json={"dimension": 3})
            if request.url.path == "/vector/batch_insert":
                body = json.loads(request.content)
                return httpx.Response(200, json={"inserted": len(body["items"]), "errors": 0})
            if request.url.path == "/vector/search":
                return httpx.Response(200, json={"results": [{"pk": "other", "distance": 1.0}]})
            if request.url.path == "/vector/by-filter":
                return httpx.Response(200, json={"deleted": 5})
            return httpx.Response(404)

        benchmark = self._benchmark()
        benchmark.client = httpx.Client(
            base_url="http://benchmark.test",
            transport=httpx.MockTransport(missing_self_match),
        )
        with self.assertRaisesRegex(RuntimeError, "self-match validation failed"):
            benchmark.run()
        self.assertTrue(any(request.url.path == "/vector/by-filter" for request in requests))

    def test_invalid_workload_sizes_are_rejected(self):
        for field in ("vector_count", "query_count", "batch_size", "k"):
            with self.subTest(field=field):
                with self.assertRaises(ValueError):
                    self._benchmark(**{field: 0})


if __name__ == "__main__":
    unittest.main()
