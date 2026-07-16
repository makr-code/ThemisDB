#!/usr/bin/env python3
"""Wave 3 benchmark suite for production-near full-function validation.

Implements:
- B3-A full-function critical workload profiles (read-heavy/write-heavy/mixed)
- B3-B scale & stress dimensions (dataset scale, parallelism, request mix)
- B3-C comparable reporting and lightweight regression guardrails
"""

from __future__ import annotations

import argparse
import json
import os
import statistics
import subprocess
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple


@dataclass(frozen=True)
class Guardrails:
    throughput_drop_percent: float
    p95_latency_increase_percent: float
    p99_latency_increase_percent: float


@dataclass(frozen=True)
class WorkloadDimension:
    dataset_scale: str
    benchmark_filter: str
    parallelism: int
    request_mix: str


@dataclass(frozen=True)
class WorkloadProfile:
    workload_id: str
    profile: str
    description: str
    benchmark_binary: str
    dimensions: Tuple[WorkloadDimension, ...]
    guardrails: Guardrails


def _default_profiles() -> List[WorkloadProfile]:
    shared_dims = (
        WorkloadDimension("small", ".*(1000|10000).*", 1, "read=90,write=10"),
        WorkloadDimension("medium", ".*(100000|50000).*", 4, "read=70,write=30"),
        WorkloadDimension("large", ".*(1000000|500000).*", 8, "read=50,write=50"),
    )
    return [
        WorkloadProfile(
            workload_id="critical_read_path",
            profile="read-heavy",
            description="YCSB + retrieval path coverage",
            benchmark_binary="bench_ycsb",
            dimensions=shared_dims,
            guardrails=Guardrails(8.0, 10.0, 15.0),
        ),
        WorkloadProfile(
            workload_id="critical_write_path",
            profile="write-heavy",
            description="Batch ingest + update path coverage",
            benchmark_binary="bench_batch_insert",
            dimensions=shared_dims,
            guardrails=Guardrails(10.0, 12.0, 18.0),
        ),
        WorkloadProfile(
            workload_id="critical_mixed_path",
            profile="mixed",
            description="Cross-component end-to-end mixed flow",
            benchmark_binary="bench_cross_functional_end_to_end",
            dimensions=shared_dims,
            guardrails=Guardrails(9.0, 10.0, 15.0),
        ),
    ]


def _percentile(values: List[float], percentile: float) -> float:
    if not values:
        return 0.0
    if len(values) == 1:
        return float(values[0])
    values = sorted(values)
    idx = (percentile / 100.0) * (len(values) - 1)
    lo = int(idx)
    hi = min(lo + 1, len(values) - 1)
    frac = idx - lo
    return values[lo] + (values[hi] - values[lo]) * frac


def _to_ms(value: float, time_unit: str) -> float:
    unit = time_unit.lower().strip()
    if unit == "s":
        return value * 1000.0
    if unit == "ms":
        return value
    if unit == "us":
        return value / 1000.0
    if unit == "ns":
        return value / 1_000_000.0
    return value


def _extract_metrics(benchmark_json: Dict[str, Any]) -> Dict[str, float]:
    rows = benchmark_json.get("benchmarks", [])
    latencies_ms: List[float] = []
    throughput: List[float] = []

    for row in rows:
        real_time = row.get("real_time")
        if isinstance(real_time, (int, float)):
            latencies_ms.append(_to_ms(float(real_time), str(row.get("time_unit", "ms"))))

        ips = row.get("items_per_second")
        if isinstance(ips, (int, float)):
            throughput.append(float(ips))

    return {
        "throughput_ops_per_sec": statistics.median(throughput) if throughput else 0.0,
        "p50_latency_ms": _percentile(latencies_ms, 50.0),
        "p95_latency_ms": _percentile(latencies_ms, 95.0),
        "p99_latency_ms": _percentile(latencies_ms, 99.0),
        "samples": float(len(latencies_ms)),
    }


