#!/usr/bin/env python3
"""Verified serial AQL equality lookups with an owned secondary index."""

from __future__ import annotations

import argparse
import json
import os
import random
import statistics
import sys
import time
import uuid
from datetime import datetime, timezone
from pathlib import Path

import httpx

from benchmarks.benchmark_batch_operations import (
    BatchOperationsBenchmark, MAX_BATCH_SIZE, RANDOM_SEED, _percentile,
)
from benchmarks.core_http_mix import _ProcessResources

LOAD_OFFSET = 1000  # warmup keys use 0..49
WORKLOAD_ID = "themisdb:e2e/core-http-aql-index-equality@1"


def _row(index: int) -> dict:
    # Alphabetic tokens avoid the default PII detector's digit-pattern masking.
    remaining = index
    letters = []
    for _ in range(4):
        remaining, digit = divmod(remaining, 26)
        letters.append(chr(ord("a") + digit))
    return {"id": index, "lookup": "key" + "".join(reversed(letters)), "value": index * 2}


class AqlIndexBenchmark(BatchOperationsBenchmark):
    """Run point equality lookups against an index created before loading."""

    def __init__(self, endpoint: str, *, records: int, queries: int,
                 timeout: float = 30, token: str | None = None,
                 server_pid: int | None = None):
        super().__init__(endpoint, items=records, batch_sizes=(100,),
                         timeout=timeout, token=token)
        if isinstance(queries, bool) or not isinstance(queries, int) or not 1 <= queries <= 100_000:
            raise ValueError("queries must be an integer between 1 and 100000")
        if records > 100_000:
            raise ValueError("records must not exceed 100000")
        self.records, self.queries = records, queries
        self.server_pid = server_pid

    def _index_request(self, client: httpx.Client, action: str) -> None:
        body = {"table": self.collection, "column": "lookup"}
        if action == "create":
            # The create middleware requires field; the handler requires column.
            body.update({"field": "lookup", "unique": False})
        response = client.post(f"/index/{action}", json=body)
        response.raise_for_status()
        payload = response.json()
        if (response.status_code != 200 or not isinstance(payload, dict)
                or payload.get("success") is not True
                or payload.get("table") != self.collection or payload.get("column") != "lookup"
                or (action == "create" and payload.get("unique") is not False)):
            raise RuntimeError(f"Unexpected index {action} response")

    def _index_present(self, client: httpx.Client, expected: bool, *, empty: bool = False) -> None:
        response = client.get("/index/stats", params={"table": self.collection})
        response.raise_for_status()
        payload = response.json()
        if response.status_code != 200 or not isinstance(payload, list):
            raise RuntimeError("Unexpected index statistics response")
        if expected:
            if (len(payload) != 1 or not isinstance(payload[0], dict)
                    or payload[0].get("table") != self.collection
                    or payload[0].get("column") != "lookup"):
                raise RuntimeError("Owned lookup index not present in statistics")
            if empty and (type(payload[0].get("entry_count")) is not int
                          or payload[0]["entry_count"] != 0):
                raise RuntimeError("Owned index entries remain after entity cleanup")
        elif payload:
            raise RuntimeError("Owned index metadata remains after drop")

    def _put_chunk(self, client: httpx.Client, case_index: int, phase: str, start: int,
                   count: int, attempted_keys: list[str]) -> float:
        operations = []
        for index in range(start, start + count):
            key = self._key(case_index, phase, index)
            attempted_keys.append(key)
            operations.append({"op": "put", "key": key,
                               "blob": json.dumps(_row(index), separators=(",", ":"))})
        started = time.perf_counter()
        self._batch_request(client, operations)
        return (time.perf_counter() - started) * 1000

    def _verify_entity(self, client: httpx.Client, case_index: int, phase: str, index: int) -> None:
        key = self._key(case_index, phase, index)
        response = client.get(f"/entities/{key}")
        response.raise_for_status()
        try:
            payload = response.json()
            stored = json.loads(payload["blob"])
            if (response.status_code != 200 or not isinstance(payload, dict)
                    or payload.get("key") != key or not self._matches(stored, _row(index))):
                raise RuntimeError(f"Entity readback mismatch for {key}")
        except (ValueError, KeyError, TypeError) as exc:
            raise RuntimeError(f"Invalid entity readback for {key}") from exc

    @staticmethod
    def _matches(stored: object, expected: dict) -> bool:
        return (isinstance(stored, dict) and stored.keys() == expected.keys()
                and all(type(stored[k]) is type(v) and stored[k] == v
                        for k, v in expected.items()))

    def _query(self, client: httpx.Client, index: int, *, explain: bool = False) -> httpx.Response:
        lookup = _row(index)["lookup"]
        return client.post("/query/aql", json={
            "query": f'FOR doc IN {self.collection} FILTER doc.lookup == "{lookup}" RETURN doc',
            "optimize": True, "allow_full_scan": False, "explain": explain,
        })

    def _validate_query(self, response: httpx.Response, index: int, *, explain: bool = False) -> None:
        response.raise_for_status()
        try:
            payload = response.json()
            if (response.status_code != 200 or not isinstance(payload, dict)
                    or payload.get("table") != self.collection
                    or type(payload.get("count")) is not int or payload["count"] != 1
                    or not isinstance(payload.get("entities"), list)
                    or len(payload["entities"]) != 1
                    or not isinstance(payload["entities"][0], str)):
                raise RuntimeError("Unexpected AQL result envelope")
            stored = json.loads(payload["entities"][0])
            expected = _row(index)
            if not self._matches(stored, expected):
                raise RuntimeError("AQL result payload mismatch")
            if explain:
                plan = payload.get("plan")
                if (not isinstance(plan, dict) or plan.get("mode") != "index_optimized"
                        or plan.get("order") != [{"column": "lookup", "value": expected["lookup"]}]):
                    raise RuntimeError("AQL explain did not report the expected indexed predicate")
        except (ValueError, KeyError, TypeError) as exc:
            raise RuntimeError("Invalid AQL result JSON") from exc

    def _cleanup_owned(self, client: httpx.Client, attempted: list[str], index_attempted: bool) -> None:
        # Erase entities while the index metadata still exists: dropIndex removes
        # metadata, not the associated index entries.
        self._cleanup(client, attempted)
        for key in attempted:
            response = client.get(f"/entities/{key}")
            if response.status_code != 404:
                raise RuntimeError(f"Expected absent entity {key}, got HTTP {response.status_code}")
        if index_attempted:
            if attempted:
                self._index_present(client, True, empty=True)
            self._index_request(client, "drop")
            self._index_present(client, False)

    def run(self, output_file: str | Path) -> dict:
        output = Path(output_file)
        if output.exists():
            raise FileExistsError(f"Report already exists: {output}")
        resources = _ProcessResources(self.endpoint, self.server_pid)
        self.collection = "aql_index_" + uuid.uuid4().hex
        attempted: list[str] = []
        index_attempted = False
        phases = {}
        headers = {"Authorization": f"Bearer {self.token}"} if self.token else {}
        with httpx.Client(base_url=self.endpoint, timeout=self.timeout, headers=headers,
                          trust_env=False) as client:
            try:
                resources.begin("load_and_index_setup")
                started = time.perf_counter()
                index_attempted = True  # a timed-out create may still take effect
                self._index_request(client, "create")
                self._index_present(client, True)
                for start in range(LOAD_OFFSET, LOAD_OFFSET + self.records, MAX_BATCH_SIZE):
                    self._put_chunk(client, 0, "l", start,
                                    min(MAX_BATCH_SIZE, LOAD_OFFSET + self.records - start), attempted)
                for index in range(LOAD_OFFSET, LOAD_OFFSET + self.records):
                    self._verify_entity(client, 0, "l", index)
                phases["load_and_index_setup"] = time.perf_counter() - started
                resources.end("load_and_index_setup")

                resources.begin("warmup_and_plan_check")
                started = time.perf_counter()
                self._warmup(client, 0, 100, attempted)
                for index in range(LOAD_OFFSET, LOAD_OFFSET + min(self.records, 10)):
                    self._validate_query(self._query(client, index), index)
                self._validate_query(self._query(client, LOAD_OFFSET, explain=True),
                                     LOAD_OFFSET, explain=True)
                phases["warmup_and_plan_check"] = time.perf_counter() - started
                resources.end("warmup_and_plan_check")

                rng = random.Random(RANDOM_SEED)
                indices = [LOAD_OFFSET + rng.randrange(self.records) for _ in range(self.queries)]
                samples, responses = [], []
                resources.begin("measurement")
                started = time.perf_counter()
                for index in indices:
                    request_start = time.perf_counter()
                    response = self._query(client, index)
                    samples.append((time.perf_counter() - request_start) * 1000)
                    response.raise_for_status()
                    responses.append((response, index))
                measurement_seconds = time.perf_counter() - started
                if measurement_seconds <= 0:
                    raise RuntimeError("Measurement duration was not positive")
                resources.end("measurement")

                resources.begin("validation")
                started = time.perf_counter()
                for response, index in responses:
                    self._validate_query(response, index)
                for index in range(LOAD_OFFSET, LOAD_OFFSET + self.records):
                    self._verify_entity(client, 0, "l", index)
                phases["validation"] = time.perf_counter() - started
                resources.end("validation")
            finally:
                pending = sys.exc_info()[1]
                observation_error = None
                if pending is None:
                    try:
                        resources.begin("cleanup")
                    except RuntimeError as exc:
                        observation_error = exc
                started = time.perf_counter()
                try:
                    self._cleanup_owned(client, attempted, index_attempted)
                except (httpx.HTTPError, RuntimeError, ValueError) as exc:
                    if pending is not None:
                        raise RuntimeError(f"Benchmark error: {pending}; cleanup error: {exc}") from exc
                    if observation_error is not None:
                        raise RuntimeError(f"Observation error: {observation_error}; cleanup error: {exc}") from exc
                    raise
                phases["cleanup"] = time.perf_counter() - started
                if observation_error is not None:
                    raise observation_error
                if pending is None:
                    resources.end("cleanup")

        report = {
            "benchmark": "themisdb_aql_lookup_equality_http",
            "workload_id": WORKLOAD_ID,
            "completed_at_utc": datetime.now(timezone.utc).isoformat(),
            "endpoint": self.endpoint, "collection": self.collection,
            "dataset": {"kind": "synthetic", "records": self.records},
            "protocol": {
                "seed": RANDOM_SEED, "concurrency": 1,
                "index": "non-unique secondary index on lookup, created before load",
                "query": "AQL equality on lookup, uniform seeded keys, full-scan fallback disabled",
                "plan_check": "explain reports index_optimized and the lookup predicate before measurement",
                "warmup": "50 writes, 100 sequential reads, 200 seeded random reads, up to 10 AQL lookups",
                "validation": "every query result and every loaded entity",
                "cleanup": "delete and verify attempted entities and zero index entries before dropping metadata",
                "request_latency": "HTTP call wall time including serialization and response download",
                "throughput": "completed requests divided by measurement wall time including loop/status checks",
                "percentiles": "median p50; nearest-rank p95/p99 require 20/100 samples",
                "durability_measured": False, "external_comparison_ready": False,
            },
            "phase_seconds": {**phases, "measurement": measurement_seconds},
            "measurement": {
                "requests": len(samples), "requests_per_second": len(samples) / measurement_seconds,
                "mean_request_latency_ms": statistics.mean(samples),
                "p50_request_latency_ms": statistics.median(samples),
                "p95_request_latency_ms": _percentile(samples, 95),
                "p99_request_latency_ms": _percentile(samples, 99),
            },
            "resources": resources.report(),
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
    parser.add_argument("--endpoint", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--records", type=int, default=1000)
    parser.add_argument("--queries", type=int, default=10000)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--token-env")
    parser.add_argument("--server-pid", type=int)
    args = parser.parse_args(argv)
    benchmark = None
    try:
        token = os.environ.get(args.token_env) if args.token_env else None
        if args.token_env and not token:
            raise ValueError(f"Token environment variable {args.token_env!r} is missing or empty")
        benchmark = AqlIndexBenchmark(args.endpoint, records=args.records, queries=args.queries,
                                      timeout=args.timeout, token=token, server_pid=args.server_pid)
        report = benchmark.run(args.output)
        print(f"{report['measurement']['requests']} verified queries; report: {args.output}")
        return 0
    except (httpx.HTTPError, OSError, RuntimeError, ValueError) as exc:
        collection = benchmark.collection if benchmark and benchmark.collection else "not assigned"
        print(f"AQL benchmark failed (collection {collection}); "
              f"timed-out writes or index changes may still commit: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
