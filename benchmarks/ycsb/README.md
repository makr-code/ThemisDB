> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# YCSB Benchmarks for ThemisDB

## Overview

This directory contains the YCSB (Yahoo! Cloud Serving Benchmark) implementation for ThemisDB using **Google Benchmark** framework in **C++**.

## About YCSB

YCSB is a framework and common set of workloads for evaluating the performance of different "key-value" and "cloud" serving stores.

**Paper:** "Benchmarking Cloud Serving Systems with YCSB"  
**Authors:** Brian F. Cooper, Adam Silberstein, et al.  
**Conference:** SoCC 2010  
**Link:** https://research.yahoo.com/files/ycsb.pdf

## Workloads

YCSB defines 6 core workloads that represent different application scenarios:

### Workload A: Update Heavy
- **Mix:** 50% reads, 50% updates
- **Distribution:** Zipfian (hot keys)
- **Use Case:** Session store recording recent actions
- **Target:** 100,000-150,000 ops/sec

### Workload B: Read Mostly
- **Mix:** 95% reads, 5% updates
- **Distribution:** Zipfian
- **Use Case:** Photo tagging (most operations read tags)
- **Target:** 150,000-200,000 ops/sec

### Workload C: Read Only
- **Mix:** 100% reads
- **Distribution:** Zipfian
- **Use Case:** User profile cache
- **Target:** 200,000-300,000 ops/sec (highest throughput)

### Workload D: Read Latest
- **Mix:** 95% reads, 5% inserts
- **Distribution:** Latest (exponential distribution favoring recent records)
- **Use Case:** User status updates (people read latest statuses)
- **Target:** 150,000-200,000 ops/sec

### Workload E: Short Ranges
- **Mix:** 95% scans (short ranges), 5% inserts
- **Distribution:** Uniform for scan start, random scan length (1-100)
- **Use Case:** Threaded conversations (scan posts in a thread)
- **Target:** Lower due to scan cost

### Workload F: Read-Modify-Write
- **Mix:** 50% reads, 50% read-modify-write
- **Distribution:** Zipfian
- **Use Case:** User database (read and modify user records)
- **Target:** 100,000-150,000 ops/sec

## Implementation Details

### Data Model
- **Table:** `usertable`
- **Key:** `user<id>` (e.g., user0, user1, user2)
- **Fields:** 10 fields per record (field0 through field9)
- **Field Size:** 100 bytes each
- **Total Record Size:** ~1KB

### Key Distributions

**Zipfian Distribution:**
- Models real-world access patterns where some keys are "hot"
- Zipfian constant: 0.99 (default)
- Most accesses go to a small subset of keys

**Latest Distribution:**
- Exponential distribution favoring most recently inserted records
- Simulates social media feed patterns

**Uniform Distribution:**
- Equal probability for all keys
- Used in Workload E for scan starting points

### Performance Targets

Based on research (8-core, 32GB RAM, NVMe):

| Workload | Description | Target ops/sec |
|----------|-------------|----------------|
| A | Update Heavy | 100-150K |
| B | Read Mostly | 150-200K |
| C | Read Only | 200-300K |
| D | Read Latest | 150-200K |
| E | Short Ranges | Variable |
| F | Read-Modify-Write | 100-150K |

## Usage

### Building

```bash
# From ThemisDB root directory
cd build
cmake ..
make bench_ycsb
```

### Running Benchmarks

```bash
# Run all YCSB workloads with 10K records
./bench_ycsb --benchmark_filter=".*10000"

# Run all workloads with 100K records
./bench_ycsb --benchmark_filter=".*100000"

# Run specific workload
./bench_ycsb --benchmark_filter="WorkloadA"

# Run with custom duration
./bench_ycsb --benchmark_min_time=30

# Export to JSON
./bench_ycsb --benchmark_out=ycsb_results.json --benchmark_out_format=json

# Export to CSV
./bench_ycsb --benchmark_out=ycsb_results.csv --benchmark_out_format=csv
```

