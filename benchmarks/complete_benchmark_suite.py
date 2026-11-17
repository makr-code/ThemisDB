"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            complete_benchmark_suite.py                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     388                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB HTTP-backed scientific and YCSB-inspired benchmark entry point.
These are local measurements, not official industry-standard comparisons.
"""

import asyncio
import argparse
import json
import math
import sys
from dataclasses import asdict
from pathlib import Path
from typing import List, Optional
from datetime import datetime
from urllib.parse import urlsplit

import httpx
from ycsb.themisdb_adapter import ThemisDBYCSBAdapter

# Import all benchmark modules
try:
    from scientific_benchmark_runner import ScientificConfig, ScientificBenchmarkRunner
    from standard_benchmarks import (
        YCSBBenchmark, YCSBWorkload,
        SysbenchWorkload,
    )
    from standard_benchmarks_integration import StandardBenchmarkRunner
except ImportError as e:
    print(f"Error importing benchmark modules: {e}")
    sys.exit(1)


class CompleteBenchmarkSuite:
    """Complete benchmark orchestrator"""
    
    def __init__(self, output_dir: str = "benchmark_results", *,
                 endpoint: Optional[str] = None, record_count: int = 1000,
                 operations: int = 10000, seed: int = 42, timeout: float = 10.0):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.endpoint = endpoint
        self.record_count = record_count
        self.operations = operations
        self.seed = seed
        self.timeout = timeout

    def _validate_http_run(self, databases: List[str]):
        if len(databases) != 1 or databases[0].lower() != "themisdb":
            raise ValueError("Only a single ThemisDB endpoint is supported")
        endpoint = urlsplit(self.endpoint or "")
        if (endpoint.scheme not in {"http", "https"} or not endpoint.hostname or
                endpoint.username is not None or endpoint.password is not None or
                endpoint.query or endpoint.fragment or endpoint.path not in {"", "/"}):
            raise ValueError("endpoint must be an HTTP(S) origin without credentials, path, query or fragment")
        if any(isinstance(value, bool) or not isinstance(value, int) or value <= 0
               for value in (self.record_count, self.operations)):
            raise ValueError("record count and operations must be positive integers")
        if not math.isfinite(self.timeout) or self.timeout <= 0:
            raise ValueError("timeout must be finite and positive")
    
    async def run_scientific_benchmark(self,
                                       database_name: str,
                                       operation: str = "read",
                                       repetitions: int = 10,
                                       warmup_runs: int = 5,
                                       iterations_per_run: int = 100) -> dict:
        """Measure validated HTTP reads or whole-record writes against ThemisDB."""
        self._validate_http_run([database_name])
        if operation not in {"read", "write"}:
            raise ValueError("scientific operation must be read or write")
        if (repetitions <= 0 or warmup_runs < 0 or iterations_per_run <= 0 or
                repetitions * iterations_per_run < 2):
            raise ValueError("measurement requires at least two samples and nonnegative warmup")
        
        print(f"\n{'='*80}")
        print(f"SCIENTIFIC BENCHMARK: {database_name}")
        print(f"{'='*80}")
        
        config = ScientificConfig(
            repetitions=repetitions,
            iterations_per_run=iterations_per_run,
            warmup_runs=warmup_runs,
            random_seed=self.seed,
            remove_outliers=False,
            confidence_level=0.95,
            operation_timeout_ms=self.timeout * 1000,
        )
        
        runner = ScientificBenchmarkRunner(config)
        with httpx.Client(base_url=self.endpoint, timeout=self.timeout, trust_env=False) as client:
            adapter = ThemisDBYCSBAdapter(
                client, "C" if operation == "read" else "A",
                record_count=self.record_count, seed=self.seed,
            )
            try:
                await adapter.load()

                async def test_operation():
                    await adapter(operation)

                analysis = await runner.run_benchmark(
                    database_name=database_name,
                    operation=operation,
                    test_fn=test_operation,
                )
            finally:
                await adapter.cleanup()
        
        return {
            "database": database_name,
            "operation": operation,
            "benchmark_kind": "http_entity_scientific",
            "external_comparison_ready": False,
            "endpoint": self.endpoint,
            "dataset": {"record_count": self.record_count, "seed": self.seed},
            "protocol": {"concurrency": 1, "key_distribution": "uniform",
                         "latency_scope": "HTTP client, thread dispatch, validation and HTTP round trips",
                         "setup_and_cleanup_in_measured_loop": False,
                         "outliers_removed": False},
            "config": asdict(config),
            "analysis": asdict(analysis),
            "hardware": {
                "scope": "benchmark client host",
                "platform": runner.hardware.platform,
                "processor": runner.hardware.processor,
                "physical_cpu_count": runner.hardware.cpu_count,
                "logical_cpu_count": runner.hardware.cpu_cores,
                "memory_total_gb": runner.hardware.memory_total_gb,
                "timestamp": runner.hardware.timestamp,
            } if runner.hardware is not None else None,
        }
    
    async def run_ycsb_benchmarks(self,
                                 databases: List[str],
                                 workloads: Optional[List[str]] = None,
                                 *, warmup_operations: int = 0):
        """Run real serial HTTP operations, not the upstream YCSB driver."""
        self._validate_http_run(databases)
        if (isinstance(warmup_operations, bool) or not isinstance(warmup_operations, int)
                or warmup_operations < 0):
            raise ValueError("warmup operations must be a nonnegative integer")
        
        if workloads is None:
            workloads = ["A", "C"]
        if not workloads or any(workload not in {"A", "B", "C", "F"} for workload in workloads):
            raise ValueError("Supported YCSB-inspired workloads are A, B, C and F")
        
        print(f"\n{'='*80}")
        print(f"YCSB BENCHMARKS")
        print(f"{'='*80}")
        
        runner = StandardBenchmarkRunner(str(self.output_dir))
        results = []
        with httpx.Client(base_url=self.endpoint, timeout=self.timeout, trust_env=False) as client:
            for workload in workloads:
                adapter = ThemisDBYCSBAdapter(
                    client, workload, record_count=self.record_count, seed=self.seed,
                )
                try:
                    await adapter.load()
                    if warmup_operations:
                        await YCSBBenchmark(databases[0]).run_workload(
                            YCSBWorkload(workload), warmup_operations, adapter, seed=self.seed
                        )
                    result = await runner.run_ycsb_suite(
                        databases, [YCSBWorkload(workload)],
                        operation_adapters={databases[0]: adapter},
                        num_operations=self.operations, seed=self.seed,
                    )
                    results.extend(result["results"])
                finally:
                    await adapter.cleanup()

        return {
            "benchmark": "YCSB-inspired",
            "benchmark_kind": "ycsb_inspired",
            "external_comparison_ready": False,
            "endpoint": self.endpoint,
            "dataset": {"record_count": self.record_count, "seed": self.seed},
            "protocol": {"concurrency": 1, "key_distribution": "uniform",
                         "write_semantics": "whole_record_replacement", "rmw_atomic": False,
                         "latency_scope": "HTTP client, thread dispatch, validation and HTTP round trips",
                         "setup_and_cleanup_in_measured_loop": False,
                         "warmup_operations_per_workload": warmup_operations},
            "results": results,
        }
    
    async def run_tpcc_benchmarks(self,
                                 databases: List[str],
                                 scales: Optional[List[str]] = None):
        """Run TPC-C benchmarks"""
        
        if scales is None:
            scales = ["small", "medium"]
        
        print(f"\n{'='*80}")
        print(f"TPC-C BENCHMARKS (OLTP)")
        print(f"{'='*80}")
        
        runner = StandardBenchmarkRunner()
        result = await runner.run_tpcc_suite(databases, scales)
        
        return result
    
    async def run_tpch_benchmarks(self,
                                 databases: List[str],
                                 scale_factors: Optional[List[int]] = None):
        """Run TPC-H benchmarks"""
        
        if scale_factors is None:
            scale_factors = [1]
        
        print(f"\n{'='*80}")
        print(f"TPC-H BENCHMARKS (OLAP)")
        print(f"{'='*80}")
        
        runner = StandardBenchmarkRunner()
        result = await runner.run_tpch_suite(databases, scale_factors)
        
        return result
    
    async def run_sysbench_benchmarks(self,
                                     databases: List[str],
                                     workloads: Optional[List[str]] = None):
        """Run Sysbench benchmarks"""
        
        if workloads is None:
            workloads = ["oltp_read_write", "oltp_read_only"]
        
        print(f"\n{'='*80}")
        print(f"SYSBENCH BENCHMARKS")
        print(f"{'='*80}")
        
        runner = StandardBenchmarkRunner()
        
        # Convert string workloads to enum
        sysbench_workloads = [
            SysbenchWorkload[w.upper()] for w in workloads
        ]
        
        result = await runner.run_sysbench_benchmarks(databases, sysbench_workloads)
        
        return result
    
    async def run_full_suite(self, databases: List[str]) -> dict:
        """Run complete benchmark suite"""
        
        print(f"\n{'='*80}")
        print(f"COMPLETE BENCHMARK SUITE")
        print(f"Databases: {', '.join(databases)}")
        print(f"{'='*80}")
        
        runner = StandardBenchmarkRunner(str(self.output_dir))
        
        all_results = await runner.run_full_suite(databases)
        
        # Export combined results
        combined_export = {
            "timestamp": datetime.now().isoformat(),
            "mode": "full",
            "databases": databases,
            "benchmarks": all_results,
            "summary": runner._generate_summary(),
        }
        
        export_file = self.output_dir / "complete_benchmark_suite.json"
        with open(export_file, 'w') as f:
            json.dump(combined_export, f, indent=2, default=str)
        
        print(f"\n✓ Complete results exported to: {export_file}")
        
        return combined_export
    
    def print_summary(self, results: dict):
        """Print results summary"""
        
        print(f"\n{'='*80}")
        print(f"BENCHMARK RESULTS SUMMARY")
        print(f"{'='*80}")
        
        if "summary" in results:
            summary = results["summary"]
            print(f"\nCompliance Summary:")
            for benchmark_name, stats in summary.get("benchmarks", {}).items():
                print(f"\n{benchmark_name.upper()}:")
                print(f"  Comparisons: {stats['total_comparisons']}")
                print(f"  Meets Expectation: {stats['meets_expectation']}/{stats['total_comparisons']}")
                print(f"  Compliance: {stats['compliance_pct']:.1f}%")
                print(f"  Avg Ratio: {stats['average_ratio']:.2f}x")
        
        print(f"\n{'='*80}")


async def main():
    """CLI Entry Point"""
    
    parser = argparse.ArgumentParser(
        description="Complete Benchmark Suite (Scientific + Industry Standards)"
    )
    
    parser.add_argument(
        "--mode",
        choices=["scientific", "ycsb", "tpcc", "tpch", "sysbench", "standards", "full"],
        default="ycsb",
        help="Benchmark mode to run"
    )
    
    parser.add_argument(
        "--database",
        type=str,
        help="Single database to benchmark (for scientific mode)"
    )
    
    parser.add_argument(
        "--databases",
        nargs="+",
        default=["ThemisDB"],
        help="Only ThemisDB is currently supported"
    )
    parser.add_argument("--endpoint", help="ThemisDB HTTP(S) origin")
    parser.add_argument("--record-count", type=int, default=1000)
    parser.add_argument("--operations", type=int, default=10000)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--operation", choices=["read", "write"], default="read",
                        help="Operation for scientific mode")
    parser.add_argument("--iterations-per-run", type=int, default=100)
    
    parser.add_argument(
        "--workloads",
        nargs="+",
        help="YCSB workloads (A-F) or Sysbench workloads"
    )
    
    parser.add_argument(
        "--scale",
        choices=["small", "medium", "large"],
        default="medium",
        help="TPC-C scale"
    )
    
    parser.add_argument(
        "--scale-factor",
        type=int,
        default=1,
        help="TPC-H scale factor (GB)"
    )
    
    parser.add_argument(
        "--repetitions",
        type=int,
        default=10,
        help="Scientific benchmark repetitions"
    )
    
    parser.add_argument(
        "--warmup-runs",
        type=int,
        default=5,
        help="Scientific benchmark warmup runs"
    )
    
    parser.add_argument(
        "--output-dir",
        default="benchmark_results",
        help="Output directory for results"
    )
    
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Verbose output"
    )
    
    args = parser.parse_args()
    
    # Create suite
    suite = CompleteBenchmarkSuite(
        args.output_dir, endpoint=args.endpoint, record_count=args.record_count,
        operations=args.operations, seed=args.seed, timeout=args.timeout,
    )
    
    # Run based on mode
    try:
        export_file = suite.output_dir / f"complete_benchmark_{args.mode}.json"
        if export_file.exists():
            raise FileExistsError(f"Result already exists: {export_file}")
        if args.mode == "scientific":
            if not args.database:
                print("Error: --database required for scientific mode")
                sys.exit(1)
            
            result = await suite.run_scientific_benchmark(
                args.database,
                operation=args.operation,
                repetitions=args.repetitions,
                warmup_runs=args.warmup_runs,
                iterations_per_run=args.iterations_per_run,
            )
            
            print("\nScientific benchmark complete")
        
        elif args.mode == "ycsb":
            result = await suite.run_ycsb_benchmarks(
                args.databases,
                args.workloads
            )
            print("\nYCSB benchmark complete")
        
        else:
            raise ValueError(
                f"Mode '{args.mode}' is unavailable: it has no validated database-backed driver"
            )

        with export_file.open("x", encoding="utf-8") as output:
            json.dump(result, output, indent=2, allow_nan=False)
        print(f"\nResults saved to: {export_file}")
    
    except Exception as e:
        print(f"\nError: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    asyncio.run(main())
