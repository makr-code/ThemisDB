# ThemisDB MINIMAL Edition

**Version:** 1.3.5+  
**Status:** Production Ready  
**License:** MIT (Open Source)

---

## 📋 Overview

ThemisDB MINIMAL Edition is the most lightweight variant of ThemisDB, designed for:
- **Embedded systems** with limited resources
- **Edge deployments** where size matters
- **Development environments** needing fast builds
- **Learning and experimentation** without complexity
- **Microservices** requiring only core database features

### What's Included

✅ **Core Database Features:**
- ACID transactions with MVCC (Multi-Version Concurrency Control)
- Multi-model storage (relational, graph, vector, document)
- Secondary indexes with automatic maintenance
- Basic query engine with AQL (Advanced Query Language)
- Time-series support
- REST API (HTTP/1.1)
- GraphQL API (basic)
- Basic backup and recovery

### What's NOT Included

❌ **Advanced Features (Not in MINIMAL):**
- LLM integration (llama.cpp)
- GPU acceleration (CUDA, Vulkan, HIP, etc.)
- Horizontal sharding
- Replication (leader-follower, multi-master)
- Advanced protocols (HTTP/2, WebSocket, gRPC, MQTT, PostgreSQL Wire, MCP)
- Voice assistant (STT/TTS)
- Content processors (audio, image, video, CAD)
- OpenTelemetry distributed tracing
- RBAC (Role-Based Access Control)
- Field-level encryption
- HSM (Hardware Security Module) support

---

## 🎯 Use Cases

### Perfect For:

1. **Embedded Systems**
   - Raspberry Pi, QNAP NAS, Synology NAS
   - IoT gateways with limited RAM
   - Edge devices in industrial settings

2. **Development & Testing**
   - Fast local development environment
   - CI/CD pipeline testing
   - Quick prototyping

3. **Microservices**
   - Single-purpose services needing only database
   - Sidecar database for stateful microservices
   - Containerized applications

4. **Learning & Education**
   - Database internals exploration
   - Multi-model database concepts
   - MVCC and transaction isolation

### Not Suitable For:

- AI/ML workloads requiring LLM integration → Use Community Edition
- GPU-accelerated vector search → Use Community Edition
- Distributed deployments → Use Enterprise Edition
- Real-time analytics with CEP → Use Enterprise Edition
- Applications requiring advanced protocols → Use Community Edition

---

## 📦 Installation

### Docker (Recommended)

```bash
# Pull from Docker Hub (when available)
docker pull themisdb/themisdb:minimal

# Or build from source
docker build -f Dockerfile.minimal -t themisdb:minimal .

# Run
docker run -d \
  --name themis-minimal \
  -p 8080:8080 \
  -v themis_data:/data \
  themisdb:minimal

# Verify
curl http://localhost:8080/health
```

### From Source (Linux/WSL)

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Build MINIMAL edition (fast: ~5-10 minutes)
./scripts/build-minimal.sh

# Run server
./build-minimal/themis_server --config config/config-minimal.yaml
```

### Binary Size Comparison

| Edition | Binary Size | Build Time | RAM Usage (Idle) |
|---------|------------|------------|------------------|
| **MINIMAL** | ~30-50 MB | ~5-10 min | ~100-200 MB |
| **COMMUNITY** | ~80-150 MB | ~20-30 min | ~300-500 MB |
| **ENTERPRISE** | ~150-250 MB | ~30-40 min | ~500-800 MB |

*(Sizes are approximate and vary by platform)*

---

## 🚀 Quick Start

### 1. Start Server

```bash
# Using Docker
docker run -d -p 8080:8080 -v themis_data:/data themisdb:minimal

# Or from source
./build-minimal/themis_server --config config/config-minimal.yaml
```

### 2. Check Health

```bash
curl http://localhost:8080/health
```

Expected response:
```json
{
  "status": "healthy",
  "edition": "MINIMAL",
  "version": "1.3.5",
  "uptime_seconds": 10
}
```

### 3. Create Your First Entity

```bash
curl -X PUT http://localhost:8080/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}"}'
```

### 4. Create an Index

```bash
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"city"}'
```

### 5. Query Data

```bash
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [{"column": "city", "value": "Berlin"}],
    "return": "entities"
  }'
```

---

## 🔧 Configuration

The minimal edition uses a simplified configuration file: `config/config-minimal.yaml`

Key settings:

```yaml
storage:
  rocksdb_path: "./data/rocksdb"
  memtable_size_mb: 64      # Reduced for minimal footprint
  block_cache_size_mb: 256  # Reduced for minimal footprint

network:
  http_port: 8080           # HTTP/1.1 only

performance:
  worker_threads: 4         # Reduced for minimal footprint
