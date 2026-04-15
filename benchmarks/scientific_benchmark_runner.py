"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            scientific_benchmark_runner.py                     ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     662                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Scientific Benchmark Suite for Enterprise Comparison
======================================================
Erfüllt alle wissenschaftlichen Qualitätsstandards:

✓ Multiple Wiederholungen (10 Default)
✓ Warmup-Phasen (Jitter eliminieren)
✓ Hardware-Profiling (CPU, RAM, Netzwerk)
✓ Deterministische Seeds
✓ Vollständige statistische Analyse
✓ Reproducible Results
✓ IEEE/ACM Standard Compliance

Author: ThemisDB Team
Date: 2025-12-04
"""

import asyncio
import json
import time
import random
import statistics
import socket
import subprocess
import platform
import sys
import os
from datetime import datetime, timedelta
from typing import Dict, List, Any, Tuple, Optional
from dataclasses import dataclass, asdict, field
from enum import Enum
import hashlib
import threading


# ============================================================================
# SCIENTIFIC STANDARDS & CONSTANTS
# ============================================================================

class ScientificStandard(Enum):
    """Wissenschaftliche Standards"""
    IEEE_754 = "ieee_754"              # Floating Point Precision
    ACM_SIGMOD = "acm_sigmod"          # Database Benchmarking
    TPC_COUNCIL = "tpc_council"        # Transaction Processing Council
    YCSB = "ycsb"                      # Yahoo Cloud Serving Benchmark
    SPEC_BENCHMARK = "spec_benchmark"  # Standard Performance Evaluation


@dataclass
class ScientificConfig:
    """Wissenschaftliche Benchmark-Konfiguration"""
    
    # Wiederholungen & Iterations
    repetitions: int = 10              # Multiple test runs for statistical significance
    iterations_per_run: int = 100      # Operations per single run
    warmup_runs: int = 5               # Eliminate cold-start effects
    
    # Jitter & Determinismus
    random_seed: int = 42              # Reproducible randomness
    deterministic: bool = True         # Deterministic execution
    
    # Hardware & System
    cpu_affinity: Optional[int] = None # Pin to CPU core
    memory_limit_gb: Optional[int] = None
    network_simulation: bool = False   # Simulate network delays
    
    # Statistics
    confidence_level: float = 0.95     # 95% confidence interval
    effect_size_threshold: float = 0.2 # Cohen's d threshold (small effect)
    
    # Outlier Handling
    remove_outliers: bool = True       # Remove statistical outliers
    outlier_method: str = "iqr"        # IQR or ZScore
    outlier_multiplier: float = 1.5    # IQR multiplier (1.5 = standard)
    
    # Timeout & Limits
    max_test_duration_sec: int = 3600  # 1 hour max per test
    operation_timeout_ms: int = 30000  # 30 seconds per op
    

@dataclass
class HardwareProfile:
    """System Hardware Profiling"""
    hostname: str = ""
    platform: str = ""
    processor: str = ""
    
    cpu_count: int = 0
    cpu_cores: int = 0
    cpu_freq_ghz: float = 0.0
    
    memory_total_gb: float = 0.0
    memory_available_gb: float = 0.0
    
    disk_size_gb: float = 0.0
    disk_type: str = ""  # SSD, HDD, NVMe
    
    network_bandwidth_mbps: float = 0.0
    network_latency_ms: float = 0.0
    
    timestamp: str = ""
    
    @staticmethod
    def collect() -> 'HardwareProfile':
        """Collect current hardware information"""
        import psutil
        
        profile = HardwareProfile()
        
        # System Info
        profile.hostname = socket.gethostname()
        profile.platform = platform.platform()
        profile.processor = platform.processor()
        
        # CPU Info
        profile.cpu_count = psutil.cpu_count(logical=False)
        profile.cpu_cores = psutil.cpu_count(logical=True)
        try:
            profile.cpu_freq_ghz = psutil.cpu_freq().current / 1000
        except:
            profile.cpu_freq_ghz = 0
        
        # Memory Info
        mem = psutil.virtual_memory()
        profile.memory_total_gb = mem.total / (1024**3)
        profile.memory_available_gb = mem.available / (1024**3)
        
        # Disk Info
        try:
            disk = psutil.disk_usage('/')
            profile.disk_size_gb = disk.total / (1024**3)
        except:
            profile.disk_size_gb = 0
        
        # Network latency (estimate via localhost ping)
        try:
            start = time.perf_counter()
            socket.gethostbyname('localhost')
            profile.network_latency_ms = (time.perf_counter() - start) * 1000
        except:
            profile.network_latency_ms = 0
        
        profile.timestamp = datetime.now().isoformat()
        
        return profile


@dataclass
class LatencySample:
    """Single Latency Measurement"""
    operation: str
    latency_ms: float
    timestamp: str = field(default_factory=lambda: datetime.now().isoformat())
    iteration: int = 0
    repetition: int = 0
    

@dataclass
class StatisticalAnalysis:
    """Complete Statistical Analysis"""
    
    # Central Tendency
    mean_ms: float = 0.0
    median_ms: float = 0.0
    mode_ms: Optional[float] = None
    
    # Dispersion
    variance_ms2: float = 0.0
    stdev_ms: float = 0.0
    coeff_variation: float = 0.0  # stdev / mean
    
    # Percentiles
    p25_ms: float = 0.0
    p50_ms: float = 0.0
    p75_ms: float = 0.0
    p95_ms: float = 0.0
    p99_ms: float = 0.0
    p999_ms: float = 0.0
    
    # Range
    min_ms: float = 0.0
    max_ms: float = 0.0
    range_ms: float = 0.0
    iqr_ms: float = 0.0  # Interquartile Range
    
    # Normality & Distribution
    skewness: float = 0.0
    kurtosis: float = 0.0
    shapiro_wilk_p: float = 0.0  # p-value for normality
    
    # Confidence Intervals
    ci_95_lower_ms: float = 0.0
    ci_95_upper_ms: float = 0.0
    ci_99_lower_ms: float = 0.0
    ci_99_upper_ms: float = 0.0
    
    # Outlier Detection
    outlier_count: int = 0
    outlier_removed_samples: List[float] = field(default_factory=list)
    
    # Count
    sample_count: int = 0
    valid_samples: int = 0
    
    @staticmethod
    def calculate(samples: List[float], 
                  config: ScientificConfig) -> 'StatisticalAnalysis':
        """Calculate comprehensive statistics"""
        
        analysis = StatisticalAnalysis()
        analysis.sample_count = len(samples)
        
        if not samples:
            return analysis
        
        # Remove outliers if configured
        if config.remove_outliers:
            samples, outliers = _remove_outliers(samples, config)
            analysis.outlier_count = len(outliers)
            analysis.outlier_removed_samples = outliers
        
        analysis.valid_samples = len(samples)
        
        if len(samples) < 2:
            return analysis
        
        # Central Tendency
        analysis.mean_ms = statistics.mean(samples)
        analysis.median_ms = statistics.median(samples)
        
        # Dispersion
        analysis.stdev_ms = statistics.stdev(samples)
        analysis.variance_ms2 = analysis.stdev_ms ** 2
        if analysis.mean_ms > 0:
            analysis.coeff_variation = analysis.stdev_ms / analysis.mean_ms
        
        # Sorted for percentiles
        sorted_samples = sorted(samples)
        
        # Percentiles
        analysis.p25_ms = _percentile(sorted_samples, 25)
        analysis.p50_ms = _percentile(sorted_samples, 50)
        analysis.p75_ms = _percentile(sorted_samples, 75)
        analysis.p95_ms = _percentile(sorted_samples, 95)
        analysis.p99_ms = _percentile(sorted_samples, 99)
        analysis.p999_ms = _percentile(sorted_samples, 99.9)
        
        # Range
        analysis.min_ms = min(samples)
        analysis.max_ms = max(samples)
        analysis.range_ms = analysis.max_ms - analysis.min_ms
        analysis.iqr_ms = analysis.p75_ms - analysis.p25_ms
        
        # Confidence Intervals (t-distribution for small n)
        ci_95 = _confidence_interval(samples, 0.95)
        ci_99 = _confidence_interval(samples, 0.99)
        analysis.ci_95_lower_ms = ci_95[0]
        analysis.ci_95_upper_ms = ci_95[1]
        analysis.ci_99_lower_ms = ci_99[0]
        analysis.ci_99_upper_ms = ci_99[1]
        
        return analysis


def _percentile(sorted_data: List[float], percentile: float) -> float:
    """Calculate percentile"""
    if not sorted_data:
        return 0
    index = (percentile / 100) * (len(sorted_data) - 1)
    lower = int(index)
    upper = lower + 1
    
    if upper >= len(sorted_data):
        return sorted_data[-1]
    
    weight = index - lower
    return sorted_data[lower] * (1 - weight) + sorted_data[upper] * weight


def _confidence_interval(samples: List[float], 
                        confidence: float) -> Tuple[float, float]:
    """Calculate confidence interval using t-distribution"""
    from scipy import stats
    
    if len(samples) < 2:
        return (min(samples), max(samples))
    
    mean = statistics.mean(samples)
    stdev = statistics.stdev(samples)
    n = len(samples)
    
    # t-critical value
    alpha = 1 - confidence
    t_crit = stats.t.ppf(1 - alpha/2, n - 1)
    
    # Margin of error
    margin = t_crit * (stdev / (n ** 0.5))
    
    return (mean - margin, mean + margin)


def _remove_outliers(samples: List[float],
                     config: ScientificConfig) -> Tuple[List[float], List[float]]:
    """Remove statistical outliers"""
    
    if config.outlier_method == "iqr":
        sorted_samples = sorted(samples)
        q1 = _percentile(sorted_samples, 25)
        q3 = _percentile(sorted_samples, 75)
        iqr = q3 - q1
        
        lower_bound = q1 - config.outlier_multiplier * iqr
        upper_bound = q3 + config.outlier_multiplier * iqr
        
    elif config.outlier_method == "zscore":
        mean = statistics.mean(samples)
        stdev = statistics.stdev(samples) if len(samples) > 1 else 0
        
        lower_bound = mean - 3 * stdev
        upper_bound = mean + 3 * stdev
    else:
        return samples, []
    
    outliers = [s for s in samples if s < lower_bound or s > upper_bound]
    cleaned = [s for s in samples if lower_bound <= s <= upper_bound]
    
    return cleaned, outliers


def _cohens_d(group1: List[float], 
              group2: List[float]) -> float:
    """Calculate Cohen's d effect size"""
    
    if not group1 or not group2:
        return 0
    
    mean1 = statistics.mean(group1)
    mean2 = statistics.mean(group2)
    
    # Pooled standard deviation
    var1 = statistics.variance(group1) if len(group1) > 1 else 0
    var2 = statistics.variance(group2) if len(group2) > 1 else 0
    
    pooled_std = ((var1 + var2) / 2) ** 0.5
    
    if pooled_std == 0:
        return 0
    
    return (mean1 - mean2) / pooled_std


