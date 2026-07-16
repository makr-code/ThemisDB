> ⚠️ **Historische Konfiguration** – Hardware-Parameter beschreiben einen bestimmten Teststand.

# Hardware Configuration Benchmark Suite
## Testing ThemisDB Across Diverse Hardware Configurations

**Version:** 1.0  
**Date:** 2025-12-23  
**Status:** Implementation Guide

---

## Executive Summary

This guide provides comprehensive benchmarking procedures for testing ThemisDB across various hardware configurations including different core counts, thread configurations, memory architectures, and storage systems. The goal is to identify optimal configurations and provide tuning recommendations for different deployment scenarios.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Core Count Scaling Tests](#2-core-count-scaling-tests)
3. [Thread Configuration Optimization](#3-thread-configuration-optimization)
4. [Memory Architecture Testing](#4-memory-architecture-testing)
5. [Storage Configuration Tests](#5-storage-configuration-tests)
6. [Network Performance Tests](#6-network-performance-tests)
7. [Implementation Examples](#7-implementation-examples)
8. [Analysis and Reporting](#8-analysis-and-reporting)

---

## 1. Overview

### 1.1 Objectives

- **Determine optimal thread counts** for different core configurations
- **Identify performance bottlenecks** at various hardware scales
- **Generate tuning recommendations** for deployment scenarios
- **Establish performance baselines** for capacity planning

### 1.2 Test Matrix

| Configuration Dimension | Test Range | Priority |
|------------------------|------------|----------|
| CPU Cores | 1, 2, 4, 8, 16, 32, 64 | Critical |
| Threads per Core | 1, 2 (hyperthreading) | Critical |
| Memory Size | 8GB, 16GB, 32GB, 64GB, 128GB | High |
| Storage Type | HDD, SATA SSD, NVMe, NVMe Gen4 | High |
| NUMA Nodes | 1, 2, 4 | Medium |
| Network Bandwidth | 1Gbps, 10Gbps, 25Gbps | Medium |

### 1.3 Key Performance Indicators

```yaml
kpis:
  throughput:
    - ops_per_second
    - transactions_per_second
    - queries_per_second
  
  latency:
    - p50_milliseconds
    - p95_milliseconds
    - p99_milliseconds
    - p999_milliseconds
  
  efficiency:
    - cpu_utilization_percent
    - memory_utilization_percent
    - cache_hit_rate_percent
    - scaling_efficiency  # speedup / cores
  
  stability:
    - coefficient_of_variation
    - error_rate
    - timeout_count
```

---

## 2. Core Count Scaling Tests

### 2.1 Test Methodology

**Objective:** Measure how performance scales from 1 to 64+ cores.

**Test Configuration:**
```python
core_scaling_test = {
    "core_counts": [1, 2, 4, 8, 16, 32, 64],
    "workload": "ycsb_workload_a",  # 50% read, 50% update
    "dataset_size": 10_000_000,  # 10M records
    "operations": 10_000_000,    # 10M operations
    "duration": 300,  # 5 minutes per test
    "repetitions": 5,  # For statistical significance
}
```

### 2.2 Workloads to Test

#### 2.2.1 Write-Heavy Workload
```yaml
write_heavy:
  name: "Sequential Write Test"
  operations:
    - insert: 80%
    - update: 20%
  expected_scaling:
    - 1_core: 10000 ops/sec (baseline)
    - 8_cores: 60000 ops/sec (6x, 75% efficiency)
    - 16_cores: 100000 ops/sec (10x, 62% efficiency)
    - 32_cores: 160000 ops/sec (16x, 50% efficiency)
  bottleneck: "Storage I/O, lock contention"
```

#### 2.2.2 Read-Heavy Workload
```yaml
read_heavy:
  name: "Point Query Test"
  operations:
    - read: 95%
    - update: 5%
  expected_scaling:
    - 1_core: 50000 ops/sec (baseline)
    - 8_cores: 380000 ops/sec (7.6x, 95% efficiency)
    - 16_cores: 700000 ops/sec (14x, 87% efficiency)
    - 32_cores: 1200000 ops/sec (24x, 75% efficiency)
  bottleneck: "Memory bandwidth, cache coherency"
```

#### 2.2.3 Analytical Workload
```yaml
analytical:
  name: "Complex Aggregation Test"
  operations:
    - aggregation: "SELECT category, COUNT(*), AVG(price) GROUP BY category"
    - scan: "Full table scan with filter"
  expected_scaling:
    - 1_core: 1 query in 10s (baseline)
    - 8_cores: 1 query in 1.5s (6.7x, 83% efficiency)
    - 16_cores: 1 query in 0.8s (12.5x, 78% efficiency)
    - 32_cores: 1 query in 0.5s (20x, 62% efficiency)
  bottleneck: "Memory bandwidth, CPU computation"
```

### 2.3 Scaling Efficiency Calculation

```python
def calculate_scaling_efficiency(baseline_perf, core_count, measured_perf):
    """
    Calculate how efficiently cores are being used.
    
    Returns:
        efficiency: Float between 0.0 and 1.0
        speedup: Float (measured_perf / baseline_perf)
    """
    speedup = measured_perf / baseline_perf
    ideal_speedup = core_count
    efficiency = speedup / ideal_speedup
    
    return {
        "speedup": speedup,
        "efficiency": efficiency,
        "ideal_speedup": ideal_speedup,
        "grade": get_efficiency_grade(efficiency)
    }

def get_efficiency_grade(efficiency):
    """Grade scaling efficiency."""
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
```

### 2.4 Expected Results Analysis

**Interpretation Guide:**

| Core Count | Expected Efficiency | Common Bottlenecks |
|-----------|--------------------|--------------------|
| 1 → 2 | 90-95% | Minimal overhead |
| 1 → 4 | 85-90% | Cache coherency begins |
| 1 → 8 | 75-85% | Memory bandwidth pressure |
| 1 → 16 | 65-75% | NUMA effects, lock contention |
| 1 → 32 | 50-65% | Cross-socket communication |
| 1 → 64+ | 40-55% | Scheduling overhead |

---

## 3. Thread Configuration Optimization

### 3.1 Thread Pool Sizing

**Objective:** Find optimal thread count for different workloads.

**Test Matrix:**
```python
thread_optimization = {
    "hardware": {
        "physical_cores": 8,
        "hyperthreading": True,
        "logical_cores": 16
    },
    "thread_counts_to_test": [
        1,    # Single-threaded
        4,    # Half physical cores
        8,    # All physical cores
        12,   # 1.5x physical cores
        16,   # All logical cores (hyperthreading)
        24,   # 1.5x logical cores
        32,   # 2x logical cores
        64    # 4x logical cores (oversubscription)
    ],
    "workloads": [
        "cpu_intensive",      # Compute-bound
        "io_intensive",       # Storage-bound
        "memory_intensive",   # Memory-bound
        "network_intensive"   # Network-bound
    ]
}
```

### 3.2 Hyperthreading Evaluation

**Test:** Compare performance with and without hyperthreading.

```yaml
hyperthreading_test:
  cpu_configuration:
    physical_cores: 16
    logical_cores: 32  # With HT
  
  test_cases:
    - name: "HT Disabled"
      threads: 16
      expected_use_case: "CPU-intensive, no I/O wait"
      
    - name: "HT Enabled - Match Physical"
      threads: 16
      expected_benefit: "Minimal (0-5%)"
      
    - name: "HT Enabled - Match Logical"
      threads: 32
      expected_benefit: "10-30% for I/O-bound workloads"
      
    - name: "HT Enabled - Oversubscribed"
      threads: 64
      expected_benefit: "Negative (-5 to -20%) due to context switching"
```

**Expected Results:**

| Workload Type | Optimal Thread Count | HT Benefit |
|--------------|---------------------|------------|
| CPU-intensive (compute) | Physical cores | 0-10% |
| I/O-intensive (storage) | 1.5-2x physical cores | 20-40% |
| Memory-intensive | Physical cores | 5-15% |
| Mixed (OLTP) | 1.2-1.5x physical cores | 15-25% |

### 3.3 Context Switching Analysis

**Monitor:**
```bash
# Linux: Monitor context switches
vmstat 1 10  # Context switches per second

# Expected ranges:
# Low:    < 5,000 cs/sec   (good)
# Medium: 5,000-20,000     (acceptable)
# High:   > 20,000         (investigate thread count)
```

**Python Monitoring:**
```python
import psutil

def monitor_context_switches(duration_seconds):
    """Monitor context switch rate."""
    initial = psutil.cpu_stats().ctx_switches
    time.sleep(duration_seconds)
    final = psutil.cpu_stats().ctx_switches
    
    rate = (final - initial) / duration_seconds
    return {
        "context_switches_per_sec": rate,
        "status": "good" if rate < 5000 else "investigate"
    }
```

---

## 4. Memory Architecture Testing

### 4.1 Memory Bandwidth Tests

**Objective:** Measure memory subsystem performance limits.

**Stream Benchmark Adaptation:**
```cpp
// Measure memory bandwidth
void benchmark_memory_bandwidth() {
    const size_t N = 100'000'000;  // 100M elements
    std::vector<double> a(N), b(N), c(N);
    
    // Copy test: c = a
    auto start = high_resolution_clock::now();
    std::copy(a.begin(), a.end(), c.begin());
    auto copy_time = duration_cast<microseconds>(
        high_resolution_clock::now() - start).count();
    
    // Scale test: b = scalar * c
    double scalar = 3.0;
    start = high_resolution_clock::now();
    for (size_t i = 0; i < N; i++) {
        b[i] = scalar * c[i];
    }
    auto scale_time = duration_cast<microseconds>(
        high_resolution_clock::now() - start).count();
    
    // Add test: c = a + b
    start = high_resolution_clock::now();
    for (size_t i = 0; i < N; i++) {
        c[i] = a[i] + b[i];
    }
    auto add_time = duration_cast<microseconds>(
        high_resolution_clock::now() - start).count();
    
    // Calculate bandwidth
    size_t bytes_per_element = sizeof(double);
    double copy_bw_gbps = (N * bytes_per_element * 2) / 
                          (copy_time * 1e-6) / 1e9;
    
    std::cout << "Copy Bandwidth: " << copy_bw_gbps << " GB/s\n";
}

// Expected results (DDR4-3200):
// Single channel: 20-25 GB/s
// Dual channel:   40-50 GB/s
// Quad channel:   80-100 GB/s
```

### 4.2 Cache Efficiency Tests

**Test Working Set Sizes:**
```python
cache_efficiency_tests = {
    "L1_cache": {
        "working_set_kb": 32,
        "expected_latency_ns": 1,
        "test": "point_select_hot_keys"
    },
    "L2_cache": {
        "working_set_kb": 256,
        "expected_latency_ns": 3,
        "test": "point_select_warm_keys"
    },
    "L3_cache": {
        "working_set_kb": 8192,  # 8MB
        "expected_latency_ns": 15,
        "test": "point_select_cached_keys"
    },
    "RAM": {
        "working_set_kb": 1048576,  # 1GB
        "expected_latency_ns": 65,
        "test": "point_select_cold_keys"
    }
}
```

**Cache Miss Rate Monitoring:**
```bash
# Linux perf tool
perf stat -e cache-references,cache-misses \
  ./themis_benchmark --workload point_select

# Target cache miss rates:
# L1: < 5%
# L2: < 20%
# L3: < 40%
```

### 4.3 NUMA Configuration Tests

**For Multi-Socket Systems:**

```yaml
numa_tests:
  hardware:
    sockets: 2
    cores_per_socket: 16
    memory_per_node_gb: 64
    
  configurations:
    - name: "Local Memory Access"
      description: "Threads and memory on same NUMA node"
      numactl: "numactl --cpunodebind=0 --membind=0"
      expected_improvement: "baseline"
      
    - name: "Remote Memory Access"
      description: "Threads on node 0, memory on node 1"
      numactl: "numactl --cpunodebind=0 --membind=1"
      expected_penalty: "30-50% slower"
      
    - name: "Interleaved Memory"
      description: "Memory interleaved across nodes"
      numactl: "numactl --interleave=all"
      expected_result: "15-25% slower than local"
      
    - name: "Preferred Node"
      description: "Prefer local, allow remote on overflow"
      numactl: "numactl --preferred=0"
      expected_result: "5-15% slower than local"
```

**NUMA Distance Matrix:**
```
# Example: 2-socket system
     Node 0   Node 1
0:     10       21
1:     21       10

# 10 = local access
# 21 = remote access (2.1x cost)
```

---

## 5. Storage Configuration Tests

### 5.1 Storage Type Comparison

**Test Matrix:**
```python
storage_benchmarks = {
    "configurations": [
        {
            "type": "HDD",
            "model": "Western Digital Blue 7200 RPM",
            "interface": "SATA III",
            "expected_iops": 200,
            "expected_bandwidth_mbps": 150,
            "expected_latency_ms": 8
        },
        {
            "type": "SATA_SSD",
            "model": "Samsung 870 EVO",
            "interface": "SATA III",
            "expected_iops": 100000,
            "expected_bandwidth_mbps": 550,
            "expected_latency_us": 100
        },
        {
            "type": "NVMe_Gen3",
            "model": "Samsung 970 EVO Plus",
            "interface": "PCIe 3.0 x4",
            "expected_iops": 500000,
            "expected_bandwidth_mbps": 3500,
            "expected_latency_us": 20
        },
        {
            "type": "NVMe_Gen4",
            "model": "Samsung 980 PRO",
            "interface": "PCIe 4.0 x4",
            "expected_iops": 1000000,
            "expected_bandwidth_mbps": 7000,
            "expected_latency_us": 10
        }
    ],
    "tests": [
        "sequential_read",
        "sequential_write",
        "random_read_4k",
        "random_write_4k",
        "mixed_70_30"  # 70% read, 30% write
    ]
}
```

### 5.2 I/O Pattern Analysis

**Database I/O Patterns:**

1. **Write-Ahead Log (WAL)**: Sequential write
2. **Memtable Flush**: Sequential write, large blocks
3. **Compaction**: Sequential read + write
4. **Point Query**: Random read, small blocks
5. **Range Scan**: Sequential read

**Test Configuration:**
```yaml
io_pattern_tests:
  wal_simulation:
    pattern: "sequential_write"
    block_size: "4KB"
    fsync: true
    expected_impact: "50-100 µs per write"
    
  compaction_simulation:
    pattern: "sequential_read_write"
    block_size: "2MB"
    threads: 4
    expected_bandwidth_mbps: 2000-3000
    
  point_query_simulation:
    pattern: "random_read"
    block_size: "4KB"
    queue_depth: 32
    expected_iops: 50000-500000  # Depends on storage
```

### 5.3 RocksDB Tuning for Storage

**Recommendations by Storage Type:**

```yaml
rocksdb_tuning:
  hdd:
    write_buffer_size_mb: 128
    max_write_buffer_number: 4
    level0_file_num_compaction_trigger: 4
    max_background_jobs: 2
    rationale: "Reduce compaction overhead, batch writes"
    
  sata_ssd:
    write_buffer_size_mb: 64
    max_write_buffer_number: 3
    level0_file_num_compaction_trigger: 4
    max_background_jobs: 4
    rationale: "Balance between memory and I/O"
    
  nvme_gen3:
    write_buffer_size_mb: 32
    max_write_buffer_number: 2
    level0_file_num_compaction_trigger: 8
    max_background_jobs: 8
    rationale: "Leverage high IOPS, faster compaction"
    
  nvme_gen4:
    write_buffer_size_mb: 16
    max_write_buffer_number: 2
    level0_file_num_compaction_trigger: 10
    max_background_jobs: 16
    rationale: "Maximize parallelism for extreme I/O"
```

---

## 6. Network Performance Tests

### 6.1 Latency Tests

**Objective:** Measure network round-trip time impact on performance.

```python
network_latency_tests = {
    "scenarios": [
        {
            "name": "Localhost",
            "latency_us": 50,
            "description": "Client and server on same machine"
        },
        {
            "name": "Same Rack",
            "latency_us": 100,
            "description": "Sub-millisecond LAN"
        },
        {
            "name": "Same Datacenter",
            "latency_us": 500,
            "description": "Cross-rack communication"
        },
        {
            "name": "Cross Datacenter",
            "latency_ms": 10,
            "description": "Regional replication"
        },
        {
            "name": "Cross Continent",
            "latency_ms": 100,
            "description": "Global replication"
        }
    ],
    "operations_to_test": [
        "point_read",
        "point_write",
        "batch_read_100",
        "transaction_2pc"
    ]
}
```

**Expected Impact:**
```
Operation Latency = DB Processing Time + Network RTT

Example for point read:
- DB processing: 0.5ms
- Network RTT: 0.1ms
- Total: 0.6ms (17% network overhead)

Example for cross-datacenter:
- DB processing: 0.5ms
- Network RTT: 10ms
- Total: 10.5ms (95% network overhead!)
```

### 6.2 Bandwidth Tests

**Bulk Transfer Performance:**
```yaml
bandwidth_tests:
  bulk_insert_1m_records:
    payload_size_per_record: 1024  # 1KB
    total_data_gb: 1
    network_speeds:
      - name: "1 Gbps"
        theoretical_max_seconds: 8
        expected_actual_seconds: 12  # 67% efficiency
      - name: "10 Gbps"
        theoretical_max_seconds: 0.8
        expected_actual_seconds: 1.2
      - name: "25 Gbps"
        theoretical_max_seconds: 0.32
        expected_actual_seconds: 0.5
```

---

## 7. Implementation Examples

### 7.1 Python Benchmark Runner

```python
#!/usr/bin/env python3
"""
Hardware Configuration Benchmark Suite
Run benchmarks across different hardware configurations.
"""

import asyncio
import psutil
import subprocess
from dataclasses import dataclass
from typing import List, Dict
import json
import time

@dataclass
class HardwareProfile:
    """System hardware profile."""
    cpu_count: int
    cpu_cores_physical: int
    cpu_freq_max_ghz: float
    memory_total_gb: float
    storage_type: str
    numa_nodes: int

@dataclass
class BenchmarkConfig:
    """Benchmark configuration."""
    name: str
    core_counts: List[int]
    thread_counts: List[int]
    workload: str
    duration_seconds: int
    repetitions: int

class HardwareScalingBenchmark:
    """Run scaling benchmarks across hardware configurations."""
    
    def __init__(self):
        self.hardware = self.detect_hardware()
        self.results = []
        
    def detect_hardware(self) -> HardwareProfile:
        """Detect system hardware configuration."""
        return HardwareProfile(
            cpu_count=psutil.cpu_count(logical=True),
            cpu_cores_physical=psutil.cpu_count(logical=False),
            cpu_freq_max_ghz=psutil.cpu_freq().max / 1000.0,
            memory_total_gb=psutil.virtual_memory().total / (1024**3),
            storage_type=self.detect_storage_type(),
            numa_nodes=self.detect_numa_nodes()
        )
    
    def detect_storage_type(self) -> str:
        """Detect primary storage type."""
        try:
            result = subprocess.run(
                ["lsblk", "-d", "-o", "name,rota"],
                capture_output=True, text=True
            )
            # 0 = SSD, 1 = HDD
            if "0" in result.stdout:
                # Check if NVMe
                if "nvme" in result.stdout:
                    return "NVMe"
                return "SSD"
            return "HDD"
        except:
            return "Unknown"
    
    def detect_numa_nodes(self) -> int:
        """Detect number of NUMA nodes."""
        try:
            result = subprocess.run(
                ["numactl", "--hardware"],
                capture_output=True, text=True
            )
            lines = result.stdout.split('\n')
            for line in lines:
                if "available:" in line:
                    return int(line.split()[1])
        except:
            pass
        return 1
    
    async def run_benchmark(self, cores: int, threads: int, 
                          workload: str, duration: int) -> Dict:
        """Run single benchmark configuration."""
        print(f"Running: cores={cores}, threads={threads}, "
              f"workload={workload}")
        
        # Build taskset command to pin to specific cores
        cpu_list = ",".join(str(i) for i in range(cores))
        
        cmd = [
            "taskset", "-c", cpu_list,
            "./themis_benchmark",
            "--workload", workload,
            "--threads", str(threads),
            "--duration", str(duration),
            "--output", "json"
        ]
        
        start_time = time.time()
        start_cpu = psutil.cpu_percent(interval=1)
        
        # Run benchmark
        result = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        stdout, stderr = await result.communicate()
        
        end_time = time.time()
        end_cpu = psutil.cpu_percent(interval=1)
        
        # Parse results
        try:
            benchmark_result = json.loads(stdout.decode())
        except:
            benchmark_result = {"error": "Failed to parse output"}
        
        return {
            "cores": cores,
            "threads": threads,
            "workload": workload,
            "duration_seconds": end_time - start_time,
            "cpu_utilization_percent": (start_cpu + end_cpu) / 2,
            "result": benchmark_result
        }
    
    async def run_scaling_suite(self, config: BenchmarkConfig):
        """Run complete scaling benchmark suite."""
        print(f"\n{'='*60}")
        print(f"Hardware Configuration Benchmark: {config.name}")
        print(f"{'='*60}")
        print(f"Hardware Profile:")
        print(f"  CPU Cores (physical): {self.hardware.cpu_cores_physical}")
        print(f"  CPU Cores (logical):  {self.hardware.cpu_count}")
        print(f"  CPU Frequency:        {self.hardware.cpu_freq_max_ghz:.2f} GHz")
        print(f"  Memory:               {self.hardware.memory_total_gb:.1f} GB")
        print(f"  Storage:              {self.hardware.storage_type}")
        print(f"  NUMA Nodes:           {self.hardware.numa_nodes}")
        print(f"{'='*60}\n")
        
        for core_count in config.core_counts:
            for thread_count in config.thread_counts:
                for rep in range(config.repetitions):
                    result = await self.run_benchmark(
                        cores=core_count,
                        threads=thread_count,
                        workload=config.workload,
                        duration=config.duration_seconds
                    )
                    self.results.append(result)
                    
                    # Brief pause between runs
                    await asyncio.sleep(5)
        
        return self.analyze_results()
    
    def analyze_results(self) -> Dict:
        """Analyze scaling efficiency."""
        analysis = {
            "hardware": self.hardware.__dict__,
            "results": self.results,
            "scaling_efficiency": self.calculate_scaling_efficiency()
        }
        return analysis
    
    def calculate_scaling_efficiency(self) -> Dict:
        """Calculate scaling efficiency metrics."""
        # Group results by configuration
        baseline = None
        efficiency_data = []
        
        for result in self.results:
            if result["cores"] == 1:
                baseline = result["result"].get("throughput", 0)
            
            if baseline and baseline > 0:
                current_throughput = result["result"].get("throughput", 0)
                speedup = current_throughput / baseline
                ideal_speedup = result["cores"]
                efficiency = speedup / ideal_speedup
                
                efficiency_data.append({
                    "cores": result["cores"],
                    "threads": result["threads"],
                    "speedup": speedup,
                    "efficiency": efficiency,
                    "grade": self.get_efficiency_grade(efficiency)
                })
        
        return efficiency_data
    
    def get_efficiency_grade(self, efficiency: float) -> str:
        """Grade scaling efficiency."""
        if efficiency >= 0.90:
            return "A+"
        elif efficiency >= 0.80:
            return "A"
        elif efficiency >= 0.70:
            return "B"
        elif efficiency >= 0.60:
            return "C"
        elif efficiency >= 0.50:
            return "D"
        else:
            return "F"
    
    def export_results(self, filename: str):
        """Export results to JSON file."""
        analysis = self.analyze_results()
        with open(filename, 'w') as f:
            json.dump(analysis, f, indent=2)
        print(f"\nResults exported to: {filename}")

async def main():
    """Main entry point."""
    benchmark = HardwareScalingBenchmark()
    
    # Configure benchmark suite
    config = BenchmarkConfig(
        name="ThemisDB Core Scaling Test",
        core_counts=[1, 2, 4, 8],  # Adjust based on available cores
        thread_counts=[1],  # Test 1 thread per core first
        workload="ycsb_workload_a",
        duration_seconds=60,
        repetitions=3
    )
    
    # Run benchmarks
    await benchmark.run_scaling_suite(config)
    
    # Export results
    benchmark.export_results("hardware_scaling_results.json")

if __name__ == "__main__":
    asyncio.run(main())
```

### 7.2 Configuration Generator

```python
#!/usr/bin/env python3
"""
Generate optimized ThemisDB configurations based on hardware.
"""

import psutil
import json

def generate_config_for_hardware() -> Dict:
    """Generate optimized configuration based on detected hardware."""
    
    cores_physical = psutil.cpu_count(logical=False)
    cores_logical = psutil.cpu_count(logical=True)
    memory_gb = psutil.virtual_memory().total / (1024**3)
    
    # Thread pool sizing
    # Rule: For OLTP, use physical cores * 1.5
    #       For OLAP, use physical cores
    thread_pool_size = int(cores_physical * 1.5)
    
    # Memory allocation
    # Rule: Allocate 60% of RAM to block cache
    #       Allocate 20% to write buffers
    block_cache_gb = int(memory_gb * 0.6)
    write_buffer_mb = int((memory_gb * 0.2) * 1024)
    
    # Compaction threads
    # Rule: 2-4 threads for HDD, 4-8 for SSD, 8-16 for NVMe
    storage_type = detect_storage_type()
    if storage_type == "NVMe":
        max_background_jobs = 16
    elif storage_type == "SSD":
        max_background_jobs = 8
    else:
        max_background_jobs = 4
    
    config = {
        "database": {
            "thread_pool_size": thread_pool_size,
            "max_connections": thread_pool_size * 4
        },
        "rocksdb": {
            "block_cache_size_gb": block_cache_gb,
            "write_buffer_size_mb": write_buffer_mb,
            "max_background_jobs": max_background_jobs,
            "max_write_buffer_number": 3,
            "level0_file_num_compaction_trigger": 8 if storage_type == "NVMe" else 4
        },
        "hardware_profile": {
            "cores_physical": cores_physical,
            "cores_logical": cores_logical,
            "memory_gb": memory_gb,
            "storage_type": storage_type
        }
    }
    
    return config

def detect_storage_type() -> str:
    """Detect storage type (simplified)."""
    # Implementation similar to previous example
    return "SSD"  # Placeholder

if __name__ == "__main__":
    config = generate_config_for_hardware()
    print(json.dumps(config, indent=2))
    
    with open("themisdb_optimized.yaml", "w") as f:
        f.write("# ThemisDB Optimized Configuration\n")
        f.write("# Generated based on hardware detection\n\n")
        for section, values in config.items():
            if isinstance(values, dict):
                f.write(f"{section}:\n")
                for key, value in values.items():
                    f.write(f"  {key}: {value}\n")
            else:
                f.write(f"{section}: {values}\n")
```

---

## 8. Analysis and Reporting

### 8.1 Performance Report Template

```markdown
# ThemisDB Hardware Configuration Performance Report

## Test Environment
- **Date**: 2025-12-23
- **ThemisDB Version**: 1.3.0
- **Hardware**: Intel Xeon Gold 6248, 32 cores @ 2.5 GHz
- **Memory**: 128 GB DDR4-2933
- **Storage**: Samsung 980 PRO 1TB (NVMe Gen4)
- **OS**: Ubuntu 22.04 LTS

## Scaling Efficiency Results

### Core Count Scaling (YCSB Workload A)

| Cores | Threads | Throughput (ops/s) | Speedup | Efficiency | Grade |
|-------|---------|-------------------|---------|------------|-------|
| 1     | 1       | 85,234            | 1.0x    | 100%       | -     |
| 2     | 2       | 162,445           | 1.9x    | 95%        | A+    |
| 4     | 4       | 310,892           | 3.6x    | 90%        | A+    |
| 8     | 8       | 580,123           | 6.8x    | 85%        | A     |
| 16    | 16      | 1,045,678         | 12.3x   | 77%        | B     |
| 32    | 32      | 1,823,456         | 21.4x   | 67%        | C     |

### Analysis
- **Linear Scaling**: Up to 4 cores (90% efficiency)
- **Good Scaling**: 4-8 cores (85% efficiency)
- **Acceptable**: 8-32 cores (67% efficiency)
- **Bottleneck**: Memory bandwidth saturation beyond 16 cores

### Recommendations
1. **Optimal Configuration**: 16 cores for best price/performance
2. **Thread Pool**: Set to 24 threads (16 * 1.5) for mixed workload
3. **NUMA**: Enable NUMA awareness for >16 core systems
4. **Memory**: Consider faster memory (DDR5) for >32 core scaling

## Workload-Specific Findings

### Read-Heavy (95% read, 5% write)
- Scales very well up to 32 cores (80% efficiency)
- Bottleneck: L3 cache capacity, memory bandwidth

### Write-Heavy (80% write, 20% read)
- Moderate scaling up to 16 cores (70% efficiency)
- Bottleneck: Storage I/O, lock contention on RocksDB memtable

### Analytical (OLAP queries)
- Good scaling up to 16 cores (75% efficiency)
- Bottleneck: Memory bandwidth during scans

## Hardware Tuning Recommendations

### For This Configuration (32-core, 128GB, NVMe)
```yaml
# Recommended ThemisDB configuration
database:
  thread_pool_size: 48
  max_connections: 192

rocksdb:
  block_cache_size: 77GB  # 60% of RAM
  write_buffer_size: 128MB
  max_write_buffer_number: 3
  max_background_jobs: 16
  level0_file_num_compaction_trigger: 8
  
  # NUMA optimization
  use_numa_aware_allocator: true
  numa_node: 0  # Pin to specific node for best performance
```

## Conclusion
ThemisDB demonstrates excellent scaling characteristics up to 16 cores with
proper configuration. Beyond 16 cores, memory bandwidth becomes the limiting
factor. For maximum throughput, deploy multiple ThemisDB instances rather
than using all available cores in a single instance.
```

### 8.2 Visualization Script

```python
#!/usr/bin/env python3
"""
Generate performance visualizations.
"""

import json
import matplotlib.pyplot as plt
import numpy as np

def plot_scaling_efficiency(results_file: str):
    """Generate scaling efficiency charts."""
    with open(results_file) as f:
        data = json.load(f)
    
    efficiency = data['scaling_efficiency']
    
    cores = [e['cores'] for e in efficiency]
    speedup = [e['speedup'] for e in efficiency]
    eff_pct = [e['efficiency'] * 100 for e in efficiency]
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # Speedup chart
    ax1.plot(cores, speedup, 'o-', label='Actual Speedup', linewidth=2)
    ax1.plot(cores, cores, '--', label='Ideal Speedup', linewidth=2)
    ax1.set_xlabel('Core Count')
    ax1.set_ylabel('Speedup')
    ax1.set_title('Scaling Speedup')
    ax1.legend()
    ax1.grid(True)
    
    # Efficiency chart
    ax2.plot(cores, eff_pct, 'o-', linewidth=2)
    ax2.axhline(y=80, color='r', linestyle='--', label='80% Target')
    ax2.set_xlabel('Core Count')
    ax2.set_ylabel('Efficiency (%)')
    ax2.set_title('Scaling Efficiency')
    ax2.legend()
    ax2.grid(True)
    
    plt.tight_layout()
    plt.savefig('scaling_efficiency.png', dpi=300)
    print("Chart saved: scaling_efficiency.png")

if __name__ == "__main__":
    plot_scaling_efficiency("hardware_scaling_results.json")
```

---

## Appendix: Quick Reference

### Optimal Thread Counts by Workload

| Workload | Formula | Example (16 cores) |
|----------|---------|-------------------|
| CPU-intensive | Physical cores | 16 threads |
| I/O-intensive | Physical cores * 2 | 32 threads |
| Mixed OLTP | Physical cores * 1.5 | 24 threads |
| OLAP | Physical cores | 16 threads |
| Vector Search | Physical cores * 0.75 | 12 threads |

### Performance Targets by Core Count

| Cores | OLTP (ops/s) | OLAP (queries/min) | Vector (queries/s) |
|-------|-------------|-------------------|-------------------|
| 4     | 200-300K    | 50-100            | 5-10K             |
| 8     | 400-600K    | 100-200           | 10-20K            |
| 16    | 700-1000K   | 200-400           | 20-40K            |
| 32    | 1.2-1.8M    | 400-800           | 40-80K            |

---

**Document Version:** 1.0  
**Last Updated:** 2026-04-06  
**Status:** ✅ Ready for Implementation
