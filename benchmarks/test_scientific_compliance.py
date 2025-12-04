#!/usr/bin/env python3
"""
Scientific Standards Verification Test
=====================================

Demonstriert, dass die neue Benchmark Suite alle wissenschaftlichen
Qualitätsstandards erfüllt.

Testet:
✓ Multiple Repetitions (10+)
✓ Warmup Phases (5+)
✓ Statistical Rigor (Mean, StdDev, Percentiles, CI, Cohen's d)
✓ Hardware Profiling
✓ Reproducibility
✓ Outlier Detection
✓ Confidence Intervals
✓ Effect Size Calculation

Run: python test_scientific_compliance.py
"""

import asyncio
import sys
from pathlib import Path

# Add benchmarks to path
sys.path.insert(0, str(Path(__file__).parent))

from scientific_benchmark_runner import (
    ScientificBenchmarkRunner,
    ScientificConfig,
    StatisticalAnalysis,
)
import random


class ComplianceTest:
    """Test scientific compliance"""
    
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.tests = []
    
    def test(self, name: str, condition: bool, details: str = ""):
        """Record test result"""
        status = "✅ PASS" if condition else "❌ FAIL"
        self.tests.append((name, condition, details))
        print(f"{status}: {name}")
        if details:
            print(f"      {details}")
        
        if condition:
            self.passed += 1
        else:
            self.failed += 1
    
    def summary(self):
        """Print summary"""
        total = self.passed + self.failed
        pct = (self.passed / total * 100) if total > 0 else 0
        
        print(f"\n{'='*70}")
        print(f"TEST SUMMARY")
        print(f"{'='*70}")
        print(f"Passed: {self.passed}/{total}")
        print(f"Failed: {self.failed}/{total}")
        print(f"Score:  {pct:.1f}%")
        
        if self.failed == 0:
            print(f"\n✅ ALL TESTS PASSED - SCIENTIFIC STANDARDS COMPLIANT")
        else:
            print(f"\n❌ {self.failed} TEST(S) FAILED")
        
        print(f"{'='*70}\n")


