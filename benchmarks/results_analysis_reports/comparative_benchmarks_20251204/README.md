> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Comparative Database Benchmark Suite

This directory contains a comprehensive benchmark framework for comparing ThemisDB against established database systems using standardized tests and datasets.

## Overview

The benchmark suite uses Docker containers to ensure consistent, reproducible testing environments across all database systems. It leverages Hugging Face datasets for standardized test data that represents real-world use cases.

## Database Candidates

| Database | Type | Use Case Comparison |
|----------|------|---------------------|
| **ThemisDB** | Multi-Model | Reference system |
| **PostgreSQL** | Relational | SQL queries, ACID |
| **MongoDB** | Document | JSON storage, flexibility |
| **Redis** | Key-Value/Cache | High-speed operations |
| **ArangoDB** | Multi-Model | Direct competitor |
| **Neo4j** | Graph | Graph traversals |
| **Milvus** | Vector | Vector similarity search |
| **Elasticsearch** | Search | Full-text search |
| **ChromaDB** | Vector | AI/ML embeddings |

## Benchmark Categories

### 1. CRUD Operations
- Single entity insert/read/update/delete
- Batch operations (100, 1000, 10000 entities)
- Concurrent operations (1, 4, 8, 16 threads)

### 2. Query Performance
- Point queries (primary key lookup)
- Range queries (numeric and string ranges)
- Complex filters (AND/OR combinations)
- Aggregations (COUNT, SUM, AVG, MIN, MAX)

### 3. Vector Search (where applicable)
- k-NN search (k=10, 50, 100)
- Hybrid search (vector + metadata filter)
- Index build time
- Recall accuracy

### 4. Graph Operations (where applicable)
- BFS traversal (depth 1-5)
- Shortest path
- Connected components

### 5. Full-Text Search (where applicable)
- Single term search
- Phrase search
- Boolean queries

### 6. Polyglot Persistence Benchmark (NEW)

Compares ThemisDB's unified multi-model approach against polyglot persistence patterns:

| Scenario | ThemisDB | Polyglot Approach |
|----------|----------|-------------------|
| **Graph + Vector** | Single DB | Neo4j + ChromaDB |
| **Relational + Graph** | Single DB | PostgreSQL + Neo4j |
| **Full Multi-Model** | Single DB | PostgreSQL + Neo4j + ChromaDB |

**Key Advantages of ThemisDB:**
- Single database for all data models
- No cross-database coordination needed
- Simpler operational complexity
- Native multi-model queries

```bash
# Run polyglot persistence benchmark
python scripts/polyglot_benchmark.py --all

# Run specific scenario
python scripts/polyglot_benchmark.py --scenario graph-vector
```

## Hugging Face Datasets

The benchmark uses the following datasets:

### Primary Dataset: `wikipedia-simple-english`
- **Source**: `wikipedia` (simple English subset)
- **Size**: ~200K documents
- **Fields**: title, text, categories
- **Use**: Document storage, full-text search, content processing

### Vector Dataset: `sentence-transformers/all-MiniLM-L6-v2-embeddings`
- **Dimensions**: 384
- **Size**: 10K-100K vectors (configurable)
- **Use**: Vector similarity search benchmarks

### Graph Dataset: `social-network-sample`
- **Generated**: Based on common social network patterns
- **Nodes**: 10K-100K users
- **Edges**: 50K-500K relationships
- **Use**: Graph traversal benchmarks

## Directory Structure

```
comparative/
├── README.md                    # This file
├── docker-compose.benchmark.yml # Multi-database Docker setup
├── Dockerfile.benchmark         # Benchmark runner image
├── config/                      # Database configurations
│   ├── themisdb.yaml
│   ├── postgresql.conf
│   ├── mongodb.conf
│   ├── redis.conf
│   └── ...
├── datasets/                    # Dataset loaders and generators
│   ├── huggingface_loader.py
│   ├── vector_generator.py
│   └── graph_generator.py
├── benchmarks/                  # Benchmark implementations
│   ├── base_benchmark.py
│   ├── crud_benchmark.py
│   ├── query_benchmark.py
│   ├── vector_benchmark.py
│   └── graph_benchmark.py
├── adapters/                    # Database-specific adapters
│   ├── themisdb_adapter.py
│   ├── postgresql_adapter.py
│   ├── mongodb_adapter.py
│   └── ...
├── results/                     # Benchmark results (generated)
│   └── .gitkeep
├── reports/                     # Generated reports
│   └── .gitkeep
├── scripts/
│   ├── run_benchmarks.py        # Main benchmark runner
│   ├── generate_report.py       # Report generation
│   └── setup_datasets.py        # Dataset preparation
└── requirements.txt             # Python dependencies
```

