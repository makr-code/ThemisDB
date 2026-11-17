#!/usr/bin/env python3
"""Measure ThemisDB's vector HTTP API with deterministic synthetic vectors.

This runner does not generate embeddings or exercise an LLM/RAG pipeline.
"""

from __future__ import annotations

import argparse
import json
import math
import random
import time
import uuid
from pathlib import Path
from typing import Any

import httpx


def percentile(samples: list[float], value: float) -> float:
    ordered = sorted(samples)
    return ordered[max(0, math.ceil(value * len(ordered)) - 1)]


class VectorHttpBenchmark:
    def __init__(
        self,
        endpoint: str,
        vector_count: int,
        query_count: int,
        batch_size: int,
        k: int,
        seed: int,
        dimension: int,
        timeout: float,
        token: str | None,
    ) -> None:
        if vector_count <= 0 or query_count <= 0 or batch_size <= 0 or k <= 0:
            raise ValueError("vector-count, query-count, batch-size, and k must be positive")
        if dimension <= 0:
            raise ValueError("dimension must be positive")

        headers = {"Accept": "application/json"}
        if token:
            headers["Authorization"] = f"Bearer {token}"
        self.client = httpx.Client(
            base_url=endpoint.rstrip("/"),
            headers=headers,
            timeout=timeout,
        )
        self.vector_count = vector_count
        self.query_count = query_count
        self.batch_size = batch_size
        self.k = k
        self.seed = seed
        self.dimension = dimension

    def _json_request(
        self,
        method: str,
        path: str,
        *,
        json_body: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        response = self.client.request(method, path, json=json_body)
        response.raise_for_status()
        try:
            payload = response.json()
        except ValueError as exc:
            raise RuntimeError(f"{method} {path} returned invalid JSON") from exc
        if not isinstance(payload, dict):
            raise RuntimeError(f"{method} {path} returned a non-object JSON response")
        return payload

    def run(self) -> dict[str, Any]:
        initial_stats = self._json_request("GET", "/vector/index/stats")
        configured_dimension = initial_stats.get("dimension")
        if not isinstance(configured_dimension, int) or configured_dimension < 0:
            raise RuntimeError("Vector index stats returned an invalid dimension")
        dimension = configured_dimension or self.dimension
        if configured_dimension and self.dimension != dimension:
            dimension = configured_dimension

        prefix = f"bench_{uuid.uuid4().hex}_"
        rng = random.Random(self.seed)
        vectors: list[list[float]] = []
        for _ in range(self.vector_count):
            vector = [rng.uniform(-1.0, 1.0) for _ in range(dimension)]
            norm = math.sqrt(sum(component * component for component in vector))
            vectors.append([component / norm for component in vector])
        primary_error: Exception | None = None
        report: dict[str, Any] | None = None

        try:
            insert_latencies_ms: list[float] = []
            inserted = 0
            insert_started = time.perf_counter()
            for offset in range(0, self.vector_count, self.batch_size):
                batch = [
                    {
                        "pk": f"{prefix}{index}",
                        "vector": vectors[index],
                        "fields": {"benchmark": "synthetic-vector-http"},
                    }
                    for index in range(offset, min(offset + self.batch_size, self.vector_count))
                ]
                started = time.perf_counter()
                result = self._json_request(
                    "POST",
                    "/vector/batch_insert",
                    json_body={"items": batch, "vector_field": "embedding"},
                )
                insert_latencies_ms.append((time.perf_counter() - started) * 1000)
                batch_inserted = result.get("inserted")
                batch_errors = result.get("errors")
                if batch_inserted != len(batch) or batch_errors != 0:
                    raise RuntimeError(
                        f"Vector batch insert was incomplete: inserted={batch_inserted}, "
                        f"errors={batch_errors}, expected={len(batch)}"
                    )
                inserted += batch_inserted
            insert_elapsed = time.perf_counter() - insert_started

            search_latencies_ms: list[float] = []
            self_matches = 0
            search_started = time.perf_counter()
            for index in range(self.query_count):
                vector_index = index % self.vector_count
                started = time.perf_counter()
                result = self._json_request(
                    "POST",
                    "/vector/search",
                    json_body={"vector": vectors[vector_index], "k": self.k},
                )
                search_latencies_ms.append((time.perf_counter() - started) * 1000)
                results = result.get("results")
                if not isinstance(results, list):
                    raise RuntimeError("Vector search response is missing its results array")
                if any(
                    isinstance(item, dict) and item.get("pk") == f"{prefix}{vector_index}"
                    for item in results
                ):
                    self_matches += 1
            if self_matches != self.query_count:
                raise RuntimeError(
                    "Vector self-match validation failed: "
                    f"{self_matches}/{self.query_count} searches returned their exact vector"
                )
            search_elapsed = time.perf_counter() - search_started

            report = {
                "benchmark": "themisdb_vector_http_search",
                "workload_data": "deterministic_synthetic_normalized_vectors",
                "llm_or_embedding_inference": False,
                "rag_pipeline": False,
                "server": {
                    "object_name": initial_stats.get("objectName"),
                    "dimension": dimension,
                    "metric": initial_stats.get("metric"),
                    "hnsw_enabled": initial_stats.get("hnswEnabled"),
                },
                "workload": {
                    "vectors_inserted": inserted,
                    "insert_batches": len(insert_latencies_ms),
                    "queries": len(search_latencies_ms),
                    "requested_k": self.k,
                    "self_match_queries": self_matches,
                    "seed": self.seed,
                },
                "insert": {
                    "elapsed_seconds": insert_elapsed,
                    "vectors_per_second": inserted / insert_elapsed if insert_elapsed else 0.0,
                    "batch_latency_ms_p50": percentile(insert_latencies_ms, 0.50),
                    "batch_latency_ms_p95": percentile(insert_latencies_ms, 0.95),
                },
                "search": {
                    "elapsed_seconds": search_elapsed,
                    "queries_per_second": len(search_latencies_ms) / search_elapsed
                    if search_elapsed
                    else 0.0,
                    "latency_ms_p50": percentile(search_latencies_ms, 0.50),
                    "latency_ms_p95": percentile(search_latencies_ms, 0.95),
                    "latency_ms_p99": percentile(search_latencies_ms, 0.99),
                },
                "external_comparison_ready": False,
            }
        except Exception as exc:
            primary_error = exc
        finally:
            try:
                cleanup = self._json_request(
                    "DELETE",
                    "/vector/by-filter",
                    json_body={"prefix": prefix},
                )
                if primary_error is None and cleanup.get("deleted") != self.vector_count:
                    raise RuntimeError(
                        "Vector benchmark cleanup was incomplete: "
                        f"deleted={cleanup.get('deleted')}, expected={self.vector_count}"
                    )
                if report is not None:
                    report["cleanup"] = {
                        "deleted": cleanup.get("deleted"),
                        "prefix": prefix,
                    }
            except Exception as cleanup_error:
                if primary_error is not None:
                    raise RuntimeError(
                        f"Benchmark failed ({primary_error}); cleanup also failed "
                        f"({cleanup_error})"
                    ) from cleanup_error
                raise

        if primary_error is not None:
            raise primary_error
        assert report is not None
        return report


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Benchmark ThemisDB vector insert/search HTTP operations using synthetic vectors."
    )
    parser.add_argument("--endpoint", required=True, help="ThemisDB base URL, e.g. http://localhost:8765")
    parser.add_argument("--vectors", type=int, default=1000, help="Number of vectors to insert")
    parser.add_argument("--queries", type=int, default=1000, help="Number of vector searches")
    parser.add_argument("--batch-size", type=int, default=100)
    parser.add_argument("--k", type=int, default=10)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument(
        "--dimension",
        type=int,
        default=32,
        help="Dimension used only when the server's vector index has not been initialized",
    )
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--token", help="Bearer token for authenticated ThemisDB deployments")
    parser.add_argument("--output", type=Path, required=True, help="New JSON report path")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    benchmark = VectorHttpBenchmark(
        endpoint=args.endpoint,
        vector_count=args.vectors,
        query_count=args.queries,
        batch_size=args.batch_size,
        k=args.k,
        seed=args.seed,
        dimension=args.dimension,
        timeout=args.timeout,
        token=args.token,
    )
    try:
        report = benchmark.run()
    finally:
        benchmark.client.close()

    if args.output.exists():
        raise FileExistsError(f"Refusing to overwrite existing report: {args.output}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
