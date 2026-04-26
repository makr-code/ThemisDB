> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Enhanced Benchmark Suite Integration Guide
## How to Use the New Advanced Benchmarks

**Version:** 1.0  
**Date:** 2025-12-23

---

## Overview

This guide explains how to use the newly added comprehensive benchmark suite that implements scientific and industrial standards for database and AI system testing.

## New Benchmark Components

### 1. Research Documentation

**File:** `ADVANCED_BENCHMARK_RESEARCH.md`

Comprehensive research document covering:
- **TPC Benchmarks** (TPC-C, TPC-H) - Industry standard OLTP/OLAP
- **YCSB Workloads** - Yahoo Cloud Serving Benchmark (Workloads A-F)
- **LDBC** - Graph database benchmarks (Social Network Benchmark)
- **ANN-Benchmarks** - Vector database standard (SIFT1M, Deep1B)
- **RAG Benchmarks** - Retrieval Augmented Generation for LLMs
- **Hardware scaling analysis** - Multi-core, memory, storage, NUMA

**Key Features:**
- Scientific methodology based on published standards
- Expected performance baselines from industry leaders
- Detailed implementation requirements
- Complete references to academic papers

### 2. Hardware Configuration Guide

**File:** `HARDWARE_CONFIGURATION_BENCHMARK_SUITE.md`

Detailed guide for testing across hardware configurations:
- **Core Count Scaling** (1, 2, 4, 8, 16, 32, 64+ cores)
- **Thread Optimization** (hyperthreading, thread pool sizing)
- **Memory Architecture** (bandwidth, cache efficiency, NUMA)
- **Storage Testing** (HDD, SSD, NVMe comparison)
- **Network Performance** (latency, bandwidth impact)

**Includes:**
- Python implementation examples
- Expected performance targets
- Configuration generators
- Analysis and reporting tools

### 3. Hardware Scaling Benchmark Script

**File:** `hardware_scaling_benchmark.py`

Automated benchmark runner that:
- Auto-detects hardware configuration
- Runs scaling tests across core counts
- Calculates scaling efficiency
- Generates reports in JSON and Markdown
- Provides performance grades (A+ to F)

## Quick Start

### Run Basic Hardware Scaling Test

```bash
cd /home/runner/work/ThemisDB/ThemisDB/benchmarks

# Run with default settings (1, 2, 4, 8 cores)
python3 hardware_scaling_benchmark.py

# Run with custom core counts
python3 hardware_scaling_benchmark.py --core-counts "1,2,4,8,16,32"

# Run with different workload
python3 hardware_scaling_benchmark.py --workload oltp --duration 120

# Full custom configuration
python3 hardware_scaling_benchmark.py \
  --core-counts "1,2,4,8,16" \
  --workload ycsb_a \
  --duration 60 \
  --repetitions 5 \
  --output-dir ./scaling_results
```

### Expected Output

```
======================================================================
HARDWARE CONFIGURATION
======================================================================
Hostname:          themis-server-01
Platform:          Linux-5.15.0-91-generic-x86_64-with-glibc2.35
Processor:         x86_64
CPU Cores:         16 physical, 32 logical
CPU Frequency:     3600 MHz (max)
Memory:            64.0 GB total, 58.3 GB available
Storage Type:      NVMe
NUMA Nodes:        1
======================================================================

======================================================================
CORE SCALING BENCHMARK SUITE
======================================================================
Workload:     ycsb_a
Duration:     60s per test
Repetitions:  3
Core counts:  [1, 2, 4, 8, 16]
======================================================================

[Rep 1] Running: cores=1, threads=1, workload=ycsb_a, duration=60s
[Rep 2] Running: cores=1, threads=1, workload=ycsb_a, duration=60s
[Rep 3] Running: cores=1, threads=1, workload=ycsb_a, duration=60s
...

====================================================================================================
SCALING EFFICIENCY ANALYSIS
====================================================================================================
Cores    Threads    Throughput      Speedup      Efficiency   Grade               
----------------------------------------------------------------------------------------------------
1        1          85,234          1.00x        100.0%       -                   
2        2          162,445         1.91x        95.3%        A+ (Excellent)      
4        4          310,892         3.65x        91.2%        A+ (Excellent)      
8        8          580,123         6.81x        85.1%        A (Very Good)       
16       16         1,045,678       12.27x       76.7%        B (Good)            
====================================================================================================

Results exported to: ./benchmark_results/hardware_scaling_results.json
Report generated: ./benchmark_results/hardware_scaling_report.md

======================================================================
BENCHMARK COMPLETE
======================================================================
Results saved to: ./benchmark_results
======================================================================
```

