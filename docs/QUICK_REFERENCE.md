# ⚡ ThemisDB Quick Reference

**Version:** 1.9.0-beta | **Updated:** 2026-05

> Quick access to the most-used commands, endpoints, and configuration options.
> For the full Getting Started walkthrough see [de/guides/QUICKSTART.md](de/guides/QUICKSTART.md).

---

## 🚀 Start / Stop

```bash
# Docker (recommended)
docker run -d --name themisdb \
  -p 8080:8080 -p 18765:18765 -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:latest

docker stop themisdb   # stop
docker start themisdb  # restart

# From source (Linux/macOS)
./build/themis_server --config config.yaml

# Health check
curl http://localhost:8080/health
# Expected: {"status":"ok","version":"1.9.0-beta"}
```

---

## 🔌 Ports

| Port  | Protocol       | Purpose                          |
|-------|----------------|----------------------------------|
| 8080  | HTTP           | REST API, GraphQL, Health checks |
| 18765 | TCP / gRPC     | Binary Wire Protocol, gRPC       |
| 4318  | HTTP           | OpenTelemetry / Prometheus       |

---

## 📦 Core REST API

### Entities (key-value style)

```bash
BASE=http://localhost:8080

# Create / update entity
curl -X PUT $BASE/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}"}'

# Read entity
curl $BASE/entities/users:alice

# Delete entity
curl -X DELETE $BASE/entities/users:alice

# Batch write
curl -X POST $BASE/entities/batch \
  -H "Content-Type: application/json" \
  -d '{"operations":[{"op":"put","id":"users:bob","blob":"{\"name\":\"Bob\"}"}]}'
```

### Query

```bash
# Create index
curl -X POST $BASE/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"city"}'

# Query with predicate
curl -X POST $BASE/query \
  -H "Content-Type: application/json" \
  -d '{"table":"users","predicates":[{"column":"city","value":"Berlin"}],"return":"entities"}'

# AQL query
curl -X POST $BASE/aql \
  -H "Content-Type: application/json" \
  -d '{"query":"SELECT * FROM users WHERE age > 25 LIMIT 10"}'
```

### Graph

```bash
# Add edge
curl -X POST $BASE/graph/edge \
  -H "Content-Type: application/json" \
  -d '{"from":"users:alice","to":"users:bob","type":"FOLLOWS"}'

# Traverse BFS
curl -X POST $BASE/graph/traverse \
  -H "Content-Type: application/json" \
  -d '{"start":"users:alice","algorithm":"bfs","max_depth":3}'
```

### Vector Search

```bash
# Insert vector
curl -X PUT $BASE/vector/embeddings:doc1 \
  -H "Content-Type: application/json" \
  -d '{"vector":[0.1,0.2,0.3],"metadata":{"text":"Hello world"}}'

# kNN search
curl -X POST $BASE/vector/search \
  -H "Content-Type: application/json" \
  -d '{"collection":"embeddings","vector":[0.1,0.2,0.3],"k":10,"metric":"cosine"}'
```

### Transactions

```bash
# Begin
curl -X POST $BASE/transaction/begin
# Expected: {"tx_id":"txn-abc123"}

# Operate within transaction
curl -X PUT $BASE/entities/products:p1 \
  -H "X-Transaction-ID: txn-abc123" \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Widget\"}"}'

# Commit
curl -X POST $BASE/transaction/commit \
  -H "Content-Type: application/json" \
  -d '{"tx_id":"txn-abc123"}'

# Rollback
curl -X POST $BASE/transaction/rollback \
  -H "Content-Type: application/json" \
  -d '{"tx_id":"txn-abc123"}'
```

---

## ⚙️ Minimal config.yaml

```yaml
server:
  host: "0.0.0.0"
  port: 8080
  threads: 8

storage:
  data_dir: "./data"
  wal_dir: "./wal"

logging:
  level: "info"          # debug | info | warn | error
  file: "./logs/themis.log"

security:
  # ⚠️ Replace stub HSM with a real provider for production!
  # See: docs/security/HSM_PRODUCTION_SETUP.md
  hsm:
    provider: stub        # DEVELOPMENT ONLY
  tls:
    enabled: false        # Set true in production

vector:
  default_metric: cosine  # cosine | euclidean | dot
  index_type: hnsw
```

---

## 🔑 Authentication

```bash
# Obtain token
curl -X POST $BASE/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"changeme"}'
# Expected: {"token":"<jwt>","expires_in":3600}

# Use token
curl $BASE/entities/users:alice \
  -H "Authorization: Bearer <jwt>"
```

---

## 📊 Monitoring

```bash
# Prometheus metrics
curl http://localhost:4318/metrics

# Health & readiness
curl http://localhost:8080/health
curl http://localhost:8080/ready

# Server info / version
curl http://localhost:8080/info
```

---

## 🛠️ Admin Operations

```bash
# Backup
curl -X POST $BASE/admin/backup
# Expected: {"status":"ok","path":"/data/backups/backup-<timestamp>.tar.gz"}

# Flush WAL
curl -X POST $BASE/admin/wal/flush

# Compact storage
curl -X POST $BASE/admin/compact
```

---

## 📚 Key Documentation Links

| Topic | Link |
|-------|------|
| Getting Started | [de/guides/QUICKSTART.md](de/guides/QUICKSTART.md) |
| Full API Reference | [api/API_REFERENCE.md](api/API_REFERENCE.md) |
| REST API | [api/REST_API_REFERENCE.md](api/REST_API_REFERENCE.md) |
| Tutorials | [tutorials/README.md](tutorials/README.md) |
| Examples Quickstart | [EXAMPLES_QUICKSTART.md](EXAMPLES_QUICKSTART.md) |
| Examples Index | [EXAMPLES_INDEX.md](EXAMPLES_INDEX.md) |
| FAQ | [FAQ.md](FAQ.md) |
| Security / HSM | [security/HSM_PRODUCTION_SETUP.md](security/HSM_PRODUCTION_SETUP.md) |
| Performance Tuning | [de/performance/performance_memory.md](de/performance/performance_memory.md) |
| Integration Guide | [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) |
| AQL Reference | [de/aql/aql_syntax.md](de/aql/aql_syntax.md) |

---

**Feedback / Issues:** [github.com/makr-code/ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)

