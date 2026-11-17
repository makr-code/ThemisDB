#!/usr/bin/env python3
"""Run real ThemisDB HTTP measurements and export a single complete report.

TPC-C, TPC-H, Sysbench and industry-compliance grades are not implemented by
this runner and are not represented in its output.
"""

import argparse
import asyncio
import json
from datetime import datetime, timezone
from pathlib import Path

from complete_benchmark_suite import CompleteBenchmarkSuite


class BenchmarkExecutor:
    """Run scientific read/write and YCSB-inspired A/B/C/F workloads."""

    def __init__(self, output_dir: str = "benchmark_results", *,
                 endpoint: str, record_count: int = 1000, operations: int = 10000,
                 repetitions: int = 10, warmup_runs: int = 5,
                 iterations_per_run: int = 100, seed: int = 42, timeout: float = 10.0):
        self.suite = CompleteBenchmarkSuite(
            output_dir, endpoint=endpoint, record_count=record_count,
            operations=operations, seed=seed, timeout=timeout,
        )
        self.repetitions = repetitions
        self.warmup_runs = warmup_runs
        self.iterations_per_run = iterations_per_run

    async def run_all_benchmarks(self) -> dict:
        """Return a report only after every measurement and cleanup succeeds."""
        scientific = {}
        for operation in ("read", "write"):
            scientific[operation] = await self.suite.run_scientific_benchmark(
                "ThemisDB", operation=operation, repetitions=self.repetitions,
                warmup_runs=self.warmup_runs, iterations_per_run=self.iterations_per_run,
            )
        ycsb = await self.suite.run_ycsb_benchmarks(
            ["ThemisDB"], ["A", "B", "C", "F"]
        )
        return {
            "schema_version": 1,
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "status": "complete",
            "external_comparison_ready": False,
            "scientific": scientific,
            "ycsb_inspired": ycsb,
        }


async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--endpoint", required=True, help="ThemisDB HTTP(S) origin")
    parser.add_argument("--output", type=Path, required=True, help="New JSON report path")
    parser.add_argument("--record-count", type=int, default=1000)
    parser.add_argument("--operations", type=int, default=10000)
    parser.add_argument("--repetitions", type=int, default=10)
    parser.add_argument("--warmup-runs", type=int, default=5)
    parser.add_argument("--iterations-per-run", type=int, default=100)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout", type=float, default=10.0)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("output already exists; choose a new result path")

    executor = BenchmarkExecutor(
        str(args.output.parent), endpoint=args.endpoint, record_count=args.record_count,
        operations=args.operations, repetitions=args.repetitions, warmup_runs=args.warmup_runs,
        iterations_per_run=args.iterations_per_run, seed=args.seed, timeout=args.timeout,
    )
    report = await executor.run_all_benchmarks()
    with args.output.open("x", encoding="utf-8") as output:
        json.dump(report, output, indent=2, allow_nan=False)
    print(f"Validated ThemisDB results saved to: {args.output}")


if __name__ == "__main__":
    asyncio.run(main())