def _load_profiles(path: Optional[Path]) -> List[WorkloadProfile]:
    if path is None:
        return _default_profiles()

    data = json.loads(path.read_text(encoding="utf-8"))
    profiles: List[WorkloadProfile] = []
    for p in data.get("profiles", []):
        dims = tuple(
            WorkloadDimension(
                dataset_scale=str(d["dataset_scale"]),
                benchmark_filter=str(d["benchmark_filter"]),
                parallelism=int(d["parallelism"]),
                request_mix=str(d["request_mix"]),
            )
            for d in p.get("dimensions", [])
        )
        g = p["guardrails"]
        profiles.append(
            WorkloadProfile(
                workload_id=str(p["workload_id"]),
                profile=str(p["profile"]),
                description=str(p.get("description", "")),
                benchmark_binary=str(p["benchmark_binary"]),
                dimensions=dims,
                guardrails=Guardrails(
                    throughput_drop_percent=float(g["throughput_drop_percent"]),
                    p95_latency_increase_percent=float(g["p95_latency_increase_percent"]),
                    p99_latency_increase_percent=float(g["p99_latency_increase_percent"]),
                ),
            )
        )
    return profiles


def _run_single(
    benchmark_bin: Path,
    dim: WorkloadDimension,
    repetitions: int,
    min_time: float,
    timeout_sec: int,
    out_dir: Path,
) -> Tuple[Dict[str, Any], float]:
    out_json = out_dir / f"{benchmark_bin.name}_{dim.dataset_scale}_p{dim.parallelism}.json"
    cmd = [
        str(benchmark_bin),
        f"--benchmark_filter={dim.benchmark_filter}",
        "--benchmark_format=json",
        f"--benchmark_out={out_json}",
        f"--benchmark_repetitions={repetitions}",
        "--benchmark_report_aggregates_only=true",
        f"--benchmark_min_time={min_time}",
    ]

    started = time.perf_counter()
    subprocess.run(cmd, check=True, timeout=timeout_sec, capture_output=True, text=True)
    elapsed = time.perf_counter() - started

    data = json.loads(out_json.read_text(encoding="utf-8"))
    return data, elapsed


def run_wave3(
    *,
    benchmark_bin_dir: Path,
    profiles: Iterable[WorkloadProfile],
    repetitions: int,
    min_time: float,
    timeout_sec: int,
    dry_run: bool,
) -> Dict[str, Any]:
    results: List[Dict[str, Any]] = []
    temp_out_dir = benchmark_bin_dir / "wave3_results"
    temp_out_dir.mkdir(parents=True, exist_ok=True)

    for profile in profiles:
        for dim in profile.dimensions:
            bench_bin = benchmark_bin_dir / profile.benchmark_binary
            if os.name == "nt" and bench_bin.suffix != ".exe":
                bench_bin = bench_bin.with_suffix(".exe")

            if dry_run:
                metrics = {
                    "throughput_ops_per_sec": 0.0,
                    "p50_latency_ms": 0.0,
                    "p95_latency_ms": 0.0,
                    "p99_latency_ms": 0.0,
                    "samples": 0.0,
                }
                elapsed = 0.0
                context = {}
            else:
                if not bench_bin.exists():
                    raise FileNotFoundError(f"Benchmark binary not found: {bench_bin}")
                raw, elapsed = _run_single(
                    bench_bin,
                    dim,
                    repetitions,
                    min_time,
                    timeout_sec,
                    temp_out_dir,
                )
                metrics = _extract_metrics(raw)
                context = raw.get("context", {})

            results.append(
                {
                    "workload_key": f"{profile.workload_id}:{dim.dataset_scale}:p{dim.parallelism}",
                    "workload_id": profile.workload_id,
                    "profile": profile.profile,
                    "description": profile.description,
                    "dataset_scale": dim.dataset_scale,
                    "parallelism": dim.parallelism,
                    "request_mix": dim.request_mix,
                    "benchmark_binary": profile.benchmark_binary,
                    "benchmark_filter": dim.benchmark_filter,
                    "metrics": metrics,
                    "resource_indicators": {
                        "run_elapsed_seconds": elapsed,
                        "runner_cpu_count": int(context.get("num_cpus", os.cpu_count() or 1)),
                        "runner_load_avg": context.get("load_avg", []),
                    },
                    "guardrails": {
                        "throughput_drop_percent": profile.guardrails.throughput_drop_percent,
                        "p95_latency_increase_percent": profile.guardrails.p95_latency_increase_percent,
                        "p99_latency_increase_percent": profile.guardrails.p99_latency_increase_percent,
                    },
                }
            )

    return {
        "suite": "wave3_full_function",
        "schema_version": "1.0",
        "timestamp": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "results": results,
        "summary": {
            "workloads_total": len(results),
            "profiles": sorted({r["profile"] for r in results}),
            "dataset_scales": sorted({r["dataset_scale"] for r in results}),
            "parallelism_levels": sorted({r["parallelism"] for r in results}),
        },
    }


