# ThemisDB Benchmarks

This directory contains performance benchmarks and testing utilities for ThemisDB.

## 🎯 NEW: Advanced Scientific Benchmark Suite

**Status:** ✅ Phase 1 Complete - Research & Core Implementation Ready  
**Documentation:** ~88KB | **Code:** ~18KB | **Timeline:** 14 weeks total

> **Zusammenfassung (German):** Als Antwort auf die Anforderung nach mehr und besseren Benchmarks für moderne Datenbanken und KI-Systeme haben wir eine umfassende Forschungs- und Implementierungsgrundlage geschaffen, die auf wissenschaftlichen und industriellen Standards (TPC, YCSB, LDBC, ANN-Benchmarks) basiert. Phase 1 ist abgeschlossen mit kompletter Dokumentation und einem funktionsfähigen Hardware-Skalierungs-Benchmark-Tool.

### 📖 Start Here

**[BENCHMARK_SUITE_EXECUTIVE_SUMMARY.md](BENCHMARK_SUITE_EXECUTIVE_SUMMARY.md)** ⭐ Executive Summary
- Complete overview of the benchmark suite
- Key findings from research
- Implementation roadmap (6 phases, 14 weeks)
- Performance targets by hardware configuration
- Quick start guide

### 📚 Comprehensive Documentation

1. **[ADVANCED_BENCHMARK_RESEARCH.md](ADVANCED_BENCHMARK_RESEARCH.md)** (25KB) - Scientific Standards Research
   - **TPC-C/TPC-H** - Transaction Processing Council standards (OLTP/OLAP)
   - **YCSB** - Yahoo Cloud Serving Benchmark (Workloads A-F)
   - **LDBC** - Linked Data Benchmark Council (Graph databases)
   - **ANN-Benchmarks** - Approximate Nearest Neighbor (Vector search)
   - **RAG Workflows** - Retrieval Augmented Generation (LLM integration)
   - Expected performance baselines from PostgreSQL, MongoDB, Neo4j, etc.
   - Complete academic and industry references

2. **[HARDWARE_CONFIGURATION_BENCHMARK_SUITE.md](HARDWARE_CONFIGURATION_BENCHMARK_SUITE.md)** (32KB) - Hardware Testing Guide
   - **Core Count Scaling** - Tests for 1, 2, 4, 8, 16, 32, 64+ cores
   - **Thread Optimization** - Hyperthreading analysis, thread pool sizing
   - **Memory Architecture** - Bandwidth, cache efficiency, NUMA testing
   - **Storage Types** - HDD, SATA SSD, NVMe Gen3/Gen4 comparison
   - **Network Performance** - Latency and bandwidth impact analysis
   - Python implementation examples
   - Performance grading system (A+ to F)

3. **[INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)** (13KB) - How to Use
   - Quick start guide for all benchmarks
   - Integration with existing infrastructure
   - CI/CD pipeline examples
   - Implementation roadmap with timeline
   - Performance targets by configuration

### 🚀 Implemented Tools

**[hardware_scaling_benchmark.py](hardware_scaling_benchmark.py)** (18KB) - ✅ Ready to Use

Automated hardware configuration testing:
- **Auto-detection** of CPU, memory, storage, NUMA nodes
- **Core count scaling** tests with configurable parameters
- **Scaling efficiency** analysis with performance grading (A+ to F)
- **Statistical analysis** with multiple repetitions
- **JSON and Markdown reports** for automation and documentation

```bash
# Quick start
cd benchmarks
python3 hardware_scaling_benchmark.py --core-counts "1,2,4,8,16"

# Custom configuration
python3 hardware_scaling_benchmark.py \
  --core-counts "1,2,4,8,16,32" \
  --workload ycsb_a \
  --duration 60 \
  --repetitions 5 \
  --output-dir ./my_results

# View results
cat ./my_results/hardware_scaling_report.md
```

**Output Example:**
```
SCALING EFFICIENCY ANALYSIS
Cores    Threads    Throughput      Speedup      Efficiency   Grade               
1        1          85,234          1.00x        100.0%       -                   
2        2          162,445         1.91x        95.3%        A+ (Excellent)      
4        4          310,892         3.65x        91.2%        A+ (Excellent)      
8        8          580,123         6.81x        85.1%        A (Very Good)       
16       16         1,045,678       12.27x       76.7%        B (Good)            
```

### 📋 Planned Implementations (Phases 2-6)

Based on scientific standards documented in research:

| Phase | Benchmark | Standard | Duration | Status |
|-------|-----------|----------|----------|--------|
| 2 | **TPC-C** (OLTP) | Transaction Processing Council | 3 weeks | 📋 Planned |
| 2 | **TPC-H** (OLAP) | 22 analytical queries | 3 weeks | 📋 Planned |
| 3 | **YCSB** (A-F) | Yahoo Cloud Serving | 2 weeks | 📋 Planned |
| 4 | **LDBC** Social Network | Graph database standard | 3 weeks | 📋 Planned |
| 4 | **ANN-Benchmarks** | Vector search (SIFT1M, Deep1B) | 3 weeks | 📋 Planned |
| 4 | **RAG Workflows** | LLM integration | 3 weeks | 📋 Planned |
| 5 | **Advanced Hardware** | Thread, memory, NUMA | 2 weeks | 🚧 Partial |
| 6 | **Reporting & Viz** | Dashboard, recommendations | 2 weeks | 📋 Planned |

