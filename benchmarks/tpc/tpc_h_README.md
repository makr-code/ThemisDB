> **Build:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# TPC-H Benchmark for ThemisDB

## Overview

TPC-H is a **decision support (OLAP) benchmark** from the Transaction Processing Performance Council (TPC). It consists of a suite of business-oriented ad-hoc queries and concurrent data modifications, designed to test systems that support decision support applications.

**Official Specification:** http://www.tpc.org/tpch/

## What is TPC-H?

TPC-H models a wholesale supplier managing sales, orders, and customers worldwide. The benchmark includes:

- **8 Tables:** REGION, NATION, SUPPLIER, PART, PARTSUPP, CUSTOMER, ORDERS, LINEITEM
- **22 Complex Queries:** Analytical queries involving joins, aggregations, sorting, and subqueries
- **Concurrent Updates:** Simulates real-world data modifications during query execution
- **Scale Factor (SF):** Configurable database size (SF=1 ≈ 1GB, SF=10 ≈ 10GB, SF=100 ≈ 100GB)

### TPC-H vs TPC-C

| Feature | TPC-C (OLTP) | TPC-H (OLAP) |
|---------|--------------|--------------|
| **Workload Type** | Transactional | Analytical |
| **Query Complexity** | Simple (single table mostly) | Complex (multi-table joins) |
| **Transaction Length** | Short (<1s) | Long (seconds to minutes) |
| **Data Access Pattern** | Random, small result sets | Sequential scans, large result sets |
| **Primary Metric** | tpmC (transactions/minute) | QphH@SF (queries/hour at scale factor) |
| **Use Case** | Order entry, inventory | Business intelligence, reporting |

## Schema

### Reference Tables (Small, Static)

1. **REGION** (5 rows)
   - r_regionkey, r_name, r_comment
   - Regions: AFRICA, AMERICA, ASIA, EUROPE, MIDDLE EAST

2. **NATION** (25 rows)
   - n_nationkey, n_name, n_regionkey, n_comment
   - 25 nations distributed across 5 regions

### Core Tables (Large, Dynamic)

3. **SUPPLIER** (10,000 × SF rows)
   - s_suppkey, s_name, s_address, s_nationkey, s_phone, s_acctbal, s_comment

4. **PART** (200,000 × SF rows)
   - p_partkey, p_name, p_mfgr, p_brand, p_type, p_size, p_container, p_retailprice, p_comment

5. **PARTSUPP** (800,000 × SF rows)
   - ps_partkey, ps_suppkey, ps_availqty, ps_supplycost, ps_comment
   - Part-Supplier relationship (4 suppliers per part)

6. **CUSTOMER** (150,000 × SF rows)
   - c_custkey, c_name, c_address, c_nationkey, c_phone, c_acctbal, c_mktsegment, c_comment

7. **ORDERS** (1,500,000 × SF rows)
   - o_orderkey, o_custkey, o_orderstatus, o_totalprice, o_orderdate, o_orderpriority, o_clerk, o_shippriority, o_comment

8. **LINEITEM** (6,000,000 × SF rows)
   - l_orderkey, l_partkey, l_suppkey, l_linenumber, l_quantity, l_extendedprice, l_discount, l_tax, l_returnflag, l_linestatus, l_shipdate, l_commitdate, l_receiptdate, l_shipinstruct, l_shipmode, l_comment

### Database Size by Scale Factor

| Scale Factor | Total Rows | Approx Size | Use Case |
|--------------|-----------|-------------|----------|
| SF=1 | ~8.7M rows | ~1 GB | Development, testing |
| SF=10 | ~87M rows | ~10 GB | Small production |
| SF=100 | ~870M rows | ~100 GB | Medium production |
| SF=1000 | ~8.7B rows | ~1 TB | Large enterprise |

## The 22 TPC-H Queries

### Query Categories

**Simple Aggregation (Q1, Q6)**
- Single table scans with filters and aggregations
- Fastest queries (~100ms - 1s)

**Joins (Q3, Q5, Q10, Q12)**
- 2-4 table joins with aggregations
- Medium complexity (~1-10s)

**Complex Analytics (Q2, Q9, Q11, Q17, Q18, Q20, Q21, Q22)**
- Subqueries, correlated subqueries, multiple aggregations
- Highest complexity (~10s - minutes)

### Implemented Queries (Phase 1)

#### Q1: Pricing Summary Report
**Type:** Simple aggregation  
**Complexity:** Low  
**Tables:** LINEITEM  
**Description:** Summary report on shipped line items grouped by return flag and line status  
**Typical Time:** 100-500ms

```sql
SELECT l_returnflag, l_linestatus, 
       SUM(l_quantity), SUM(l_extendedprice),
       SUM(l_extendedprice*(1-l_discount)),
       AVG(l_quantity), AVG(l_extendedprice), COUNT(*)
FROM lineitem
WHERE l_shipdate <= '1998-09-02'
GROUP BY l_returnflag, l_linestatus
ORDER BY l_returnflag, l_linestatus
```

