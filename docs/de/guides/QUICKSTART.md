# ThemisDB Quick Start Guide

Get ThemisDB up and running in 5 minutes! 🚀

## ⚠️ SECURITY WARNING - HSM Configuration

**CRITICAL:** ThemisDB defaults to a **stub HSM provider** for development convenience. This is **INSECURE** and must be changed for production deployments.

### What's at Risk?
- Master encryption keys are stored **without hardware protection**
- **All encrypted data can be compromised** if an attacker gains system access
- **Compliance violations**: NIST SP 800-53, ISO 27001, PCI DSS, GDPR

### For Production Deployments:

1. **Configure a real HSM provider** before production use:
   - PKCS#11 HSM (Thales Luna, AWS CloudHSM, etc.)
   - AWS KMS
   - Azure Key Vault
   - GCP Cloud KMS

2. **See the complete setup guide**: [docs/security/HSM_PRODUCTION_SETUP.md](docs/security/HSM_PRODUCTION_SETUP.md)

3. **For development** (to suppress warnings): Use `--allow-stub-hsm` flag
   ```bash
   ./themis_server --allow-stub-hsm
   ```

### Detection
ThemisDB will display a **prominent warning banner** at startup if stub HSM is active:
- Logs ERROR-level warnings every 5 minutes
- Exposes `themis_hsm_insecure_config` metric in `/metrics`

---

## Prerequisites

- **Docker** (recommended) OR
- **Linux/macOS/Windows** with build tools (for source builds)

## Option 1: Docker (Fastest)

### 1. Pull and Run

```bash
# Pull the latest image
docker pull themisdb/themisdb:latest

# Run ThemisDB
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:latest
```

### 2. Verify Installation

```bash
# Check health
curl http://localhost:8080/health
# → {"status":"ok","version":"1.9.0-beta"}
```

### 3. Your First Query

```bash
# Create an entity
curl -X PUT http://localhost:8080/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}"}'

# Read it back
curl http://localhost:8080/entities/users:alice

# Create an index
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"city"}'

# Query using the index
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{"table":"users","predicates":[{"column":"city","value":"Berlin"}],"return":"entities"}'
```

## Option 2: From Source

### 1. Clone and Build

**Linux/macOS:**
```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Setup dependencies and build
./scripts/setup.sh
./scripts/build.sh
```

**Windows:**
```powershell
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Setup dependencies and build
.\scripts\setup.ps1
.\scripts\build.ps1
```

### 2. Start Server

```bash
# Start with default configuration
./build/themis_server --config config.yaml
```

### 3. Verify

```bash
curl http://localhost:8080/health
```

## Next Steps

### Learn the Basics

