# ThemisDB Benchmarks - CHIMERA Suite

**CHIMERA Suite** - _Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment_

> **"Benchmark the Unbenchmarkable"** - The industry's first comprehensive benchmark suite for hybrid multi-model databases with native AI/LLM integration.

This directory contains the CHIMERA Suite, a scientifically rigorous benchmark framework for ThemisDB and comparative database systems.

## 🎯 What is CHIMERA Suite?

The CHIMERA Suite represents a new generation of database benchmarking that goes beyond traditional single-model tests. Like the mythical Chimera - a creature composed of multiple beings - this suite evaluates the hybrid nature of modern databases:

- **Multi-Model Workloads**: Graph + Vector + Relational + Document in unified transactions
- **AI/LLM Integration**: Native LLM inference, LoRA adapters, RAG workflows
- **Hybrid Inferencing**: Combining database queries with machine learning inference
- **Scientific Standards**: IEEE/ACM compliant methodology with vendor neutrality

## 📚 Scientific Foundation & Standards

**[CHIMERA Suite Documentation](chimera/README.md)** ⭐ Core Framework
- Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment
- Vendor-neutral reporting framework with IEEE/ACM compliance
- Statistical rigor (t-test, Mann-Whitney, Cohen's d, confidence intervals)
- Color-blind friendly visualizations (Okabe-Ito, Paul Tol palettes)
- Multi-format reports (HTML, CSV, PDF)

**[CHIMERA Scientific Foundation](../docs/benchmarks/CHIMERA_SCIENTIFIC_FOUNDATION.md)** ⭐ Methodology
- IEEE/ACM standard citations for all benchmark categories
- Statistical methodology with complete mathematical foundations
- Reproducibility standards (ACM Artifact Badging compliance)
- Hardware profiling and dataset transparency specifications
- Complete bibliography with BibTeX export

**[BibTeX References](../docs/benchmarks/references.bib)** - Complete bibliography for scientific papers

**[Configuration Template](../docs/benchmarks/benchmark_config_template.toml)** - Hardware profiling and reproducibility template

## 🎯 CHIMERA Suite: Advanced Scientific Benchmarks

**Status:** ✅ Phases 1-4 Complete (Weeks 1-8) - Production Ready  
**Documentation:** ~125KB | **Code:** ~70KB C++ | **Framework:** Python + C++

> **Mission:** Provide the world's first comprehensive benchmark suite for hybrid multi-model databases with native AI/LLM integration, following scientific standards (TPC, YCSB, LDBC, ANN-Benchmarks) with vendor neutrality.

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

### ✅ Completed Scientific Benchmarks (Phases 2-4)

Based on scientific standards - All implemented in C++ with Google Benchmark:

| Phase | Benchmark | Standard | Status | File |
|-------|-----------|----------|--------|------|
| 2 | **TPC-C** (OLTP) | Transaction Processing Council | ✅ Complete | [bench_tpcc.cpp](bench_tpcc.cpp) |
| 3 | **YCSB** (A-F) | Yahoo Cloud Serving | ✅ Complete | [bench_ycsb.cpp](bench_ycsb.cpp) |
| 4 | **MMDB-E** | Multi-Modal + Embeddings + LLM | ✅ Complete | [bench_mmdb.cpp](bench_mmdb.cpp) |

**Usage:**
```bash
# Build all benchmarks
cd build
cmake .. -DTHEMIS_BUILD_BENCHMARKS=ON
make bench_tpcc bench_ycsb bench_mmdb

# Run TPC-C (5 transaction types: New Order, Payment, Order Status, Stock Level, Delivery)
./bench_tpcc
./bench_tpcc --benchmark_filter=NewOrderTransaction

# Run YCSB (Workloads A-F: Update Heavy, Read Mostly, Read Only, Read Latest, Scan, RMW)
./bench_ycsb
./bench_ycsb --benchmark_filter="WorkloadC"  # Read-only (fastest)

# Run MMDB-E (Multi-Modal: Hybrid CRUD, Semantic Search, Graph, RAG, Analytics)
./bench_mmdb
./bench_mmdb --benchmark_filter="SemanticSearch"

# Export results to JSON
./bench_tpcc --benchmark_out=tpcc_results.json --benchmark_out_format=json
```

**Documentation:**
- [TPC README](tpc/README.md) - TPC-C benchmark details
- [YCSB README](ycsb/README.md) - YCSB workloads documentation
- [MMDB-E README](mmdb/README.md) - Multi-modal benchmark (Deutsch)
- [MMDB-E Design](MMDB_E_BENCHMARK_DESIGN.md) - Complete specification

### 📋 Planned Implementations (Phases 5-6)

| Phase | Benchmark | Standard | Duration | Status |
|-------|-----------|----------|----------|--------|
| 5 | **TPC-H** (OLAP) | 22 analytical queries | 3 weeks | 📋 Planned |
| 5 | **LDBC** Social Network | Graph database standard | 3 weeks | 📋 Planned |
| 5 | **ANN-Benchmarks** | Vector search (SIFT1M, Deep1B) | 3 weeks | 📋 Planned |
| 5 | **Advanced Hardware** | Thread, memory, NUMA | 2 weeks | 🚧 Partial |
| 6 | **Reporting & Viz** | Dashboard, recommendations | 2 weeks | 📋 Planned |

**Total Timeline:** 14 weeks | **Current:** Week 8 (Phases 1-4 Complete) | **Status:** Ahead of Schedule

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

---

## 🚨 Performance Regression Detection & Baseline Management

**Automated performance monitoring with PR blocking for regressions > 10%**

ThemisDB includes an automated performance regression detection system that:

- 📊 **Stores baselines** per branch (main/develop) and release
- 🚫 **Blocks PRs** automatically if regressions exceed 10%
- 📈 **Tracks trends** via Grafana dashboards
- 🔔 **Sends alerts** for performance violations
- 📝 **Full documentation** of thresholds and pipelines

### Quick Start

```bash
# Check for regressions in your changes
python performance_regression_detector.py \
  --baseline baselines/main/latest.json \
  --current my_results.json \
  --output report.txt

# View available baselines
python baseline_manager.py list

# Export metrics for Grafana
python metrics_exporter.py \
  --baseline baselines/main/latest.json \
  --output metrics.prom
```

### Key Features

- **Thresholds**: Minor (5%), Major (10% - blocks PR), Critical (20%)
- **Metrics**: Throughput, latency, CPU time, memory usage
- **Workflows**: 
  - `nightly.yml` - Nightly performance benchmarks
  - `tests-extended.yml` - Extended performance tests
  - `ops-automation.yml` - Baseline updates
- **Dashboard**: Grafana configuration in `monitoring/`

### Documentation

- 📖 **[Performance Regression Detection Guide](../docs/PERFORMANCE_REGRESSION_DETECTION.md)** - Complete documentation
- 📋 **[Quick Reference](../docs/PERFORMANCE_REGRESSION_QUICK_REFERENCE.md)** - Common commands and scenarios
- 🔔 **[Alerting Configuration](../docs/PERFORMANCE_ALERTING_CONFIG.md)** - Slack, email, PagerDuty setup

### Files

```
benchmarks/
├── baselines/                    # Baseline storage
│   ├── main/latest.json         # Main branch baseline
│   ├── develop/latest.json      # Develop branch baseline
│   └── releases/v*.json         # Release baselines
├── baseline_manager.py           # Manage baselines
├── performance_regression_detector.py  # Detect regressions
├── metrics_exporter.py           # Export to Prometheus
└── monitoring/
    └── performance_regression_dashboard.json  # Grafana dashboard
```