#### Q3: Shipping Priority
**Type:** Join + aggregation  
**Complexity:** Medium  
**Tables:** CUSTOMER, ORDERS, LINEITEM  
**Description:** Top 10 unshipped orders with highest revenue for a market segment  
**Typical Time:** 1-5s

```sql
SELECT l_orderkey, SUM(l_extendedprice*(1-l_discount)) as revenue,
       o_orderdate, o_shippriority
FROM customer, orders, lineitem
WHERE c_mktsegment = 'BUILDING' AND c_custkey = o_custkey 
  AND l_orderkey = o_orderkey
  AND o_orderdate < '1995-03-15' AND l_shipdate > '1995-03-15'
GROUP BY l_orderkey, o_orderdate, o_shippriority
ORDER BY revenue DESC, o_orderdate
LIMIT 10
```

#### Q6: Forecasting Revenue Change
**Type:** Simple scan + filter  
**Complexity:** Low  
**Tables:** LINEITEM  
**Description:** Revenue from discounted line items in a specific timeframe  
**Typical Time:** 100-500ms (fastest query)

```sql
SELECT SUM(l_extendedprice * l_discount) as revenue
FROM lineitem
WHERE l_shipdate >= '1994-01-01' AND l_shipdate < '1995-01-01'
  AND l_discount BETWEEN 0.05 AND 0.07
  AND l_quantity < 24
```

#### Q10: Returned Item Reporting
**Type:** Multi-table join  
**Complexity:** Medium  
**Tables:** CUSTOMER, ORDERS, LINEITEM, NATION  
**Description:** Top 20 customers by revenue from returned items  
**Typical Time:** 2-10s

```sql
SELECT c_custkey, c_name, SUM(l_extendedprice * (1 - l_discount)) as revenue,
       c_acctbal, n_name, c_address, c_phone, c_comment
FROM customer, orders, lineitem, nation
WHERE c_custkey = o_custkey AND l_orderkey = o_orderkey 
  AND o_orderdate >= '1993-10-01' AND o_orderdate < '1994-01-01'
  AND l_returnflag = 'R' AND c_nationkey = n_nationkey
GROUP BY c_custkey, c_name, c_acctbal, c_phone, n_name, c_address, c_comment
ORDER BY revenue DESC
LIMIT 20
```

### Future Queries (Phase 2)

Remaining 18 queries cover advanced scenarios:
- **Q2:** Minimum cost supplier query (correlated subquery)
- **Q4:** Order priority checking
- **Q5:** Local supplier volume (5-table join)
- **Q7-Q9:** International shipping and profit analysis
- **Q11:** Important stock identification (GROUP BY with HAVING)
- **Q12-Q14:** Shipping modes, customer distribution, promotion effect
- **Q15:** Top supplier (materialized view)
- **Q16-Q22:** Complex analytical queries with multiple subqueries

## Performance Targets

### Industry Baselines (SF=100, 8-core, 32GB RAM, NVMe)

| Database | QphH@100 | Configuration |
|----------|----------|---------------|
| **PostgreSQL** | ~30,000 | Tuned for OLAP |
| **MySQL** | ~25,000 | InnoDB, optimized |
| **Oracle** | ~50,000 | Enterprise, partitioned |
| **SQL Server** | ~45,000 | Enterprise, columnstore |

### ThemisDB Targets (SF=10, 8-core, 32GB RAM, NVMe)

| Metric | Target | Baseline |
|--------|--------|----------|
| **QphH@10** | 25,000 - 35,000 | PostgreSQL: ~30,000 |
| **Q6 (fastest)** | < 500ms | PostgreSQL: ~200ms |
| **Q1, Q3 (medium)** | < 5s | PostgreSQL: ~2s |
| **Q10 (complex)** | < 15s | PostgreSQL: ~8s |
| **Power Test** | < 10 minutes | All 22 queries sequential |
| **Throughput Test** | > 5 streams | Concurrent query execution |

**Geometric Mean:** Aggregate performance across all 22 queries

## Usage

### Build

```bash
cd build
cmake .. -DTHEMIS_BUILD_BENCHMARKS=ON
make bench_tpch
```

### Run All Queries

```bash
# Run all implemented queries
./bench_tpch

# Output:
# -------------------------------------------------------------------
# Benchmark                            Time             CPU   Iterations
# -------------------------------------------------------------------
# TPCHFixture/Query1_PricingSummary   234 ms          231 ms           3
# TPCHFixture/Query3_ShippingPriority 1.23 s          1.22 s           1
# TPCHFixture/Query6_ForecastingRevenue 89 ms          88 ms          8
# TPCHFixture/Query10_ReturnedItems   5.67 s          5.64 s           1
# TPCHFixture/MixedWorkload           1.89 s          1.87 s           1
```