## Integration with Existing Benchmarks

### 1. Add to Existing Benchmark Suite

The new benchmarks complement existing tests in:
- `benchmarks/comparative/` - Database comparisons
- `benchmarks/scientific_benchmark_runner.py` - Scientific standards
- `benchmarks/standard_benchmarks.py` - Standard tests

### 2. Use with Docker Benchmarks

```bash
# Run hardware scaling in Docker
docker-compose -f docker-compose.benchmark.yml run \
  benchmark python3 /benchmarks/hardware_scaling_benchmark.py

# Or integrate with existing Docker benchmark suite
docker-compose -f docker-compose.benchmark.yml run \
  benchmark python3 /benchmarks/run_complete_benchmarks.py --include-hardware-scaling
```

### 3. CI/CD Integration

Add to GitHub Actions or CI pipeline:

```yaml
# .github/workflows/benchmarks.yml
- name: Run Hardware Scaling Benchmarks
  run: |
    cd benchmarks
    python3 hardware_scaling_benchmark.py \
      --core-counts "1,2,4,8" \
      --duration 30 \
      --repetitions 2 \
      --output-dir ${{ github.workspace }}/benchmark-results
    
- name: Upload Results
  uses: actions/upload-artifact@v3
  with:
    name: hardware-scaling-results
    path: benchmark-results/
```

## Benchmark Types Covered

### 1. TPC Benchmarks (To Be Implemented)

Based on `ADVANCED_BENCHMARK_RESEARCH.md`:

**TPC-C** (OLTP):
- Simulates wholesale supplier with warehouses, customers, orders
- 5 transaction types: New Order, Payment, Order Status, Delivery, Stock Level
- Key metric: tpmC (transactions per minute)
- Target: 150,000-200,000 tpmC on 8-core system

**TPC-H** (Decision Support):
- 22 complex analytical queries
- Tests aggregations, joins, subqueries, window functions
- Key metric: QphH@Size (Queries per Hour at Scale Factor)
- Scale factors: SF1 (1GB) to SF1000 (1TB)
- Target: 25,000-35,000 QphH@100GB

**Implementation Status:** 📋 Planned
**Priority:** High
**Estimated Effort:** 3-4 weeks

### 2. YCSB Benchmarks (To Be Implemented)

Based on Yahoo Cloud Serving Benchmark standard:

**Workloads:**
- **Workload A**: 50% read, 50% update (Session store)
- **Workload B**: 95% read, 5% update (Read-heavy cache)
- **Workload C**: 100% read (Read-only cache)
- **Workload D**: 95% read, 5% insert (Read-latest)
- **Workload E**: 5% insert, 95% scan (Short scans)
- **Workload F**: 50% read-modify-write

**Expected Performance:**
- Workload A: 100,000-150,000 ops/sec
- Workload C: 200,000-300,000 ops/sec

**Implementation Status:** 📋 Planned
**Priority:** High
**Estimated Effort:** 2-3 weeks

### 3. Hardware Scaling (Implemented) ✅

**Current Implementation:**
- Core count scaling tests (1-64 cores)
- Scaling efficiency analysis
- Hardware auto-detection
- Performance grading

**Features:**
- Automatic hardware profiling
- Statistical analysis (mean, p95, p99)
- Efficiency grading (A+ to F)
- JSON and Markdown reports

**Usage:** See Quick Start section above

### 4. Vector Database Benchmarks (To Be Implemented)

Based on ANN-Benchmarks standard:

**Datasets:**
- SIFT1M (128D, 1M vectors)
- Deep1B (96D, 1B vectors)
- Custom embeddings (384D, 768D, 1536D)

**Metrics:**
- Recall@10, Recall@100
- Queries per second (QPS)
- Build time
- Index size

**Implementation Status:** 📋 Planned
**Priority:** Medium
**Estimated Effort:** 2-3 weeks

### 5. Graph Benchmarks (To Be Implemented)

Based on LDBC Social Network Benchmark:

**Queries:**
- IC2: Recent messages by friends
- IC13: Shortest path
- Complex reads (multi-hop traversal)
- Short reads (neighbor queries)

**Scale Factors:**
- SF1: 3.5M vertices, 17M edges (~1GB)
- SF10: 35M vertices, 170M edges (~10GB)

**Implementation Status:** 📋 Planned
**Priority:** Medium
**Estimated Effort:** 3-4 weeks

