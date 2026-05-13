# Getting Started with ThemisDB

Welcome to ThemisDB! This comprehensive tutorial will take you from zero to productive in 45 minutes. By the end, you'll understand how to install ThemisDB, connect to it, and perform basic database operations.

## 🎯 What You'll Learn

- ✅ Three ways to install ThemisDB
- ✅ How to verify your installation
- ✅ Creating your first database connection
- ✅ Basic CRUD (Create, Read, Update, Delete) operations
- ✅ Simple query patterns
- ✅ Creating and using indexes
- ✅ Understanding entities and attributes

**Prerequisites:** Basic command line knowledge  
**Time Required:** 30-45 minutes  
**Difficulty:** Beginner

---

## Part 1: Installation (10 minutes)

Choose the installation method that works best for you:

### Option A: Docker (Recommended) ⭐

Docker is the fastest way to get started. No build tools required!

```bash
# 1. Pull the latest ThemisDB image
docker pull themisdb/themisdb:latest

# 2. Run ThemisDB container
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themisdb_data:/data \
  themisdb/themisdb:latest

# 3. Verify it's running
docker ps | grep themisdb
```

**Expected Output:**
```
CONTAINER ID   IMAGE                      STATUS         PORTS
abc123def456   themisdb/themisdb:latest   Up 5 seconds   0.0.0.0:8080->8080/tcp, ...
```

**Port Reference:**
- `8080` - HTTP/REST API, GraphQL
- `18765` - Binary Wire Protocol, gRPC
- `4318` - Prometheus metrics

**💡 Pro Tip:** Use Docker Compose for production deployments. See [deployment docs](../deployment/).

---

### Option B: Pre-built Binary

Download pre-compiled binaries from our releases page:

```bash
# 1. Download latest release (Linux example)
wget https://github.com/makr-code/ThemisDB/releases/latest/download/themisdb-linux-x64.tar.gz

# 2. Extract
tar -xzf themisdb-linux-x64.tar.gz
cd themisdb

# 3. Run server
./themis_server --config config.yaml

# 4. Verify (in another terminal)
curl http://localhost:8080/health
```

**Available Platforms:**
- Linux (x64, ARM64)
- macOS (Intel, Apple Silicon)
- Windows (x64)

---

### Option C: Build from Source

For developers who want the latest features or need to customize:

**Linux/macOS:**
```bash
# 1. Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Install dependencies and build
./scripts/setup.sh
./scripts/build.sh

# 3. Start server
./build/themis_server --config config.yaml
```

**Windows:**
```powershell
# 1. Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Install dependencies and build
.\scripts\setup.ps1
.\scripts\build.ps1

# 3. Start server
.\build\themis_server.exe --config config.yaml
```

**Build Requirements:**
- C++20 compiler (GCC 11+, Clang 14+, MSVC 2022+)
- CMake 3.20+
- vcpkg (automatically managed by scripts)

**Build Time:** 15-30 minutes on first build

---

## Part 2: First Connection (5 minutes)

Now that ThemisDB is running, let's connect to it!

### Verify Server Health

```bash
curl http://localhost:8080/health
```

**Expected Output:**
```json
{
  "status": "healthy",
  "version": "1.4.1-dev",
  "uptime": 42,
  "database": "ready"
}
```

**❌ Not working?**
- Check if the port is already in use: `lsof -i :8080` (Linux/macOS)
- Verify Docker container is running: `docker logs themisdb`
- Check firewall settings

### Get Server Information

```bash
curl http://localhost:8080/info
```

**Expected Output:**
```json
{
  "version": "1.4.1-dev",
  "build_date": "2025-01-24",
  "features": ["multi-model", "vector-search", "graph", "llm"],
  "storage_engine": "RocksDB",
  "protocols": ["http", "grpc", "websocket"]
}
```

---

## Part 3: Creating Your First Database (10 minutes)

ThemisDB uses an **entity-attribute model**. Let's create a simple user database.

### Understanding Entities

An **entity** is a unique identifier with associated data:
- Format: `namespace:key` (e.g., `users:alice`, `products:12345`)
- Each entity has **attributes** (key-value pairs)
- Attributes are versioned with MVCC (Multi-Version Concurrency Control)

### Create Your First Entity

```bash
# Create a user entity
curl -X PUT http://localhost:8080/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"Alice Johnson\",\"email\":\"alice@example.com\",\"age\":30,\"city\":\"Berlin\"}"
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "entity": "users:alice",
  "version": 1
}
```

**What happened?**
- Created entity with ID `users:alice`
- Stored JSON data as a blob attribute
- Server assigned version number `1`

**💡 Pro Tip:** Use namespaces (the part before `:`) to organize different types of entities.

### Read the Entity

```bash
curl http://localhost:8080/entities/users:alice
```