### Run Specific Query

```bash
# Run only Q6 (fastest)
./bench_tpch --benchmark_filter="Query6"

# Run only Q3 (shipping priority)
./bench_tpch --benchmark_filter="Query3"

# Run mixed workload
./bench_tpch --benchmark_filter="MixedWorkload"
```

### Export Results

```bash
# Export to JSON
./bench_tpch --benchmark_out=tpch_results.json --benchmark_out_format=json

# Export to CSV
./bench_tpch --benchmark_out=tpch_results.csv --benchmark_out_format=csv

# Run for extended time
./bench_tpch --benchmark_min_time=60  # 60 seconds minimum
```

### Adjust Scale Factor

Modify the constants in `bench_tpch.cpp`:

```cpp
// For SF=1 (full scale, ~1GB)
loadSuppliers(SUPPLIERS_COUNT);      // 10,000 rows
loadParts(PARTS_COUNT);              // 200,000 rows
loadOrders(ORDERS_COUNT);            // 1,500,000 rows

// For SF=0.1 (10%, default, ~100MB) 
loadSuppliers(SUPPLIERS_COUNT / 10); // 1,000 rows
loadParts(PARTS_COUNT / 10);         // 20,000 rows
loadOrders(ORDERS_COUNT / 10);       // 150,000 rows
```

## Optimization Tips

### For ThemisDB

1. **Indexes:** TPC-H queries benefit from indexes on:
   - Date columns (shipdate, orderdate, commitdate)
   - Foreign keys (custkey, orderkey, partkey, suppkey)
   - Frequently filtered columns (returnflag, linestatus, mktsegment)

2. **Batch Operations:** Use batch inserts for data loading

3. **Scan Optimization:** Optimize sequential scans for LINEITEM (largest table)

4. **Aggregation:** Efficient GROUP BY and SUM/AVG implementations

5. **Join Strategy:** Hash joins for large tables, nested loop for small

### Query Execution Plan

**Q1 (Simple):**
```
Scan LINEITEM (filter: shipdate) → Aggregate (GROUP BY) → Sort
```

**Q3 (Join):**
```
Scan CUSTOMER (filter: mktsegment) →
  Join ORDERS (filter: orderdate) →
    Join LINEITEM (filter: shipdate) →
      Aggregate (GROUP BY) → Sort → Limit
```

**Q6 (Fastest):**
```
Scan LINEITEM (filters: shipdate, discount, quantity) → Aggregate (SUM)
```

## TPC-H Metrics

### Primary Metric: QphH@SF

**QphH@SF** = Queries per Hour at Scale Factor

Calculated as:
```
QphH@SF = 3600 / (Power@SF + Throughput@SF)

Where:
- Power@SF = Geometric mean of 22 query times (seconds)
- Throughput@SF = Total time for all streams to complete (hours)
```

### Secondary Metrics

- **Power Test:** Sequential execution of all 22 queries
- **Throughput Test:** Concurrent execution with multiple streams
- **Refresh Functions:** INSERT/DELETE operations during queries
- **Composite Score:** Combination of Power and Throughput

## Roadmap

### Phase 1: Foundation (✅ Complete)
- [x] TPC-H schema (8 tables)
- [x] Data generation (SF=0.1 for testing)
- [x] 4 representative queries (Q1, Q3, Q6, Q10)
- [x] Build system integration
- [x] Documentation

### Phase 2: Full Query Suite (Weeks 1-2)
- [ ] Implement remaining 18 queries
- [ ] Query optimization
- [ ] Execution plan analysis
- [ ] Performance tuning

### Phase 3: Concurrent Testing (Week 3)
- [ ] Multi-stream throughput test
- [ ] Refresh functions (INSERT/DELETE)
- [ ] Load test with concurrent queries
- [ ] Resource utilization monitoring

### Phase 4: Scale Factor Testing (Week 4)
- [ ] SF=1 (1GB) testing
- [ ] SF=10 (10GB) testing
- [ ] Performance comparison with PostgreSQL/MySQL
- [ ] Official QphH@SF calculation

## References

- **TPC-H Specification 3.0.1:** http://www.tpc.org/tpc_documents_current_versions/pdf/tpc-h_v3.0.1.pdf
- **TPC-H Results:** http://www.tpc.org/tpch/results/tpch_results5.asp
- **PostgreSQL TPC-H:** https://www.postgresql.org/ftp/projects/pgFoundry/pg_tpch/
- **Academic Paper:** "TPC-H Analyzed: Hidden Messages and Lessons Learned" (TPCTC 2013)

## License

TPC Benchmark™ H is a registered trademark of the Transaction Processing Performance Council (TPC). This implementation is for internal benchmarking and research purposes only and is not an official TPC-H implementation.
