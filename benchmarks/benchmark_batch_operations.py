#!/usr/bin/env python3
"""Measure ThemisDB entity batch PUTs over HTTP with verified readback."""

from __future__ import annotations

import argparse
import json
import math
import os
import random
import statistics
import sys
import time
import uuid
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import urlsplit

import httpx


MAX_BATCH_SIZE = 10_000
WARMUP_WRITES = 50
WARMUP_SEQUENTIAL_READS = 100
WARMUP_RANDOM_READS = 200
RANDOM_SEED = 42


def _entity(index: int) -> dict:
    return {
        "id": index,
        "name": f"User {index}",
        "email": f"user{index}@example.com",
        "age": 20 + index % 50,
        "status": "active" if index % 2 == 0 else "pending",
    }


def _percentile(samples: list[float], percentile: int) -> float | None:
    if len(samples) < 100 // (100 - percentile):
        return None
    ordered = sorted(samples)
    return ordered[math.ceil(percentile * len(ordered) / 100) - 1]


class BatchOperationsBenchmark:
    """Run isolated batch PUT cases against an explicitly selected HTTP origin."""

    def __init__(
        self,
        endpoint: str,
        *,
        items: int = 10_000,
        batch_sizes: tuple[int, ...] = (100, 500, 1000, 5000),
        timeout: float = 30.0,
        token: str | None = None,
    ):
        try:
            parsed = urlsplit(endpoint)
            _ = parsed.port
        except ValueError as exc:
            raise ValueError("endpoint must be a valid HTTP(S) origin") from exc
        if (
            parsed.scheme not in {"http", "https"}
            or not parsed.hostname
            or parsed.username is not None
            or parsed.password is not None
            or parsed.path not in {"", "/"}
            or parsed.query
            or parsed.fragment
        ):
            raise ValueError(
                "endpoint must be an HTTP(S) origin without credentials, path, query or fragment"
            )
        if isinstance(items, bool) or not isinstance(items, int) or items <= 0:
            raise ValueError("items must be a positive integer")
        if (
            not batch_sizes
            or any(
                isinstance(size, bool)
                or not isinstance(size, int)
                or not 1 <= size <= MAX_BATCH_SIZE
                for size in batch_sizes
            )
            or len(set(batch_sizes)) != len(batch_sizes)
        ):
            raise ValueError(f"batch sizes must be distinct integers between 1 and {MAX_BATCH_SIZE}")
        if (
            isinstance(timeout, bool)
            or not isinstance(timeout, (int, float))
            or not math.isfinite(timeout)
            or timeout <= 0
        ):
            raise ValueError("timeout must be finite and positive")
        if token is not None and not token:
            raise ValueError("token must not be empty")

        self.endpoint = endpoint.rstrip("/")
        self.items = items
        self.batch_sizes = tuple(batch_sizes)
        self.timeout = timeout
        self.token = token
        self.collection = ""

    def _key(self, case_index: int, phase: str, index: int) -> str:
        return f"{self.collection}:c{case_index}{phase}{index:012d}"

    @staticmethod
    def _batch_request(client: httpx.Client, operations: list[dict]) -> None:
        response = client.post("/entities/batch", json={"operations": operations})
        if response.status_code != 200:
            message = f"Batch request returned HTTP {response.status_code}: {response.text[:500]}"
            if response.status_code >= 300:
                raise httpx.HTTPStatusError(message, request=response.request, response=response)
            raise RuntimeError(message)
        try:
            result = response.json()
        except ValueError as exc:
            raise RuntimeError("ThemisDB returned invalid batch JSON") from exc
        if (
            not isinstance(result, dict)
            or result.get("success") is not True
            or type(result.get("total")) is not int
            or result["total"] != len(operations)
            or type(result.get("succeeded")) is not int
            or result["succeeded"] != len(operations)
            or type(result.get("failed")) is not int
            or result["failed"] != 0
            or result.get("errors", []) != []
        ):
            raise RuntimeError(f"ThemisDB did not confirm every batch operation: {result!r}")

    def _put_chunk(
        self,
        client: httpx.Client,
        case_index: int,
        phase: str,
        start: int,
        count: int,
        attempted_keys: list[str],
    ) -> float:
        operations = []
        for index in range(start, start + count):
            key = self._key(case_index, phase, index)
            operations.append(
                {
                    "op": "put",
                    "key": key,
                    "blob": json.dumps(_entity(index), separators=(",", ":"), allow_nan=False),
                }
            )
            attempted_keys.append(key)

        started = time.perf_counter()
        self._batch_request(client, operations)
        return (time.perf_counter() - started) * 1000

    def _verify_entity(
        self, client: httpx.Client, case_index: int, phase: str, index: int
    ) -> None:
        key = self._key(case_index, phase, index)
        response = client.get(f"/entities/{key}")
        response.raise_for_status()
        if response.status_code != 200:
            raise RuntimeError(f"Unexpected readback HTTP status for {key}: {response.status_code}")
        try:
            result = response.json()
            if not isinstance(result, dict) or result.get("key") != key:
                raise RuntimeError(f"Unexpected readback key for {key}")
            blob = result.get("blob")
            stored = json.loads(blob) if isinstance(blob, str) else None
            expected = _entity(index)
            if (
                not isinstance(stored, dict)
                or stored.keys() != expected.keys()
                or any(
                    type(stored[name]) is not type(value) or stored[name] != value
                    for name, value in expected.items()
                )
            ):
                raise RuntimeError(f"Readback payload mismatch for {key}")
        except ValueError as exc:
            raise RuntimeError(f"Invalid readback JSON for {key}") from exc

    def _warmup(
        self, client: httpx.Client, case_index: int, batch_size: int, attempted_keys: list[str]
    ) -> None:
        for start in range(0, WARMUP_WRITES, batch_size):
            self._put_chunk(
                client,
                case_index,
                "w",
                start,
                min(batch_size, WARMUP_WRITES - start),
                attempted_keys,
            )
        for index in range(WARMUP_SEQUENTIAL_READS):
            self._verify_entity(client, case_index, "w", index % WARMUP_WRITES)
        rng = random.Random(RANDOM_SEED)
        for _ in range(WARMUP_RANDOM_READS):
            self._verify_entity(client, case_index, "w", rng.randrange(WARMUP_WRITES))

    def _cleanup(self, client: httpx.Client, attempted_keys: list[str]) -> None:
        failures = []
        for start in range(0, len(attempted_keys), MAX_BATCH_SIZE):
            operations = [
                {"op": "delete", "key": key}
                for key in attempted_keys[start : start + MAX_BATCH_SIZE]
            ]
            try:
                self._batch_request(client, operations)
            except httpx.HTTPStatusError as exc:
                retry_after = exc.response.headers.get("Retry-After", "")
                if (
                    exc.response.status_code == 429
                    and retry_after.isascii()
                    and retry_after.isdecimal()
                    and len(retry_after) <= 2
                    and int(retry_after) <= 30
                ):
                    time.sleep(int(retry_after))
                    try:
                        self._batch_request(client, operations)
                    except (httpx.HTTPError, RuntimeError, ValueError) as retry_error:
                        failures.append(
                            f"cleanup batch {start // MAX_BATCH_SIZE + 1} was rate-limited "
                            f"and retry failed: {retry_error}"
                        )
                    else:
                        failures.append(
                            f"cleanup batch {start // MAX_BATCH_SIZE + 1} was rate-limited; "
                            "retry succeeded but the run is rejected"
                        )
                else:
                    failures.append(f"cleanup batch {start // MAX_BATCH_SIZE + 1}: {exc}")
            except (httpx.HTTPError, RuntimeError, ValueError) as exc:
                failures.append(f"cleanup batch {start // MAX_BATCH_SIZE + 1}: {exc}")
        if failures:
            raise RuntimeError(
                f"Failed to clean up collection {self.collection}; "
                f"some writes may remain: {'; '.join(failures)}"
            )

    def _run_case(self, client: httpx.Client, case_index: int, batch_size: int) -> dict:
        attempted_keys: list[str] = []
        latencies: list[float] = []
        try:
            started = time.perf_counter()
            self._warmup(client, case_index, batch_size, attempted_keys)
            warmup_seconds = time.perf_counter() - started

            started = time.perf_counter()
            for index in range(0, self.items, batch_size):
                latencies.append(
                    self._put_chunk(
                        client,
                        case_index,
                        "m",
                        index,
                        min(batch_size, self.items - index),
                        attempted_keys,
                    )
                )
            measurement_seconds = time.perf_counter() - started
            if measurement_seconds <= 0:
                raise RuntimeError("Batch measurement duration was not positive")

            started = time.perf_counter()
            for index in range(self.items):
                self._verify_entity(client, case_index, "m", index)
            verification_seconds = time.perf_counter() - started
        finally:
            pending_error = sys.exc_info()[1]
            started = time.perf_counter()
            try:
                self._cleanup(client, attempted_keys)
            except RuntimeError as cleanup_error:
                if pending_error is not None:
                    raise RuntimeError(
                        f"Benchmark error: {pending_error}; cleanup error: {cleanup_error}"
                    ) from cleanup_error
                raise
            cleanup_seconds = time.perf_counter() - started

        return {
            "batch_size": batch_size,
            "batch_requests": len(latencies),
            "items_written": self.items,
            "items_verified": self.items,
            "measurement_seconds": measurement_seconds,
            "items_per_second": self.items / measurement_seconds,
            "mean_batch_latency_ms": statistics.mean(latencies),
            "p50_batch_latency_ms": statistics.median(latencies),
            "p95_batch_latency_ms": _percentile(latencies, 95),
            "p99_batch_latency_ms": _percentile(latencies, 99),
            "warmup_seconds": warmup_seconds,
            "verification_seconds": verification_seconds,
            "cleanup_seconds": cleanup_seconds,
        }

    def run(self, output_file: str | Path) -> dict:
        output = Path(output_file)
        if output.exists():
            raise FileExistsError(f"Report already exists: {output}")

        self.collection = "batch_bench_" + uuid.uuid4().hex
        headers = {"Authorization": f"Bearer {self.token}"} if self.token else {}
        with httpx.Client(
            base_url=self.endpoint, timeout=self.timeout, headers=headers, trust_env=False
        ) as client:
            try:
                results = [
                    self._run_case(client, index, size)
                    for index, size in enumerate(self.batch_sizes)
                ]
            except (httpx.HTTPError, RuntimeError, ValueError) as exc:
                raise RuntimeError(
                    f"Batch benchmark failed for collection {self.collection}; "
                    f"a timed-out request may still commit: {exc}"
                ) from exc

        report = {
            "benchmark": "themisdb_entity_batch_put_http",
            "completed_at_utc": datetime.now(timezone.utc).isoformat(),
            "endpoint": self.endpoint,
            "collection": self.collection,
            "dataset": {"kind": "synthetic", "items_per_case": self.items},
            "protocol": {
                "operation": "POST /entities/batch (put)",
                "batch_sizes_in_order": list(self.batch_sizes),
                "concurrency": 1,
                "warmup_writes": WARMUP_WRITES,
                "warmup_sequential_reads": WARMUP_SEQUENTIAL_READS,
                "warmup_random_reads": WARMUP_RANDOM_READS,
                "random_seed": RANDOM_SEED,
                "throughput_scope": (
                    "wall time including batch preparation, HTTP requests and response validation; "
                    "excluding warmup, readback and cleanup"
                ),
                "batch_latency_scope": (
                    "individual HTTP request including serialization and response validation"
                ),
                "readback": "every measured entity, after the measurement window",
                "cleanup": "batch-delete only attempted keys from this run",
                "percentiles": (
                    "p50 is the median; p95/p99 use nearest-rank and require 20/100 batches"
                ),
                "durability_measured": False,
                "raid_replication_measured": False,
                "external_comparison_ready": False,
            },
            "results": results,
        }
        serialized = json.dumps(report, indent=2, allow_nan=False) + "\n"
        output.parent.mkdir(parents=True, exist_ok=True)
        handle = output.open("x", encoding="utf-8")
        written = False
        try:
            with handle:
                handle.write(serialized)
            written = True
        finally:
            if not written:
                output.unlink(missing_ok=True)
        return report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--endpoint", required=True, help="ThemisDB HTTP(S) origin")
    parser.add_argument("--output", required=True, help="New JSON report path")
    parser.add_argument("--items", type=int, default=10_000, help="Items per batch size")
    parser.add_argument(
        "--batch-sizes", type=int, nargs="+", default=[100, 500, 1000, 5000]
    )
    parser.add_argument("--timeout", type=float, default=30.0, help="HTTP timeout in seconds")
    parser.add_argument(
        "--token-env", help="Name of an environment variable containing a bearer token"
    )
    args = parser.parse_args(argv)

    try:
        token = None
        if args.token_env:
            token = os.environ.get(args.token_env)
            if not token:
                raise ValueError(f"Token environment variable {args.token_env!r} is missing or empty")
        benchmark = BatchOperationsBenchmark(
            args.endpoint,
            items=args.items,
            batch_sizes=tuple(args.batch_sizes),
            timeout=args.timeout,
            token=token,
        )
        report = benchmark.run(args.output)
    except (httpx.HTTPError, OSError, RuntimeError, ValueError) as exc:
        print(f"Batch benchmark failed: {exc}", file=sys.stderr)
        return 1

    for result in report["results"]:
        print(
            f"batch={result['batch_size']}: {result['items_per_second']:.2f} items/s, "
            f"{result['mean_batch_latency_ms']:.2f} ms/batch "
            f"({result['batch_requests']} requests, {result['items_verified']} verified)"
        )
    print(f"Report: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