**Expected Output:**
```json
{
  "entity_id": "users:alice",
  "version": 1,
  "blob": "{\"name\":\"Alice Johnson\",\"email\":\"alice@example.com\",\"age\":30,\"city\":\"Berlin\"}",
  "created_at": "2025-01-24T10:30:45Z",
  "updated_at": "2025-01-24T10:30:45Z"
}
```

### Create More Entities

```bash
# Create Bob
curl -X PUT http://localhost:8080/entities/users:bob \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"Bob Smith\",\"email\":\"bob@example.com\",\"age\":25,\"city\":\"Munich\"}"
  }'

# Create Charlie
curl -X PUT http://localhost:8080/entities/users:charlie \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"Charlie Brown\",\"email\":\"charlie@example.com\",\"age\":35,\"city\":\"Berlin\"}"
  }'
```

---

## Part 4: Basic CRUD Operations (10 minutes)

Now let's master Create, Read, Update, and Delete operations.

### Create (C)

We already created entities above. Here's a batch create:

```bash
# Create multiple entities at once
curl -X POST http://localhost:8080/batch/create \
  -H "Content-Type: application/json" \
  -d '{
    "entities": [
      {
        "entity_id": "users:diana",
        "blob": "{\"name\":\"Diana Prince\",\"email\":\"diana@example.com\",\"age\":28,\"city\":\"Hamburg\"}"
      },
      {
        "entity_id": "users:evan",
        "blob": "{\"name\":\"Evan Davis\",\"email\":\"evan@example.com\",\"age\":32,\"city\":\"Berlin\"}"
      }
    ]
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "created": 2,
  "entities": ["users:diana", "users:evan"]
}
```

### Read (R)

```bash
# Read single entity
curl http://localhost:8080/entities/users:alice

# Read multiple entities
curl -X POST http://localhost:8080/batch/read \
  -H "Content-Type: application/json" \
  -d '{
    "entity_ids": ["users:alice", "users:bob", "users:charlie"]
  }'
```

**Expected Output:**
```json
{
  "entities": [
    {
      "entity_id": "users:alice",
      "blob": "{\"name\":\"Alice Johnson\", ...}"
    },
    {
      "entity_id": "users:bob",
      "blob": "{\"name\":\"Bob Smith\", ...}"
    }
  ]
}
```

### Update (U)

```bash
# Update Alice's city
curl -X PUT http://localhost:8080/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"Alice Johnson\",\"email\":\"alice@example.com\",\"age\":30,\"city\":\"Frankfurt\"}"
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "entity": "users:alice",
  "version": 2
}
```

**Note:** Version incremented from 1 to 2!

### Delete (D)

```bash
# Delete an entity
curl -X DELETE http://localhost:8080/entities/users:evan
```

**Expected Output:**
```json
{
  "status": "success",
  "entity": "users:evan",
  "deleted": true
}
```

**Verify deletion:**
```bash
curl http://localhost:8080/entities/users:evan
```

**Expected:** `404 Not Found` or `{"status": "error", "message": "Entity not found"}`

---

## Part 5: Simple Queries (8 minutes)

Now let's query our data!

### Create an Index

Before querying, create an index for better performance:

```bash
# Create index on 'city' attribute
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "column": "city",
    "type": "btree"
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "index": "users_city_idx",
  "type": "btree"
}
```

### Query by City

```bash
# Find all users in Berlin
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [
      {
        "column": "city",
        "operator": "=",
        "value": "Berlin"
      }
    ],
    "return": "entities"
  }'
```

**Expected Output:**
```json
{
  "count": 2,
  "entities": [
    {
      "entity_id": "users:alice",
      "blob": "{\"name\":\"Alice Johnson\",\"city\":\"Berlin\", ...}"
    },
    {
      "entity_id": "users:charlie",
      "blob": "{\"name\":\"Charlie Brown\",\"city\":\"Berlin\", ...}"
    }
  ]
}
```

### Range Query

```bash
# Find users aged 25-30
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [
      {
        "column": "age",
        "operator": ">=",
        "value": 25
      },
      {
        "column": "age",
        "operator": "<=",
        "value": 30
      }
    ],
    "return": "entities"
  }'
```

### Query with Sorting

```bash
# Find users in Berlin, sorted by age
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [
      {
        "column": "city",
        "value": "Berlin"
      }
    ],
    "order_by": "age",
    "order": "DESC",
    "return": "entities"
  }'
```

---

## Part 6: Working with Indexes (7 minutes)

Indexes dramatically improve query performance!

### Index Types

ThemisDB supports multiple index types:

1. **B-Tree** - General purpose, range queries
2. **Hash** - Exact match lookups
3. **Vector** - Similarity search (for embeddings)
4. **Full-Text** - Text search

### Create Multiple Indexes

