# CHIMERA Suite: Database Adapter Implementation Guide

**Version:** 1.0.0  
**License:** Apache-2.0 OR MIT  
**Standards Compliance:** Vendor-neutral, system-agnostic design

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Adapter Architecture](#adapter-architecture)
4. [Step-by-Step Implementation Guide](#step-by-step-implementation-guide)
5. [Building and Testing](#building-and-testing)
6. [Reference Implementations](#reference-implementations)
7. [Best Practices](#best-practices)
8. [Troubleshooting](#troubleshooting)
9. [Contributing](#contributing)

---

## 1. Introduction

The CHIMERA Suite provides a vendor-neutral, system-independent adapter framework for database benchmarking. This guide walks you through implementing custom adapters for any database system.

### 1.1 Purpose

This framework enables:
- **Vendor-neutral benchmarking** of diverse database systems
- **Unified API** for multi-model, relational, vector, and graph databases
- **Fair comparisons** without system-specific optimizations or branding
- **Easy integration** of new database systems into benchmark suites

### 1.2 Design Principles

- **Vendor Neutrality**: No branding, no favoritism, no system-specific naming
- **Interface Segregation**: Clear separation between interface and implementation
- **Type Safety**: Strong typing with C++ templates and variants
- **Error Handling**: Comprehensive error reporting with standard codes
- **Minimal Dependencies**: Interface depends only on C++ standard library

### 1.3 Supported Database Types

The adapter interface supports:
- Relational databases (PostgreSQL, MySQL, etc.)
- Document stores (MongoDB, CouchDB, etc.)
- Graph databases (Neo4j, ArangoDB, etc.)
- Vector databases (Weaviate, Pinecone, Milvus, etc.)
- Multi-model databases (ThemisDB, OrientDB, etc.)
- Key-value stores (Redis, DynamoDB, etc.)

---

## 2. Getting Started

### 2.1 Prerequisites

**Required:**
- C++17 or later compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15 or later
- Access to the target database system

**Optional:**
- Database client libraries (e.g., libpqxx for PostgreSQL)
- HTTP client library (for REST-based databases)
- Testing framework (Google Test recommended)

### 2.2 Directory Structure

```
adapters/chimera/
├── themisdb_adapter.hpp      # ThemisDB reference implementation
├── postgresql_adapter.hpp    # PostgreSQL + pgvector implementation
├── weaviate_adapter.hpp      # Weaviate REST API implementation
├── qdrant_adapter.hpp        # Qdrant HNSW vector search implementation
├── pinecone_adapter.hpp      # Pinecone managed vector search implementation
├── template_adapter.hpp      # Template for new adapters
└── your_adapter.hpp          # Your custom adapter

include/chimera/
└── database_adapter.hpp      # Core interface (vendor-neutral)

tests/chimera/
├── adapter_tests.hpp         # Common test framework
├── themisdb_adapter_test.cpp
├── postgresql_adapter_test.cpp
└── your_adapter_test.cpp

docs/chimera/
├── ADAPTER_GUIDE.md          # This file
└── API_REFERENCE.md          # Detailed API documentation
```

### 2.3 Quick Start

1. **Copy the template:**
   ```bash
   cp adapters/chimera/template_adapter.hpp adapters/chimera/mydb_adapter.hpp
   ```

2. **Rename classes:**
   - Replace `Template` with `MyDB` throughout
   - Update namespace and factory function names

3. **Implement core methods:**
   - Start with connection management
   - Implement basic CRUD operations
   - Add transaction support (if applicable)
   - Implement vector operations (if applicable)

4. **Create implementation file:**
   ```bash
   touch adapters/chimera/mydb_adapter.cpp
   ```

5. **Write tests:**
   ```bash
   cp tests/chimera/template_adapter_test.cpp tests/chimera/mydb_adapter_test.cpp
   ```

---

## 3. Adapter Architecture

### 3.1 Interface Overview

The `DatabaseAdapter` base class defines a vendor-neutral interface:

```cpp
namespace chimera {
    class DatabaseAdapter {
        // Connection management
        virtual Status connect(const ConnectionConfig& config) = 0;
        virtual Status disconnect() = 0;
        
        // CRUD operations
        virtual Status insert(const std::string& collection, 
                             const Document& doc, ...) = 0;
        virtual Status find(...) = 0;
        virtual Status update(...) = 0;
        virtual Status remove(...) = 0;
        
        // Vector operations
        virtual bool supports_vector_search() const = 0;
        virtual Status vector_search(...) = 0;
        
        // Transaction support
        virtual bool supports_transactions() const = 0;
        virtual Status begin_transaction(...) = 0;
        
        // Query execution
        virtual Status execute_query(...) = 0;
        
        // Metadata
        virtual std::string get_adapter_info() const = 0;
    };
}
```

### 3.2 Key Types

**Document:**
```cpp
using Value = std::variant<
    std::monostate,        // NULL
    bool, int64_t, double, // Primitives
    std::string,           // Text
    std::vector<uint8_t>,  // Binary
    std::vector<float>     // Vectors
>;

using Document = std::map<std::string, Value>;
using ResultSet = std::vector<Document>;
```

**Status:**
```cpp
enum class ErrorCode {
    OK, CONNECTION_FAILED, OPERATION_FAILED,
    NOT_FOUND, INVALID_ARGUMENT, ...
};

struct Status {
    ErrorCode code;
    std::string message;
    bool ok() const;
};
```

### 3.3 PIMPL Pattern

Use the PIMPL (Pointer to Implementation) pattern to hide database-specific dependencies:

```cpp
// Header (mydb_adapter.hpp)
class MyDBAdapter : public DatabaseAdapter {
public:
    Status connect(const ConnectionConfig& config) override;
    // ... other methods
    
private:
    class Impl;  // Forward declaration
    std::unique_ptr<Impl> impl_;
};

// Implementation (mydb_adapter.cpp)
class MyDBAdapter::Impl {
public:
    // Database-specific members
    NativeDBConnection* connection;
    std::mutex mutex;
    
    Status connect_impl(const ConnectionConfig& config) {
        // Implementation details
    }
};

Status MyDBAdapter::connect(const ConnectionConfig& config) {
    return impl_->connect_impl(config);
}
```

---

## 4. Step-by-Step Implementation Guide

### Step 1: Set Up Your Adapter Class

Create a new header file based on the template:

```cpp
// mydb_adapter.hpp
#pragma once
#include "chimera/database_adapter.hpp"

namespace chimera {
namespace adapters {

class MyDBAdapter : public DatabaseAdapter {
public:
    MyDBAdapter();
    ~MyDBAdapter() override;
    
    // Implement all virtual methods...
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

std::unique_ptr<DatabaseAdapter> create_mydb_adapter();

} // namespace adapters
} // namespace chimera
```

### Step 2: Implement Connection Management

Start with the connection methods:

```cpp
// mydb_adapter.cpp
#include "mydb_adapter.hpp"
#include <mydb_native_client.h>  // Your DB's client library

class MyDBAdapter::Impl {
public:
    MyDB::Connection* connection = nullptr;
    bool connected = false;
    
    Status connect(const ConnectionConfig& config) {
        try {
            std::string conn_str = build_connection_string(config);
            connection = MyDB::connect(conn_str);
            connected = true;
            return Status(ErrorCode::OK);
        } catch (const MyDB::Exception& e) {
            return Status(ErrorCode::CONNECTION_FAILED, e.what());
        }
    }
    
    Status disconnect() {
        if (connection) {
            MyDB::disconnect(connection);
            connection = nullptr;
            connected = false;
        }
        return Status(ErrorCode::OK);
    }
};

Status MyDBAdapter::connect(const ConnectionConfig& config) {
    return impl_->connect(config);
}
```

### Step 3: Implement Basic CRUD Operations

Implement insert, find, update, and delete:

```cpp
Status MyDBAdapter::insert(const std::string& collection,
                           const Document& document,
                           const QueryOptions& options) {
    if (!is_connected()) {
        return Status(ErrorCode::CONNECTION_FAILED, "Not connected");
    }
    
    try {
        // Convert Document to native format
        auto native_doc = convert_to_native(document);
        
        // Execute native insert
        impl_->connection->insert(collection, native_doc);
        
        return Status(ErrorCode::OK);
    } catch (const MyDB::Exception& e) {
        return Status(ErrorCode::OPERATION_FAILED, e.what());
    }
}

Status MyDBAdapter::find(const std::string& collection,
                         const Document& filter,
                         ResultSet& results,
                         const QueryOptions& options) {
    if (!is_connected()) {
        return Status(ErrorCode::CONNECTION_FAILED, "Not connected");
    }
    
    try {
        // Convert filter to native query
        auto native_query = convert_filter_to_query(filter);
        
        // Execute query
        auto native_results = impl_->connection->query(collection, native_query);
        
        // Convert results back to Document format
        results.clear();
        for (const auto& native_doc : native_results) {
            results.push_back(convert_from_native(native_doc));
        }
        
        // Apply limit/offset if specified
        apply_query_options(results, options);
        
        return Status(ErrorCode::OK);
    } catch (const MyDB::Exception& e) {
        return Status(ErrorCode::OPERATION_FAILED, e.what());
    }
}
```

### Step 4: Implement Vector Operations (Optional)

If your database supports vector similarity search:

```cpp
bool MyDBAdapter::supports_vector_search() const {
    // Check if vector extension is available
    return check_vector_extension_availability();
}

Status MyDBAdapter::vector_search(const std::string& collection,
                                  const std::string& vector_field,
                                  const VectorSearchParams& params,
                                  ResultSet& results) {
    if (!supports_vector_search()) {
        return Status(ErrorCode::UNSUPPORTED_OPERATION, 
                     "Vector search not supported");
    }
    
    try {
        // Convert metric type to native format
        auto native_metric = convert_metric(params.metric);
        
        // Perform vector search
        auto native_results = impl_->connection->vector_search(
            collection,
            vector_field,
            params.query_vector,
            params.k,
            native_metric
        );
        
        // Convert and return results
        results = convert_results(native_results);
        return Status(ErrorCode::OK);
    } catch (const MyDB::Exception& e) {
        return Status(ErrorCode::OPERATION_FAILED, e.what());
    }
}
```

### Step 5: Implement Transaction Support (Optional)

If your database supports transactions:

```cpp
class MyDBTransaction : public Transaction {
public:
    explicit MyDBTransaction(MyDB::Transaction* txn)
        : txn_(txn), active_(true) {}
    
    Status commit() override {
        if (!active_) return Status(ErrorCode::OPERATION_FAILED, "Not active");
        try {
            txn_->commit();
            active_ = false;
            return Status(ErrorCode::OK);
        } catch (...) {
            return Status(ErrorCode::OPERATION_FAILED, "Commit failed");
        }
    }
    
    Status rollback() override {
        if (!active_) return Status(ErrorCode::OK);
        try {
            txn_->rollback();
            active_ = false;
            return Status(ErrorCode::OK);
        } catch (...) {
            return Status(ErrorCode::OPERATION_FAILED, "Rollback failed");
        }
    }
    
    bool is_active() const override { return active_; }
    
private:
    MyDB::Transaction* txn_;
    bool active_;
};

bool MyDBAdapter::supports_transactions() const {
    return true;  // If your DB supports transactions
}

Status MyDBAdapter::begin_transaction(
    std::unique_ptr<Transaction>& transaction,
    IsolationLevel level) {
    
    try {
        auto native_txn = impl_->connection->begin_transaction();
        transaction = std::make_unique<MyDBTransaction>(native_txn);
        return Status(ErrorCode::OK);
    } catch (...) {
        return Status(ErrorCode::OPERATION_FAILED, "Failed to begin transaction");
    }
}
```

### Step 6: Implement Metadata Methods

Provide information about your adapter:

```cpp
std::string MyDBAdapter::get_adapter_info() const {
    return "MyDBAdapter v1.0.0 - CHIMERA Suite";
}

std::string MyDBAdapter::get_database_version() const {
    if (!is_connected()) return "Unknown";
    return impl_->connection->get_version();
}

std::vector<std::string> MyDBAdapter::get_capabilities() const {
    std::vector<std::string> caps = {
        "crud",
        "queries"
    };
    
    if (supports_transactions()) {
        caps.push_back("transactions");
    }
    
    if (supports_vector_search()) {
        caps.push_back("vector_search");
    }
    
    return caps;
}
```

### Step 7: Add Factory Function

Create a factory function for easy instantiation:

```cpp
std::unique_ptr<DatabaseAdapter> create_mydb_adapter() {
    return std::make_unique<MyDBAdapter>();
}
```

---

## 5. Building and Testing

### 5.1 CMake Integration

Add your adapter to the build system:

```cmake
# CMakeLists.txt in adapters/chimera/
add_library(chimera_mydb_adapter
    mydb_adapter.cpp
)

target_link_libraries(chimera_mydb_adapter
    PUBLIC chimera_interface
    PRIVATE mydb_client_library
)

target_include_directories(chimera_mydb_adapter
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
```

### 5.2 Writing Tests

Create comprehensive tests:

```cpp
// mydb_adapter_test.cpp
#include <gtest/gtest.h>
#include "chimera/mydb_adapter.hpp"

class MyDBAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter = create_mydb_adapter();
        
        ConnectionConfig config;
        config.host = "localhost";
        config.port = 5432;
        config.database = "test_db";
        
        auto status = adapter->connect(config);
        ASSERT_TRUE(status.ok()) << status.message;
    }
    
    void TearDown() override {
        adapter->disconnect();
    }
    
    std::unique_ptr<DatabaseAdapter> adapter;
};

TEST_F(MyDBAdapterTest, BasicInsert) {
    Document doc;
    doc["id"] = std::string("test_1");
    doc["name"] = std::string("Test");
    doc["value"] = int64_t(42);
    
    auto status = adapter->insert("test_collection", doc);
    EXPECT_TRUE(status.ok()) << status.message;
}

TEST_F(MyDBAdapterTest, FindById) {
    Document result;
    auto status = adapter->find_by_id("test_collection", "test_1", result);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(std::get<std::string>(result["name"]), "Test");
    EXPECT_EQ(std::get<int64_t>(result["value"]), 42);
}

TEST_F(MyDBAdapterTest, VectorSearch) {
    if (!adapter->supports_vector_search()) {
        GTEST_SKIP() << "Vector search not supported";
    }
    
    VectorSearchParams params;
    params.query_vector = {0.1f, 0.2f, 0.3f};
    params.k = 10;
    params.metric = "cosine";
    
    ResultSet results;
    auto status = adapter->vector_search(
        "embeddings_collection",
        "vector_field",
        params,
        results
    );
    
    EXPECT_TRUE(status.ok());
    EXPECT_LE(results.size(), 10);
}
```

### 5.3 Running Tests

```bash
# Build with tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Run adapter tests
./build/tests/chimera_mydb_adapter_test
```

---

## 6. Reference Implementations

### 6.1 ThemisDB Adapter

**File:** `adapters/chimera/themisdb_adapter.hpp`

**Features:**
- Native multi-model support (relational, graph, document, vector)
- Full ACID transaction support with MVCC
- Native vector search with GPU acceleration
- Complete API coverage

**Key Mappings:**
- Collections → ThemisDB tables
- Documents → ThemisDB rows (using JSON or structured format)
- Vector search → Native ThemisDB vector index operations

### 6.2 PostgreSQL + pgvector Adapter

**File:** `adapters/chimera/postgresql_adapter.hpp`

**Features:**
- Standard SQL operations via libpqxx
- Optional pgvector extension for vector operations
- Full transaction support
- Feature detection for pgvector availability

**Key Mappings:**
- Collections → PostgreSQL tables
- Documents → JSONB columns
- Vector search → pgvector operators (<->, <#>, <=>)

**Special Considerations:**
- Automatically detects if pgvector extension is installed
- Falls back gracefully if vector operations are unavailable
- Uses parameterized queries for SQL injection prevention

### 6.3 Weaviate Adapter

**File:** `adapters/chimera/weaviate_adapter.hpp`

**Features:**
- REST/GraphQL API client
- Native vector search capabilities
- Schema-on-write
- No transaction support (NoSQL database)

**Key Mappings:**
- Collections → Weaviate classes
- Documents → Weaviate objects
- Vector search → Native Weaviate nearVector queries

**Special Considerations:**
- Uses GraphQL for complex queries
- REST API for CRUD operations
- Automatic vectorization (if configured)

### 6.4 Qdrant Adapter

**File:** `adapters/chimera/qdrant_adapter.hpp`

**Features:**
- REST/gRPC API client
- Native HNSW vector similarity search
- Payload (document) storage alongside vectors
- Metadata filtering during vector search

**Key Mappings:**
- Collections → Qdrant collections
- Documents → Qdrant point payloads
- Vector search → Native HNSW nearest-neighbor queries

**Special Considerations:**
- No ACID transaction support
- Supports both http:// and https:// schemes
- API key passed via the `api_key` connection option

### 6.5 Pinecone Adapter

**File:** `adapters/chimera/pinecone_adapter.hpp`

**Features:**
- Managed serverless vector search (Pinecone v3)
- REST API client
- Cosine similarity search (default metric)
- Metadata filtering during vector search
- Batch upsert operations

**Key Mappings:**
- Collections → Pinecone namespaces
- Vector metadata → Pinecone sparse metadata
- Vector search → Managed ANN queries

**Special Considerations:**
- Requires **HTTPS** connections only (plain http is rejected)
- No ACID transaction support
- No standalone document store (metadata stored alongside vectors)
- API key passed via the `api_key` connection option; never surfaced in system info

---

## 7. Best Practices

### 7.1 Error Handling

- **Always return Status:** Never throw exceptions from adapter methods
- **Provide context:** Include meaningful error messages
- **Map errors correctly:** Convert native errors to appropriate ErrorCodes

```cpp
try {
    // Native operation
} catch (const NativeException& e) {
    if (e.is_connection_error()) {
        return Status(ErrorCode::CONNECTION_FAILED, e.what());
    } else if (e.is_not_found()) {
        return Status(ErrorCode::NOT_FOUND, e.what());
    } else {
        return Status(ErrorCode::OPERATION_FAILED, e.what());
    }
}
```

### 7.2 Thread Safety

- **Use mutexes:** Protect shared state in multi-threaded environments
- **Connection pooling:** Consider implementing connection pools
- **Document thread-safety:** Clearly document thread-safety guarantees

### 7.3 Resource Management

- **RAII:** Use RAII for automatic resource cleanup
- **Clean up in destructor:** Ensure disconnect() is called
- **Handle partial failures:** Clean up on error paths

### 7.4 Performance

- **Batch operations:** Use native batch APIs when available
- **Connection reuse:** Don't recreate connections unnecessarily
- **Prepared statements:** Use prepared statements for repeated queries
- **Avoid unnecessary conversions:** Minimize data format conversions

### 7.5 Documentation

- **Document limitations:** Clearly state what's not supported
- **Configuration examples:** Provide working configuration samples
- **Version compatibility:** Document supported database versions

---

## 8. Troubleshooting

### Common Issues

**Problem:** Compilation errors about missing types
**Solution:** Include `chimera/database_adapter.hpp` and ensure C++17 mode

**Problem:** Linker errors about undefined references
**Solution:** Link against the database client library in CMake

**Problem:** Tests fail with connection errors
**Solution:** Ensure database is running and accessible; check credentials

**Problem:** Vector operations not working
**Solution:** Verify vector extension is installed; check `supports_vector_search()`

---

## 9. Contributing

### 9.1 Submission Checklist

Before submitting your adapter:

- [ ] Compiles against `chimera/database_adapter.hpp`
- [ ] No vendor-specific branding in code or documentation
- [ ] All virtual methods implemented (or return UNSUPPORTED_OPERATION)
- [ ] Comprehensive error handling
- [ ] Unit tests with >80% coverage
- [ ] Documentation of special features or limitations
- [ ] CMake integration
- [ ] Example configuration file
- [ ] License header (Apache-2.0 OR MIT)

### 9.2 Code Review Criteria

Reviewers will check:
- Vendor neutrality (no branding)
- Code quality and style
- Error handling completeness
- Test coverage
- Documentation clarity
- Performance considerations

### 9.3 Licensing

All adapters must be dual-licensed under:
- **Apache License 2.0** OR
- **MIT License**

This ensures maximum reusability and compatibility.

---

## Appendix: Quick Reference

### Essential Methods (Minimum Implementation)

```cpp
// Connection
connect()
disconnect()
is_connected()

// Basic CRUD
insert()
find()
find_by_id()
update()
remove()
count()

// Metadata
get_adapter_info()
get_database_version()
get_capabilities()

// Feature flags
supports_vector_search()
supports_transactions()
```

### Optional Methods

```cpp
// Vector operations
vector_search()
create_vector_index()

// Transactions
begin_transaction()

// Batch operations
insert_batch()

// Native queries
execute_query()
execute_query_params()

// Schema
create_collection()
drop_collection()
list_collections()
```

---

**For more information:**
- API Reference: `docs/chimera/API_REFERENCE.md`
- Example configurations: `benchmarks/chimera/benchmark_config_schema.yaml`
- CHIMERA Suite documentation: `docs/benchmarks/CHIMERA_*.md`

**Questions or issues?**
- Open an issue on GitHub
- Contact: benchmarks@example.org
