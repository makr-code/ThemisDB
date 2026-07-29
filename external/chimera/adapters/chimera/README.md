# CHIMERA Suite: Database Adapters

**Vendor-Neutral, System-Independent Database Adapter Implementations**

[![License: Apache-2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Vendor Neutral](https://img.shields.io/badge/Vendor-Neutral-green.svg)]()

---

## Overview

This directory contains vendor-neutral database adapter implementations for the **CHIMERA** (Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment) benchmark suite. These adapters provide a unified interface for benchmarking diverse database systems without vendor bias.

### Key Features

✅ **Vendor-Neutral Design** - No branding, no favoritism, fair comparisons  
✅ **Unified Interface** - Single API for all database types  
✅ **Multiple Implementations** - ThemisDB, PostgreSQL, Weaviate, Qdrant, and more  
✅ **Extensible Architecture** - Easy to add new database systems  
✅ **Comprehensive Testing** - Independent test framework  
✅ **Permissive Licensing** - Apache-2.0 OR MIT for maximum reusability

---

## Available Adapters

### Production Implementations

| Adapter | Database Type | Features | Status |
|---------|--------------|----------|--------|
| **ThemisDB** | Multi-model | CRUD, Vectors, Transactions, Graph | ✅ Complete |
| **MongoDB** | Document Store | CRUD, Atlas Vector Search, Transactions | ✅ Complete |
| **PostgreSQL** | Relational | CRUD, SQL, pgvector, Transactions | ✅ Complete |
| **Weaviate** | Vector DB | CRUD, Vector Search, REST/GraphQL | ✅ Complete |
| **Qdrant** | Vector DB | CRUD, Vector Search (HNSW), REST/gRPC | ✅ Complete |
| **Pinecone** | Managed Vector DB | Vector Search, Batch Upsert, Metadata Filtering | ✅ Complete |
| **Elasticsearch** | Search Engine | Full-text Search, k-NN Vector Search | ✅ Complete |
| **Neo4j** | Graph DB | Cypher Queries, Graph Traversal, ACID Transactions | ✅ Complete |

### Template & Examples

| File | Purpose | Use For |
|------|---------|---------|
| `template_adapter.hpp` | Base template | Creating new adapters |
| Documentation | Step-by-step guide | Implementation reference |

### Future Adapters (Contributions Welcome)

- ArangoDB (Multi-model)
- Cassandra (Wide-column)
- Redis (Key-value)
- Milvus (Vector database)

---

## Quick Start

### 1. Using an Existing Adapter

```cpp
#include "chimera/database_adapter.hpp"
#include "chimera/postgresql_adapter.hpp"

using namespace chimera;
using namespace chimera::adapters;

int main() {
    // Create adapter
    auto adapter = create_postgresql_adapter();
    
    // Configure connection
    ConnectionConfig config;
    config.host = "localhost";
    config.port = 5432;
    config.database = "benchmark_db";
    config.username = "bench_user";
    config.password = "secure_password";
    
    // Connect
    auto status = adapter->connect(config);
    if (!status.ok()) {
        std::cerr << "Connection failed: " << status.message << std::endl;
        return 1;
    }
    
    // Insert a document
    Document doc;
    doc["id"] = std::string("user_001");
    doc["name"] = std::string("Alice");
    doc["age"] = int64_t(30);
    
    status = adapter->insert("users", doc);
    if (!status.ok()) {
        std::cerr << "Insert failed: " << status.message << std::endl;
        return 1;
    }
    
    // Find documents
    Document filter;
    filter["age"] = int64_t(30);
    
    ResultSet results;
    status = adapter->find("users", filter, results);
    if (status.ok()) {
        std::cout << "Found " << results.size() << " users" << std::endl;
    }
    
    // Clean up
    adapter->disconnect();
    return 0;
}
```

### 2. Building an Adapter

```bash
# Clone the repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DBUILD_CHIMERA_ADAPTERS=ON

# Build specific adapter
cmake --build . --target chimera_postgresql_adapter

# Run tests
ctest -R chimera_postgresql
```

### 3. Creating a New Adapter

```bash
# Copy template
cp adapters/chimera/template_adapter.hpp adapters/chimera/mydb_adapter.hpp

# Edit and implement
vim adapters/chimera/mydb_adapter.hpp
vim adapters/chimera/mydb_adapter.cpp

# Add to CMake
# Edit adapters/chimera/CMakeLists.txt

# Build and test
cmake --build . --target chimera_mydb_adapter
```

---

## Architecture

### Interface Design

The adapter interface is defined in `include/chimera/database_adapter.hpp`:

```
┌─────────────────────────────────────────┐
│      DatabaseAdapter Interface          │
│  (Vendor-Neutral, System-Agnostic)      │
├─────────────────────────────────────────┤
│ Connection Management                   │
│ • connect() / disconnect()              │
│ • is_connected() / ping()               │
├─────────────────────────────────────────┤
│ CRUD Operations                         │
│ • insert() / insert_batch()             │
│ • find() / find_by_id()                 │
│ • update() / remove() / count()         │
├─────────────────────────────────────────┤
│ Vector Operations (Optional)            │
│ • supports_vector_search()              │
│ • vector_search()                       │
│ • create_vector_index()                 │
├─────────────────────────────────────────┤
│ Transaction Support (Optional)          │
│ • supports_transactions()               │
│ • begin_transaction()                   │
├─────────────────────────────────────────┤
│ Schema Operations                       │
│ • create_collection()                   │
│ • drop_collection()                     │
│ • list_collections()                    │
├─────────────────────────────────────────┤
│ Query Execution                         │
│ • execute_query()                       │
│ • execute_query_params()                │
├─────────────────────────────────────────┤
│ Metadata & Capabilities                 │
│ • get_adapter_info()                    │
│ • get_database_version()                │
│ • get_capabilities()                    │
└─────────────────────────────────────────┘
           │           │          │          │
           ▼           ▼          ▼          ▼
    ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
    │ ThemisDB │ │ MongoDB  │ │PostgreSQL│ │ Weaviate │
    │ Adapter  │ │ Adapter  │ │ Adapter  │ │ Adapter  │
    └──────────┘ └──────────┘ └──────────┘ └──────────┘
```

### Data Types

All adapters use standard, vendor-neutral types:

```cpp
// Values can be NULL, bool, int, double, string, binary, or vector
using Value = std::variant<
    std::monostate,           // NULL
    bool, int64_t, double,    // Primitives
    std::string,              // Text
    std::vector<uint8_t>,     // Binary
    std::vector<float>        // Vectors
>;

// Documents are key-value maps
using Document = std::map<std::string, Value>;

// Result sets are collections of documents
using ResultSet = std::vector<Document>;
```

### Error Handling

Consistent error reporting across all adapters:

```cpp
enum class ErrorCode {
    OK,
    CONNECTION_FAILED,
    OPERATION_FAILED,
    NOT_FOUND,
    INVALID_ARGUMENT,
    UNSUPPORTED_OPERATION,
    // ...
};

struct Status {
    ErrorCode code;
    std::string message;
    bool ok() const;
};
```

---

## Adapter Implementations

### ThemisDB Adapter

**File:** `themisdb_adapter.hpp`

```cpp
#include "chimera/themisdb_adapter.hpp"

auto adapter = create_themisdb_adapter();
```

**Features:**
- Native multi-model support (relational, document, graph, vector)
- Full ACID transactions with MVCC
- GPU-accelerated vector search
- Complete API implementation

**Configuration:**
```cpp
ConnectionConfig config;
config.host = "localhost";
config.port = 18765;  // ThemisDB default port
config.database = "benchmark_db";
config.parameters["protocol"] = "binary_wire";
```

**Mappings:**
- Collections → ThemisDB tables
- Documents → Structured rows or JSON
- Transactions → Native MVCC transactions
- Vector search → ThemisDB vector index

---

### MongoDB Adapter

**File:** `mongodb_adapter.hpp`

```cpp
#include "chimera/mongodb_adapter.hpp"

auto adapter = create_mongodb_adapter();
```

**Features:**
- Native document CRUD operations via mongocxx driver
- Atlas Vector Search for vector similarity queries
- Multi-document ACID transactions (MongoDB 4.0+, replica set required)
- Full-text search via text indexes
- Secondary and compound index management
- JSON schema validation support

**Configuration:**
```cpp
ConnectionConfig config;
config.host = "localhost";
config.port = 27017;
config.database = "benchmark_db";
config.username = "bench_user";
config.password = "password";
config.use_tls = true;
// Or use a full connection string:
config.parameters["connection_string"] =
    "mongodb+srv://user:pass@cluster.mongodb.net/benchmark_db";
```

**Mappings:**
- Collections → MongoDB collections
- Documents → BSON documents with `_id` field
- Vectors → Numeric array fields + Atlas Vector Search index
- Transactions → MongoDB multi-document sessions

**Vector Operations:**
```json
// Atlas $vectorSearch aggregation stage (requires Atlas Vector Search index)
{
  "$vectorSearch": {
    "index": "embedding_index",
    "path": "embedding",
    "queryVector": [0.1, 0.2, 0.3],
    "numCandidates": 100,
    "limit": 10
  }
}
```

---

### PostgreSQL + pgvector Adapter

**File:** `postgresql_adapter.hpp`

```cpp
#include "chimera/postgresql_adapter.hpp"

auto adapter = create_postgresql_adapter();
```

**Features:**
- Standard SQL operations via libpqxx
- Optional pgvector extension for vector operations
- Full ACID transaction support
- Automatic feature detection

**Configuration:**
```cpp
ConnectionConfig config;
config.host = "localhost";
config.port = 5432;
config.database = "benchmark_db";
config.username = "bench_user";
config.password = "password";
config.use_tls = true;
```

**Mappings:**
- Collections → PostgreSQL tables
- Documents → JSONB columns + structured columns
- Vectors → pgvector `vector` type
- Transactions → PostgreSQL transactions

**Vector Operations:**
```sql
-- Automatically uses pgvector if available
-- Distance operators: <-> (L2), <#> (inner product), <=> (cosine)
SELECT * FROM items 
ORDER BY embedding <-> '[0.1,0.2,0.3]' 
LIMIT 10;
```

---

### Weaviate Adapter

**File:** `weaviate_adapter.hpp`

```cpp
#include "chimera/weaviate_adapter.hpp"

auto adapter = create_weaviate_adapter();
```

**Features:**
- REST/GraphQL API client
- Native vector search
- Schema-on-write
- Automatic vectorization (optional)

**Configuration:**
```cpp
ConnectionConfig config;
config.host = "localhost";
config.port = 8080;
config.parameters["scheme"] = "http";  // or "https"
config.parameters["api_key"] = "your-api-key";
```

**Mappings:**
- Collections → Weaviate classes
- Documents → Weaviate objects
- Vector search → `nearVector` queries
- No transaction support (NoSQL)

**Special Features:**
- Automatic embedding generation (if vectorizer configured)
- Hybrid search (vector + keyword)
- Cross-references between objects

---

## Testing

### Running Tests

```bash
# Build tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Run all adapter tests
cd build
ctest -R chimera

# Run specific adapter tests
./tests/chimera_themisdb_adapter_test
./tests/chimera_postgresql_adapter_test
./tests/chimera_weaviate_adapter_test
```

### Test Structure

Tests are vendor-neutral and exercise common functionality:

```cpp
TEST(AdapterTest, BasicInsertAndFind) {
    auto adapter = create_test_adapter();
    
    // Connect
    auto status = adapter->connect(get_test_config());
    ASSERT_TRUE(status.ok());
    
    // Insert
    Document doc = create_test_document();
    status = adapter->insert("test_collection", doc);
    ASSERT_TRUE(status.ok());
    
    // Find
    ResultSet results;
    status = adapter->find("test_collection", {}, results);
    ASSERT_TRUE(status.ok());
    ASSERT_GE(results.size(), 1);
    
    adapter->disconnect();
}
```

### Test Coverage

Each adapter should have tests for:
- ✅ Connection management
- ✅ Basic CRUD operations
- ✅ Batch operations
- ✅ Query options (limit, offset, projection)
- ✅ Error handling
- ✅ Vector search (if supported)
- ✅ Transactions (if supported)
- ✅ Schema operations

---

## Documentation

### Complete Guides

1. **[ADAPTER_GUIDE.md](../../docs/chimera/ADAPTER_GUIDE.md)** - Step-by-step implementation guide
2. **[API_REFERENCE.md](../../docs/chimera/API_REFERENCE.md)** - Detailed API documentation (coming soon)
3. **[CHIMERA_USER_GUIDE_NEUTRAL_INTEGRATION.md](../../docs/benchmarks/CHIMERA_USER_GUIDE_NEUTRAL_INTEGRATION.md)** - Benchmark integration guide

### Quick Links

- **Template Adapter:** `template_adapter.hpp` - Start here for new adapters
- **Interface Definition:** `include/chimera/database_adapter.hpp`
- **Example Benchmarks:** `benchmarks/chimera/`

---

## Contributing

We welcome contributions of new adapters! Please follow these guidelines:

### Contribution Checklist

- [ ] Implements all required `DatabaseAdapter` methods
- [ ] Uses vendor-neutral naming (no branding)
- [ ] Includes comprehensive error handling
- [ ] Provides unit tests with >80% coverage
- [ ] Includes documentation and examples
- [ ] Licensed under Apache-2.0 OR MIT
- [ ] Compiles without warnings
- [ ] Follows existing code style

### Submitting an Adapter

1. **Fork the repository**
2. **Create your adapter** following the template
3. **Write tests** for all functionality
4. **Update CMakeLists.txt** to include your adapter
5. **Add documentation** (README section, examples)
6. **Submit a pull request** with description

### Code Review

All contributions will be reviewed for:
- Vendor neutrality
- Code quality
- Test coverage
- Documentation completeness
- License compliance

---

## Licensing

All adapters in this directory are dual-licensed under:

- **Apache License 2.0** OR
- **MIT License**

You may choose either license for your use. This dual-licensing ensures maximum compatibility and reusability across different projects and organizations.

See [LICENSE-APACHE](../../LICENSE-APACHE) and [LICENSE-MIT](../../LICENSE-MIT) for details.

---

## Vendor Neutrality Commitment

This adapter suite adheres to strict vendor neutrality principles:

✅ **No Branding:** No vendor logos, colors, or marketing materials  
✅ **No Favoritism:** All systems treated equally and fairly  
✅ **No Bias:** Results sorted alphabetically or by metric value only  
✅ **Open Standards:** Based on IEEE/ACM/ISO standards  
✅ **Fair Comparisons:** Identical test conditions for all systems

---

## Support

### Getting Help

- **Documentation:** See `docs/chimera/ADAPTER_GUIDE.md`
- **Issues:** Open an issue on GitHub
- **Discussions:** Use GitHub Discussions for questions

### Commercial Support

While the adapters are open source, commercial support may be available from:
- Database vendors (for their specific adapters)
- Independent consultants
- Benchmark operators

---

## Acknowledgments

The CHIMERA adapter framework was developed with input from:
- Database researchers and practitioners
- Open source contributors
- Standards organizations (IEEE, ISO, ACM)

Special thanks to all contributors who have implemented adapters for their database systems.

---

## Roadmap

### Version 1.x

- [x] Core interface definition
- [x] ThemisDB adapter
- [x] MongoDB adapter
- [x] PostgreSQL adapter
- [x] Weaviate adapter
- [x] Qdrant adapter
- [x] Pinecone adapter
- [x] Elasticsearch adapter (full-text + k-NN vector search)
- [x] Neo4j adapter (Cypher queries, graph traversal, ACID transactions)
- [x] Template and documentation

### Version 2.x (Planned)

- [ ] ArangoDB adapter (Multi-model)
- [ ] Performance benchmarks
- [ ] Async operation support

### Community Contributions Welcome

We especially welcome adapters for:
- Graph databases (Neptune, TigerGraph)
- Document stores (CouchDB, DynamoDB)
- Vector databases (Milvus)
- Search engines (Solr, MeiliSearch)
- Time-series databases (InfluxDB, TimescaleDB, Prometheus)

---

**For detailed implementation instructions, see [ADAPTER_GUIDE.md](../../docs/chimera/ADAPTER_GUIDE.md)**

**For benchmark integration, see [CHIMERA_USER_GUIDE_NEUTRAL_INTEGRATION.md](../../docs/benchmarks/CHIMERA_USER_GUIDE_NEUTRAL_INTEGRATION.md)**