```bash
# Create B-Tree index on age
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "column": "age",
    "type": "btree"
  }'

# Create Hash index on email (fast exact lookups)
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "column": "email",
    "type": "hash"
  }'
```

### List All Indexes

```bash
curl http://localhost:8080/index/list?table=users
```

**Expected Output:**
```json
{
  "indexes": [
    {
      "name": "users_city_idx",
      "table": "users",
      "column": "city",
      "type": "btree",
      "entries": 5
    },
    {
      "name": "users_age_idx",
      "table": "users",
      "column": "age",
      "type": "btree",
      "entries": 5
    },
    {
      "name": "users_email_idx",
      "table": "users",
      "column": "email",
      "type": "hash",
      "entries": 5
    }
  ]
}
```

### Query with Index (Fast!)

```bash
# This query will use the email hash index
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [
      {
        "column": "email",
        "value": "alice@example.com"
      }
    ],
    "use_index": true,
    "return": "entities"
  }'
```

### Drop an Index

```bash
# Remove the age index
curl -X DELETE http://localhost:8080/index/users_age_idx
```

---

## Common Pitfalls to Avoid

### ❌ Pitfall 1: Forgetting to Create Indexes

**Problem:** Queries are slow on large datasets.

**Solution:** Create indexes on frequently queried columns:
```bash
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{"table": "users", "column": "city", "type": "btree"}'
```

### ❌ Pitfall 2: Not Using Namespaces

**Problem:** Entity IDs clash between different types.

**Solution:** Always use namespaces:
- ✅ Good: `users:alice`, `products:123`, `orders:456`
- ❌ Bad: `alice`, `123`, `456`

### ❌ Pitfall 3: Storing Large BLOBs

**Problem:** Performance degrades with huge JSON documents.

**Solution:** 
- Keep entities under 1MB
- Split large objects into multiple entities
- Use references for relationships

### ❌ Pitfall 4: Not Handling Errors

**Problem:** Client crashes on network errors.

**Solution:** Always check response status:
```bash
response=$(curl -s -w "\n%{http_code}" http://localhost:8080/entities/users:alice)
http_code=$(echo "$response" | tail -n1)
if [ "$http_code" != "200" ]; then
  echo "Error: HTTP $http_code"
fi
```

---

## Pro Tips 💡

### 1. Use Transactions for Consistency

```bash
# Start a transaction
curl -X POST http://localhost:8080/tx/begin
# Returns: {"tx_id": "tx_12345"}

# Perform operations within transaction
curl -X PUT http://localhost:8080/tx/tx_12345/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob": "..."}'

# Commit transaction
curl -X POST http://localhost:8080/tx/tx_12345/commit
```

### 2. Batch Operations for Performance

Always batch when creating/updating multiple entities:
```bash
# 100x faster than individual requests!
curl -X POST http://localhost:8080/batch/create -d '{...}'
```

### 3. Monitor with Metrics

```bash
# Check database metrics
curl http://localhost:4318/metrics
```

### 4. Use EXPLAIN for Query Optimization

```bash
# See query execution plan
curl -X POST http://localhost:8080/query/explain \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [{"column": "city", "value": "Berlin"}]
  }'
```

---

## What You've Learned ✅

Congratulations! You now know:

- ✅ How to install ThemisDB (3 methods)
- ✅ How to create and read entities
- ✅ How to update and delete data
- ✅ How to perform basic queries
- ✅ How to create and use indexes
- ✅ Common pitfalls and pro tips

---

## Next Steps 🚀

### Beginner Path
1. **[CRUD Tutorial](CRUD_TUTORIAL.md)** - Deep dive into operations
2. **[Interactive Examples](INTERACTIVE_EXAMPLES.md)** - Try code snippets
3. **Try Example Apps** - Start with [Hello World](../../examples/01_hello_world/)

### Intermediate Path
1. **[Batch Operations](BATCH_OPERATIONS.md)** - Optimize performance
2. **[Schema Design](SCHEMA_DESIGN.md)** - Design better databases
3. **[Best Practices](BEST_PRACTICES.md)** - Production patterns

### Advanced Path
1. **[Vector Search Tutorial](../features/VECTOR_SEARCH.md)** - Semantic search
2. **[Graph Queries](../features/GRAPH_QUERIES.md)** - Relationship queries
3. **[Distributed Setup](../deployment/DISTRIBUTED_SETUP.md)** - Scale horizontally

---

## Getting Help

- **Documentation:** [docs.themisdb.com](https://makr-code.github.io/ThemisDB/)
- **Examples:** Check `/examples` directory
- **Issues:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Discussions:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **FAQ:** [Frequently Asked Questions](../FAQ.md)

---

**Ready for more?** Continue to [CRUD Tutorial](CRUD_TUTORIAL.md) →
