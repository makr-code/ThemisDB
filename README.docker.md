# ThemisDB - Hybrid Vector-Geo Database

![Docker Pulls](https://img.shields.io/docker/pulls/themisdb/themisdb)
![Docker Image Size](https://img.shields.io/docker/image-size/themisdb/themisdb/latest)
![License](https://img.shields.io/github/license/makr-code/ThemisDB)

ThemisDB is a high-performance hybrid database combining **vector search**, **geospatial queries**, and **traditional CRUD operations** in a single system. Built with C++20, RocksDB, and modern AI/ML integrations.

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

| Tag | Description | Use Case |
|-----|-------------|----------|
| `latest` | Latest stable release | Production deployments |
| `1.0.0` | Specific version v1.0.0 | Version pinning |
| `1.0` | Latest patch in 1.0.x | Minor version tracking |
| `qnap` | QNAP NAS optimized | QNAP Container Station |
| `1.0.0-qnap` | QNAP-specific v1.0.0 | QNAP version pinning |

## 🎯 Key Features

### Vector Search
- **HNSW Index** - Fast approximate nearest neighbor search
- **Euclidean, Cosine, IP** - Multiple distance metrics
- **SIMD Optimized** - Hardware-accelerated computations
- **Metadata Filtering** - Pre-filter vectors by attributes

### Geospatial
- **Point, Polygon, LineString** - Full GeoJSON support
- **Spatial Indexing** - R-tree for efficient queries
- **Distance Queries** - Within radius, bounding box
- **Geo + Vector** - Hybrid search capabilities

### CRUD & Hybrid
- **Key-Value Store** - Fast document storage (RocksDB)
- **AQL Queries** - Advanced Query Language with filters
- **MVCC** - Multi-version concurrency control
- **ACID Transactions** - Full data consistency

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

### Store a Document
```bash
curl -X POST http://localhost:8765/api/documents \
  -H "Content-Type: application/json" \
  -d '{
    "id": "doc1",
    "content": "Machine learning advances",
    "metadata": {"category": "AI"}
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

### Hybrid Search (Vector + Geo)
```bash
curl -X POST http://localhost:8765/api/search/hybrid \
  -H "Content-Type: application/json" \
  -d '{
    "query_vector": [0.1, 0.2, ...],
    "center": {"lat": 52.52, "lon": 13.405},
    "radius_km": 10.0,
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
┌─────────────────────────────────────────┐
│         HTTP Server (Port 8765)         │
├─────────────────────────────────────────┤
│  REST API │ GraphQL │ WebSocket │ gRPC  │
├─────────────────────────────────────────┤
│  Vector Engine │ Geo Engine │ AQL      │
│  (HNSW Index)  │ (R-tree)   │ (Parser) │
├─────────────────────────────────────────┤
│     Storage Layer (RocksDB + MVCC)      │
├─────────────────────────────────────────┤
│  Encryption │ PII │ Audit │ Compression │
└─────────────────────────────────────────┘
```

## 🌍 Use Cases

- **Semantic Search** - Document retrieval with embeddings
- **Location-based Services** - Geo + vector hybrid queries
- **RAG Systems** - Retrieval-Augmented Generation backends
- **Recommendation Engines** - Content similarity search
- **Fraud Detection** - Anomaly detection with vectors
- **IoT Data** - Geospatial time-series analysis

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

## 📚 Documentation

- **GitHub Repository**: https://github.com/makr-code/ThemisDB
- **API Documentation**: https://github.com/makr-code/ThemisDB/tree/main/docs/apis
- **Configuration Guide**: https://github.com/makr-code/ThemisDB/blob/main/docs/guides/configuration.md
- **Deployment Guide**: https://github.com/makr-code/ThemisDB/blob/main/docs/deployment/deployment_strategy.md
- **Release Notes**: https://github.com/makr-code/ThemisDB/releases

## 🐛 Issues & Support

- **Report Bugs**: https://github.com/makr-code/ThemisDB/issues
- **Feature Requests**: https://github.com/makr-code/ThemisDB/discussions
- **Security Issues**: security@themisdb.org

## 📝 License

ThemisDB is licensed under the MIT License. See [LICENSE](https://github.com/makr-code/ThemisDB/blob/main/LICENSE) for details.

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](https://github.com/makr-code/ThemisDB/blob/main/CONTRIBUTING.md) for guidelines.

---

**Need help?** Check the [documentation](https://github.com/makr-code/ThemisDB) or [open an issue](https://github.com/makr-code/ThemisDB/issues).