## Hardware Configuration Testing

### Supported Test Dimensions

1. **Core Count Scaling** ✅ Implemented
   - Test: 1, 2, 4, 8, 16, 32, 64 cores
   - Metric: Scaling efficiency (speedup vs ideal)
   - Status: Available in `hardware_scaling_benchmark.py`

2. **Thread Configuration** 📋 Planned
   - Test: 1x, 1.5x, 2x cores (hyperthreading)
   - Metric: Optimal thread count per workload
   - Implementation: Add `--thread-multiplier` flag

3. **Memory Bandwidth** 📋 Planned
   - Test: Sequential vs random access
   - Test: Working set sizes (L1, L2, L3, RAM)
   - Metric: Cache miss rates, bandwidth utilization

4. **Storage Types** 📋 Planned
   - Test: HDD, SATA SSD, NVMe Gen3, NVMe Gen4
   - Metric: IOPS, bandwidth, latency
   - Configuration: RocksDB tuning per storage type

5. **NUMA Configuration** 📋 Planned
   - Test: Local vs remote memory access
   - Test: Interleaved vs preferred node
   - Metric: Performance penalty of remote access

## Performance Targets

### By Hardware Configuration

Based on research in documentation:

| Configuration | OLTP (ops/s) | OLAP (queries/min) | Vector Search (QPS) |
|--------------|-------------|-------------------|-------------------|
| 4-core, 8GB  | 200-300K    | 50-100            | 5-10K             |
| 8-core, 16GB | 400-600K    | 100-200           | 10-20K            |
| 16-core, 32GB| 700-1000K   | 200-400           | 20-40K            |
| 32-core, 64GB| 1.2-1.8M    | 400-800           | 40-80K            |

### Scaling Efficiency Grades

| Efficiency | Grade | Interpretation |
|-----------|-------|----------------|
| ≥ 90% | A+ | Excellent scaling |
| 80-90% | A | Very good scaling |
| 70-80% | B | Good scaling |
| 60-70% | C | Acceptable |
| 50-60% | D | Poor (investigate) |
| < 50% | F | Critical (major bottleneck) |

## Next Steps for Implementation

### Phase 1: Current Status ✅
- [x] Research documentation complete
- [x] Hardware configuration guide complete
- [x] Basic hardware scaling benchmark implemented
- [x] Integration guide created

### Phase 2: TPC Benchmarks (Weeks 1-4)
- [ ] Implement TPC-C data generator
- [ ] Implement TPC-C transaction mix
- [ ] Implement TPC-H 22 queries
- [ ] Validate against published results

### Phase 3: YCSB Integration (Weeks 5-7)
- [ ] Create YCSB binding for ThemisDB
- [ ] Implement all 6 workloads (A-F)
- [ ] Multi-threaded execution
- [ ] Comparison with Redis, MongoDB

### Phase 4: Vector & AI (Weeks 8-10)
- [ ] ANN-Benchmarks integration
- [ ] RAG workflow benchmarks
- [ ] LLM inference performance
- [ ] Image analysis pipeline

### Phase 5: Advanced Hardware (Weeks 11-12)
- [ ] Thread optimization tests
- [ ] Memory bandwidth analysis
- [ ] NUMA configuration tests
- [ ] Storage type comparison

### Phase 6: Documentation & Reporting (Weeks 13-14)
- [ ] Comprehensive user guide
- [ ] Performance tuning recommendations
- [ ] Interactive dashboard
- [ ] CI/CD integration examples

## Contributing

To add new benchmarks:

1. Follow the structure in `hardware_scaling_benchmark.py`
2. Use scientific methodology (warmup, repetitions, statistical analysis)
3. Document expected baselines from research
4. Include hardware profiling
5. Generate both JSON and Markdown reports
6. Add tests to CI/CD pipeline

## References

All benchmarks are based on industry standards documented in:
- `ADVANCED_BENCHMARK_RESEARCH.md` - Scientific standards and baselines
- `HARDWARE_CONFIGURATION_BENCHMARK_SUITE.md` - Hardware testing methodology
- Existing benchmark infrastructure in `benchmarks/comparative/`

## Support

For questions or issues:
- Review documentation in `benchmarks/` directory
- Check existing benchmark implementations
- Refer to scientific standards in research documentation
- Open GitHub issue for specific problems

---

**Status:** ✅ Phase 1 Complete - Ready for Phase 2 Implementation  
**Last Updated:** 2026-04-06
