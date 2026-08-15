#!/usr/bin/env python3
"""
Phase 5 Analytics Benchmark Runner
Orchestrates execution of analytics benchmarks with proper hygiene and reporting.

Usage:
    python3 benchmark_runner.py --build-dir build --output results.json
"""

import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class BenchmarkRunner:
    """Runs analytics benchmarks and parses results."""

    # Benchmark gate thresholds
    GATES = {
        "BCP_01_JITAggregationIterator": {
            "name": "Iterator Invalidation (JIT Aggregation)",
            "metric": "throughput",
            "unit": "ops/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BCP_02_AutoMLSpanAccess": {
            "name": "Span-Based Access (AutoML)",
            "metric": "throughput",
            "unit": "ops/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BCP_03_OLAPNestedLoop": {
            "name": "Nested Loop (OLAP)",
            "metric": "throughput",
            "unit": "ops/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BCP_04_PoolAcquireRelease": {
            "name": "Pool Acquire/Release",
            "metric": "throughput",
            "unit": "ops/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BCP_05_ConcurrentPoolAccess": {
            "name": "Concurrent Pool Access",
            "metric": "throughput",
            "unit": "ops/sec",
            "target": 0.5e6,  # Lower target due to thread synchronization
            "tolerance": 0.05,
        },
        "BCP_06_AggregationLockContention": {
            "name": "Aggregation Lock Contention",
            "metric": "throughput",
            "unit": "ops/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BM_TumblingWindow_IngestThroughput": {
            "name": "Tumbling Window Ingest",
            "metric": "throughput",
            "unit": "records/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BM_TumblingWindow_SustainedLoad_Bounded": {
            "name": "Tumbling Window (Bounded)",
            "metric": "throughput",
            "unit": "records/sec",
            "target": 0.8e6,
            "tolerance": 0.10,
        },
        "BM_TumblingWindow_FlushLatency": {
            "name": "Tumbling Window Flush",
            "metric": "latency_p99",
            "unit": "µs",
            "target": 1000,
            "tolerance": 0.05,
        },
        "BM_SlidingWindow_IngestThroughput": {
            "name": "Sliding Window Ingest",
            "metric": "throughput",
            "unit": "records/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BM_SlidingWindow_RecordLimitDrop": {
            "name": "Sliding Window (Record Limit)",
            "metric": "throughput",
            "unit": "records/sec",
            "target": 1e6,
            "tolerance": 0.10,
        },
        "BM_SessionWindow_IngestThroughput": {
            "name": "Session Window Ingest",
            "metric": "throughput",
            "unit": "records/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BM_SessionWindow_BoundedSessions": {
            "name": "Session Window (Bounded)",
            "metric": "throughput",
            "unit": "records/sec",
            "target": 0.8e6,
            "tolerance": 0.10,
        },
        "BM_ARG01_AggregationThroughput": {
            "name": "ARG-01: Aggregation Throughput",
            "metric": "throughput",
            "unit": "rows/sec",
            "target": 1e6,
            "tolerance": 0.05,
        },
        "BM_ARG02_WindowEvaluation": {
            "name": "ARG-02: Window Evaluation",
            "metric": "latency_p99",
            "unit": "µs",
            "target": 1000,
            "tolerance": 0.05,
        },
        "BM_ARG03_OlapPlanLookup": {
            "name": "ARG-03: OLAP Plan Lookup",
            "metric": "latency_p99",
            "unit": "µs",
            "target": 500,
            "tolerance": 0.05,
        },
        "BM_ARG04_AnomalyCheck": {
            "name": "ARG-04: Anomaly Check",
            "metric": "latency_p99",
            "unit": "µs",
            "target": 100,
            "tolerance": 0.05,
        },
        "BM_ARG05_CepPatternMatch": {
            "name": "ARG-05: CEP Pattern Match",
            "metric": "latency_p99",
            "unit": "µs",
            "target": 500,
            "tolerance": 0.05,
        },
        "BM_ARG06_ForecastInferenceStub": {
            "name": "ARG-06: Forecast Inference",
            "metric": "latency_p99",
            "unit": "µs",
            "target": 5000,
            "tolerance": 0.05,
        },
    }

    def __init__(self, build_dir: str, output_file: str):
        self.build_dir = Path(build_dir)
        self.output_file = Path(output_file)
        self.results = {}

    def run_benchmark(self, executable: str) -> Optional[Dict]:
        """Run a single benchmark executable and capture results."""
        bench_path = self.build_dir / executable
        if not bench_path.exists():
            print(f"Warning: {executable} not found at {bench_path}")
            return None

        print(f"Running {executable}...")
        try:
            result = subprocess.run(
                [str(bench_path)],
                capture_output=True,
                text=True,
                timeout=300,
            )
            return self.parse_output(result.stdout, result.stderr, executable)
        except subprocess.TimeoutExpired:
            print(f"Error: {executable} timed out after 300s")
            return None
        except Exception as e:
            print(f"Error running {executable}: {e}")
            return None

    def parse_output(self, stdout: str, stderr: str, executable: str) -> Optional[Dict]:
        """Parse Google Benchmark output to extract results."""
        # Google Benchmark JSON output format
        # Example: name          iterations ns/iter      seconds  
        lines = stdout.split("\n")
        results = {}

        # Simple pattern matching for benchmark output
        for line in lines:
            if re.search(r"\d+\s+\d+\s+ns/iter", line):
                parts = line.split()
                if len(parts) >= 3:
                    try:
                        name = parts[0]
                        time_ns = float(parts[1])
                        # Convert to ops/sec
                        ops_per_sec = 1e9 / time_ns if time_ns > 0 else 0
                        results[name] = ops_per_sec
                    except (ValueError, IndexError):
                        continue

        return results if results else None

    def validate_gates(self, results: Dict) -> Tuple[int, int]:
        """Validate results against performance gates."""
        passed = 0
        failed = 0

        for bench_name, gate_info in self.GATES.items():
            if bench_name in results:
                actual = results[bench_name]
                target = gate_info["target"]
                tolerance = gate_info["tolerance"]
                min_allowed = target * (1 - tolerance)

                if actual >= min_allowed:
                    print(f"✓ {gate_info['name']}: PASS ({actual:.2e} {gate_info['unit']})")
                    passed += 1
                else:
                    print(
                        f"✗ {gate_info['name']}: FAIL ({actual:.2e} < {min_allowed:.2e} {gate_info['unit']})"
                    )
                    failed += 1

        return passed, failed

    def run_all(self) -> int:
        """Run all benchmarks and generate report."""
        print("=" * 70)
        print("Phase 5 Analytics Benchmark Suite")
        print(f"Timestamp: {datetime.now().isoformat()}")
        print("=" * 70)

        benchmarks = [
            "bench_analytics_critical_paths_focused",
            "bench_streaming_window",
            "bench_analytics_release_gates",
        ]

        all_results = {}
        for bench in benchmarks:
            result = self.run_benchmark(bench)
            if result:
                all_results.update(result)

        print("\n" + "=" * 70)
        print("Performance Gate Validation")
        print("=" * 70)

        passed, failed = self.validate_gates(all_results)

        print("\n" + "=" * 70)
        print(f"Results: {passed} passed, {failed} failed")
        print("=" * 70)

        # Write results to JSON
        report = {
            "timestamp": datetime.now().isoformat(),
            "passed": passed,
            "failed": failed,
            "results": all_results,
            "gates": self.GATES,
        }

        with open(self.output_file, "w") as f:
            json.dump(report, f, indent=2)

        print(f"\nResults written to {self.output_file}")
        return 0 if failed == 0 else 1


def main():
    parser = argparse.ArgumentParser(
        description="Phase 5 Analytics Benchmark Runner"
    )
    parser.add_argument(
        "--build-dir", default="build", help="CMake build directory"
    )
    parser.add_argument(
        "--output", default="phase5_benchmark_results.json", help="Output JSON file"
    )

    args = parser.parse_args()

    runner = BenchmarkRunner(args.build_dir, args.output)
    return runner.run_all()


if __name__ == "__main__":
    sys.exit(main())
