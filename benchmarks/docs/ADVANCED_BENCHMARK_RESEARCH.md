> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeipunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../README.md) verwenden.

# Advanced Benchmark Research & Implementation Guide
## Scientific and Industrial Standards for Modern Database & AI Systems

**Version:** 2.0  
**Date:** 2025-12-23  
**Status:** Research Complete - Implementation Ready

---

## Executive Summary

This document provides a comprehensive research-based guide for extending ThemisDB benchmarks to meet scientific and industrial standards. Based on analysis of leading benchmark frameworks (TPC, YCSB, LDBC, ANN-Benchmarks) and modern database requirements, we propose a structured approach to rigorous performance evaluation.

### Key Objectives

1. **Scientific Rigor**: Implement industry-standard benchmarks (TPC-C, TPC-H, YCSB)
2. **AI/ML Readiness**: Add specialized benchmarks for vector search and LLM integration
3. **Hardware Optimization**: Test across diverse hardware configurations (cores, threads, memory)
4. **Modern Workloads**: Support hybrid OLTP/OLAP, graph, and time-series scenarios
5. **Reproducibility**: Ensure all benchmarks are deterministic and well-documented

---

## Table of Contents

1. [Scientific Benchmark Standards](#1-scientific-benchmark-standards)
2. [AI/ML System Benchmarks](#2-aiml-system-benchmarks)
3. [Hardware Configuration Testing](#3-hardware-configuration-testing)
4. [Modern Database Workloads](#4-modern-database-workloads)
5. [Implementation Roadmap](#5-implementation-roadmap)
6. [References](#6-references)

---

## 1. Scientific Benchmark Standards

### 1.1 TPC Benchmarks (Transaction Processing Performance Council)

**Why TPC?** Industry-standard since 1988, used by all major database vendors for performance claims.

#### TPC-C: Online Transaction Processing (OLTP)

**Description:** Simulates a wholesale supplier managing orders with warehouses, districts, customers, and orders.

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

**ThemisDB Implementation Requirements:**
```python
# Tables needed:
- Warehouse (W rows, W = number of warehouses)
- District (10*W rows)
- Customer (30,000*W rows)
- Orders (30,000*W rows initially)
- Order-Line (variable, ~10 per order)
- Item (100,000 rows)
- Stock (100,000*W rows)
- History (30,000*W rows initially)

# Key Challenges:
- High write contention (warehouse stock levels)
- Referential integrity maintenance
- Concurrent transaction isolation
```

**Expected Performance Baseline:**
- PostgreSQL: ~200,000 tpmC (8-core, 32GB RAM)
- MySQL: ~180,000 tpmC
- Target for ThemisDB: 150,000-200,000 tpmC (80-100% of PostgreSQL)

#### TPC-H: Decision Support Benchmark

**Description:** Ad-hoc queries against large datasets for business intelligence.

**Key Metrics:**
- **QphH@Size** (Queries per Hour at Scale Factor)
- **Query Response Times**: 22 complex analytical queries
- **Refresh Function Times**: Data loading performance

**Scale Factors:**
- SF1: 1GB (~6 million rows in LINEITEM)
- SF10: 10GB
- SF100: 100GB
- SF1000: 1TB

**Query Types:**
- Aggregations with GROUP BY
- Multi-table JOINs (up to 8 tables)
- Subqueries and correlated subqueries
- Window functions (RANK, ROW_NUMBER)

**ThemisDB Implementation Requirements:**
```sql
-- Example: TPC-H Query 1 (Pricing Summary Report)
SELECT
    l_returnflag,
    l_linestatus,
    SUM(l_quantity) as sum_qty,
    SUM(l_extendedprice) as sum_base_price,
    SUM(l_extendedprice * (1 - l_discount)) as sum_disc_price,
    SUM(l_extendedprice * (1 - l_discount) * (1 + l_tax)) as sum_charge,
    AVG(l_quantity) as avg_qty,
    AVG(l_extendedprice) as avg_price,
    AVG(l_discount) as avg_disc,
    COUNT(*) as count_order
FROM
    lineitem
WHERE
    l_shipdate <= DATE '1998-12-01' - INTERVAL '90' DAY
GROUP BY
    l_returnflag,
    l_linestatus
ORDER BY
    l_returnflag,
    l_linestatus;
```

**Expected Performance:**
- PostgreSQL: ~30,000 QphH@100GB
- ClickHouse: ~100,000 QphH@100GB (columnar optimized)
- Target for ThemisDB: 25,000-35,000 QphH@100GB

### 1.2 YCSB (Yahoo! Cloud Serving Benchmark)

**Why YCSB?** De facto standard for NoSQL and cloud databases, highly configurable.

**Core Workloads:**

| Workload | Read% | Update% | Insert% | Scan% | Use Case |
|----------|-------|---------|---------|-------|----------|
| **A** | 50% | 50% | 0% | 0% | Session store |
| **B** | 95% | 5% | 0% | 0% | Read-heavy cache |
| **C** | 100% | 0% | 0% | 0% | Read-only cache |
| **D** | 95% | 0% | 5% | 0% | Read-latest workload |
| **E** | 0% | 0% | 5% | 95% | Short scan workload |
| **F** | 50% | 0% | 0% | 0% | Read-modify-write |

**Key Parameters:**
- **Request Distribution**: Zipfian, Uniform, Latest, Hotspot
- **Record Count**: 1M-1B records
- **Operations**: 1M-100M operations
- **Thread Count**: 1-256 threads
- **Field Count**: 10 fields per record (default)
- **Field Size**: 100 bytes per field (default)

**ThemisDB Implementation:**
```python
# Configuration example
recordcount = 10000000  # 10M records
operationcount = 10000000  # 10M operations
workload = "workloada"  # 50% read, 50% update
requestdistribution = "zipfian"  # 80/20 rule
threadcount = 16

# Expected Throughput Targets:
# Workload A (50/50): 80,000-120,000 ops/sec
# Workload B (95/5):  150,000-200,000 ops/sec
# Workload C (100% read): 200,000-300,000 ops/sec
```

### 1.3 LDBC (Linked Data Benchmark Council)

**Why LDBC?** Scientific standard for graph databases, used by Neo4j, TigerGraph, etc.

#### LDBC Social Network Benchmark (SNB)

**Interactive Workload:**
- **Complex Reads**: Multi-hop graph traversals (1-4 hops)
- **Short Reads**: Simple neighbor queries
- **Inserts/Updates**: Dynamic graph updates

**Scale Factors:**
- SF1: ~3.5M vertices, ~17M edges (~1GB)
- SF10: ~35M vertices, ~170M edges (~10GB)
- SF100: ~350M vertices, ~1.7B edges (~100GB)

**Example Queries:**
```cypher
-- IC2: Recent messages by friends
MATCH (person:Person {id: $personId})-[:KNOWS]-(friend:Person),
      (friend)<-[:HAS_CREATOR]-(message:Message)
WHERE message.creationDate <= $maxDate
RETURN friend.id, friend.firstName, friend.lastName,
       message.id, message.content, message.creationDate
ORDER BY message.creationDate DESC, message.id ASC
LIMIT 20;

-- IC13: Shortest path between two persons
MATCH (person1:Person {id: $person1Id}),
      (person2:Person {id: $person2Id}),
      path = shortestPath((person1)-[:KNOWS*]-(person2))
RETURN length(path) as pathLength;
```

**Expected Performance:**
- Neo4j: IC2 ~20ms, IC13 ~50ms (SF1)
- Target for ThemisDB: 15-30ms (IC2), 40-80ms (IC13)

---

## 2. AI/ML System Benchmarks

### 2.1 ANN-Benchmarks (Approximate Nearest Neighbor)

**Why ANN-Benchmarks?** Standard for vector database evaluation, used by FAISS, HNSW, etc.

**Key Metrics:**
- **Recall@K**: Percentage of true top-K neighbors found
- **QPS** (Queries Per Second): Throughput
- **Build Time**: Index construction time
- **Memory Usage**: Index size vs dataset size

**Standard Datasets:**

| Dataset | Dimensions | Count | Distance Metric | Use Case |
|---------|-----------|--------|-----------------|----------|
| **SIFT1M** | 128 | 1M | Euclidean | Image features |
| **GIST1M** | 960 | 1M | Euclidean | Image descriptors |
| **Deep1B** | 96 | 1B | Euclidean | Deep learning embeddings |
| **GloVe** | 100-300 | 1.2M | Cosine | Word embeddings |
| **BERT** | 768 | Variable | Cosine | Sentence embeddings |

**ThemisDB Vector Benchmark Configuration:**
```yaml
vector_benchmarks:
  - name: "SIFT1M_HNSW"
    dataset: "SIFT1M"
    algorithm: "HNSW"
    parameters:
      M: 16  # Connections per layer
      ef_construction: 200  # Build-time search width
      ef_search: 50  # Query-time search width
    metrics:
      - recall@10
      - recall@100
      - qps
      - build_time_seconds
      - index_size_mb

  - name: "OpenAI_Ada002_Cosine"
    dataset: "custom"  # OpenAI Ada-002 embeddings
    dimensions: 1536
    count: 1000000
    distance: "cosine"
    algorithm: "FAISS_IVF"
    parameters:
      nlist: 1000  # Number of clusters
      nprobe: 10   # Clusters to search
```

**Expected Performance Targets:**

| Configuration | Recall@10 | QPS | Build Time (1M vectors) |
|--------------|-----------|-----|------------------------|
| HNSW (M=16, ef=50) | 95% | 10,000 | 300s |
| FAISS IVF (nprobe=10) | 90% | 50,000 | 60s |
| ThemisDB Target | 90-95% | 8,000-15,000 | 200-400s |

### 2.2 RAG (Retrieval Augmented Generation) Benchmarks

**Why RAG?** Critical for LLM applications, combines vector search with language models.

**Benchmark Components:**

1. **Retrieval Latency**: Time to find top-K relevant documents
2. **Context Window Assembly**: Prepare context for LLM
3. **End-to-End Latency**: Retrieval + LLM inference
4. **Accuracy Metrics**: Answer relevance, faithfulness

**RAG Workflow Test:**
```python
# Benchmark: Question Answering with RAG
dataset = "SQuAD2.0"  # 150K questions
embedding_model = "all-MiniLM-L6-v2"  # 384 dimensions
llm_model = "llama-3-8B"  # Optional: local LLM

# Metrics:
# 1. Retrieval Precision@5: Are relevant docs in top-5?
# 2. Retrieval Latency: Time to get top-5 docs
# 3. LLM Latency: Time to generate answer (if using ThemisDB LLM)
# 4. End-to-End Latency: Total time
# 5. Answer Accuracy: F1 score vs ground truth

# Expected Performance:
# Retrieval Latency: 5-15ms (1M documents)
# LLM Latency: 500-2000ms (8B model, CPU)
# End-to-End: 505-2015ms
# Retrieval Precision@5: 85-95%
```

### 2.3 Embedding Generation Benchmarks

**Test Scenarios:**

1. **Batch Embedding**: Generate embeddings for large datasets
2. **Streaming Embedding**: Real-time embedding generation
3. **Multi-Modal**: Text, image, and video embeddings

**Configuration:**
```yaml
embedding_benchmarks:
  - name: "Text_Batch_1M"
    type: "text"
    model: "all-MiniLM-L6-v2"
    batch_size: 512
    document_count: 1000000
    avg_tokens_per_doc: 100
    metrics:
      - docs_per_second
      - total_time_seconds
      - memory_usage_gb
    target_throughput: 5000-10000 docs/sec

  - name: "Image_CLIP_100K"
    type: "image"
    model: "CLIP-ViT-B-32"
    batch_size: 32
    image_count: 100000
    resolution: "224x224"
    metrics:
      - images_per_second
      - gpu_utilization_percent
    target_throughput: 200-500 images/sec (GPU)
```

---

## 3. Hardware Configuration Testing

### 3.1 Multi-Core Scaling Analysis

**Objective:** Determine optimal thread configuration for different workloads.

**Test Matrix:**

| Cores | Threads per Core | Total Threads | Test Cases |
|-------|-----------------|---------------|------------|
| 1 | 1 | 1 | Baseline performance |
| 2 | 1 | 2 | 2x speedup check |
| 4 | 1 | 4 | Small parallel scaling |
| 8 | 1 | 8 | Typical server config |
| 8 | 2 | 16 | Hyperthreading benefit |
| 16 | 1 | 16 | Multi-core server |
| 32 | 1 | 32 | High-end server |
| 64+ | 1 | 64+ | Enterprise/cloud |

**Workloads to Test:**
- CRUD operations (write-heavy)
- Read-only queries (read-heavy)
- Mixed OLTP (50/50 read/write)
- Analytical queries (OLAP)
- Vector search (parallel)
- Graph traversal (parallel)

**Key Metrics:**
```python
# For each configuration:
metrics = {
    "throughput_ops_sec": float,
    "latency_p50_ms": float,
    "latency_p99_ms": float,
    "cpu_utilization_percent": float,
    "context_switches_per_sec": int,
    "cache_miss_rate_percent": float,
    "scaling_efficiency": float,  # speedup / cores
}

# Scaling Efficiency Thresholds:
# > 0.85: Excellent scaling
# 0.70-0.85: Good scaling
# 0.50-0.70: Acceptable
# < 0.50: Poor scaling (investigate bottlenecks)
```

### 3.2 Memory Bandwidth Utilization

**Test:** Measure memory-bound vs CPU-bound performance.

**Scenarios:**

1. **Sequential Scan**: Tests memory bandwidth
2. **Random Access**: Tests cache efficiency
3. **Large Aggregations**: Tests memory + CPU
4. **Index Scans**: Tests cache + memory hierarchy

**Configuration:**
```yaml
memory_tests:
  - name: "Sequential_Scan_Large_Table"
    table_size_gb: 50
    operation: "SELECT * FROM table"
    expected_bottleneck: "memory_bandwidth"
    target_throughput_gb_sec: 40-60  # DDR4
    
  - name: "Random_Key_Lookup"
    dataset_size: 10000000
    operation: "point_select"
    expected_bottleneck: "cache_efficiency"
    target_latency_us: 5-15
    
  - name: "Hash_Join_Large_Tables"
    left_table_gb: 10
    right_table_gb: 5
    expected_bottleneck: "memory_capacity"
    target_time_seconds: 30-60
```

### 3.3 CPU Cache Efficiency

**Levels to Test:**

| Cache Level | Size (typical) | Latency | Test Approach |
|------------|----------------|---------|---------------|
| L1 Data | 32-64 KB | ~1 ns | Working set < 32KB |
| L2 | 256 KB - 1 MB | ~3-5 ns | Working set 64KB-512KB |
| L3 | 8-64 MB | ~10-20 ns | Working set 1-8MB |
| RAM | 8-256 GB | ~65 ns | Working set > L3 |

**Benchmark:**
```cpp
// Cache efficiency test
void benchmark_cache_levels() {
    // Test various working set sizes
    size_t sizes[] = {
        16 * 1024,      // 16KB - fits in L1
        128 * 1024,     // 128KB - fits in L2
        2 * 1024 * 1024,  // 2MB - fits in L3
        64 * 1024 * 1024  // 64MB - spills to RAM
    };
    
    for (auto size : sizes) {
        benchmark_random_access(size);
        // Measure: latency, cache miss rate
    }
}
```

### 3.4 Storage I/O Characterization

**Test Configurations:**

| Storage Type | IOPS | Bandwidth | Latency | Test Scenarios |
|-------------|------|-----------|---------|----------------|
| HDD | ~200 | 150 MB/s | 5-10 ms | Sequential scan |
| SATA SSD | ~100K | 550 MB/s | 50-100 μs | Mixed workload |
| NVMe SSD | ~500K | 3.5 GB/s | 20-50 μs | Write-heavy |
| NVMe Gen4 | ~1M | 7 GB/s | 10-20 μs | Extreme performance |

**Benchmark:**
```python
storage_benchmarks = {
    "sequential_write": {
        "block_size": "1MB",
        "duration": "60s",
        "metric": "mb_per_second"
    },
    "random_read_4k": {
        "block_size": "4KB",
        "queue_depth": 32,
        "metric": "iops"
    },
    "mixed_70_30": {
        "read_percent": 70,
        "write_percent": 30,
        "metric": "total_ops_per_second"
    }
}
```

### 3.5 NUMA Configuration Testing

**For Multi-Socket Systems:**

```yaml
numa_tests:
  - name: "Local_Memory_Access"
    description: "All threads on socket 0, memory on node 0"
    expected_latency_ns: 65
    
  - name: "Remote_Memory_Access"
    description: "Threads on socket 0, memory on node 1"
    expected_latency_ns: 130  # 2x slower
    
  - name: "Interleaved_Memory"
    description: "Memory interleaved across nodes"
    expected_latency_ns: 90-100  # Average
    
  - name: "Thread_Pinning"
    description: "Pin threads to cores with local memory"
    expected_improvement_percent: 20-40
```

---

## 4. Modern Database Workloads

### 4.1 Hybrid OLTP/OLAP (HTAP)

**Objective:** Test mixed transactional and analytical workloads simultaneously.

**Workload Mix:**
```python
htap_workload = {
    "oltp_threads": 16,  # Transactional: inserts, updates, point queries
    "olap_threads": 4,   # Analytical: aggregations, scans, joins
    "oltp_ops_per_sec": 50000,
    "olap_queries_per_min": 60,
    "duration_minutes": 30
}

# Key Metrics:
# 1. OLTP throughput degradation: < 10% acceptable
# 2. OLAP query latency: Should remain < 5 seconds
# 3. Isolation: OLAP shouldn't see dirty OLTP data
```

### 4.2 Time-Series Data Ingestion

**Test Cases:**

1. **High-Frequency IoT**: 100K-1M events/sec
2. **Financial Tick Data**: Sub-millisecond timestamps
3. **Log Aggregation**: Variable-size events

**Configuration:**
```yaml
timeseries_benchmarks:
  - name: "IoT_Sensor_Stream"
    event_rate_per_sec: 500000
    event_size_bytes: 128
    unique_sensors: 10000
    duration_minutes: 10
    queries:
      - "Last 1 hour aggregates"
      - "Anomaly detection (z-score > 3)"
      - "Downsampling to 1-minute intervals"
    metrics:
      - ingestion_throughput
      - query_latency_ms
      - compression_ratio
    targets:
      ingestion_mb_sec: 60-80
      query_latency_p95_ms: 100-500
```

### 4.3 Graph Pattern Matching

**Complex Graph Queries:**

1. **Triangle Counting**: Find all triangles in social network
2. **Community Detection**: Louvain or Label Propagation
3. **PageRank**: Iterative graph algorithm
4. **Shortest Path with Constraints**: Path with specific edge types

**Benchmark:**
```cypher
-- Query: Find influential users (simplified PageRank)
MATCH (user:User)
OPTIONAL MATCH (user)<-[:FOLLOWS]-(follower:User)
WITH user, COUNT(follower) as followerCount
MATCH (user)-[:FOLLOWS]->(following:User)
WITH user, followerCount, COUNT(following) as followingCount
RETURN user.id, followerCount, followingCount,
       followerCount * 1.0 / (followingCount + 1) as influence
ORDER BY influence DESC
LIMIT 100;
```

**Expected Performance:**
- Triangle counting (1M nodes): 5-30 seconds
- PageRank (10 iterations, 1M nodes): 10-60 seconds
- Shortest path with constraints: 50-500ms

### 4.4 Multi-Model Query Complexity

**Test:** Single query using multiple models simultaneously.

**Example Scenarios:**

```sql
-- Scenario 1: Relational + Vector + Graph
-- "Find similar products purchased by friends"
WITH user_id = 12345
-- Graph: Get friends
MATCH (user:User {id: user_id})-[:FRIEND]->(friend:User)
-- Relational: Get friend purchases
MATCH (friend)-[:PURCHASED]->(product:Product)
-- Vector: Find similar products to user's interests
VECTOR_SEARCH (
    embedding: user.interest_embedding,
    index: product_embeddings,
    k: 50
) AS similar_products
RETURN product.id, product.name,
       COUNT(friend) as friends_purchased,
       similar_products.score as similarity
ORDER BY friends_purchased DESC, similarity DESC
LIMIT 20;

-- Expected latency: 50-200ms (depends on graph size and vector index)
```

---

## 5. Implementation Roadmap

### Phase 1: Infrastructure Setup (Weeks 1-2)

**Tasks:**
- [ ] Set up benchmark harness with pluggable workloads
- [ ] Implement result collection and analysis framework
- [ ] Create hardware profiling tools (CPU, memory, I/O)
- [ ] Establish baseline measurements

**Deliverables:**
- `benchmarks/framework/` - Core benchmark infrastructure
- `benchmarks/harness.py` - Main orchestration script
- Hardware profiling integrated with existing `hardware_constraints_analyzer.py`

### Phase 2: TPC Benchmarks (Weeks 3-5)

**Tasks:**
- [ ] TPC-C data generator and workload
- [ ] TPC-H query templates (22 queries)
- [ ] Result validation against published benchmarks
- [ ] ThemisDB-specific optimizations

**Deliverables:**
- `benchmarks/tpc/tpc_c_runner.py`
- `benchmarks/tpc/tpc_h_queries.sql`
- `benchmarks/tpc/TPC_RESULTS.md`

### Phase 3: YCSB Integration (Weeks 6-7)

**Tasks:**
- [ ] YCSB workload configurations (A-F)
- [ ] ThemisDB binding for YCSB
- [ ] Multi-threaded execution
- [ ] Comparison with MongoDB, Redis

**Deliverables:**
- `benchmarks/ycsb/themisdb_binding.py`
- `benchmarks/ycsb/workload_configs/`
- `benchmarks/ycsb/YCSB_RESULTS.md`

### Phase 4: Vector & AI Benchmarks (Weeks 8-10)

**Tasks:**
- [ ] ANN-Benchmarks integration (SIFT1M, GIST1M)
- [ ] RAG workflow benchmarks
- [ ] LLM integration performance tests
- [ ] Image analysis pipeline tests

**Deliverables:**
- `benchmarks/vector/ann_benchmark.py`
- `benchmarks/ai/rag_benchmark.py`
- `benchmarks/ai/embedding_benchmark.py`
- `benchmarks/vector/ANN_RESULTS.md`

### Phase 5: Hardware Scaling Tests (Weeks 11-12)

**Tasks:**
- [ ] Multi-core scaling (1-64 cores)
- [ ] Thread configuration optimization
- [ ] Memory bandwidth tests
- [ ] Cache efficiency analysis
- [ ] NUMA configuration tests

**Deliverables:**
- `benchmarks/hardware/scaling_tests.py`
- `benchmarks/hardware/HARDWARE_TUNING_GUIDE.md`
- Performance recommendations by hardware profile

### Phase 6: Documentation & Reporting (Weeks 13-14)

**Tasks:**
- [ ] Comprehensive benchmark guide
- [ ] Performance comparison reports
- [ ] Tuning recommendations
- [ ] Visualization dashboard

**Deliverables:**
- `benchmarks/COMPREHENSIVE_BENCHMARK_GUIDE.md`
- `benchmarks/reports/` - Auto-generated reports
- `benchmarks/dashboard/` - Interactive visualization

---

## 6. References

### Scientific Publications

1. **Transaction Processing Performance Council (TPC)**
   - TPC-C Specification: http://www.tpc.org/tpcc/
   - TPC-H Specification: http://www.tpc.org/tpch/
   - TPC Benchmark Standards: All results must be audited and published

2. **Yahoo! Cloud Serving Benchmark (YCSB)**
   - Cooper et al., "Benchmarking Cloud Serving Systems with YCSB" (SoCC 2010)
   - GitHub: https://github.com/brianfrankcooper/YCSB
   - Paper: https://research.yahoo.com/files/ycsb.pdf

3. **Linked Data Benchmark Council (LDBC)**
   - Erling et al., "The LDBC Social Network Benchmark" (SIGMOD 2015)
   - Website: https://ldbcouncil.org/
   - Specification: https://github.com/ldbc/ldbc_snb_docs

4. **ANN-Benchmarks**
   - Aumüller et al., "ANN-Benchmarks: A Benchmarking Tool for Approximate Nearest Neighbor Algorithms" (SISAP 2017)
   - Website: http://ann-benchmarks.com/
   - GitHub: https://github.com/erikbern/ann-benchmarks

### Industry Benchmarks

5. **MLPerf** (AI/ML Performance)
   - Website: https://mlperf.org/
   - Inference benchmarks for various neural network models
   - Includes recommendation systems, object detection, NLP

6. **SPEC (Standard Performance Evaluation Corporation)**
   - SPEC CPU: CPU-intensive benchmarks
   - SPEC Storage: I/O performance benchmarks
   - Website: https://www.spec.org/

7. **ACM SIGMOD**
   - "Good Benchmarking Practices for Database Systems" (various papers)
   - Database benchmarking best practices and methodologies

### Database-Specific Research

8. **Hybrid OLTP/OLAP (HTAP)**
   - Raza et al., "HTAP Databases: What is New and What is Next" (CIDR 2020)
   - MemSQL (now SingleStore) HTAP benchmarks
   - TiDB HTAP architecture papers

9. **Vector Databases**
   - Milvus benchmark reports: https://milvus.io/docs/benchmark.md
   - Pinecone performance guides
   - Weaviate benchmarking methodology

10. **Graph Databases**
    - Neo4j performance tuning guides
    - JanusGraph benchmarking
    - Amazon Neptune best practices

### Hardware Performance

11. **Intel and AMD Architecture Guides**
    - Intel 64 and IA-32 Architectures Optimization Reference Manual
    - AMD Zen Architecture Documentation
    - NUMA optimization guides

12. **Memory and Storage**
    - "What Every Programmer Should Know About Memory" (Ulrich Drepper)
    - NVMe performance characteristics (SNIA)
    - Storage performance papers (FAST conference)

### Tools and Frameworks

13. **Google Benchmark** (C++)
    - GitHub: https://github.com/google/benchmark
    - Used by ThemisDB for micro-benchmarks

14. **Apache JMeter / Gatling**
    - Load testing frameworks
    - Can be adapted for database benchmarking

15. **Linux Performance Tools**
    - `perf`: Performance monitoring
    - `iostat`: I/O statistics
    - `vmstat`: Virtual memory statistics
    - Brendan Gregg's tools: https://www.brendangregg.com/

---

## Appendix A: Benchmark Checklist

Before running any benchmark, ensure:

### Pre-Benchmark
- [ ] System is dedicated (no other processes)
- [ ] CPU frequency scaling disabled
- [ ] Disk caches cleared (`sync; echo 3 > /proc/sys/vm/drop_caches`)
- [ ] Network latency measured (if distributed)
- [ ] Temperature stable (not thermal throttling)

### During Benchmark
- [ ] Monitor CPU, memory, I/O usage
- [ ] Record hardware counters (cache misses, branch mispredictions)
- [ ] Log all configuration parameters
- [ ] Capture system logs for errors

### Post-Benchmark
- [ ] Calculate statistical significance (p-value < 0.05)
- [ ] Remove outliers (IQR method)
- [ ] Compute confidence intervals (95%, 99%)
- [ ] Compare against baselines
- [ ] Document any anomalies

---

## Appendix B: Expected Performance Ranges

| Benchmark | Metric | Conservative | Target | Stretch |
|-----------|--------|--------------|--------|---------|
| TPC-C | tpmC | 120K | 180K | 220K |
| TPC-H SF100 | QphH | 20K | 30K | 40K |
| YCSB-A (50/50) | ops/sec | 60K | 100K | 150K |
| YCSB-C (read-only) | ops/sec | 150K | 250K | 350K |
| Vector SIFT1M | QPS @90% recall | 5K | 10K | 20K |
| LDBC IC2 | Latency (ms) | 40 | 25 | 15 |
| Graph BFS (1M nodes) | ms | 100 | 50 | 20 |

**Note:** Actual performance depends heavily on hardware configuration.

---

## Appendix C: Hardware Recommendations

### Benchmark Server Specifications

**Minimum:**
- CPU: 8 cores @ 3.0 GHz
- RAM: 32 GB
- Storage: 500GB SSD
- Network: 1 Gbps

**Recommended:**
- CPU: 16 cores @ 3.5 GHz
- RAM: 64 GB
- Storage: 1TB NVMe SSD
- Network: 10 Gbps

**Ideal:**
- CPU: 32+ cores @ 3.8 GHz
- RAM: 128 GB (or more)
- Storage: 2TB+ NVMe Gen4
- Network: 25+ Gbps

---

**Document Version:** 2.0  
**Last Updated:** 2026-04-06  
**Status:** ✅ Ready for Implementation

