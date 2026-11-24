#!/usr/bin/env python3
"""Bounded, verified Entity API CRUD mix; not a reference benchmark."""

from __future__ import annotations

import argparse
import ipaddress
import json
import math
import os
import random
import statistics
import sys
import time
import uuid
from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import urlsplit

import httpx

try:
    import psutil
except ImportError:
    psutil = None

from benchmarks.benchmark_batch_operations import (
    BatchOperationsBenchmark, MAX_BATCH_SIZE, RANDOM_SEED, _entity, _percentile,
)

MAX_CONCURRENCY = 64
WORKLOAD_ID = "themisdb:e2e/core-http-crud-mix@1"


def _ensure_alive(process):
    try:
        if not process.is_running() or process.status() == psutil.STATUS_ZOMBIE:
            raise RuntimeError(f"Observed process {process.pid} exited or was reused")
    except (psutil.NoSuchProcess, psutil.ZombieProcess) as exc:
        raise RuntimeError(f"Observed process {process.pid} exited or was reused") from exc
    except (psutil.Error, OSError, NotImplementedError) as exc:
        raise RuntimeError(f"Cannot verify observed process {process.pid}") from exc


def _observed_process(pid: int):
    """Create a stable psutil process handle or reject an unavailable PID."""
    try:
        process = psutil.Process(pid)
        _ensure_alive(process)
        # Capture identity before writes; psutil's is_running() detects PID reuse.
        created = process.create_time()
        try:
            executable = process.exe()
        except (psutil.AccessDenied, NotImplementedError, OSError):
            executable = None
        return process, created, executable
    except (psutil.NoSuchProcess, psutil.ZombieProcess) as exc:
        raise RuntimeError(f"Observed process {pid} exited or was reused") from exc
    except (psutil.Error, OSError, NotImplementedError) as exc:
        raise RuntimeError(f"Cannot identify observed process {pid}") from exc


def _server_process(endpoint: str, pid: int):
    """Require the selected local PID to own the numeric loopback listener."""
    if psutil is None:
        raise RuntimeError("--server-pid requires psutil")
    if isinstance(pid, bool) or not isinstance(pid, int) or pid <= 0 or pid == os.getpid():
        raise ValueError("server PID must be a positive PID distinct from the client")
    parsed = urlsplit(endpoint)
    try:
        address = ipaddress.ip_address(parsed.hostname or "")
    except ValueError as exc:
        raise ValueError("--server-pid requires a numeric loopback endpoint") from exc
    if not address.is_loopback:
        raise ValueError("--server-pid requires a numeric loopback endpoint")
    port = parsed.port if parsed.port is not None else (443 if parsed.scheme == "https" else 80)
    if port == 0:
        raise ValueError("--server-pid requires a nonzero endpoint port")
    process, created, executable = _observed_process(pid)
    try:
        connections = process.net_connections(kind="tcp")
        matched = False
        for connection in connections:
            if connection.status != psutil.CONN_LISTEN or not connection.laddr:
                continue
            try:
                host = ipaddress.ip_address(connection.laddr.ip)
            except ValueError:
                continue
            if (connection.laddr.port == port and host.version == address.version
                    and (host == address or host.is_unspecified)):
                matched = True
                break
        if not matched:
            raise RuntimeError(f"Server PID {pid} is not listening on the requested endpoint")
        _ensure_alive(process)
    except (psutil.Error, OSError, NotImplementedError) as exc:
        raise RuntimeError(f"Cannot verify listener for server PID {pid}: {exc}") from exc
    return process, created, executable


