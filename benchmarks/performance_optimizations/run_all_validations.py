"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            run_all_validations.py                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     232                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Run all performance optimization benchmarks and tests

Executes functional tests and performance benchmarks for all Phase 1 optimizations:
- Mimalloc: Allocator performance
- Huge Pages: Memory access performance  
- RCU Index: Lock-free read performance

Validates that each optimization meets its performance targets.
"""

import argparse
import subprocess
import sys
from pathlib import Path


def run_command(cmd: list, description: str) -> bool:
    """Run a command and return success status"""
    print(f"\n{'='*70}")
    print(f"Running: {description}")
    print(f"Command: {' '.join(cmd)}")
    print(f"{'='*70}\n")
    
    try:
        result = subprocess.run(cmd, check=False)
        success = result.returncode == 0
        
        if success:
            print(f"\n✅ {description}: PASSED")
        else:
            print(f"\n❌ {description}: FAILED (exit code: {result.returncode})")
        
        return success
    except Exception as e:
        print(f"\n❌ {description}: ERROR - {e}")
        return False


def run_unit_tests(test_filter: str = None) -> bool:
    """Run C++ unit tests"""
    print(f"\n{'#'*70}")
    print(f"# PHASE 1: UNIT TESTS")
    print(f"{'#'*70}")
    
    build_dir = Path("build")
    test_binary = build_dir / "tests" / "themis_tests"
    
    if not test_binary.exists():
        print(f"\n⚠️  Test binary not found: {test_binary}")
        print(f"   Please build the project first:")
        print(f"   cmake -B build -S .")
        print(f"   cmake --build build")
        return False
    
    cmd = [str(test_binary)]
    if test_filter:
        cmd.extend(["--gtest_filter", test_filter])
    
    return run_command(cmd, "Unit Tests")


def run_benchmarks(iterations: int = 10) -> dict:
    """Run all performance benchmarks"""
    print(f"\n{'#'*70}")
    print(f"# PHASE 2: PERFORMANCE BENCHMARKS")
    print(f"{'#'*70}")
    
    bench_dir = Path("benchmarks/performance_optimizations")
    results = {}
    
    # Mimalloc benchmark
    mimalloc_script = bench_dir / "benchmark_mimalloc.py"
    if mimalloc_script.exists():
        success = run_command(
            ["python3", str(mimalloc_script), 
             "--iterations", str(iterations),
             "--min-improvement", "10"],
            "Mimalloc Benchmark"
        )
        results["mimalloc"] = success
    else:
        print(f"\n⚠️  Mimalloc benchmark not found: {mimalloc_script}")
        results["mimalloc"] = None
    
    # Huge Pages benchmark
    huge_pages_script = bench_dir / "benchmark_huge_pages.py"
    if huge_pages_script.exists():
        success = run_command(
            ["python3", str(huge_pages_script),
             "--iterations", str(iterations),
             "--min-improvement", "15"],
            "Huge Pages Benchmark"
        )
        results["huge_pages"] = success
    else:
        print(f"\n⚠️  Huge Pages benchmark not found: {huge_pages_script}")
        results["huge_pages"] = None
    
    # RCU Index benchmark
    rcu_script = bench_dir / "benchmark_rcu_index.py"
    if rcu_script.exists():
        success = run_command(
            ["python3", str(rcu_script),
             "--iterations", str(iterations),
             "--min-improvement", "200",
             "--read-ratio", "0.95"],
            "RCU Index Benchmark"
        )
        results["rcu_index"] = success
    else:
        print(f"\n⚠️  RCU Index benchmark not found: {rcu_script}")
        results["rcu_index"] = None
    
    return results


def print_summary(unit_tests_passed: bool, benchmark_results: dict):
    """Print summary of all results"""
    print(f"\n{'#'*70}")
    print(f"# VALIDATION SUMMARY")
    print(f"{'#'*70}\n")
    
    # Unit Tests
    print(f"Unit Tests: {'✅ PASSED' if unit_tests_passed else '❌ FAILED'}")
    
    # Benchmarks
    print(f"\nPerformance Benchmarks:")
    for name, result in benchmark_results.items():
        if result is None:
            status = "⚠️  NOT RUN"
        elif result:
            status = "✅ PASSED"
        else:
            status = "❌ FAILED"
        
        print(f"  {name.replace('_', ' ').title()}: {status}")
    
    # Overall status
    print(f"\n{'='*70}")
    
    all_passed = unit_tests_passed and all(
        r for r in benchmark_results.values() if r is not None
    )
    
    if all_passed:
        print(f"✅ ALL VALIDATIONS PASSED")
        print(f"   Phase 1 optimizations are working as expected!")
    else:
        print(f"❌ SOME VALIDATIONS FAILED")
        print(f"   Review the results above for details.")
    
    print(f"{'='*70}\n")
    
    return all_passed


def main():
    parser = argparse.ArgumentParser(
        description="Run all performance optimization tests and benchmarks"
    )
    parser.add_argument(
        "--unit-tests-only",
        action="store_true",
        help="Run only unit tests, skip benchmarks"
    )
    parser.add_argument(
        "--benchmarks-only",
        action="store_true",
        help="Run only benchmarks, skip unit tests"
    )
    parser.add_argument(
        "--test-filter",
        type=str,
        help="GTest filter for unit tests (e.g., 'RCU*' or '*Allocator*')"
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=10,
        help="Number of benchmark iterations (default: 10)"
    )
    
    args = parser.parse_args()
    
    print(f"\n{'#'*70}")
    print(f"# PERFORMANCE OPTIMIZATION VALIDATION")
    print(f"# Testing Phase 1: Mimalloc + Huge Pages + RCU Index")
    print(f"{'#'*70}\n")
    
    unit_tests_passed = True
    benchmark_results = {}
    
    # Run unit tests
    if not args.benchmarks_only:
        unit_tests_passed = run_unit_tests(args.test_filter)
    
    # Run benchmarks
    if not args.unit_tests_only:
        benchmark_results = run_benchmarks(args.iterations)
    
    # Print summary
    all_passed = print_summary(unit_tests_passed, benchmark_results)
    
    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())