### Example Output

```
---------------------------------------------------------------------------
Benchmark                                 Time             CPU   Iterations
---------------------------------------------------------------------------
YCSBFixture/WorkloadA/10000            8.45 us         8.42 us        83045
YCSBFixture/WorkloadA/100000          10.23 us        10.19 us        68632
YCSBFixture/WorkloadB/10000            7.12 us         7.09 us        98654
YCSBFixture/WorkloadB/100000           8.87 us         8.83 us        79213
YCSBFixture/WorkloadC/10000            5.34 us         5.32 us       131478
YCSBFixture/WorkloadC/100000           6.98 us         6.95 us       100574
YCSBFixture/WorkloadD/10000            7.45 us         7.42 us        94312
YCSBFixture/WorkloadD/100000           9.12 us         9.08 us        77045
YCSBFixture/WorkloadE/10000           45.67 us        45.42 us        15412
YCSBFixture/WorkloadE/100000          52.34 us        52.10 us        13421
YCSBFixture/WorkloadF/10000            9.23 us         9.19 us        76145
YCSBFixture/WorkloadF/100000          11.45 us        11.40 us        61342
```

### Calculating ops/sec

Operations per second = 1,000,000 / Time(us)

Example:
- WorkloadC/10000: 5.32 us → ~188,000 ops/sec ✅
- WorkloadC/100000: 6.95 us → ~144,000 ops/sec ✅

## Configuration

### Dataset Sizes

Benchmarks are registered with different dataset sizes:
- **10,000 records:** ~10MB (quick test)
- **100,000 records:** ~100MB (standard)
- **1,000,000 records:** ~1GB (large scale - add manually)

### Customization

To add custom configurations, modify the BENCHMARK_REGISTER_F calls:

```cpp
BENCHMARK_REGISTER_F(YCSBFixture, WorkloadA)
    ->Arg(10000)      // 10K records
    ->Arg(100000)     // 100K records
    ->Arg(1000000)    // 1M records
    ->Unit(benchmark::kMicrosecond);
```

### Google Benchmark Flags

Common flags:
- `--benchmark_min_time=<seconds>` - Minimum time to run each benchmark
- `--benchmark_repetitions=<N>` - Number of repetitions
- `--benchmark_report_aggregates_only=true` - Show only aggregate statistics
- `--benchmark_filter=<regex>` - Filter benchmarks by name
- `--benchmark_out=<file>` - Output file
- `--benchmark_out_format=<json|csv|console>` - Output format

## Comparison with Other Databases

### Expected Performance (8-core, 32GB, NVMe)

| Database | Workload A | Workload B | Workload C |
|----------|-----------|-----------|-----------|
| Redis | 150K | 200K | 300K |
| Cassandra | 120K | 180K | 250K |
| MongoDB | 100K | 150K | 200K |
| **ThemisDB Target** | **100-150K** | **150-200K** | **200-300K** |

## References

1. **Original Paper:**
   - Cooper et al., "Benchmarking Cloud Serving Systems with YCSB", SoCC 2010
   - https://research.yahoo.com/files/ycsb.pdf

2. **YCSB GitHub:**
   - https://github.com/brianfrankcooper/YCSB

3. **Workload Specifications:**
   - https://github.com/brianfrankcooper/YCSB/wiki/Core-Workloads

4. **Research:**
   - See `../ADVANCED_BENCHMARK_RESEARCH.md` for detailed analysis

## Development Status

- [x] Workload A: Update Heavy
- [x] Workload B: Read Mostly
- [x] Workload C: Read Only
- [x] Workload D: Read Latest
- [x] Workload E: Short Ranges
- [x] Workload F: Read-Modify-Write
- [x] Zipfian distribution implementation
- [x] Latest distribution implementation
- [ ] Batch operations (future)
- [ ] Multi-table workloads (future)

**Current Phase:** Phase 3 - YCSB Complete  
**Implementation:** C++ with Google Benchmark  
**Completed:** 2025-12-23