def _snapshot(process):
    """Observe one process only; unavailable platform counters stay null."""
    try:
        _ensure_alive(process)

        def read(method):
            try:
                return method()
            except (psutil.NoSuchProcess, psutil.ZombieProcess) as exc:
                raise RuntimeError(f"Observed process {process.pid} exited or was reused") from exc
            except (psutil.Error, NotImplementedError, OSError, AttributeError):
                return None

        memory = read(lambda: process.memory_info())
        cpu = read(lambda: process.cpu_times())
        io = read(lambda: process.io_counters())

        def counter(obj, name):
            value = getattr(obj, name, None)
            return (value if not isinstance(value, bool) and isinstance(value, (int, float))
                    and math.isfinite(value) and value >= 0 else None)

        user, system = counter(cpu, "user"), counter(cpu, "system")
        result = {
            "monotonic_seconds": time.perf_counter(),
            "rss_bytes": counter(memory, "rss"),
            "virtual_memory_bytes": counter(memory, "vms"),
            "private_bytes": counter(memory, "private"),
            "peak_working_set_lifetime_bytes": counter(memory, "peak_wset"),
            "peak_commit_lifetime_bytes": counter(memory, "peak_pagefile"),
            "page_fault_count": counter(memory, "num_page_faults"),
            "cpu_seconds": user + system if user is not None and system is not None else None,
            "io_read_bytes": counter(io, "read_bytes"),
            "io_write_bytes": counter(io, "write_bytes"),
        }
        _ensure_alive(process)
        return result
    except (psutil.NoSuchProcess, psutil.ZombieProcess) as exc:
        raise RuntimeError(f"Observed process {process.pid} exited or was reused") from exc
    except (psutil.Error, OSError) as exc:
        raise RuntimeError(f"Cannot verify observed process {process.pid}") from exc


class _ProcessResources:
    """Capture phase endpoint snapshots, separately for client and optional server."""

    def __init__(self, endpoint: str, server_pid: int | None):
        self.observed = {}
        self.phases = {}
        self.before = {}
        if server_pid is not None:
            self.observed["server"] = _server_process(endpoint, server_pid)
        if psutil is not None:
            try:
                self.observed["client"] = _observed_process(os.getpid())
            except RuntimeError:
                # Client observation is optional; do not make an otherwise
                # valid workload depend on process-inspection permissions.
                pass
        self.phases = {name: [] for name in self.observed}

    def begin(self, phase: str):
        self.before = {name: _snapshot(info[0]) for name, info in self.observed.items()}

    def end(self, phase: str):
        for name, info in self.observed.items():
            before, after = self.before[name], _snapshot(info[0])

            def delta(key):
                first, last = before[key], after[key]
                return last - first if first is not None and last is not None and last >= first else None

            self.phases[name].append({
                "phase": phase, "before": before, "after": after,
                "wall_seconds": delta("monotonic_seconds"),
                "cpu_seconds": delta("cpu_seconds"),
                "io_read_bytes": delta("io_read_bytes"),
                "io_write_bytes": delta("io_write_bytes"),
            })
        self.before = {}

    def report(self):
        result = {"client": None, "server": None}
        for name, (process, created, executable) in self.observed.items():
            result[name] = {
                "pid": process.pid, "process_create_time": created, "executable": executable,
                "scope": f"{name} process only; excludes children and the other process",
                "limitations": ("Phase endpoint memory samples are not phase peaks. Windows peak "
                                "counters are process-lifetime high waters. Process I/O is not "
                                "physical disk I/O. GPU, energy and disk footprint are not measured."),
                "phases": self.phases[name],
            }
        return result