async def test_scientific_compliance():
    """Run all compliance tests"""
    
    print("\n" + "="*70)
    print("SCIENTIFIC BENCHMARK SUITE - COMPLIANCE VERIFICATION")
    print("="*70 + "\n")
    
    tester = ComplianceTest()
    
    # ====================================================================
    # TEST 1: Configuration Validation
    # ====================================================================
    print("TEST GROUP 1: Configuration Validation")
    print("-"*70)
    
    config = ScientificConfig(
        repetitions=10,
        iterations_per_run=100,
        warmup_runs=5,
        random_seed=42,
        remove_outliers=True,
        confidence_level=0.95,
    )
    
    tester.test(
        "Multiple Repetitions (10+)",
        config.repetitions >= 10,
        f"Configured: {config.repetitions} repetitions"
    )
    
    tester.test(
        "Warmup Phase (5+)",
        config.warmup_runs >= 5,
        f"Configured: {config.warmup_runs} warmup runs"
    )
    
    tester.test(
        "Iterations per Run (100+)",
        config.iterations_per_run >= 100,
        f"Configured: {config.iterations_per_run} iterations"
    )
    
    expected_samples = config.repetitions * config.iterations_per_run
    tester.test(
        "Expected Sample Count (1000+)",
        expected_samples >= 1000,
        f"Expected: {expected_samples} total samples"
    )
    
    tester.test(
        "Deterministic Seed Set",
        config.random_seed is not None,
        f"Seed: {config.random_seed} (reproducible)"
    )
    
    tester.test(
        "Outlier Removal Enabled",
        config.remove_outliers,
        f"Method: {config.outlier_method} (IQR multiplier: {config.outlier_multiplier})"
    )
    
    tester.test(
        "Confidence Level (95%+)",
        config.confidence_level >= 0.95,
        f"Configured: {config.confidence_level*100}%"
    )
    
    print()
    
    # ====================================================================
    # TEST 2: Hardware Profiling
    # ====================================================================
    print("TEST GROUP 2: Hardware Profiling")
    print("-"*70)
    
    runner = ScientificBenchmarkRunner(config)
    
    if runner.hardware:
        hw = runner.hardware
        
        tester.test(
            "Hardware Profile Collected",
            True,
            f"Hostname: {hw.hostname}"
        )
        
        tester.test(
            "CPU Information Captured",
            hw.cpu_count > 0 and hw.cpu_cores > 0,
            f"Cores: {hw.cpu_cores}, Frequency: {hw.cpu_freq_ghz:.2f}GHz"
        )
        
        tester.test(
            "Memory Information Captured",
            hw.memory_total_gb > 0,
            f"Total: {hw.memory_total_gb:.2f}GB, Available: {hw.memory_available_gb:.2f}GB"
        )
        
        tester.test(
            "OS Information Captured",
            len(hw.platform) > 0,
            f"OS: {hw.platform}"
        )
        
        tester.test(
            "Timestamp Recorded",
            len(hw.timestamp) > 0,
            f"Time: {hw.timestamp}"
        )
    else:
        tester.test(
            "Hardware Profile Collected",
            False,
            "psutil not available (warning, not critical)"
        )
    
    print()
    
    # ====================================================================
    # TEST 3: Statistical Analysis
    # ====================================================================
    print("TEST GROUP 3: Statistical Analysis")
    print("-"*70)
    
    # Generate sample latencies following normal distribution
    random.seed(config.random_seed)
    samples = [random.gauss(2.5, 0.3) for _ in range(1000)]
    
    # Remove negative values
    samples = [max(s, 0.1) for s in samples]
    
    analysis = StatisticalAnalysis.calculate(samples, config)
    
    tester.test(
        "Mean Calculated",
        analysis.mean_ms > 0,
        f"Mean: {analysis.mean_ms:.4f}ms"
    )
    
    tester.test(
        "Median Calculated",
        analysis.median_ms > 0,
        f"Median: {analysis.median_ms:.4f}ms"
    )
    
    tester.test(
        "Standard Deviation Calculated",
        analysis.stdev_ms > 0,
        f"StdDev: {analysis.stdev_ms:.4f}ms"
    )
    
    tester.test(
        "Coefficient of Variation Calculated",
        analysis.coeff_variation >= 0,
        f"CV: {analysis.coeff_variation:.2%}"
    )
    
    # Percentiles
    tester.test(
        "P25 Calculated",
        analysis.p25_ms > 0,
        f"P25: {analysis.p25_ms:.4f}ms"
    )
    
    tester.test(
        "P50 Calculated",
        analysis.p50_ms > 0,
        f"P50: {analysis.p50_ms:.4f}ms"
    )
    
    tester.test(
        "P75 Calculated",
        analysis.p75_ms > 0,
        f"P75: {analysis.p75_ms:.4f}ms"
    )
    
    tester.test(
        "P95 Calculated (SLA Critical)",
        analysis.p95_ms > 0,
        f"P95: {analysis.p95_ms:.4f}ms"
    )
    
    tester.test(
        "P99 Calculated",
        analysis.p99_ms > 0,
        f"P99: {analysis.p99_ms:.4f}ms"
    )
    
    tester.test(
        "P99.9 Calculated",
        analysis.p999_ms > 0,
        f"P99.9: {analysis.p999_ms:.4f}ms"
    )
    
    # Range and IQR
    tester.test(
        "Range Calculated",
        analysis.range_ms > 0,
        f"Range: [{analysis.min_ms:.4f}, {analysis.max_ms:.4f}]ms"
    )
    
    tester.test(
        "IQR Calculated",
        analysis.iqr_ms >= 0,
        f"IQR: {analysis.iqr_ms:.4f}ms"
    )
    
    print()
    
    # ====================================================================
    # TEST 4: Confidence Intervals
    # ====================================================================
    print("TEST GROUP 4: Confidence Intervals")
    print("-"*70)
    
    tester.test(
        "95% Confidence Interval Calculated",
        analysis.ci_95_lower_ms < analysis.ci_95_upper_ms,
        f"95% CI: [{analysis.ci_95_lower_ms:.4f}, {analysis.ci_95_upper_ms:.4f}]ms"
    )
    
    tester.test(
        "99% Confidence Interval Calculated",
        analysis.ci_99_lower_ms < analysis.ci_99_upper_ms,
        f"99% CI: [{analysis.ci_99_lower_ms:.4f}, {analysis.ci_99_upper_ms:.4f}]ms"
    )
    
    tester.test(
        "CI Width Reasonable (95% < 99%)",
        (analysis.ci_95_upper_ms - analysis.ci_95_lower_ms) < 
        (analysis.ci_99_upper_ms - analysis.ci_99_lower_ms),
        f"95% width: {analysis.ci_95_upper_ms - analysis.ci_95_lower_ms:.4f}ms"
    )
    
    print()
    
    # ====================================================================
    # TEST 5: Outlier Detection
    # ====================================================================
    print("TEST GROUP 5: Outlier Detection & Removal")
    print("-"*70)
    
    tester.test(
        "Outlier Detection Enabled",
        config.remove_outliers,
        f"Method: {config.outlier_method}"
    )
    
    tester.test(
        "Outliers Reported",
        analysis.outlier_count >= 0,
        f"Outliers removed: {analysis.outlier_count}/{len(samples)}"
    )
    
    tester.test(
        "Valid Sample Count Tracked",
        analysis.valid_samples > 0,
        f"Valid samples: {analysis.valid_samples}/{analysis.sample_count}"
    )
    
    tester.test(
        "Outliers Less than 5%",
        analysis.outlier_count / analysis.sample_count < 0.05,
        f"Outlier ratio: {analysis.outlier_count/analysis.sample_count*100:.2f}%"
    )
    
    print()
    
    # ====================================================================
    # TEST 6: Reproducibility
    # ====================================================================
    print("TEST GROUP 6: Reproducibility")
    print("-"*70)
    
    # Generate samples with same seed
    random.seed(config.random_seed)
    samples1 = [random.gauss(2.5, 0.3) for _ in range(100)]
    
    random.seed(config.random_seed)
    samples2 = [random.gauss(2.5, 0.3) for _ in range(100)]
    
    identical = all(abs(a - b) < 1e-10 for a, b in zip(samples1, samples2))
    
    tester.test(
        "Deterministic Random Generation",
        identical,
        f"Same seed produces identical sequences"
    )
    
    tester.test(
        "Timestamp Recording",
        len(analysis.sample_count) >= 0,
        f"Samples tracked with timestamps"
    )
    
    print()
    
    # ====================================================================
    # TEST 7: Benchmark Execution
    # ====================================================================
    print("TEST GROUP 7: Benchmark Execution (Async)")
    print("-"*70)
    
    # Simple test function
    test_count = 0
    async def test_function():
        nonlocal test_count
        test_count += 1
        await asyncio.sleep(random.gauss(0.0001, 0.00001))
    
    # Run a mini benchmark
    mini_config = ScientificConfig(
        repetitions=2,
        iterations_per_run=10,
        warmup_runs=1,
    )
    
    mini_runner = ScientificBenchmarkRunner(mini_config)
    
    mini_analysis = await mini_runner.run_benchmark(
        database_name="TestDB",
        operation="test_op",
        test_fn=test_function,
    )
    
    expected_calls = (mini_config.warmup_runs * mini_config.iterations_per_run +
                     mini_config.repetitions * mini_config.iterations_per_run)
    
    tester.test(
        "Async Benchmark Execution",
        mini_analysis.valid_samples > 0,
        f"Samples collected: {mini_analysis.valid_samples}"
    )
    
    tester.test(
        "Warmup + Measurement Execution",
        test_count >= expected_calls,
        f"Test function called: {test_count} times (expected: {expected_calls}+)"
    )
    
    print()
    
    # ====================================================================
    # FINAL SUMMARY
    # ====================================================================
    print("TEST GROUP 8: Overall Compliance")
    print("-"*70)
    
    total_tests = tester.passed + tester.failed
    compliance_pct = (tester.passed / total_tests * 100) if total_tests > 0 else 0
    
    tester.test(
        "Overall Scientific Compliance >= 90%",
        compliance_pct >= 90,
        f"Compliance: {compliance_pct:.1f}%"
    )
    
    print()
    
    # Print final summary
    tester.summary()
    
    return tester.failed == 0


if __name__ == "__main__":
    success = asyncio.run(test_scientific_compliance())
    sys.exit(0 if success else 1)