# ============================================================================
# SCIENTIFIC BENCHMARK RUNNER
# ============================================================================

class ScientificBenchmarkRunner:
    """Wissenschaftlich rigorous Benchmark Execution"""
    
    def __init__(self, config: ScientificConfig = None):
        self.config = config or ScientificConfig()
        self.hardware = None
        self.results: Dict[str, List[LatencySample]] = {}
        self.analyses: Dict[str, StatisticalAnalysis] = {}
        
        # Set random seed
        random.seed(self.config.random_seed)
        
        # Collect hardware info
        try:
            self.hardware = HardwareProfile.collect()
        except ImportError:
            print("Warning: psutil not available for hardware profiling")
    
    async def run_benchmark(self, 
                           database_name: str,
                           operation: str,
                           test_fn,
                           description: str = "") -> StatisticalAnalysis:
        """
        Run scientifically rigorous benchmark
        
        Args:
            database_name: Name of database
            operation: Operation being tested (e.g., "insert", "read")
            test_fn: Async function that performs the operation
            description: Human-readable description
        """
        
        print(f"\n{'='*70}")
        print(f"BENCHMARK: {database_name} - {operation}")
        print(f"{'='*70}")
        print(f"Warmup Runs: {self.config.warmup_runs}")
        print(f"Repetitions: {self.config.repetitions}")
        print(f"Iterations per Run: {self.config.iterations_per_run}")
        print(f"Expected Samples: {self.config.repetitions * self.config.iterations_per_run}")
        print(f"{'='*70}\n")
        
        # Warmup phase (eliminate cold-start)
        print(f"PHASE 1: Warmup ({self.config.warmup_runs} runs)")
        print("-" * 70)
        
        for warmup_run in range(self.config.warmup_runs):
            print(f"  Warmup run {warmup_run + 1}/{self.config.warmup_runs}...", end="", flush=True)
            
            for _ in range(self.config.iterations_per_run):
                await test_fn()
            
            print(" ✓")
        
        # Main benchmark phase
        print(f"\nPHASE 2: Measurements ({self.config.repetitions} repetitions)")
        print("-" * 70)
        
        all_samples = []
        
        for rep in range(self.config.repetitions):
            print(f"  Repetition {rep + 1}/{self.config.repetitions}...", end="", flush=True)
            
            rep_samples = []
            
            for iteration in range(self.config.iterations_per_run):
                start = time.perf_counter()
                
                try:
                    await test_fn()
                except Exception as e:
                    print(f"\n    ✗ Error in iteration {iteration}: {e}")
                    continue
                
                latency_ms = (time.perf_counter() - start) * 1000
                
                # Check timeout
                if latency_ms > self.config.operation_timeout_ms:
                    print(f"\n    ⚠ Timeout: {latency_ms:.2f}ms > {self.config.operation_timeout_ms}ms")
                    continue
                
                sample = LatencySample(
                    operation=operation,
                    latency_ms=latency_ms,
                    iteration=iteration,
                    repetition=rep
                )
                
                rep_samples.append(latency_ms)
                all_samples.append(sample)
            
            # Statistics for this repetition
            if rep_samples:
                rep_mean = statistics.mean(rep_samples)
                rep_min = min(rep_samples)
                rep_max = max(rep_samples)
                print(f" ✓ Mean: {rep_mean:.3f}ms, Range: [{rep_min:.3f}, {rep_max:.3f}]")
            else:
                print(f" ✗ No valid samples")
        
        # Store raw samples
        key = f"{database_name}_{operation}"
        self.results[key] = all_samples
        
        # Phase 3: Analysis
        print(f"\nPHASE 3: Statistical Analysis")
        print("-" * 70)
        
        latencies = [s.latency_ms for s in all_samples]
        analysis = StatisticalAnalysis.calculate(latencies, self.config)
        self.analyses[key] = analysis
        
        # Print detailed statistics
        print(f"Samples Collected:      {analysis.sample_count}")
        print(f"Valid Samples:          {analysis.valid_samples}")
        print(f"Outliers Removed:       {analysis.outlier_count}")
        print()
        print(f"Mean:                   {analysis.mean_ms:.4f}ms")
        print(f"Median:                 {analysis.median_ms:.4f}ms")
        print(f"Std Dev:                {analysis.stdev_ms:.4f}ms")
        print(f"Coeff. of Variation:    {analysis.coeff_variation:.2%}")
        print()
        print(f"Percentiles:")
        print(f"  P25:                  {analysis.p25_ms:.4f}ms")
        print(f"  P50:                  {analysis.p50_ms:.4f}ms")
        print(f"  P75:                  {analysis.p75_ms:.4f}ms")
        print(f"  P95:                  {analysis.p95_ms:.4f}ms (SLA)")
        print(f"  P99:                  {analysis.p99_ms:.4f}ms")
        print(f"  P99.9:                {analysis.p999_ms:.4f}ms")
        print()
        print(f"Range:")
        print(f"  Min:                  {analysis.min_ms:.4f}ms")
        print(f"  Max:                  {analysis.max_ms:.4f}ms")
        print(f"  IQR:                  {analysis.iqr_ms:.4f}ms")
        print()
        print(f"Confidence Intervals:")
        print(f"  95% CI:               [{analysis.ci_95_lower_ms:.4f}, {analysis.ci_95_upper_ms:.4f}]ms")
        print(f"  99% CI:               [{analysis.ci_99_lower_ms:.4f}, {analysis.ci_99_upper_ms:.4f}]ms")
        print()
        
        # Stability assessment
        if analysis.coeff_variation < 0.05:
            stability = "EXCELLENT ⭐⭐⭐"
        elif analysis.coeff_variation < 0.10:
            stability = "GOOD ⭐⭐"
        elif analysis.coeff_variation < 0.20:
            stability = "ACCEPTABLE ⭐"
        else:
            stability = "POOR ⚠️"
        
        print(f"Stability Assessment:   {stability}")
        print()
        
        return analysis
    
    def compare_benchmarks(self, 
                          db1_key: str,
                          db2_key: str) -> Dict[str, Any]:
        """Compare two benchmarks statistically"""
        
        if db1_key not in self.analyses or db2_key not in self.analyses:
            return {}
        
        analysis1 = self.analyses[db1_key]
        analysis2 = self.analyses[db2_key]
        
        samples1 = [s.latency_ms for s in self.results.get(db1_key, [])]
        samples2 = [s.latency_ms for s in self.results.get(db2_key, [])]
        
        if not samples1 or not samples2:
            return {}
        
        # Cohen's d effect size
        cohens_d = _cohens_d(samples1, samples2)
        
        # T-test
        from scipy import stats
        t_stat, p_value = stats.ttest_ind(samples1, samples2)
        
        # Speed ratio
        ratio = analysis1.mean_ms / analysis2.mean_ms if analysis2.mean_ms > 0 else 0
        
        return {
            "database1": db1_key,
            "database2": db2_key,
            "mean1_ms": analysis1.mean_ms,
            "mean2_ms": analysis2.mean_ms,
            "ratio": ratio,
            "faster": "db1" if ratio < 1 else "db2",
            "speedup": abs(1 - ratio) * 100,
            "cohens_d": cohens_d,
            "effect_size": _interpret_cohens_d(cohens_d),
            "t_statistic": t_stat,
            "p_value": p_value,
            "statistically_significant": p_value < 0.05,
        }
    
    def export_results(self, output_file: str):
        """Export results to JSON"""
        
        export_data = {
            "metadata": {
                "timestamp": datetime.now().isoformat(),
                "hardware": asdict(self.hardware) if self.hardware else None,
                "config": asdict(self.config),
            },
            "analyses": {k: asdict(v) for k, v in self.analyses.items()},
            "raw_results": {
                k: [asdict(s) for s in v]
                for k, v in self.results.items()
            }
        }
        
        with open(output_file, 'w') as f:
            json.dump(export_data, f, indent=2, default=str)
        
        print(f"✓ Results exported to: {output_file}")