class CoreHttpMix(BatchOperationsBenchmark):
    """Measure one create/read/update/delete request per record."""

    def __init__(self, endpoint: str, *, records: int, timeout: float = 30,
                 token: str | None = None, server_pid: int | None = None,
                 concurrency: int = 1):
        super().__init__(endpoint, items=records, batch_sizes=(100,),
                         timeout=timeout, token=token)
        if (isinstance(concurrency, bool) or not isinstance(concurrency, int)
                or not 1 <= concurrency <= MAX_CONCURRENCY):
            raise ValueError(f"concurrency must be an integer between 1 and {MAX_CONCURRENCY}")
        self.records = records
        self.server_pid = server_pid
        self.concurrency = concurrency

    def _read(self, client: httpx.Client, key: str, expected: dict | None) -> None:
        response = client.get(f"/entities/{key}")
        if expected is None:
            if response.status_code != 404:
                raise RuntimeError(f"Expected absent entity {key}, got HTTP {response.status_code}")
            return
        self._check_read(response, key, expected)

    @staticmethod
    def _check_read(response: httpx.Response, key: str, expected: dict) -> None:
        response.raise_for_status()
        if response.status_code != 200:
            raise RuntimeError(f"Unexpected read HTTP status for {key}")
        try:
            payload = response.json()
            stored = json.loads(payload["blob"])
            if (not isinstance(payload, dict) or payload.get("key") != key
                    or not isinstance(stored, dict) or stored.keys() != expected.keys()
                    or any(type(stored[k]) is not type(v) or stored[k] != v
                           for k, v in expected.items())):
                raise RuntimeError(f"Readback payload mismatch for {key}")
        except (ValueError, KeyError, TypeError) as exc:
            raise RuntimeError(f"Invalid readback for {key}") from exc

    def _measure_operation(self, client: httpx.Client, operation: tuple) -> tuple:
        kind, key, value = operation
        request_start = time.perf_counter()
        if kind in ("create", "update"):
            response = client.put(f"/entities/{key}", json={
                "blob": json.dumps(value, separators=(",", ":"), allow_nan=False)})
        elif kind == "read":
            response = client.get(f"/entities/{key}")
        else:
            response = client.delete(f"/entities/{key}")
        elapsed = (time.perf_counter() - request_start) * 1000
        response.raise_for_status()
        if kind != "read":
            payload = response.json()
            if (response.status_code not in ((200, 201) if kind != "delete" else (200,))
                    or not isinstance(payload, dict) or payload.get("success") is not True
                    or payload.get("key") != key):
                raise RuntimeError(f"Unexpected {kind} response for {key}")
        return kind, key, value, response, elapsed

    def _measure_concurrently(self, client, operations, register, record):
        """Bound outstanding work and join every worker before returning or raising."""
        iterator = iter(operations)
        pending = set()
        failure = None
        with ThreadPoolExecutor(max_workers=self.concurrency) as executor:
            def submit_next():
                operation = next(iterator, None)
                if operation is not None:
                    register(operation)
                    pending.add(executor.submit(self._measure_operation, client, operation))

            for _ in range(min(self.concurrency, len(operations))):
                submit_next()
            while pending:
                finished, pending = wait(pending, return_when=FIRST_COMPLETED)
                for future in finished:
                    try:
                        record(future.result())
                    except Exception as exc:
                        if failure is None:
                            failure = exc
                if failure is not None:
                    break
                for _ in finished:
                    submit_next()
        # Executor shutdown waits for all in-flight calls, including on errors.
        if failure is not None:
            raise failure

    def run(self, output_file: str | Path) -> dict:
        output = Path(output_file)
        if output.exists():
            raise FileExistsError(f"Report already exists: {output}")
        resources = _ProcessResources(self.endpoint, self.server_pid)
        self.collection = "core_mix_" + uuid.uuid4().hex
        attempted: list[str] = []
        headers = {"Authorization": f"Bearer {self.token}"} if self.token else {}
        phase_seconds = {}
        samples = {name: [] for name in ("create", "read", "update", "delete")}
        with httpx.Client(base_url=self.endpoint, timeout=self.timeout, headers=headers,
                          trust_env=False) as client:
            try:
                resources.begin("load")
                started = time.perf_counter()
                for start in range(0, 3 * self.records, MAX_BATCH_SIZE):
                    self._put_chunk(client, 0, "l", start,
                                    min(MAX_BATCH_SIZE, 3 * self.records - start), attempted)
                for index in range(3 * self.records):
                    self._verify_entity(client, 0, "l", index)
                phase_seconds["load"] = time.perf_counter() - started
                resources.end("load")

                resources.begin("warmup")
                started = time.perf_counter()
                self._warmup(client, 0, 100, attempted)
                phase_seconds["warmup"] = time.perf_counter() - started
                resources.end("warmup")

                # Disjoint keys make shuffling reproducible without inter-operation dependencies.
                operations = []
                expected_final = {}
                for index in range(self.records):
                    create_key = self._key(0, "m", index)
                    read_key = self._key(0, "l", 3 * index)
                    update_key = self._key(0, "l", 3 * index + 1)
                    delete_key = self._key(0, "l", 3 * index + 2)
                    created = _entity(index)
                    updated = _entity(3 * index + 1)
                    updated["name"] += " updated"
                    operations.extend([
                        ("create", create_key, created),
                        ("read", read_key, _entity(3 * index)),
                        ("update", update_key, updated),
                        ("delete", delete_key, None),
                    ])
                    expected_final.update({create_key: created, read_key: _entity(3 * index),
                                           update_key: updated, delete_key: None})
                random.Random(RANDOM_SEED).shuffle(operations)
                measured_reads = []

                def register(operation):
                    kind, key, _ = operation
                    if kind == "create":
                        # A timed-out request may still commit.
                        attempted.append(key)

                def record(result):
                    kind, key, value, response, elapsed = result
                    samples[kind].append(elapsed)
                    if kind == "read":
                        measured_reads.append((response, key, value))

                resources.begin("measurement")
                started = time.perf_counter()
                if self.concurrency == 1:
                    for operation in operations:
                        register(operation)
                        record(self._measure_operation(client, operation))
                else:
                    self._measure_concurrently(client, operations, register, record)
                measurement_seconds = time.perf_counter() - started
                if measurement_seconds <= 0:
                    raise RuntimeError("Measurement duration was not positive")
                resources.end("measurement")

                resources.begin("validation")
                started = time.perf_counter()
                for response, key, expected in measured_reads:
                    self._check_read(response, key, expected)
                for key, expected in expected_final.items():
                    self._read(client, key, expected)
                phase_seconds["validation"] = time.perf_counter() - started
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
                    self._cleanup(client, attempted)
                    for key in attempted:
                        self._read(client, key, None)
                except (httpx.HTTPError, RuntimeError, ValueError) as exc:
                    if pending is not None:
                        raise RuntimeError(f"Benchmark error: {pending}; cleanup error: {exc}") from exc
                    if observation_error is not None:
                        raise RuntimeError(f"Observation error: {observation_error}; cleanup error: {exc}") from exc
                    raise
                phase_seconds["cleanup"] = time.perf_counter() - started
                if observation_error is not None:
                    raise observation_error
                if pending is None:
                    resources.end("cleanup")

        per_operation = {}
        for kind, values in samples.items():
            per_operation[kind] = {
                "requests": len(values),
                "request_latencies_ms": values,
                "mean_request_latency_ms": statistics.mean(values),
                "p50_request_latency_ms": statistics.median(values),
                "p95_request_latency_ms": _percentile(values, 95),
                "p99_request_latency_ms": _percentile(values, 99),
            }
        report = {
            "benchmark": "themisdb_entity_crud_mix_http",
            "workload_id": WORKLOAD_ID,
            "completed_at_utc": datetime.now(timezone.utc).isoformat(),
            "endpoint": self.endpoint, "collection": self.collection,
            "dataset": {"kind": "synthetic", "records_per_operation": self.records},
            "protocol": {
                "seed": RANDOM_SEED, "concurrency": self.concurrency,
                "max_in_flight_requests": min(self.concurrency, len(operations)),
                "arrival_model": "closed-loop; submit the next request after a completion, no fixed arrival rate",
                "mix": "equal create, read, update and delete on disjoint keys",
                "load": "batch put and readback of three records per operation set",
                "warmup": "50 batch writes, 100 sequential and 200 seeded random reads",
                "measurement": "one HTTP request per operation; response status and write envelopes checked",
                "validation": "measured reads plus final readback of all keys, including deleted keys",
                "cleanup": "batch delete attempted keys, then verify absence",
                "request_latency": "client wall time for HTTP call, including request serialization and response download",
                "throughput": "completed requests divided by measurement wall time including scheduling and envelope checks",
                "percentiles": "median p50; nearest-rank p95/p99 require 20/100 samples per operation",
                "durability_measured": False, "external_comparison_ready": False,
            },
            "phase_seconds": {**phase_seconds, "measurement": measurement_seconds},
            "measurement": {"requests": sum(map(len, samples.values())),
                            "requests_per_second": sum(map(len, samples.values())) / measurement_seconds,
                            "operations": per_operation},
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
    parser.add_argument("--concurrency", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--token-env")
    parser.add_argument("--server-pid", type=int)
    args = parser.parse_args(argv)
    benchmark = None
    try:
        token = os.environ.get(args.token_env) if args.token_env else None
        if args.token_env and not token:
            raise ValueError(f"Token environment variable {args.token_env!r} is missing or empty")
        benchmark = CoreHttpMix(args.endpoint, records=args.records, timeout=args.timeout,
                                token=token, server_pid=args.server_pid,
                                concurrency=args.concurrency)
        report = benchmark.run(args.output)
        print(f"{report['measurement']['requests']} verified requests; report: {args.output}")
        return 0
    except (httpx.HTTPError, OSError, RuntimeError, ValueError) as exc:
        collection = benchmark.collection if benchmark and benchmark.collection else "not assigned"
        print(f"CRUD mix failed (collection {collection}); "
              f"timed-out writes may still commit: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