**Total Timeline:** 14 weeks | **Current:** Week 2 (Phase 1 Complete)

See [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) for detailed roadmap.

### 🎯 Performance Targets

Based on industry research (PostgreSQL, MongoDB, Neo4j, FAISS):

| Configuration | OLTP (ops/s) | OLAP (queries/min) | Vector Search (QPS) |
|--------------|-------------|-------------------|-------------------|
| 4-core, 8GB, SSD | 200-300K | 50-100 | 5-10K |
| 8-core, 16GB, NVMe | 400-600K | 100-200 | 10-20K |
| 16-core, 32GB, NVMe | 700-1000K | 200-400 | 20-40K |
| 32-core, 64GB+, NVMe Gen4 | 1.2-1.8M | 400-800 | 40-80K |

### 🔬 Scientific Rigor

All benchmarks follow scientific methodology:
- ✅ Multiple repetitions (10+ per test)
- ✅ Warmup phases (5+ runs)
- ✅ Statistical analysis (mean, stddev, percentiles, CI)
- ✅ Hardware profiling (CPU, RAM, storage, NUMA)
- ✅ Deterministic execution (reproducible seeds)
- ✅ Outlier detection (IQR method)
- ✅ Confidence intervals (95% & 99%)
- ✅ Effect size calculation (Cohen's d)

---

## Comparative Benchmark Suite (EXISTING)

**Directory:** [`comparative/`](comparative/)

A comprehensive benchmark framework for comparing ThemisDB against established database systems:

- **Multi-Database Support**: PostgreSQL, MongoDB, Redis, ArangoDB, Neo4j, Milvus, Elasticsearch
- **Docker-based Environment**: Consistent, reproducible testing with resource-limited containers
- **Hugging Face Datasets**: Standardized test data (Wikipedia Simple English, synthetic vectors/graphs)
- **Comprehensive Metrics**: Latency percentiles (p50/p95/p99), throughput, resource usage
- **HTML/Markdown Reports**: Interactive charts and documentation-ready output

### Quick Start

```bash
# Navigate to comparative benchmarks
cd benchmarks/comparative

# Install dependencies
pip install -r requirements.txt

# Start all database containers
docker-compose -f docker-compose.benchmark.yml up -d

# Run benchmarks
python scripts/run_benchmarks.py --all --databases themisdb,postgresql,mongodb

# Generate reports
python scripts/generate_report.py --format html --output reports/
```

See [`comparative/README.md`](comparative/README.md) for detailed documentation.

---

## Internal Benchmarks

### Sharding Performance Benchmarks
**File:** `bench_sharding_performance.cpp`

Comprehensive performance benchmarks for distributed sharding:
- **Scatter-Gather Latency**: Query distribution across 10-100 shards
- **Cross-Shard Join**: Broadcast Hash Join and Co-Located Join strategies
- **Rebalancing Throughput**: Batch serialization/deserialization performance
- **P2P Gossip Overhead**: Message serialization, fanout selection, version vector merge
- **Multi-DC Routing**: Datacenter proximity and cross-DC latency simulation
- **Concurrent Access**: Multi-threaded shard operations (1-16 threads)

### Shard Routing Benchmarks
**File:** `bench_shard_routing.cpp`

- Single shard lookup performance
- Consistent hash distribution quality
- Batch routing operations
- Hot shard pattern simulation

### Graph Traversal Benchmarks
**File:** `bench_graph_traversal.cpp`

- BFS/DFS traversal performance
- Shortest path (Dijkstra)
- Degree centrality
- Connected components

### Hybrid Query Benchmarks
**File:** `README_HYBRID_BENCH.md`

Performance benchmarks for hybrid queries combining:
- Full-text search
- Vector similarity search
- Graph traversal
- Geospatial queries

## Running Benchmarks

Benchmarks can be executed using the build system. See the main [README.md](../README.md) for build instructions.

```bash
# Build benchmarks
cmake -DCMAKE_BUILD_TYPE=Release ..
make benchmarks

# Run specific benchmark
./bench_sharding_performance --benchmark_filter=ScatterGather
./bench_shard_routing --benchmark_filter=ConsistentHash

# Run all benchmarks with JSON output
./bench_sharding_performance --benchmark_out=results.json --benchmark_out_format=json
```

## Results and Analysis

Benchmark results and analysis can be found in:
- [Compression Benchmarks](../docs/compression_benchmarks.md)
- [Performance Benchmarks](../docs/performance_benchmarks.md)
- [Hybrid Query Benchmarks](../docs/HYBRID_QUERY_BENCHMARKS.md)
- [Sharding Performance](../docs/SCALING_TODO.md)

## Benchmark Categories

| Category | File | Key Metrics |
|----------|------|-------------|
| Sharding | `bench_sharding_performance.cpp` | Latency, Throughput, Overhead |
| Routing | `bench_shard_routing.cpp` | Lookup time, Distribution quality |
| Graph | `bench_graph_traversal.cpp` | Nodes/sec, Path length |
| CRUD | `bench_crud.cpp` | Ops/sec, Index overhead |
| Vector | `bench_vector_search.cpp` | Queries/sec, Recall |
| Queries | `bench_query.cpp` | Query latency, TPS |

## Contributing

When adding new benchmarks, please:
1. Follow the existing benchmark structure (Google Benchmark framework)
2. Document the benchmark methodology
3. Include baseline comparisons
4. Update this README with benchmark descriptions
5. Add appropriate `--benchmark_filter` tags