## Quick Start

### 1. Prerequisites

- Docker and Docker Compose
- Python 3.10+
- 16GB+ RAM recommended
- 50GB+ disk space

### 2. Setup

```bash
# Navigate to benchmark directory
cd benchmarks/comparative

# Install Python dependencies
pip install -r requirements.txt

# Start database containers
docker-compose -f docker-compose.benchmark.yml up -d

# Wait for all databases to be ready
./scripts/wait_for_databases.sh

# Load test datasets
python scripts/setup_datasets.py --dataset wikipedia-simple --size 10000
```

### 3. Run Benchmarks

```bash
# Run all benchmarks
python scripts/run_benchmarks.py --all

# Run specific benchmark category
python scripts/run_benchmarks.py --category crud

# Run for specific databases only
python scripts/run_benchmarks.py --databases themisdb,postgresql,mongodb

# Run with custom dataset size
python scripts/run_benchmarks.py --dataset-size 100000
```

### 4. Generate Reports

```bash
# Generate HTML report
python scripts/generate_report.py --format html --output reports/

# Generate Markdown report for documentation
python scripts/generate_report.py --format markdown --output reports/

# Export raw data as CSV
python scripts/generate_report.py --format csv --output results/
```

## Configuration

### Environment Variables

```bash
# Dataset configuration
BENCHMARK_DATASET_SIZE=10000      # Number of documents to use
BENCHMARK_VECTOR_DIM=384          # Vector dimensions
BENCHMARK_GRAPH_NODES=10000       # Number of graph nodes

# Execution configuration
BENCHMARK_WARMUP_ITERATIONS=100   # Warmup iterations
BENCHMARK_ITERATIONS=1000         # Measurement iterations
BENCHMARK_THREADS=4               # Concurrent threads

# Resource limits
BENCHMARK_MEMORY_LIMIT=4g         # Memory limit per container
BENCHMARK_CPU_LIMIT=4             # CPU limit per container
```

### Database-Specific Configuration

Each database has its own configuration file in `config/`. These are tuned for fair comparison:

- Similar memory allocation
- Comparable thread/connection pools
- Equivalent persistence settings
- Normalized index configurations

## Metrics Collected

### Latency Metrics
- p50, p95, p99, p99.9 percentiles
- Mean and standard deviation
- Min and max values

### Throughput Metrics
- Operations per second
- Bytes per second (where applicable)
- Concurrent operation scaling

### Resource Metrics
- CPU utilization
- Memory usage
- Disk I/O
- Network I/O

### Quality Metrics (Vector Search)
- Recall@k
- Precision@k
- Index build time
- Index size

## Interpreting Results

### Fair Comparison Guidelines

1. **Apples to Apples**: Compare similar operation types
2. **Configuration Parity**: All databases use equivalent resources
3. **Warm Cache**: Results measured after warmup phase
4. **Statistical Significance**: Multiple iterations with confidence intervals
5. **Real-World Data**: Hugging Face datasets represent realistic workloads

### Expected Trade-offs

- **ThemisDB**: Balanced multi-model performance
- **PostgreSQL**: Excellent SQL/ACID, weaker at vectors
- **MongoDB**: Good document flexibility, moderate query performance
- **Redis**: Fastest simple operations, limited query capabilities
- **Milvus**: Best vector search, limited general querying
- **Neo4j**: Best graph traversals, limited other operations

## Contributing

To add a new database to the benchmark:

1. Create an adapter in `adapters/`
2. Add configuration in `config/`
3. Update `docker-compose.benchmark.yml`
4. Implement required benchmark interfaces
5. Add to documentation

## References

- [ThemisDB Performance Documentation](../../docs/performance/)
- [Google Benchmark Framework](https://github.com/google/benchmark)
- [Hugging Face Datasets](https://huggingface.co/docs/datasets/)
- [Database Benchmarking Best Practices](https://www.vldb.org/pvldb/)

## License

This benchmark suite is part of ThemisDB and follows the same MIT license.
