# ThemisDB - Multi-Model Database System

![Docker Pulls](https://img.shields.io/docker/pulls/themisdb/themisdb)
![Docker Image Size](https://img.shields.io/docker/image-size/themisdb/themisdb/latest)
![License](https://img.shields.io/github/license/makr-code/ThemisDB)

ThemisDB is a high-performance **multi-model database** combining **Graph**, **Relational**, **Vector**, **Document**, **Geospatial**, and **Time-Series** capabilities in a unified system. Built with C++20 on a LSM-tree foundation (RocksDB) with enterprise-grade security and AI/ML integration.

## 🚀 Quick Start

```bash
# Pull and run ThemisDB
docker pull themisdb/themisdb:latest
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -v themisdb-data:/var/lib/themisdb/data \
  themisdb/themisdb:latest

# Access the API
curl http://localhost:8765/health
```

## 📦 Available Tags

### Multi-Architecture Images

| Tag | Description | Architectures | Use Case |
|-----|-------------|---------------|----------|
| `latest` | Latest stable release | amd64, arm64 | Production deployments |
| `1.0.0` | Specific version v1.0.0 | amd64, arm64 | Version pinning |
| `1.0` | Latest patch in 1.0.x | amd64, arm64 | Minor version tracking |

### Platform-Specific Images

| Tag | Description | Architecture | Use Case |
|-----|-------------|--------------|----------|
| `qnap` | QNAP NAS optimized (Ubuntu 20.04) | amd64 | QNAP Container Station |
| `1.0.0-qnap` | QNAP-specific v1.0.0 | amd64 | QNAP version pinning |
| `rpi` | Raspberry Pi optimized | arm64 | Raspberry Pi 4/5 |
| `1.0.0-rpi` | Raspberry Pi v1.0.0 | arm64 | Raspberry Pi version pinning |

## 🎯 Key Features

### Multi-Model Architecture
- **Graph Model** - Vertices, edges, property graphs with Cypher-like queries
- **Relational Model** - Tables, schemas, SQL-style queries, JOINs
- **Vector Model** - HNSW index for semantic search with SIMD optimization
- **Document Model** - JSON documents with flexible schemas (RocksDB)
- **Geospatial** - GeoJSON, R-tree indexing, spatial queries
- **Time-Series** - Temporal data with retention policies

### Graph Database
- **Property Graphs** - Vertices and edges with rich attributes
- **Pattern Matching** - Cypher-like graph traversal queries
- **Relationship Navigation** - Multi-hop queries and path finding
- **Graph Analytics** - PageRank, community detection, centrality

### Vector Search
- **HNSW Index** - Fast approximate nearest neighbor search
- **Multiple Metrics** - Euclidean, Cosine, Inner Product
- **SIMD Optimized** - Hardware-accelerated distance computations
- **Hybrid Queries** - Combine vector search with filters and geo

### Relational Capabilities
- **Schema Support** - Typed columns with constraints
- **SQL-like Queries** - AQL (Advanced Query Language)
- **Transactions** - ACID compliance with MVCC
- **Indexes** - B-tree and hash indexes for fast lookups

### Security & Compliance
- **Encryption at Rest** - AES-256-GCM
- **Encryption in Transit** - TLS 1.3
- **PII Detection** - Automatic sensitive data masking
- **Audit Logging** - Comprehensive activity tracking
- **Content Policies** - DLP and governance

### AI/ML Integration
- **OpenAI API** - Direct embedding generation
- **Cohere Support** - Multi-model compatibility
- **Hot Reload** - Zero-downtime ML model updates
- **Semantic Cache** - Reduce API costs

## 🐳 Docker Compose

```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb/themisdb:latest
    container_name: themisdb
    ports:
      - "8765:8765"
    volumes:
      - themisdb-data:/var/lib/themisdb/data
      - ./config:/etc/themisdb/config:ro
    environment:
      - THEMIS_LOG_LEVEL=info
      - THEMIS_ENABLE_METRICS=true
      - THEMIS_UPDATE_CHECK_ENABLED=true
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8765/health"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 40s

volumes:
  themisdb-data:
```

## ⚙️ Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_LOG_LEVEL` | `info` | Logging level (debug, info, warn, error) |
| `THEMIS_DATA_DIR` | `/var/lib/themisdb/data` | Data directory path |
| `THEMIS_PORT` | `8765` | HTTP server port |
| `THEMIS_ENABLE_METRICS` | `false` | Enable Prometheus metrics |
| `THEMIS_UPDATE_CHECK_ENABLED` | `true` | Enable version update checks |
| `THEMIS_GITHUB_API_TOKEN` | - | GitHub token for update checks |

### Volume Mounts

```bash
# Data directory (persistent storage)
-v themisdb-data:/var/lib/themisdb/data

# Configuration files
-v ./config:/etc/themisdb/config:ro

# Logs (optional)
-v ./logs:/var/log/themisdb
```

## 🔌 API Examples

### Graph Queries
```bash
# Create vertex
curl -X POST http://localhost:8765/api/graph/vertices \
  -H "Content-Type: application/json" \
  -d '{"id": "person:alice", "properties": {"name": "Alice", "age": 30}}'

# Create edge
curl -X POST http://localhost:8765/api/graph/edges \
  -H "Content-Type: application/json" \
  -d '{"from": "person:alice", "to": "person:bob", "type": "knows"}'

# Graph traversal
curl -X POST http://localhost:8765/api/graph/query \
  -H "Content-Type: application/json" \
  -d '{"start": "person:alice", "pattern": "()-[:knows*1..3]->()"}'
```

### Relational Queries
```bash
# Query with AQL
curl -X POST http://localhost:8765/api/query \
  -H "Content-Type: application/json" \
  -d '{
    "query": "SELECT name, age FROM users WHERE age > 25 ORDER BY age"
  }'
```

### Vector Search
```bash
curl -X POST http://localhost:8765/api/search/vector \
  -H "Content-Type: application/json" \
  -d '{
    "query_vector": [0.1, 0.2, 0.3, ...],
    "top_k": 10,
    "filter": {"category": "AI"}
  }'
```

### Geospatial Query
```bash
curl -X POST http://localhost:8765/api/search/geo \
  -H "Content-Type: application/json" \
  -d '{
    "center": {"lat": 52.52, "lon": 13.405},
    "radius_km": 5.0,
    "limit": 20
  }'
```

### Multi-Model Hybrid Query
```bash
# Graph + Vector + Geo combined
curl -X POST http://localhost:8765/api/search/hybrid \
  -H "Content-Type: application/json" \
  -d '{
    "graph_pattern": "()-[:located_in]->(city)",
    "query_vector": [0.1, 0.2, ...],
    "geo_filter": {"center": {"lat": 52.52, "lon": 13.405}, "radius_km": 10},
    "top_k": 5
  }'
```

## 📊 Monitoring

### Health Check
```bash
curl http://localhost:8765/health
```

### Metrics (Prometheus)
```bash
curl http://localhost:8765/metrics
```

### Update Check
```bash
curl http://localhost:8765/api/updates
```

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────┐
│           HTTP Server (Port 8765)                       │
├─────────────────────────────────────────────────────────┤
│  REST API │ GraphQL │ WebSocket │ gRPC │ Cypher Query  │
├─────────────────────────────────────────────────────────┤
│ Multi-Model Query Engine                               │
│ ┌──────────┬──────────┬─────────┬──────────┬─────────┐ │
│ │  Graph   │Relational│ Vector  │   Geo    │TimeSeries│ │
│ │  Engine  │  Engine  │ Engine  │  Engine  │  Engine  │ │
│ │(Property)│  (AQL)   │ (HNSW)  │ (R-tree) │(Retention)│ │
│ └──────────┴──────────┴─────────┴──────────┴─────────┘ │
├─────────────────────────────────────────────────────────┤
│         Unified Storage Layer (RocksDB + MVCC)          │
│         LSM-Tree with Column Families                   │
├─────────────────────────────────────────────────────────┤
│ Security & Compliance Layer                             │
│ Encryption │ PII Detection │ Audit Log │ Compression   │
│ (AES-256)  │ (Regex + ML)  │ (Append)  │ (Zstd/LZ4)   │
└─────────────────────────────────────────────────────────┘
```

## 🌍 Use Cases

### Enterprise Applications
- **Knowledge Graphs** - Organizational data with relationships and context
- **Master Data Management** - Unified view across relational and graph data
- **Fraud Detection** - Graph pattern matching + vector anomaly detection
- **Customer 360** - Multi-model customer profiles (graph + relational + time-series)

### AI/ML Applications
- **RAG Systems** - Retrieval-Augmented Generation with vector + graph context
- **Semantic Search** - Document retrieval with embeddings and knowledge graphs
- **Recommendation Engines** - Graph-based recommendations with vector similarity
- **Content Discovery** - Multi-modal search (text, images, relationships)

### Geospatial & IoT
- **Location Intelligence** - Geo + graph + time-series analysis
- **Smart Cities** - Infrastructure monitoring with spatial and temporal data
- **Fleet Management** - Real-time tracking with route optimization
- **IoT Analytics** - Sensor networks with geospatial context

## 🔧 QNAP Deployment

For QNAP NAS users, use the optimized image:

```bash
# Pull QNAP-optimized image
docker pull themisdb/themisdb:qnap

# Run on QNAP Container Station
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -v /share/Container/themisdb:/var/lib/themisdb/data \
  themisdb/themisdb:qnap
```

## 🍓 Raspberry Pi Deployment

For Raspberry Pi 4/5 users (64-bit OS required):

```bash
# Pull Raspberry Pi-optimized image
docker pull themisdb/themisdb:rpi

# Run on Raspberry Pi
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -v themisdb-data:/var/lib/themisdb/data \
  themisdb/themisdb:rpi
```

**System Requirements:**
- Raspberry Pi 4 or 5 (4GB+ RAM recommended)
- Raspberry Pi OS 64-bit (Bullseye or Bookworm)
- Docker installed (`curl -sSL https://get.docker.com | sh`)

## 🏗️ Multi-Architecture Support

ThemisDB Docker images support multiple architectures:

| Architecture | Supported Images | Typical Hardware |
|--------------|------------------|------------------|
| `amd64` | `latest`, `qnap` | x86_64 servers, desktops, QNAP NAS |
| `arm64` | `latest`, `rpi` | Raspberry Pi 4/5, ARM servers, Apple Silicon |

The `latest` tag automatically selects the correct architecture for your system.

## 🔨 Local Docker Build (Docker Desktop)

Build Docker images locally using the hybrid pre-built binary approach:

### PowerShell (Windows)

```powershell
# Build Docker image with existing binary
.\docker-build.ps1

# Build specific variant
.\docker-build.ps1 -Variant qnap      # QNAP only
.\docker-build.ps1 -Variant standard  # Standard

# Build with specific version
.\docker-build.ps1 -Version 1.0.1

# Build and push to registry
.\docker-build.ps1 -Push
```

### Bash (Linux/macOS)

```bash
# Build Docker image with existing binary
./docker-build.sh

# Build specific variant
./docker-build.sh -b qnap      # QNAP only
./docker-build.sh -b standard  # Standard

# Build with specific version
./docker-build.sh -v 1.0.1

# Build and push to registry
./docker-build.sh --push
```

### Build Variants

| Variant | Dockerfile | Base | Use Case |
|---------|------------|------|----------|
| `standard` | `Dockerfile.simple` | Ubuntu 24.04 | General use, Servers, Desktop |
| `qnap` | `Dockerfile.simple` | Ubuntu 24.04 | QNAP NAS (monolithic binary) |

## 📚 Documentation

- **GitHub Repository**: https://github.com/makr-code/ThemisDB
- **API Documentation**: https://github.com/makr-code/ThemisDB/tree/main/docs/apis
- **Configuration Guide**: https://github.com/makr-code/ThemisDB/blob/main/docs/guides/configuration.md
- **Deployment Guide**: https://github.com/makr-code/ThemisDB/blob/main/docs/deployment/deployment_strategy.md
- **Release Notes**: https://github.com/makr-code/ThemisDB/releases

## 🐛 Issues & Support

- **Report Bugs**: https://github.com/makr-code/ThemisDB/issues
- **Feature Requests**: https://github.com/makr-code/ThemisDB/discussions
- **Security Issues**: ma.krueger@outlook.com

## 📝 License

ThemisDB is licensed under the MIT License. See [LICENSE](https://github.com/makr-code/ThemisDB/blob/main/LICENSE) for details.

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](https://github.com/makr-code/ThemisDB/blob/main/CONTRIBUTING.md) for guidelines.

---

**Need help?** Check the [documentation](https://github.com/makr-code/ThemisDB) or [open an issue](https://github.com/makr-code/ThemisDB/issues).
