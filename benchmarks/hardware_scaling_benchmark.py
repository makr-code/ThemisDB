"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hardware_scaling_benchmark.py                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     547                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Hardware Configuration Benchmark Runner for ThemisDB
Implements scientific benchmarks across various hardware configurations.

Based on:
- TPC-C/TPC-H standards
- YCSB workloads
- Hardware scaling analysis
- NUMA optimization tests
"""

import asyncio
import json
import sys
import time
import psutil
import subprocess
import argparse
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional
from pathlib import Path
import statistics

@dataclass
class HardwareProfile:
    """Detected hardware configuration."""
    hostname: str
    platform: str
    processor: str
    cpu_count_logical: int
    cpu_count_physical: int
    cpu_freq_current_mhz: float
    cpu_freq_max_mhz: float
    memory_total_gb: float
    memory_available_gb: float
    storage_type: str
    numa_nodes: int
    
    @classmethod
    def detect(cls):
        """Auto-detect hardware configuration."""
        import platform
        
        cpu_freq = psutil.cpu_freq()
        memory = psutil.virtual_memory()
        
        return cls(
            hostname=platform.node(),
            platform=platform.platform(),
            processor=platform.processor() or "Unknown",
            cpu_count_logical=psutil.cpu_count(logical=True),
            cpu_count_physical=psutil.cpu_count(logical=False),
            cpu_freq_current_mhz=cpu_freq.current if cpu_freq else 0,
            cpu_freq_max_mhz=cpu_freq.max if cpu_freq else 0,
            memory_total_gb=memory.total / (1024**3),
            memory_available_gb=memory.available / (1024**3),
            storage_type=detect_storage_type(),
            numa_nodes=detect_numa_nodes()
        )

def detect_storage_type() -> str:
    """Detect primary storage device type."""
    try:
        # Linux: check if NVMe or rotational disk
        result = subprocess.run(
            ["lsblk", "-d", "-o", "NAME,ROTA"],
            capture_output=True, text=True, timeout=5
        )
        output = result.stdout.lower()
        if "nvme" in output and "0" in output:
            return "NVMe"
        elif "0" in output:
            return "SSD"
        elif "1" in output:
            return "HDD"
    except (subprocess.SubprocessError, OSError, subprocess.TimeoutExpired):
        pass
    
    # Windows: use wmic (simplified)
    try:
        result = subprocess.run(
            ["wmic", "diskdrive", "get", "MediaType"],
            capture_output=True, text=True, timeout=5
        )
        output = result.stdout.lower()
        if "ssd" in output or "solid state" in output:
            return "SSD"
    except (subprocess.SubprocessError, OSError, subprocess.TimeoutExpired):
        pass
    
    return "Unknown"

def detect_numa_nodes() -> int:
    """Detect number of NUMA nodes."""
    try:
        result = subprocess.run(
            ["numactl", "--hardware"],
            capture_output=True, text=True, timeout=5
        )
        for line in result.stdout.split('\n'):
            if "available:" in line:
                return int(line.split()[1])
    except (subprocess.SubprocessError, OSError, subprocess.TimeoutExpired, ValueError, IndexError):
        pass
    return 1

@dataclass
class BenchmarkResult:
    """Single benchmark run result."""
    timestamp: str
    cores: int
    threads: int
    workload: str
    duration_seconds: float
    throughput_ops_per_sec: float
    latency_mean_ms: float
    latency_p50_ms: float
    latency_p95_ms: float
    latency_p99_ms: float
    cpu_utilization_percent: float
    memory_used_gb: float
    errors: int
    
@dataclass
class ScalingAnalysis:
    """Scaling efficiency analysis."""
    cores: int
    threads: int
    throughput: float
    speedup: float
    ideal_speedup: float
    efficiency: float
    efficiency_grade: str

class HardwareScalingBenchmark:
    """
    Comprehensive hardware configuration benchmark suite.
    
    Tests ThemisDB performance across:
    - Different core counts (1, 2, 4, 8, 16, 32, 64)
    - Various thread configurations
    - Multiple workload types
    - NUMA configurations
    """
    
    def __init__(self, themis_binary: str = "./themis_benchmark",
                 output_dir: str = "./benchmark_results"):
        self.themis_binary = themis_binary
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        
        self.hardware = HardwareProfile.detect()
        self.results: List[BenchmarkResult] = []
        
    def print_hardware_info(self):
        """Display detected hardware configuration."""
        print("\n" + "="*70)
        print("HARDWARE CONFIGURATION")
        print("="*70)
        print(f"Hostname:          {self.hardware.hostname}")
        print(f"Platform:          {self.hardware.platform}")
        print(f"Processor:         {self.hardware.processor}")
        print(f"CPU Cores:         {self.hardware.cpu_count_physical} physical, "
              f"{self.hardware.cpu_count_logical} logical")
        print(f"CPU Frequency:     {self.hardware.cpu_freq_max_mhz:.0f} MHz (max)")
        print(f"Memory:            {self.hardware.memory_total_gb:.1f} GB total, "
              f"{self.hardware.memory_available_gb:.1f} GB available")
        print(f"Storage Type:      {self.hardware.storage_type}")
        print(f"NUMA Nodes:        {self.hardware.numa_nodes}")
        print("="*70 + "\n")
    
    async def run_single_benchmark(self, cores: int, threads: int,
                                   workload: str, duration: int,
                                   repetition: int = 1) -> BenchmarkResult:
        """
        Run a single benchmark configuration.
        
        Args:
            cores: Number of CPU cores to use
            threads: Number of worker threads
            workload: Workload type (e.g., 'ycsb_a', 'oltp', 'olap')
            duration: Duration in seconds
            repetition: Repetition number (for averaging)
        """
        print(f"[Rep {repetition}] Running: cores={cores}, threads={threads}, "
              f"workload={workload}, duration={duration}s")
        
        # Build command with CPU affinity
        cpu_list = ",".join(str(i) for i in range(min(cores, self.hardware.cpu_count_logical)))
        
        cmd = [
            "taskset", "-c", cpu_list,
            self.themis_binary,
            "--workload", workload,
            "--threads", str(threads),
            "--duration", str(duration),
            "--output", "json"
        ]
        
        # Capture initial metrics
        start_time = time.time()
        start_cpu = psutil.cpu_percent(interval=0.1)
        start_mem = psutil.virtual_memory().used / (1024**3)
        
        # Run benchmark
        try:
            result = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            stdout, stderr = await result.communicate()
            
            # Capture final metrics
            end_time = time.time()
            end_cpu = psutil.cpu_percent(interval=0.1)
            end_mem = psutil.virtual_memory().used / (1024**3)
            
            # Parse benchmark output
            output = stdout.decode()
            if output:
                bench_data = json.loads(output)
            else:
                print(f"Warning: Empty output. stderr: {stderr.decode()}", file=sys.stderr)
                bench_data = {}
        except json.JSONDecodeError as e:
            print(f"Error parsing benchmark output: {e}", file=sys.stderr)
            bench_data = {"error": f"JSON parsing error: {str(e)}"}
        except Exception as e:
            print(f"Error running benchmark: {e}", file=sys.stderr)
            bench_data = {"error": str(e)}
        
        # Extract metrics
        return BenchmarkResult(
            timestamp=time.strftime("%Y-%m-%d %H:%M:%S"),
            cores=cores,
            threads=threads,
            workload=workload,
            duration_seconds=end_time - start_time,
            throughput_ops_per_sec=bench_data.get("throughput", 0),
            latency_mean_ms=bench_data.get("latency_mean_ms", 0),
            latency_p50_ms=bench_data.get("latency_p50_ms", 0),
            latency_p95_ms=bench_data.get("latency_p95_ms", 0),
            latency_p99_ms=bench_data.get("latency_p99_ms", 0),
            cpu_utilization_percent=(start_cpu + end_cpu) / 2,
            memory_used_gb=(end_mem - start_mem),
            errors=bench_data.get("errors", 0)
        )
    
    async def run_scaling_suite(self, core_counts: List[int],
                               workload: str = "ycsb_a",
                               duration: int = 60,
                               repetitions: int = 3):
        """
        Run complete core scaling benchmark suite.
        
        Args:
            core_counts: List of core counts to test
            workload: Workload type
            duration: Duration per test in seconds
            repetitions: Number of repetitions for statistical significance
        """
        print(f"\n{'='*70}")
        print(f"CORE SCALING BENCHMARK SUITE")
        print(f"{'='*70}")
        print(f"Workload:     {workload}")
        print(f"Duration:     {duration}s per test")
        print(f"Repetitions:  {repetitions}")
        print(f"Core counts:  {core_counts}")
        print(f"{'='*70}\n")
        
        for core_count in core_counts:
            # Use 1 thread per core for scaling test
            thread_count = core_count
            
            for rep in range(1, repetitions + 1):
                result = await self.run_single_benchmark(
                    cores=core_count,
                    threads=thread_count,
                    workload=workload,
                    duration=duration,
                    repetition=rep
                )
                self.results.append(result)
                
                # Brief cooldown between runs
                await asyncio.sleep(2)
        
        return self.analyze_scaling()
    
    def analyze_scaling(self) -> List[ScalingAnalysis]:
        """
        Analyze scaling efficiency from benchmark results.
        
        Returns:
            List of scaling analysis per core configuration
        """
        # Group results by core count
        core_groups = {}
        for result in self.results:
            key = (result.cores, result.threads)
            if key not in core_groups:
                core_groups[key] = []
            core_groups[key].append(result)
        
        # Calculate average throughput per configuration
        avg_throughput = {}
        for key, results in core_groups.items():
            throughputs = [r.throughput_ops_per_sec for r in results if r.throughput_ops_per_sec > 0]
            if throughputs:
                avg_throughput[key] = statistics.mean(throughputs)
        
        # Find baseline (1 core)
        baseline = None
        for key in sorted(avg_throughput.keys()):
            if key[0] == 1:  # cores == 1
                baseline = avg_throughput[key]
                break
        
        if not baseline or baseline == 0:
            print("Warning: No valid baseline (1-core) result found")
            return []
        
        # Calculate scaling metrics
        analyses = []
        for key in sorted(avg_throughput.keys()):
            cores, threads = key
            throughput = avg_throughput[key]
            
            speedup = throughput / baseline
            ideal_speedup = cores
            efficiency = speedup / ideal_speedup
            
            analyses.append(ScalingAnalysis(
                cores=cores,
                threads=threads,
                throughput=throughput,
                speedup=speedup,
                ideal_speedup=ideal_speedup,
                efficiency=efficiency,
                efficiency_grade=self.get_efficiency_grade(efficiency)
            ))
        
        return analyses
    
    def get_efficiency_grade(self, efficiency: float) -> str:
        """Assign letter grade to scaling efficiency."""
        if efficiency >= 0.90:
            return "A+ (Excellent)"
        elif efficiency >= 0.80:
            return "A (Very Good)"
        elif efficiency >= 0.70:
            return "B (Good)"
        elif efficiency >= 0.60:
            return "C (Acceptable)"
        elif efficiency >= 0.50:
            return "D (Poor)"
        else:
            return "F (Critical)"
    
    def print_scaling_analysis(self, analyses: List[ScalingAnalysis]):
        """Print scaling analysis results in table format."""
        print("\n" + "="*100)
        print("SCALING EFFICIENCY ANALYSIS")
        print("="*100)
        print(f"{'Cores':<8} {'Threads':<10} {'Throughput':<15} {'Speedup':<12} "
              f"{'Efficiency':<12} {'Grade':<20}")
        print("-"*100)
        
        for analysis in analyses:
            print(f"{analysis.cores:<8} {analysis.threads:<10} "
                  f"{analysis.throughput:<15,.0f} "
                  f"{analysis.speedup:<12.2f}x "
                  f"{analysis.efficiency*100:<11.1f}% "
                  f"{analysis.efficiency_grade:<20}")
        
        print("="*100 + "\n")
    
    def export_results(self, filename: str = "hardware_scaling_results.json"):
        """Export all results to JSON file."""
        output_file = self.output_dir / filename
        
        export_data = {
            "metadata": {
                "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
                "themis_version": "1.3.0",
            },
            "hardware": asdict(self.hardware),
            "results": [asdict(r) for r in self.results],
            "scaling_analysis": [asdict(a) for a in self.analyze_scaling()]
        }
        
        with open(output_file, 'w') as f:
            json.dump(export_data, f, indent=2)
        
        print(f"Results exported to: {output_file}")
        return output_file
    
    def generate_markdown_report(self, filename: str = "hardware_scaling_report.md"):
        """Generate markdown report."""
        output_file = self.output_dir / filename
        analyses = self.analyze_scaling()
        
        with open(output_file, 'w') as f:
            f.write("# ThemisDB Hardware Scaling Benchmark Report\n\n")
            f.write(f"**Date:** {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            # Hardware section
            f.write("## Hardware Configuration\n\n")
            f.write(f"- **Hostname:** {self.hardware.hostname}\n")
            f.write(f"- **Platform:** {self.hardware.platform}\n")
            f.write(f"- **Processor:** {self.hardware.processor}\n")
            f.write(f"- **CPU Cores:** {self.hardware.cpu_count_physical} physical, "
                   f"{self.hardware.cpu_count_logical} logical\n")
            f.write(f"- **Memory:** {self.hardware.memory_total_gb:.1f} GB\n")
            f.write(f"- **Storage:** {self.hardware.storage_type}\n")
            f.write(f"- **NUMA Nodes:** {self.hardware.numa_nodes}\n\n")
            
            # Scaling results
            f.write("## Scaling Efficiency Results\n\n")
            f.write("| Cores | Threads | Throughput (ops/s) | Speedup | Efficiency | Grade |\n")
            f.write("|-------|---------|-------------------|---------|------------|-------|\n")
            
            for analysis in analyses:
                f.write(f"| {analysis.cores} | {analysis.threads} | "
                       f"{analysis.throughput:,.0f} | "
                       f"{analysis.speedup:.2f}x | "
                       f"{analysis.efficiency*100:.1f}% | "
                       f"{analysis.efficiency_grade} |\n")
            
            f.write("\n")
            
            # Recommendations
            f.write("## Recommendations\n\n")
            
            # Find optimal configuration
            best_efficiency = max(analyses, key=lambda a: a.efficiency)
            best_throughput = max(analyses, key=lambda a: a.throughput)
            
            f.write(f"- **Best Efficiency:** {best_efficiency.cores} cores "
                   f"({best_efficiency.efficiency*100:.1f}%)\n")
            f.write(f"- **Maximum Throughput:** {best_throughput.cores} cores "
                   f"({best_throughput.throughput:,.0f} ops/s)\n")
            f.write(f"- **Recommended Configuration:** {best_efficiency.cores} cores "
                   f"for optimal price/performance\n")
        
        print(f"Report generated: {output_file}")
        return output_file

async def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="ThemisDB Hardware Configuration Benchmark Suite"
    )
    parser.add_argument(
        "--themis-binary",
        default="./themis_benchmark",
        help="Path to ThemisDB benchmark binary"
    )
    parser.add_argument(
        "--core-counts",
        default="1,2,4,8",
        help="Comma-separated list of core counts to test (e.g., '1,2,4,8,16')"
    )
    parser.add_argument(
        "--workload",
        default="ycsb_a",
        help="Workload type (ycsb_a, oltp, olap, etc.)"
    )
    parser.add_argument(
        "--duration",
        type=int,
        default=60,
        help="Duration per test in seconds"
    )
    parser.add_argument(
        "--repetitions",
        type=int,
        default=3,
        help="Number of repetitions for statistical significance"
    )
    parser.add_argument(
        "--output-dir",
        default="./benchmark_results",
        help="Output directory for results"
    )
    
    args = parser.parse_args()
    
    # Parse core counts
    core_counts = [int(c.strip()) for c in args.core_counts.split(',')]
    
    # Initialize benchmark suite
    benchmark = HardwareScalingBenchmark(
        themis_binary=args.themis_binary,
        output_dir=args.output_dir
    )
    
    # Display hardware info
    benchmark.print_hardware_info()
    
    # Run benchmarks
    analyses = await benchmark.run_scaling_suite(
        core_counts=core_counts,
        workload=args.workload,
        duration=args.duration,
        repetitions=args.repetitions
    )
    
    # Display results
    benchmark.print_scaling_analysis(analyses)
    
    # Export results
    benchmark.export_results()
    benchmark.generate_markdown_report()
    
    print("\n" + "="*70)
    print("BENCHMARK COMPLETE")
    print("="*70)
    print(f"Results saved to: {benchmark.output_dir}")
    print("="*70 + "\n")

if __name__ == "__main__":
    asyncio.run(main())
