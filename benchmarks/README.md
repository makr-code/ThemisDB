# ThemisDB Benchmarks

This directory contains performance benchmarks and testing utilities for ThemisDB.

## Comparative Benchmark Suite (NEW)

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