- **[5-Minute Tutorial](README.md#5-minute-tutorial)** - CRUD operations
- **[10-Minute Quickstart](docs/EXAMPLES_QUICKSTART.md)** - Advanced features
- **[Examples Index](docs/EXAMPLES_INDEX.md)** - Browse 37+ examples

### Explore Features

#### Multi-Model Operations

**Relational:**
```bash
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [{"column": "age", "operator": ">", "value": 25}],
    "return": "entities"
  }'
```

**Graph:**
```bash
curl -X POST http://localhost:8080/graph/traverse \
  -H "Content-Type: application/json" \
  -d '{
    "start": "users:alice",
    "algorithm": "bfs",
    "max_depth": 3
  }'
```

**Vector Search:**
```bash
curl -X POST http://localhost:8080/vector/search \
  -H "Content-Type: application/json" \
  -d '{
    "collection": "embeddings",
    "vector": [0.1, 0.2, 0.3, ...],
    "k": 10
  }'
```

### Configuration

Create a custom `config.yaml`:

```yaml
server:
  host: "0.0.0.0"
  port: 8080
  threads: 8

storage:
  data_dir: "./data"
  wal_dir: "./wal"

logging:
  level: "info"
  file: "./logs/themis.log"

security:
  # ⚠️ WARNING: Configure HSM for production!
  # See: docs/security/HSM_PRODUCTION_SETUP.md
  hsm:
    provider: stub  # DEVELOPMENT ONLY - CHANGE FOR PRODUCTION
  
  tls:
    enabled: true
    cert_file: "./certs/server.crt"
    key_file: "./certs/server.key"
```

See [docs/en/guides/guides_configuration.md](docs/en/guides/guides_configuration.md) for all options.

### Development Setup

If you're developing with ThemisDB, open the project in VS Code to get:
- IntelliSense and code completion
- Debugging configurations
- Build tasks
- Recommended extensions

The `.vscode/` directory has been pre-configured for you!

### Monitoring

View metrics:
```bash
curl http://localhost:8080/metrics
```

Or use Prometheus + Grafana:
```bash
docker-compose -f docker/monitoring/docker-compose.yml up -d
```

Access Grafana at http://localhost:3000 (admin/admin)

## Common Tasks

### Create a Collection

```bash
curl -X POST http://localhost:8080/collections \
  -H "Content-Type: application/json" \
  -d '{"name": "products", "schema": {"type": "object"}}'
```

### Insert Multiple Entities

```bash
curl -X POST http://localhost:8080/batch/insert \
  -H "Content-Type: application/json" \
  -d '{
    "entities": [
      {"urn": "products:1", "blob": "{\"name\":\"Laptop\",\"price\":999}"},
      {"urn": "products:2", "blob": "{\"name\":\"Mouse\",\"price\":29}"},
      {"urn": "products:3", "blob": "{\"name\":\"Keyboard\",\"price\":79}"}
    ]
  }'
```

### Create a Graph Relationship

```bash
curl -X POST http://localhost:8080/graph/edges \
  -H "Content-Type: application/json" \
  -d '{
    "from": "users:alice",
    "to": "users:bob",
    "type": "FOLLOWS"
  }'
```

### Backup Data

```bash
# Online backup
curl -X POST http://localhost:8080/admin/backup \
  -d '{"path": "/backups/backup-2026-01-22"}'
```

### Stop Server

```bash
# Docker
docker stop themis

# From source
# Press Ctrl+C in the server terminal
```

## Troubleshooting

### Port Already in Use
```bash
# Change port in config.yaml
server:
  port: 8081
```

### Out of Memory
```bash
# Reduce memory usage in config.yaml
storage:
  cache_size_mb: 512
```

### Check Logs
```bash
# Docker
docker logs themis

# From source
tail -f logs/themis.log
```

## Learning Paths

Choose your path based on your role:

### 🚀 Application Developer
1. [5-Minute Tutorial](README.md#5-minute-tutorial)
2. [REST API Reference](docs/api/API_REFERENCE.md)
3. [Client SDKs](clients/README.md)

### 🔧 DevOps Engineer
1. [Docker Deployment](docs/de/deployment/DOCKER_DEPLOYMENT.md)
2. [Configuration Guide](docs/en/guides/guides_configuration.md)
3. [Monitoring Setup](docs/de/observability/observability_prometheus.md)

### 🎓 Database Developer
1. [AQL Syntax](docs/de/aql/aql_syntax.md)
2. [Multi-Model Design](docs/de/architecture/architecture_base_entity.md)
3. [Transaction Management](docs/de/features/features_transactions.md)

### 🧠 AI/ML Engineer
1. [Vector Search](docs/de/features/features_vector_ops.md)
2. [LLM Integration](docs/LLM_CORE_STATUS_MASTER.md)
3. [Embeddings Examples](docs/EXAMPLES_INDEX.md#vector-search)

## Resources

- 📚 **[Full Documentation](https://makr-code.github.io/ThemisDB/)**
- 💬 **[GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)** - Ask questions
- 🐛 **[GitHub Issues](https://github.com/makr-code/ThemisDB/issues)** - Report bugs
- 🤝 **[Contributing Guide](CONTRIBUTING.md)** - Contribute to ThemisDB
- ❓ **[FAQ](docs/FAQ.md)** - Frequently asked questions

## What's Next?

- Explore [37+ Examples](docs/EXAMPLES_INDEX.md)
- Read about [Architecture](docs/de/architecture/ARCHITECTURE_OVERVIEW.md)
- Learn [Best Practices](docs/en/guides/guides_configuration.md)
- Join the [Community](https://github.com/makr-code/ThemisDB/discussions)

---

**Need Help?** Check the [FAQ](docs/FAQ.md) or [ask in Discussions](https://github.com/makr-code/ThemisDB/discussions/new?category=q-a)

**Found a Bug?** [Report it](https://github.com/makr-code/ThemisDB/issues/new?template=bug_report.md)

**Want to Contribute?** Read the [Contributing Guide](CONTRIBUTING.md)