def _pct_delta(baseline: float, current: float, higher_is_better: bool) -> float:
    if baseline == 0:
        return 0.0
    if higher_is_better:
        return ((baseline - current) / baseline) * 100.0
    return ((current - baseline) / baseline) * 100.0


def compare_runs(baseline: Dict[str, Any], current: Dict[str, Any]) -> Dict[str, Any]:
    base_map = {r["workload_key"]: r for r in baseline.get("results", [])}
    cur_map = {r["workload_key"]: r for r in current.get("results", [])}

    comparisons: List[Dict[str, Any]] = []
    blocking = 0

    for key, cur in cur_map.items():
        if key not in base_map:
            continue
        base = base_map[key]
        cur_m = cur["metrics"]
        base_m = base["metrics"]
        g = cur["guardrails"]

        throughput_reg = _pct_delta(
            float(base_m.get("throughput_ops_per_sec", 0.0)),
            float(cur_m.get("throughput_ops_per_sec", 0.0)),
            True,
        )
        p95_reg = _pct_delta(
            float(base_m.get("p95_latency_ms", 0.0)),
            float(cur_m.get("p95_latency_ms", 0.0)),
            False,
        )
        p99_reg = _pct_delta(
            float(base_m.get("p99_latency_ms", 0.0)),
            float(cur_m.get("p99_latency_ms", 0.0)),
            False,
        )

        breaches = {
            "throughput_drop": throughput_reg > float(g["throughput_drop_percent"]),
            "p95_latency": p95_reg > float(g["p95_latency_increase_percent"]),
            "p99_latency": p99_reg > float(g["p99_latency_increase_percent"]),
        }
        is_blocking = any(breaches.values())
        if is_blocking:
            blocking += 1

        comparisons.append(
            {
                "workload_key": key,
                "profile": cur["profile"],
                "regressions_percent": {
                    "throughput_drop": throughput_reg,
                    "p95_latency_increase": p95_reg,
                    "p99_latency_increase": p99_reg,
                },
                "breaches": breaches,
                "blocking": is_blocking,
            }
        )

    return {
        "suite": "wave3_full_function",
        "timestamp": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "summary": {
            "compared": len(comparisons),
            "blocking": blocking,
            "status": "failed" if blocking > 0 else "passed",
        },
        "comparisons": comparisons,
    }


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Wave 3 benchmark suite")
    sub = parser.add_subparsers(dest="command", required=True)

    run = sub.add_parser("run", help="Run Wave 3 suite")
    run.add_argument("--benchmark-bin-dir", required=True)
    run.add_argument("--output", required=True)
    run.add_argument("--profiles-file")
    run.add_argument("--repetitions", type=int, default=3)
    run.add_argument("--min-time", type=float, default=0.2)
    run.add_argument("--timeout-sec", type=int, default=300)
    run.add_argument("--dry-run", action="store_true")

    cmp_cmd = sub.add_parser("compare", help="Compare current run with baseline")
    cmp_cmd.add_argument("--baseline", required=True)
    cmp_cmd.add_argument("--current", required=True)
    cmp_cmd.add_argument("--output", required=True)

    return parser


def main(argv: Optional[List[str]] = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    if args.command == "run":
        profiles = _load_profiles(Path(args.profiles_file) if args.profiles_file else None)
        report = run_wave3(
            benchmark_bin_dir=Path(args.benchmark_bin_dir),
            profiles=profiles,
            repetitions=args.repetitions,
            min_time=args.min_time,
            timeout_sec=args.timeout_sec,
            dry_run=bool(args.dry_run),
        )
        out = Path(args.output)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"Wave 3 run report written: {out}")
        return 0

    if args.command == "compare":
        baseline = json.loads(Path(args.baseline).read_text(encoding="utf-8"))
        current = json.loads(Path(args.current).read_text(encoding="utf-8"))
        report = compare_runs(baseline, current)
        out = Path(args.output)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"Wave 3 comparison report written: {out}")
        return 1 if report["summary"]["status"] == "failed" else 0

    parser.print_help()
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