```

See [`config/config-minimal.yaml`](../config/config-minimal.yaml) for complete reference.

---

## 📊 Performance Characteristics

### Benchmarks (Minimal Edition on Standard Hardware)

| Operation | Throughput | Latency | Notes |
|-----------|------------|---------|-------|
| Entity PUT | ~20,000 ops/s | 0.05 ms | Write throughput |
| Entity GET | ~60,000 ops/s | 0.015 ms | Read throughput |
| Indexed Query | ~500k queries/s | 2 μs | AQL WHERE clause |
| Graph Traverse | ~2M ops/s | 0.5 μs | BFS (depth=3) |
| Vector Search (CPU) | ~100k queries/s | 10 μs | 384D embeddings |

*Hardware: 4-core CPU, 8 GB RAM, SSD storage*

### Resource Usage

- **RAM (Idle):** ~100-200 MB
- **RAM (Active):** ~300-500 MB under load
- **Disk (Installation):** ~50-80 MB
- **CPU (Idle):** <1%
- **CPU (Active):** Scales with workload

---

## 🔄 Upgrade Path

Need more features? Upgrade to a higher edition:

### To Community Edition
```bash
# Rebuild with Community edition
cmake -DTHEMIS_EDITION=COMMUNITY ...
```

**Adds:**
- LLM integration (optional)
- GPU acceleration (optional)
- Advanced protocols (HTTP/2, WebSocket, gRPC)
- Content processors
- OpenTelemetry tracing

### To Enterprise Edition
```bash
# Contact sales@themisdb.com for license
cmake -DTHEMIS_EDITION=ENTERPRISE ...
```

**Adds (on top of Community):**
- Horizontal sharding
- Replication (leader-follower, multi-master)
- RBAC and advanced security
- OLAP and CEP
- 24/7 support

---

## 🛠️ Build Options

### CMake Configuration

```bash
cmake -S . -B build-minimal \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_ENABLE_GPU=OFF \
  -DTHEMIS_ENABLE_GRPC=OFF \
  -DTHEMIS_ENABLE_HTTP2=OFF \
  -DTHEMIS_ENABLE_WEBSOCKET=OFF \
  -DTHEMIS_ENABLE_CONTENT_PROCESSORS=OFF \
  -DTHEMIS_ENABLE_TRACING=OFF \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF
```

All minimal-specific options are automatically set when `THEMIS_EDITION=MINIMAL`.

### Custom Minimal Build

You can customize further:

```bash
# Enable tests for development
cmake -DTHEMIS_EDITION=MINIMAL -DTHEMIS_BUILD_TESTS=ON ...

# Enable tracing for debugging
cmake -DTHEMIS_EDITION=MINIMAL -DTHEMIS_ENABLE_TRACING=ON ...
```

---

## 📚 API Reference

MINIMAL edition supports a subset of the full ThemisDB API:

### Supported Endpoints

- ✅ `GET /health` - Health check
- ✅ `GET /metrics` - Basic Prometheus metrics
- ✅ `PUT /entities/{key}` - Create/update entity
- ✅ `GET /entities/{key}` - Retrieve entity
- ✅ `DELETE /entities/{key}` - Delete entity
- ✅ `POST /query` - Query with AQL
- ✅ `POST /index/create` - Create index
- ✅ `POST /index/drop` - Drop index
- ✅ `POST /transaction/begin` - Begin transaction
- ✅ `POST /transaction/commit` - Commit transaction
- ✅ `POST /transaction/rollback` - Rollback transaction
- ✅ `POST /graphql` - GraphQL endpoint (basic)

### NOT Supported in MINIMAL

- ❌ LLM endpoints (`/llm/*`)
- ❌ Sharding endpoints (`/shard/*`)
- ❌ Replication endpoints (`/replication/*`)
- ❌ Advanced protocol endpoints
- ❌ Content processing endpoints

See [API Documentation](../docs/api/api_reference.md) for details.

---

## 🔒 Security

MINIMAL edition includes basic security features:

- ✅ TLS/SSL support (optional)
- ✅ Basic authentication (optional)
- ✅ Audit logging
- ❌ RBAC (use Community+ for RBAC)
- ❌ Field-level encryption (use Enterprise)
- ❌ HSM integration (use Enterprise)

For production deployments, enable TLS:

```yaml
security:
  enable_tls: true
  tls_cert: "/path/to/cert.pem"
  tls_key: "/path/to/key.pem"
```

---

## 🐛 Troubleshooting

### Build Issues

**Problem:** CMake can't find vcpkg  
**Solution:**
```bash
export VCPKG_ROOT=$HOME/vcpkg
./scripts/build-minimal.sh
```

**Problem:** Out of memory during build  
**Solution:**
```bash
NUM_JOBS=2 ./scripts/build-minimal.sh  # Reduce parallel jobs
```

### Runtime Issues

**Problem:** Server won't start  
**Solution:**
```bash
# Check configuration
./build-minimal/themis_server --config config/config-minimal.yaml --validate

# Check logs
./build-minimal/themis_server --config config/config-minimal.yaml --log-level debug
```

**Problem:** Port 8080 already in use  
**Solution:**
```yaml
# Edit config/config-minimal.yaml
network:
  http_port: 8081  # Use different port
```

---

## 📖 Additional Resources

- [ThemisDB Documentation](https://makr-code.github.io/ThemisDB/)
- [Architecture Overview](../docs/architecture/ARCHITECTURE_OVERVIEW.md)
- [API Reference](../docs/api/api_reference.md)
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [Contributing Guide](../CONTRIBUTING.md)

---

## 📄 License

ThemisDB MINIMAL Edition is released under the **MIT License**.

- ✅ Free to use, modify, and distribute
- ✅ Commercial use allowed
- ✅ No attribution required (but appreciated!)

See [LICENSE](../LICENSE) for details.

---

## 🙏 Acknowledgments

MINIMAL edition builds upon the same core technologies as full ThemisDB:

- **RocksDB** - LSM-Tree storage engine
- **nlohmann/json** - JSON parsing
- **OpenSSL** - TLS/SSL support
- **vcpkg** - Package management

See [ATTRIBUTIONS.md](../ATTRIBUTIONS.md) for complete list.

---

**Built with ❤️ for embedded and edge computing**

[⭐ Star us on GitHub](https://github.com/makr-code/ThemisDB) · [🐛 Report Issues](https://github.com/makr-code/ThemisDB/issues) · [📖 Read the Docs](https://makr-code.github.io/ThemisDB/)
