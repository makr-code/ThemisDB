---
category: "📋 Guides"
version: "v1.3.0"
status: "✅"
date: "December 22, 2025"
---

# 📋 Quick Start Guide

Get ThemisDB up and running in 5 minutes

## 📋 Table of Contents

- [✨ Features & Highlights](#-features--highlights)
- [🚀 Quick Start](#-quick-start)
- [📖 Installation & Setup](#-installation--setup)
- [💡 First Steps](#-first-steps)
- [🔧 Configuration](#-configuration)
- [🛠️ Troubleshooting](#-troubleshooting)
- [📚 See Also](#-see-also)
- [📝 Changelog](#-changelog)

---

## ✨ Features & Highlights

ThemisDB offers:
- 🐳 **Docker Support** - Fastest installation
- 📦 **Multi-Platform** - Windows, Linux, macOS
- ⚡ **Zero-Config** - Start in seconds
- 🔗 **REST API** - Easy integration
- 📊 **Multi-Model** - Relational, Document, Graph, Vector

---

## 🚀 Quick Start (5 Minutes)

### Prerequisites

- **Docker** (recommended) OR
- **Linux/macOS/Windows** with build tools

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

**Learn More:**
- [User Guide](USER_GUIDE.md) - Complete feature tour
- [Administrator Guide](ADMINISTRATOR_GUIDE.md) - Operations & deployment
- [AQL Documentation](../aql/README.md) - Query language reference
- [API Reference](../apis/README.md) - Complete API documentation

**Try Advanced Features:**
- Graph traversals with AQL
- Vector similarity search
- Multi-model transactions
- Real-time subscriptions

**Production Setup:**
- Configure authentication
- Set up monitoring
- Enable backups
- Tune performance

---

## Configuration

### Basic Configuration

Create `config.yaml`:

```yaml
server:
  host: "0.0.0.0"
  port: 8765
  
database:
  path: "/data/themisdb"
  cache_size_mb: 1024
  
logging:
  level: "info"
  file: "/var/log/themisdb/themisdb.log"
```

### Environment Variables

```bash
# Override config with environment variables
export THEMIS_SERVER_PORT=8765
export THEMIS_DB_PATH=/data/themisdb
export THEMIS_LOG_LEVEL=debug
```

---

## Troubleshooting

### Server Won't Start

**Check logs:**
```bash
# Docker
docker logs themisdb

# System service
journalctl -u themisdb -f

# Direct run
cat /var/log/themisdb/themisdb.log
```

### Connection Refused

**Verify server is listening:**
```bash
netstat -tulpn | grep 8765
# or
lsof -i :8765
```

**Check firewall:**
```bash
# Linux
sudo ufw status
sudo ufw allow 8765

# macOS
sudo pfctl -sr
```

### Performance Issues

**Check system resources:**
```bash
# Memory usage
docker stats themisdb

# Disk I/O
iostat -x 1
```

**Tune cache size:**
```yaml
database:
  cache_size_mb: 4096  # Increase for better performance
```

---

## See Also

### Documentation
- [User Guide](../../de/guides/USER_GUIDE.md) - Complete feature documentation
- [Administrator Guide](ADMINISTRATOR_GUIDE.md) - Operations guide
- [Power User Guide](../../de/guides/POWER_USER_GUIDE.md) - Advanced features
- [AQL Reference](../aql/README.md) - Query language

### Resources
- [GitHub Repository](https://github.com/makr-code/ThemisDB)
- [Issues & Support](https://github.com/makr-code/ThemisDB/issues)
- [Release Notes](https://github.com/makr-code/ThemisDB/releases)

---

## Changelog

### v1.3.0 - December 22, 2025
- ✅ Updated to new documentation template
- ✅ Added Docker quick start
- ✅ Added 10-step tutorial
- ✅ Added troubleshooting section

### v1.0.0 - December 5, 2025
- 🚀 Initial quick start guide
- 📖 Basic installation instructions
- 💡 First query examples

---

> **Note:** For the most detailed and up-to-date information, please refer to the [German quick start guide](../../de/guides/QUICK_START.md).

**Version:** 1.3.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
