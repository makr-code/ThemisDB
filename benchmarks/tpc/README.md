# TPC Benchmarks for ThemisDB

## Overview

This directory contains implementations of Transaction Processing Performance Council (TPC) benchmarks for ThemisDB.

## Benchmarks

### TPC-C (Online Transaction Processing)

**Status:** 🚧 In Progress

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

**Files:**
- `tpc_c_schema.py` - Database schema definition
- `tpc_c_data_generator.py` - Test data generation
- `tpc_c_transactions.py` - Transaction implementations
- `tpc_c_runner.py` - Benchmark orchestration
- `tpc_c_config.yaml` - Configuration file

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

**Files:**
- `tpc_h_schema.py` - Database schema definition
- `tpc_h_data_generator.py` - Test data generation
- `tpc_h_queries.sql` - 22 query templates
- `tpc_h_runner.py` - Benchmark orchestration

## Performance Targets

Based on research in `../ADVANCED_BENCHMARK_RESEARCH.md`:

**TPC-C:**
- PostgreSQL Baseline: ~200,000 tpmC (8-core, 32GB RAM)
- ThemisDB Target: 150,000-200,000 tpmC (80-100% of PostgreSQL)

**TPC-H:**
- PostgreSQL Baseline: ~30,000 QphH@100GB
- ThemisDB Target: 25,000-35,000 QphH@100GB

## Usage

### TPC-C Benchmark

```bash
# Generate test data (1 warehouse = ~100MB)
python3 tpc_c_data_generator.py --warehouses 10 --output /tmp/tpc_c_data

# Run benchmark
python3 tpc_c_runner.py \
  --warehouses 10 \
  --duration 300 \
  --threads 8 \
  --output-dir ./results

# View results
cat ./results/tpc_c_report.md
```

### TPC-H Benchmark

```bash
# Generate test data (SF10 = 10GB)
python3 tpc_h_data_generator.py --scale-factor 10 --output /tmp/tpc_h_data

# Run benchmark
python3 tpc_h_runner.py \
  --scale-factor 10 \
  --database-url themisdb://localhost:8765 \
  --output-dir ./results

# View results
cat ./results/tpc_h_report.md
```

## Configuration

Both benchmarks use YAML configuration files:

```yaml
# tpc_c_config.yaml
database:
  host: localhost
  port: 8765
  protocol: direct  # or http, grpc, wire

benchmark:
  warehouses: 10
  duration_seconds: 300
  ramp_up_seconds: 30
  think_time_ms: 0  # For maximum throughput
  threads: 8

reporting:
  output_format: ["json", "markdown", "csv"]
  include_percentiles: [50, 95, 99, 99.9]
```

## References

- TPC-C Specification: http://www.tpc.org/tpcc/
- TPC-H Specification: http://www.tpc.org/tpch/
- Research: See `../ADVANCED_BENCHMARK_RESEARCH.md` for detailed analysis

## Development Status

- [x] Research and documentation (Phase 1)
- [ ] TPC-C schema implementation
- [ ] TPC-C data generator
- [ ] TPC-C transaction implementations
- [ ] TPC-C benchmark runner
- [ ] TPC-H schema implementation
- [ ] TPC-H data generator
- [ ] TPC-H query implementations
- [ ] TPC-H benchmark runner
- [ ] Validation against published results

**Current Phase:** Phase 2 - TPC-C Implementation  
**Estimated Completion:** 3 weeks from start  
**Started:** 2025-12-23
