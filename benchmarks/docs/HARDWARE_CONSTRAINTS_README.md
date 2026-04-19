> ⚠️ **Historische Randbedingungen** – Hardware-Constraints beschreiben einen bestimmten Teststand.

# Hardware Constraints & Performance Characteristics
## Analyzing ThemisDB Against RocksDB & TBB Baselines

---

## Table of Contents

1. [Overview](#overview)
2. [Hardware Constraint Factors](#hardware-constraint-factors)
3. [RocksDB Baseline Performance](#rocksdb-baseline-performance)
4. [TBB Scaling Expectations](#tbb-scaling-expectations)
5. [ThemisDB Compliance Analysis](#themisdb-compliance-analysis)
6. [Benchmarking Hardware Impact](#benchmarking-hardware-impact)
7. [Usage & Integration](#usage--integration)
8. [Interpretation Guide](#interpretation-guide)

---

## Overview

Modern databases are fundamentally constrained by hardware limitations. This document explains how to analyze and compare ThemisDB performance against industry standards (RocksDB, TBB) while accounting for hardware differences.

### Why Hardware Constraints Matter

```
┌─────────────────────────────────────────────────┐
│ Database Performance = f(Algorithm, Hardware)   │
│                                                 │
│ Same code performs differently on:              │
│ • 8-core vs 32-core systems                     │
│ • DDR4 vs DDR5 memory                           │
│ • SSD vs NVMe storage                           │
│ • Local vs distributed systems                  │
└─────────────────────────────────────────────────┘
```

### Key Principle: Expectation Management

**Performance expectations must be normalized to hardware limits:**

```
Database Efficiency % = Actual Performance / Theoretical Hardware Maximum
```

If hardware max = 100 Mops/sec and ThemisDB = 80 Mops/sec:
- ✅ 80% efficiency (good!)
- NOT a direct comparison to RocksDB's 100 Mops/sec on different hardware

---

## Hardware Constraint Factors

### 1. CPU Constraints

**Impact on throughput:**
- Single core: Sequential operations, no parallelism
- Multiple cores: Parallel operations, but with coordination overhead
- Cache efficiency: L1 (fast) → L2 (medium) → L3 (slow) → Memory (very slow)

**ThemisDB Relevance:**
- Vector operations benefit from multiple cores
- Cache locality affects search performance
- Context switching reduces efficiency with too many threads

**Measurement:**
```python
cpu_bound_throughput = cores * frequency * ipc / latency_cycles
# IPC = Instructions Per Cycle (typically 4-5)
```

### 2. Memory Constraints

**Impact on throughput:**
- Bandwidth ceiling: ~50-100 GB/sec per 8 cores
- Latency floor: ~65ns for DDR4, ~40ns for L3 cache
- Access patterns: Sequential (fast) vs Random (slow)

**ThemisDB Relevance:**
- MVCC metadata stored in memory
- Index structures benefit from sequential prefetching
- Large datasets require efficient cache usage

**Measurement:**
```python
memory_bound_throughput = memory_bandwidth_gb_sec * 1000 / (bytes_per_operation)
```

### 3. Storage I/O Constraints

**Impact on throughput:**
- IOPS ceiling: ~100K (SSD) to ~500K (NVMe)
- Throughput ceiling: ~3.5 GB/sec (SSD) to ~7 GB/sec (NVMe)
- Latency floor: ~50-100µs for SSD

**ThemisDB Relevance:**
- Compaction requires intensive I/O
- LSM tree structure affects I/O patterns
- Write amplification determined by storage characteristics

**Measurement:**
```python
io_bound_throughput = min(storage_iops, storage_bandwidth_gb_sec * (1e9 / bytes_per_op))
```

### 4. Network Constraints (Distributed)

**Impact on throughput:**
- Latency: ~0.1ms (LAN) to ~100ms (WAN)
- Bandwidth: ~1 Gb/sec to ~100 Gb/sec (datacenter)

**Relevance:** Future distributed ThemisDB features

---

## RocksDB Baseline Performance

RocksDB is Facebook's production-grade key-value store used as baseline:

### Reference Values by Operation Type

#### Random Read Performance

| Thread Count | Throughput (ops/sec) | Latency P99 (µs) |
|:---:|:---:|:---:|
| 1 | 500,000 | 50 |
| 8 | 2,000,000 | 45 |
| 32 | 4,000,000 | 40 |

**Scaling Factor:** ~4x improvement from 1→8 threads (87.5% efficiency)

#### Random Write Performance

| Thread Count | Throughput (ops/sec) | Latency P99 (µs) |
|:---:|:---:|:---:|
| 1 | 150,000 | 100 |
| 8 | 500,000 | 95 |
| 32 | 1,000,000 | 90 |

**Scaling Factor:** ~3.3x improvement from 1→8 threads (write-amplification overhead)

#### Sequential Scan Performance

| Thread Count | Throughput (MB/sec) |
|:---:|:---:|
| 1 | 500 |
| 8 | 2,000 |
| 32 | 3,500 |

**Scaling Factor:** ~4x improvement (good memory utilization)

### Why These Numbers?

**Read Performance Limited By:**
- CPU cache misses (L3 cache miss ratio ~5-10%)
- Memory latency (65ns per access)
- TBB task overhead (~1µs per operation)

**Write Performance Lower Due To:**
- Write-amplification (compaction overhead)
- Fsync operations (expensive on SSD)
- WAL (Write-Ahead Log) serialization

**Scan Performance Near-Linear Because:**
- Sequential memory access (prefetcher works well)
- Minimal cache misses
- Better CPU utilization

---

## TBB Scaling Expectations

Intel's Threading Building Blocks (TBB) provides expected scaling characteristics:

### Amdahl's Law

```
Speedup = 1 / ((1 - p) + p/N)

Where:
  p = parallelizable fraction (typically 0.95-0.99)
  N = number of threads
```

### Practical Scaling Targets

| Threads | Ideal Speedup | TBB Expected | Acceptable |
|:---:|:---:|:---:|:---:|
| 1 | 1.0x | 1.0x | 1.0x |
| 8 | 8.0x | 7.0x | 6.5x |
| 16 | 16.0x | 13.0x | 12.0x |
| 32 | 32.0x | 24.0x | 22.0x |

### Why Not Linear?

**Scaling Inefficiency Factors:**
- Lock contention (increasing with threads)
- Cache coherency overhead (NUMA effects)
- Task spawn/context switch overhead
- Memory bandwidth saturation

**Efficiency = Speedup / Thread Count**

```
8 threads: 7.0x / 8 = 87.5% efficiency ✅
16 threads: 13.0x / 16 = 81.25% efficiency ✅
32 threads: 24.0x / 32 = 75.0% efficiency ⚠️
```

---

## ThemisDB Compliance Analysis

### Compliance Metrics

ThemisDB is evaluated on:

1. **Performance Ratio (vs RocksDB)**
   - Ratio = ThemisDB Performance / RocksDB Baseline
   - Target: ≥ 0.80 (80% of RocksDB on same hardware)

2. **Scaling Efficiency (vs TBB)**
   - Efficiency = Speedup / Threads
   - Target: ≥ 0.75 (75% efficiency, TBB-like)

3. **Hardware Efficiency**
   - Efficiency = Actual / Theoretical Maximum
   - Target: ≥ 0.60 (60% of hardware maximum)

### Grading System

| Grade | Range | Meaning |
|:---:|:---:|:---|
| 🟢 A | ≥ 95% | Exceeds expectations |
| 🟢 A- | 90-95% | Matches expectations |
| 🟡 B+ | 85-90% | Near expectations |
| 🟡 B | 75-85% | Good performance |
| 🟠 C | 60-75% | Acceptable |
| 🔴 D | 40-60% | Below expectation |
| 🔴 F | < 40% | Critical issue |

### Example Analysis

**Scenario:** ThemisDB on 8-core system

```
Hardware Maximum:        100 Mops/sec
RocksDB Baseline (8t):   2,000,000 ops/sec
ThemisDB Measured:       1,600,000 ops/sec

Calculations:
  Ratio to RocksDB = 1,600,000 / 2,000,000 = 0.80 = 80% ✅ Grade: B+
  Hardware Efficiency = 1,600,000 / 100,000,000 = 1.6% ⚠️ (needs investigation)
```

**Interpretation:**
- ✅ Good: ThemisDB matches RocksDB performance (80%)
- ⚠️ Flag: Low hardware efficiency suggests room for optimization

---

## Benchmarking Hardware Impact

### Case Study: Same Database, Different Hardware

#### System A (Entry-Level)
- 4 cores @ 3.2 GHz
- 8 GB RAM (20 GB/sec bandwidth)
- SSD storage (100K IOPS)
- RocksDB baseline: 500K ops/sec (1 thread), 1.5M ops/sec (4 threads)

#### System B (Mid-Range)
- 8 cores @ 3.4 GHz
- 16 GB RAM (50 GB/sec bandwidth)
- NVMe storage (500K IOPS)
- RocksDB baseline: 500K ops/sec (1 thread), 2M ops/sec (8 threads)

#### System C (High-Performance)
- 32 cores @ 3.8 GHz
- 128 GB RAM (100 GB/sec bandwidth)
- Enterprise NVMe (1M IOPS)
- RocksDB baseline: 500K ops/sec (1 thread), 4M ops/sec (32 threads)

**Key Insight:** Single-thread performance similar, multi-thread varies 3-8x!

### Correct Analysis

```
❌ WRONG:
  "ThemisDB on System A: 1.2M ops/sec"
  "ThemisDB on System C: 3.2M ops/sec"
  "Conclusion: System C is 2.67x faster"

✅ RIGHT:
  System A: 1.2M / 1.5M (RocksDB) = 80% efficiency
  System C: 3.2M / 4.0M (RocksDB) = 80% efficiency
  Conclusion: ThemisDB scales equally well on both systems
```

---

## Usage & Integration

> **Build:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`


### Python API

#### 1. Basic Compliance Check

```python
from hardware_constraints_integration import HardwareConstraintsIntegration
import asyncio

async def check_compliance():
    # Create integrator
    integrator = HardwareConstraintsIntegration("ThemisDB")
    
    # Your ThemisDB metrics
    metrics = {
        "read_ops_sec": 1600000,
        "write_ops_sec": 450000,
        "scan_mb_sec": 1800,
        "read_latency_p50_us": 6.0,
        "read_latency_p99_us": 55.0,
    }
    
    # Run analysis
    result = await integrator.run_compliance_analysis(metrics)
    
    # Export results
    files = await integrator.export_results(result)
    
asyncio.run(check_compliance())
```

#### 2. Scaling Efficiency Analysis

```python
scaling_metrics = {
    8: 7200000,    # 8 threads → 7.2M ops/sec
    16: 12800000,  # 16 threads → 12.8M ops/sec
    32: 21600000,  # 32 threads → 21.6M ops/sec
}

result = await integrator.run_compliance_analysis(
    metrics,
    scaling_metrics=scaling_metrics
)
```

#### 3. Hardware Impact Analysis

```python
from hardware_constraints_analyzer import HardwareLimits

# Use detected hardware
hardware = integrator.hardware

# Or specify custom hardware
custom_hw = HardwareLimits(
    cpu_cores=16,
    cpu_freq_ghz=3.5,
    memory_total_gb=64,
)

result = await integrator.validator.hardware_constraint_impact(
    custom_hw,
    metrics
)
```

### Command-Line Usage

```bash
# Run compliance analysis
python hardware_constraints_analyzer.py

# Export results
python hardware_constraints_integration.py > results.txt

# Integration with complete benchmark suite
python complete_benchmark_suite.py --mode full --analyze-hardware
```

### Integration with Existing Benchmarks

**In `complete_benchmark_suite.py`:**

```python
from hardware_constraints_integration import HardwareConstraintsIntegration

# After running benchmarks
benchmark_results = await suite.run_benchmarks()

# Add hardware analysis
integrator = HardwareConstraintsIntegration("ThemisDB")
compliance = await integrator.run_compliance_analysis(benchmark_results)

# Export combined report
combined_report = {
    "benchmarks": benchmark_results,
    "compliance": compliance,
}
```

---

## Interpretation Guide

### Reading Compliance Reports

#### Output Example

```
╔════════════════════════════════════════════════════════════╗
║         THEMIS vs ROCKSDB COMPLIANCE CHECK                ║
╚════════════════════════════════════════════════════════════╝

Metric                     ThemisDB          RocksDB          Ratio   Grade
──────────────────────────────────────────────────────────────────────────
Read Performance           1,600,000         2,000,000        0.80x   🟡 B+
Write Performance          400,000           500,000          0.80x   🟡 B+
Scan Performance           1,800 MB/sec      2,000 MB/sec     0.90x   🟡 B+
Read Latency P99           55.0 µs           50.0 µs          1.10x   ✅
Write Latency P99          110.0 µs          100.0 µs         1.10x   ✅

Overall Compliance: 84%
Primary Bottleneck: Memory Bandwidth
```

#### What This Means

| Metric | Status | Action |
|:---|:---|:---|
| Read/Write at 80% | ⚠️ Acceptable | Monitor for optimization opportunities |
| Latency slightly higher | ⚠️ Minor | Check CPU cache utilization |
| Memory bottleneck | 🔴 Action | Consider: vectorization, cache tuning, data layout |

### Common Scenarios

#### Scenario 1: CPU-Bound Bottleneck

**Symptoms:**
- Low throughput despite high memory bandwidth available
- High context switching
- Cache misses increasing with thread count

**Causes:**
- Inefficient algorithms (O(n²) loops)
- False sharing (threads competing for cache lines)
- Too many threads (context switch overhead)

**Solutions:**
- Profile with `perf` to identify hot loops
- Reduce thread count to cores/2 or cores
- Use NUMA-aware thread pinning

#### Scenario 2: Memory-Bound Bottleneck

**Symptoms:**
- Throughput increases with clock speed only
- SIMD instructions not effective
- Large data structures causing cache misses

**Causes:**
- Random memory access patterns
- Large working set (> L3 cache)
- Cache-unfriendly data structures

**Solutions:**
- Improve data layout (SoA vs AoS)
- Implement cache-efficient algorithms
- Use vectorization (AVX-512) with prefetching

#### Scenario 3: I/O Bottleneck

**Symptoms:**
- Write performance especially low
- Latency increases with concurrent writes
- Fsync operations dominate profile

**Causes:**
- Write amplification (LSM tree compaction)
- Synchronous I/O in critical path
- Poor I/O scheduling

**Solutions:**
- Batch writes (increase memtable size)
- Use async I/O or write-through cache
- Optimize compaction strategy

### Benchmarking Checklist

Before running compliance analysis:

```
Pre-Benchmark Checklist:
□ System is idle (no background processes)
□ CPU frequency scaling disabled (or pinned to max)
□ NUMA settings known (if applicable)
□ Storage backend characterized (SSD model, health check)
□ Network latency measured (if distributed)
□ Warm-up runs completed (cache primed)

During Benchmark:
□ Multiple repetitions run (≥10 for stability)
□ Outliers removed (IQR method)
□ Hardware counters collected (perf, Intel VTune)
□ System load monitored (top, iostat)

After Benchmark:
□ Confidence intervals calculated (95% CI minimum)
□ Hardware efficiency calculated
□ Bottleneck identified
□ Comparison to baseline valid (same hardware generation)
```

---

## Key Metrics Summary

### Performance Baselines

| Database | Benchmark | Threads | Throughput | Latency P99 |
|:---|:---|:---:|:---|:---|
| RocksDB | Random Read | 8 | 2.0M ops/sec | 45 µs |
| RocksDB | Random Write | 8 | 500K ops/sec | 95 µs |
| RocksDB | Sequential Scan | 8 | 2,000 MB/sec | - |
| TBB Task Pool | Speedup target | 8 | 7.0x (87.5%) | - |
| TBB Task Pool | Speedup target | 32 | 24.0x (75%) | - |

### Hardware Limits (Typical 8-Core System)

| Metric | Value |
|:---|:---|
| CPU Throughput | ~3-5 Gops/sec |
| Memory Bandwidth | ~50 GB/sec |
| Storage IOPS | ~100K (SSD) |
| Storage Bandwidth | ~3.5 GB/sec |
| Max Practical Ops/sec | 100-200M (data structure dependent) |

### Expected ThemisDB Performance

| Operation | Expected (vs RocksDB) | Range |
|:---|:---|:---|
| Random Read | 75-100% | 0.75-1.0x |
| Random Write | 70-90% | 0.70-0.9x |
| Sequential Scan | 85-100% | 0.85-1.0x |
| Scaling (8→32 threads) | 70-85% | 0.70-0.85x efficiency |

---

## References

### RocksDB Benchmarks
- https://github.com/facebook/rocksdb/wiki/Benchmarks
- https://github.com/facebook/rocksdb/wiki/Performance-Benchmarks

### TBB Documentation
- https://spec.oneapi.io/versions/latest/elements/oneTBB/source/index.html
- Intel TBB GitHub: https://github.com/oneapi-src/oneTBB

### Hardware Performance Analysis
- Intel VTune: https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html
- Linux Perf: https://perf.wiki.kernel.org/
- Brendan Gregg's Performance Tools: https://brendangregg.com/linuxperf.html

---

**Last Updated:** 2026-04-06  
**Version:** 1.0  
**Status:** Production Ready
