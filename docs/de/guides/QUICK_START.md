---
category: "📋 Guides"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 📋 Quick Start Guide

Get ThemisDB up and running in 5 minutes

## 📋 Inhaltsverzeichnis

- [✨ Features & Highlights](#-features--highlights)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Installation & Setup](#-installation--setup)
- [💡 Erste Schritte](#-erste-schritte)
- [🔧 Konfiguration](#-konfiguration)
- [🛠️ Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## ✨ Features & Highlights

ThemisDB bietet:
- 🐳 **Docker-Support** - Schnellste Installation
- 📦 **Multi-Plattform** - Windows, Linux, macOS
- ⚡ **Zero-Config** - Start in seconds
- 🔗 **REST API** - Einfache Integration
- 📊 **Multi-Model** - Relational, Document, Graph, Vector

---

## 🚀 Schnellstart (5 Minuten)

### Voraussetzungen

- **Docker** (empfohlen) ODER
- **Linux/macOS/Windows** mit Build-Tools

### Option 1: Docker (Recommended)

**Fastest way to get started:**

```bash
# Pull the latest image
docker pull themisdb/themisdb:latest

# Run ThemisDB
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -p 8080:8080 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Check if it's running
curl http://localhost:8765/health
```

**Expected response:**
```json
{"status":"ok","uptime":5}
```

### Option 2: Pre-built Packages

**Debian/Ubuntu:**
```bash
wget https://github.com/makr-code/ThemisDB/releases/latest/download/themisdb_1.2.0-1_amd64.deb
sudo apt install ./themisdb_1.2.0-1_amd64.deb
sudo systemctl start themisdb
```

**macOS (Homebrew):**
```bash
brew install themisdb
brew services start themisdb
```

**Windows (Chocolatey):**
```powershell
choco install themisdb
```

### Option 3: Build from Source

**Clone and build:**
```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Linux/macOS
./scripts/setup.sh
./scripts/build.sh

# Windows
.\scripts\setup.ps1
.\build.ps1

# Start server
./build/themis_server --config config.yaml
```

---

## Your First Queries

### 1. Check Server Health

```bash
curl http://localhost:8765/health
```

### 2. Create Your First Entity

```bash
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\",\"role\":\"developer\"}"
  }'
```

**Response:**
```json
{"status":"success","key":"users:alice"}
```

### 3. Read the Entity

```bash
curl http://localhost:8765/entities/users:alice
```

**Response:**
```json
{
  "key": "users:alice",
  "blob": "{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\",\"role\":\"developer\"}"
}
```

### 4. Create More Entities

```bash
# Create Bob
curl -X PUT http://localhost:8765/entities/users:bob \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Bob\",\"age\":35,\"city\":\"Berlin\",\"role\":\"manager\"}"}'

# Create Charlie
curl -X PUT http://localhost:8765/entities/users:charlie \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Charlie\",\"age\":28,\"city\":\"Munich\",\"role\":\"developer\"}"}'

# Create David
curl -X PUT http://localhost:8765/entities/users:david \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"David\",\"age\":32,\"city\":\"Hamburg\",\"role\":\"designer\"}"}'
```

### 5. Create an Index for Queries

```bash
curl -X POST http://localhost:8765/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"city"}'
```

**Response:**
```json
{"status":"success","index":"users.city"}
```

### 6. Query by Index

**Find all users in Berlin:**
```bash
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [{"column": "city", "value": "Berlin"}],
    "return": "entities"
  }'
```

**Response:**
```json
{
  "table": "users",
  "count": 2,
  "entities": [
    "{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\",\"role\":\"developer\"}",
    "{\"name\":\"Bob\",\"age\":35,\"city\":\"Berlin\",\"role\":\"manager\"}"
  ]
}
```

### 7. Create a Range Index

```bash
curl -X POST http://localhost:8765/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"age","type":"range"}'
```

### 8. Range Query with Sorting

**Find users aged 28-33, sorted by age:**
```bash
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "range": [{"column": "age", "gte": "28", "lte": "33"}],
    "order_by": {"column": "age", "desc": false},
    "return": "entities"
  }'
```

### 9. Using AQL (Advanced Query Language)

**AQL provides SQL-like syntax with graph and vector support:**

```bash
curl -X POST http://localhost:8765/query/aql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR u IN users FILTER u.city == \"Berlin\" AND u.age >= 30 RETURN u"
  }'
```

### 10. View Server Metrics

```bash
# JSON statistics
curl http://localhost:8765/stats

# Prometheus metrics
curl http://localhost:8765/metrics
```

---

## Next Steps

### Working with Graphs

**Create edges (relationships):**

```bash
# Alice knows Bob
curl -X PUT http://localhost:8765/entities/edge:e1 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"id\":\"e1\",\"_from\":\"users:alice\",\"_to\":\"users:bob\",\"type\":\"knows\"}"
  }'

# Bob knows Charlie
curl -X PUT http://localhost:8765/entities/edge:e2 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"id\":\"e2\",\"_from\":\"users:bob\",\"_to\":\"users:charlie\",\"type\":\"knows\"}"
  }'

# Graph traversal (BFS)
curl -X POST http://localhost:8765/graph/traverse \
  -H "Content-Type: application/json" \
  -d '{"start_vertex":"users:alice","max_depth":2}'
```

### Working with Vectors

**Create vector index:**
```bash
curl -X POST http://localhost:8765/vector/create-index \
  -H "Content-Type: application/json" \
  -d '{
    "table": "documents",
    "dimension": 128,
    "metric": "l2"
  }'
```

**Add vector embeddings:**
```bash
curl -X PUT http://localhost:8765/entities/documents:doc1 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"title\":\"Introduction\",\"embedding\":[0.1,0.2,...]}"
  }'
```

**Vector similarity search:**
```bash
curl -X POST http://localhost:8765/vector/search \
  -H "Content-Type: application/json" \
  -d '{
    "table": "documents",
    "vector": [0.1, 0.2, ...],
    "k": 10
  }'
```

---

## Common Tasks

### Update an Entity

```bash
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":31,\"city\":\"Berlin\",\"role\":\"senior developer\"}"}'
```

### Delete an Entity

```bash
curl -X DELETE http://localhost:8765/entities/users:alice
```

### Batch Operations

```bash
curl -X POST http://localhost:8765/entities/batch \
  -H "Content-Type: application/json" \
  -d '{
    "operations": [
      {"op":"PUT","key":"users:eve","blob":"{\"name\":\"Eve\"}"},
      {"op":"PUT","key":"users:frank","blob":"{\"name\":\"Frank\"}"},
      {"op":"DELETE","key":"users:old_user"}
    ]
  }'
```

### Backup Database

```bash
curl -X POST http://localhost:8765/admin/backup \
  -H "Content-Type: application/json" \
  -d '{"path":"/backups/backup-2025-12-15"}'
```

---

## Configuration

**Basic config.yaml:**

```yaml
storage:
  rocksdb_path: ./data/themis_server
  memtable_size_mb: 256
  block_cache_size_mb: 1024

server:
  host: 0.0.0.0
  port: 8765
  worker_threads: 8

vector_index:
  engine: hnsw
  hnsw_m: 16
  hnsw_ef_construction: 200
```

**See:** [Configuration Guide](../operations/CONFIGURATION.md)

---

## Troubleshooting

### Server won't start

**Check logs:**
```bash
# Docker
docker logs themisdb

# Package installation
sudo journalctl -u themisdb

# From source
./build/themis_server --log-level debug
```

### Connection refused

**Check if server is running:**
```bash
# Health check
curl http://localhost:8765/health

# Process check
ps aux | grep themis_server

# Docker check
docker ps | grep themisdb
```

### Port already in use

**Change port in config.yaml:**
```yaml
server:
  port: 8766  # Use different port
```

### Database not open error

**Check data directory permissions:**
```bash
# Linux/macOS
chmod 755 ./data
chown $USER:$USER ./data

# Docker - use volumes
docker run -v themis_data:/data themisdb/themisdb:latest
```

---

## Learn More

- **[Installation Guide](INSTALLATION.md)** - Detailed installation instructions
- **[Configuration Guide](../operations/CONFIGURATION.md)** - All configuration options
- **[AQL Documentation](../aql/aql_syntax.md)** - Learn the query language
- **[REST API Reference](../apis/REST_API.md)** - Complete API documentation
- **[Client SDKs](../../clients/README.md)** - Use ThemisDB from your favorite language

---

## Getting Help

- **Documentation:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **GitHub Issues:** [Report a bug](https://github.com/makr-code/ThemisDB/issues)
- **Discussions:** [Ask questions](https://github.com/makr-code/ThemisDB/discussions)
- **Security:** [Security policy](../../SECURITY.md)

---

**Congratulations!** You've completed the quick start guide. You now know how to:
- ✅ Install and run ThemisDB
- ✅ Create, read, update, and delete entities
- ✅ Create indexes and query data
- ✅ Work with graphs and vectors
- ✅ Monitor your database

**Next:** Explore the [full documentation](../INDEX.md) to learn about advanced features like transactions, sharding, replication, and GPU acceleration.