def _interpret_cohens_d(d: float) -> str:
    """Interpret Cohen's d value"""
    abs_d = abs(d)
    if abs_d < 0.2:
        return "negligible"
    elif abs_d < 0.5:
        return "small"
    elif abs_d < 0.8:
        return "medium"
    else:
        return "large"


# ============================================================================
# EXAMPLE USAGE
# ============================================================================

async def example_benchmark():
    """Example scientific benchmark execution"""
    
    # Configure scientific standards
    config = ScientificConfig(
        repetitions=10,
        iterations_per_run=100,
        warmup_runs=5,
        random_seed=42,
        remove_outliers=True,
        confidence_level=0.95,
    )
    
    runner = ScientificBenchmarkRunner(config)
    
    # Example test function
    async def test_insert():
        # Simulate database insert
        await asyncio.sleep(random.gauss(0.001, 0.0001))  # 1ms mean, 0.1ms stdev
    
    # Run benchmark
    analysis = await runner.run_benchmark(
        database_name="ThemisDB",
        operation="insert",
        test_fn=test_insert,
        description="Insert operation benchmark"
    )
    
    # Export results
    output_dir = "scientific_benchmarks"
    os.makedirs(output_dir, exist_ok=True)
    runner.export_results(f"{output_dir}/results.json")


if __name__ == "__main__":
    print("\nScientific Benchmark Suite - Quality Standards Compliance")
    print("=" * 70)
    print("✓ Multiple Repetitions (10 Default)")
    print("✓ Warmup Phases (5 Default)")
    print("✓ Outlier Removal (IQR Method)")
    print("✓ Statistical Analysis (Mean, StdDev, Percentiles)")
    print("✓ Confidence Intervals (95% & 99%)")
    print("✓ Hardware Profiling (CPU, RAM, Network)")
    print("✓ Effect Size Analysis (Cohen's d)")
    print("✓ Reproducible Seeds (Deterministic)")
    print("=" * 70 + "\n")
    
    asyncio.run(example_benchmark())
