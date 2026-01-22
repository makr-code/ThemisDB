# Frequently Asked Questions (FAQ)

## Table of Contents
- [General](#general)
- [Installation & Setup](#installation--setup)
- [Performance](#performance)
- [Features & Capabilities](#features--capabilities)
- [Development](#development)
- [Troubleshooting](#troubleshooting)
- [Licensing](#licensing)

---

## General

### What is ThemisDB?
ThemisDB is a high-performance multi-model database that combines relational, graph, vector, and document models in a single system with full ACID transaction support. It's built on RocksDB for reliability and includes optional native LLM integration.

### How does ThemisDB differ from other databases?
ThemisDB uniquely combines:
- Multiple data models (relational, graph, vector, document) in one unified system
- Full ACID transactions with MVCC
- Optional native LLM integration (llama.cpp)
- GPU-accelerated vector search
- Production-ready performance (45K writes/s, 120K reads/s)

### Which edition should I use?
- **Minimal Edition**: Embedded systems, IoT, edge devices (core database only)
- **Community Edition**: Development, startups, single-server deployments (full-featured)
- **Enterprise Edition**: Large-scale production with horizontal scaling, HA, and replication

### Is ThemisDB production-ready?
Yes! The Community Edition is production-ready for single-node deployments. The Enterprise Edition adds horizontal scaling and high availability for large-scale production environments.

---

## Installation & Setup

### What are the system requirements?
**Minimum:**
- CPU: 2 cores
- RAM: 4 GB
- Storage: 10 GB SSD
- OS: Linux (Ubuntu 20.04+), macOS (10.15+), Windows (10+)

**Recommended:**
- CPU: 8+ cores
- RAM: 16+ GB
- Storage: 100+ GB NVMe SSD
- OS: Linux (Ubuntu 22.04+)

### How do I install ThemisDB?
**Docker (Recommended):**
```bash
docker pull themisdb/themisdb:latest
docker run -d -p 8080:8080 -v themis_data:/data themisdb/themisdb:latest
```

**From Source:**
```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
./scripts/setup.sh
./scripts/build.sh
./build/themis_server --config config.yaml
```

See the [Quick Start](README.md#quick-start) guide for more options.

### What ports does ThemisDB use?
- `8080`: HTTP/REST API, GraphQL
- `18765`: Binary Wire Protocol, gRPC
- `4318`: OpenTelemetry/Prometheus metrics

See [docs/de/deployment/PORT_REFERENCE.md](docs/de/deployment/PORT_REFERENCE.md) for complete details.

### Can I run ThemisDB on ARM64/Apple Silicon?
Yes! ThemisDB supports ARM64 architecture including Apple Silicon (M1/M2/M3). Use the appropriate build configuration or Docker image for your platform.

### How do I upgrade ThemisDB?
1. Backup your data directory
2. Stop the current server
3. Install the new version
4. Start the server with the same data directory

ThemisDB handles data migration automatically when needed.

---

## Performance

### What performance can I expect?
Typical performance (release build, 20 cores @ 3.7 GHz):
- Entity PUT: 45,000 ops/s
- Entity GET: 120,000 ops/s
- Indexed Query: 3.4M queries/s
- Graph Traverse: 9.56M ops/s
- Vector Search: 59.7M queries/s

Actual performance varies based on hardware, data size, and workload complexity.

### How do I optimize performance?
1. Use SSDs (preferably NVMe) for storage
2. Allocate sufficient memory (recommend 16+ GB)
3. Enable appropriate indexes for your query patterns
4. Use GPU acceleration for vector search (if available)
5. Tune RocksDB settings in configuration

See [docs/de/performance/performance_memory.md](docs/de/performance/performance_memory.md) for detailed tuning.

### Does ThemisDB support GPU acceleration?
Yes! ThemisDB supports GPU acceleration for:
- Vector similarity search (CUDA-enabled GPUs)
- Some LLM operations (when LLM feature is enabled)

GPU support is optional and automatically detected at runtime.

### How much memory does ThemisDB need?
Memory requirements depend on:
- **Base system**: 2-4 GB
- **Data working set**: 1-2x your active dataset size for optimal performance
- **Index structures**: Varies by index type and data size
- **LLM (optional)**: 4-16 GB depending on model size

For production, we recommend 16+ GB RAM.

---

## Features & Capabilities

### What query languages does ThemisDB support?
ThemisDB uses **AQL (Advanced Query Language)**, which supports:
- SQL-like syntax for relational operations
- Graph traversal operations (BFS, Dijkstra, A*)
- Vector similarity search
- Document operations with JSON

See [docs/de/aql/aql_syntax.md](docs/de/aql/aql_syntax.md) for syntax reference.

### Does ThemisDB support transactions?
Yes! ThemisDB provides:
- Full ACID guarantees
- Snapshot isolation (MVCC)
- Write-write conflict detection
- Atomic updates across all index types

### Can I use multiple data models in the same transaction?
Yes! ThemisDB's unified storage engine allows you to combine relational, graph, vector, and document operations in a single ACID transaction.

### What vector search algorithms are supported?
ThemisDB supports:
- **HNSW (Hierarchical Navigable Small World)**: Fast approximate nearest neighbor search
- **FAISS**: GPU-accelerated similarity search
- Multiple distance metrics: Euclidean, Cosine, Inner Product

### Does ThemisDB support full-text search?
Yes! ThemisDB includes full-text search capabilities with support for:
- Tokenization and stemming
- Boolean operators
- Phrase matching
- Relevance scoring

### What security features are included?
ThemisDB includes:
- TLS 1.3 with mutual TLS (mTLS) support
- Role-Based Access Control (RBAC)
- Field-level encryption (AES-256-GCM)
- Audit logging with SIEM integration
- Input validation and SQL injection prevention

See [docs/de/security/security_implementation.md](docs/de/security/security_implementation.md) for details.

---

## Development

### What programming languages can I use with ThemisDB?
ThemisDB provides:
- **REST API**: Any language with HTTP support
- **GraphQL**: Any GraphQL client
- **gRPC**: Multiple language bindings
- **Native SDKs**: C++, Python, JavaScript, Go, Java

See [clients/README.md](clients/README.md) for SDK details.

### How do I contribute to ThemisDB?
We welcome contributions! Please:
1. Read the [Contributing Guide](CONTRIBUTING.md)
2. Check [Good First Issues](https://github.com/makr-code/ThemisDB/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
3. Follow our [Code of Conduct](CODE_OF_CONDUCT.md)
4. Submit a pull request

### Where can I find API documentation?
- **REST API**: [docs/api/API_REFERENCE.md](docs/api/API_REFERENCE.md)
- **GraphQL Schema**: Available at `/graphql` endpoint
- **gRPC**: Protocol buffers in `proto/` directory
- **Full Documentation**: [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)

### How do I run tests?
```bash
# Run all tests
cmake --build build --target test

# Run specific test suite
./build/tests/themis_tests --gtest_filter=VectorIndex*

# Run benchmarks
./build/benchmarks/themis_benchmarks
```

### What build systems are supported?
ThemisDB uses CMake as the primary build system with support for:
- Make (Unix/Linux)
- Ninja (all platforms)
- MSBuild (Windows)
- Xcode (macOS)

---

## Troubleshooting

### ThemisDB won't start. What should I check?
1. Check if ports 8080 or 18765 are already in use
2. Verify configuration file syntax (`config.yaml`)
3. Check disk space (need at least 1 GB free)
4. Review logs in `logs/` directory
5. Ensure proper file permissions on data directory

### How do I enable debug logging?
Edit `config.yaml`:
```yaml
logging:
  level: debug
  file: logs/themis.log
```

Or set environment variable:
```bash
export THEMIS_LOG_LEVEL=debug
```

### I'm getting "out of memory" errors. What should I do?
1. Increase system RAM or reduce memory limits in config
2. Reduce RocksDB block cache size
3. Disable LLM features if not needed
4. Use the Minimal Edition for lower memory usage
5. Check for memory leaks (enable memory profiling)

### Vector search is slow. How can I improve it?
1. Ensure proper index type (HNSW recommended)
2. Enable GPU acceleration if available
3. Tune HNSW parameters (M, ef_construction)
4. Consider reducing vector dimensions
5. Check if CPU is bottleneck (add more cores)

### How do I backup my data?
**Online backup (recommended):**
```bash
curl -X POST http://localhost:8080/admin/backup
```

**Offline backup:**
1. Stop ThemisDB server
2. Copy the entire data directory
3. Restart server

See [docs/de/guides/guides_deployment.md](docs/de/guides/guides_deployment.md#backup--recovery) for details.

### Where can I get help?
- **Documentation**: [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **GitHub Issues**: [Report bugs or request features](https://github.com/makr-code/ThemisDB/issues)
- **GitHub Discussions**: [Community Q&A](https://github.com/makr-code/ThemisDB/discussions)
- **Support Guide**: [SUPPORT.md](SUPPORT.md)

---

## Licensing

### What license is ThemisDB under?
- **Community Edition**: MIT License (free, open source)
- **Enterprise Edition**: Commercial license

See [LICENSE](LICENSE) for details.

### Can I use ThemisDB in commercial projects?
Yes! The Community Edition (MIT License) allows commercial use, modification, and distribution. The Enterprise Edition is available for large-scale deployments requiring additional features.

### What's the difference between Community and Enterprise editions?
| Feature | Community | Enterprise |
|---------|-----------|------------|
| Core Database | ✅ | ✅ |
| Single-Node | ✅ | ✅ |
| Horizontal Sharding | ❌ | ✅ |
| High Availability | ❌ | ✅ |
| Auto-Rebalancing | ❌ | ✅ |
| Multi-Region | ❌ | ✅ |
| Support SLA | ❌ | ✅ |

See [docs/reports/ENTERPRISE.md](docs/reports/ENTERPRISE.md) for Enterprise details.

### How do I get an Enterprise license?
Contact us at: sales@themisdb.com

---

## Additional Resources

- 📚 [Full Documentation](https://makr-code.github.io/ThemisDB/)
- 🚀 [Quick Start Guide](README.md#quick-start)
- 📖 [Examples & Tutorials](docs/EXAMPLES_INDEX.md)
- 🤝 [Contributing Guide](CONTRIBUTING.md)
- 🔒 [Security Policy](SECURITY.md)
- 💬 [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)

---

**Didn't find your answer?** [Ask a question in GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions/new?category=q-a)
