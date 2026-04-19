> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# TPC Benchmarks for ThemisDB

## Overview

This directory contains implementations of Transaction Processing Performance Council (TPC) benchmarks for ThemisDB using **Google Benchmark** framework in **C++**.

## Benchmarks

### TPC-C (Online Transaction Processing)

**Status:** ✅ Implemented

TPC-C is an OLTP benchmark that simulates a wholesale supplier managing orders with warehouses, districts, customers, and orders.

**Key Metrics:**
- **tpmC** (Transactions per Minute): Primary metric
- **$/tpmC**: Price-performance ratio  
- **Response Time**: 5-second average, 90th percentile < 80 seconds

**Workload Mix:**
- New Order (45%): Create new order
- Payment (43%): Update customer balance
- Order Status (4%): Query order status
- Delivery (4%): Batch delivery processing
- Stock Level (4%): Warehouse inventory check

**Implementation:**
- `../bench_tpcc.cpp` - Complete TPC-C benchmark using Google Benchmark
- `tpc_c_config.yaml` - Configuration file (for reference)

### TPC-H (Decision Support)

**Status:** 📋 Planned

TPC-H is a decision support benchmark featuring 22 complex analytical queries against large datasets.

**Key Metrics:**
- **QphH@Size** (Queries per Hour at Scale Factor)
- **Query Response Times**: 22 complex analytical queries
- **Refresh Function Times**: Data loading performance

**Scale Factors:**
- SF1: 1GB (~6 million rows in LINEITEM)
- SF10: 10GB
- SF100: 100GB

**Implementation:** Will be added in future phase

## Performance Targets

Based on research in `../ADVANCED_BENCHMARK_RESEARCH.md`:

**TPC-C:**
- PostgreSQL Baseline: ~200,000 tpmC (8-core, 32GB RAM)
- ThemisDB Target: 150,000-200,000 tpmC (80-100% of PostgreSQL)

**TPC-H:**
- PostgreSQL Baseline: ~30,000 QphH@100GB
- ThemisDB Target: 25,000-35,000 QphH@100GB

## Usage

### Building

```bash
# From ThemisDB root directory
mkdir -p build && cd build
cmake ..
make bench_tpcc
```

### Running TPC-C Benchmark

```bash
# Run all TPC-C benchmarks
./build/bench_tpcc

# Run specific transaction
./build/bench_tpcc --benchmark_filter=NewOrderTransaction

# Run with custom iterations
./build/bench_tpcc --benchmark_min_time=10

# Export results to JSON
./build/bench_tpcc --benchmark_out=tpcc_results.json --benchmark_out_format=json

# Export results to CSV
./build/bench_tpcc --benchmark_out=tpcc_results.csv --benchmark_out_format=csv
```

### Benchmark Parameters

The TPC-C benchmark accepts warehouse count as a parameter:
- `Arg(1)`: 1 warehouse (~100MB data)
- `Arg(10)`: 10 warehouses (~1GB data)
- `Arg(100)`: 100 warehouses (~10GB data)

Example with multiple warehouse configurations:
```cpp
BENCHMARK_REGISTER_F(TPCCFixture, NewOrderTransaction)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Unit(benchmark::kMillisecond);
```

## Configuration

Configuration is done via Google Benchmark command-line flags:

```bash
# Run for specific time
./build/bench_tpcc --benchmark_min_time=60

# Filter specific benchmarks
./build/bench_tpcc --benchmark_filter="Payment|NewOrder"

# Control repetitions
./build/bench_tpcc --benchmark_repetitions=10

# Display statistics
./build/bench_tpcc --benchmark_report_aggregates_only=true

# Verbose output
./build/bench_tpcc -v
```

The `tpc_c_config.yaml` file is kept for reference but not used by the C++ implementation.

## References

- TPC-C Specification: http://www.tpc.org/tpcc/
- TPC-H Specification: http://www.tpc.org/tpch/
- Research: See `../ADVANCED_BENCHMARK_RESEARCH.md` for detailed analysis

## Development Status

- [x] Research and documentation (Phase 1)
- [x] TPC-C schema implementation (C++)
- [x] TPC-C transaction implementations (5 transactions)
- [x] TPC-C benchmark runner (Google Benchmark)
- [x] Mixed workload simulation
- [ ] TPC-H implementation (planned)
- [ ] Validation against published results

**Current Phase:** Phase 2 - TPC-C Complete  
**Implementation:** C++ with Google Benchmark  
**Completed:** 2025-12-23
