# CHIMERA Suite: Architecture Interface Documentation

## Table of Contents
1. [Overview](#overview)
2. [Interface Neutrality](#interface-neutrality)
3. [Architecture](#architecture)
4. [API Reference](#api-reference)
5. [Implementing Custom Adapters](#implementing-custom-adapters)
6. [UML Diagrams](#uml-diagrams)
7. [Examples](#examples)
8. [Standards Compliance](#standards-compliance)

---

## Overview

The CHIMERA Suite provides a **strictly vendor-neutral** adapter architecture for integrating arbitrary hybrid database systems into benchmark workloads. This architecture enables fair, scientific comparison of database systems without vendor bias.

### Design Goals

- **Complete Vendor Neutrality**: No system-specific names, colors, or concepts in interfaces
- **Scientific Rigor**: IEEE-compliant design and documentation
- **Extensibility**: Easy integration of new database systems
- **Capability Discovery**: Explicit querying of supported features
- **Type Safety**: Strong typing with modern C++ features
- **Error Handling**: Comprehensive error reporting without exceptions

### Key Features

✓ **Multi-Model Support**: Relational, Vector, Graph, Document, and Transaction interfaces  
✓ **Factory Pattern**: Dynamic adapter registration and instantiation  
✓ **Capability Querying**: Runtime discovery of supported features  
✓ **Generic Data Types**: System-agnostic data structures  
✓ **Cross-Platform**: Linux, macOS, Windows compatibility  

---

## Interface Neutrality

### Neutrality Guarantees

The CHIMERA adapter architecture strictly enforces vendor neutrality through:

#### 1. **No Vendor-Specific Naming**
- All interface methods use generic terminology
- No references to specific products or vendors
- Example: `execute_query()` instead of `runPostgreSQLQuery()`

#### 2. **No Internal System Concepts**
- Interfaces expose only generic abstractions
- No assumptions about internal implementations
- Systems report capabilities, not implementation details

#### 3. **No Brand Colors or Identifiers**
- Results sorted alphabetically or by metric value only
- No color coding by vendor
- No logo or brand references in reports

#### 4. **Explicit Capability Declaration**
```cpp
// Systems declare what they support
bool has_capability(Capability cap);
std::vector<Capability> get_capabilities();
```

#### 5. **Generic Error Handling**
```cpp
enum class ErrorCode {
    SUCCESS,
    NOT_IMPLEMENTED,  // Feature not supported
    INVALID_ARGUMENT,
    // ... standard error codes
};
```

### Neutrality Verification

All adapter implementations undergo neutrality review to ensure:
- No vendor bias in method implementations
- Fair reporting of capabilities
- Consistent error handling
- Generic metric reporting

---

## Architecture

### Component Hierarchy

```
IDatabaseAdapter (root interface)
├── IRelationalAdapter     - SQL/relational operations
├── IVectorAdapter         - Vector similarity search
├── IGraphAdapter          - Graph traversal and analytics
├── IDocumentAdapter       - Document store operations
├── ITransactionAdapter    - ACID transaction management
└── ISystemInfoAdapter     - System info and metrics
```

### Core Components

#### 1. **Abstract Interfaces**
- Define generic operations for each data model
- Return `Result<T>` types for error handling
- Accept only system-agnostic parameters

#### 2. **AdapterFactory**
- Factory Pattern for creating adapters
- Runtime registration of new adapters
- Query supported systems

#### 3. **Data Structures**
- `Vector`: Generic vector/embedding
- `Document`: Schema-flexible document
- `GraphNode`/`GraphEdge`: Graph elements
- `RelationalRow`/`RelationalTable`: Relational data

#### 4. **Result Types**
- `Result<T>`: Success value or error
- `ErrorCode`: Standard error enumeration
- No exceptions for better benchmarking

---

## API Reference

### AdapterFactory

**Purpose**: Create and manage database adapters

#### Methods

```cpp
// Create an adapter instance
static std::unique_ptr<IDatabaseAdapter> create(const std::string& system_name);

// Register a new adapter type
static bool register_adapter(const std::string& system_name, 
                            AdapterCreator creator);

// Get list of supported systems (alphabetically sorted)
static std::vector<std::string> get_supported_systems();

// Check if a system is supported
static bool is_supported(const std::string& system_name);
```

#### Example Usage

```cpp
// Register ThemisDB adapter
AdapterFactory::register_adapter("ThemisDB", 
    []() { return std::make_unique<ThemisDBAdapter>(); });

// Create adapter
auto adapter = AdapterFactory::create("ThemisDB");
if (!adapter) {
    std::cerr << "System not supported\n";
    return;
}

// Connect to database
auto result = adapter->connect("themisdb://localhost:7777");
if (result.is_err()) {
    std::cerr << "Connection failed: " << result.error_message << "\n";
    return;
}
```

---

### IRelationalAdapter

**Purpose**: SQL-like relational operations

#### Methods

```cpp
// Execute a query
Result<RelationalTable> execute_query(
    const std::string& query,
    const std::vector<Scalar>& params = {});

// Insert a single row
Result<size_t> insert_row(
    const std::string& table_name,
    const RelationalRow& row);

// Batch insert rows
Result<size_t> batch_insert(
    const std::string& table_name,
    const std::vector<RelationalRow>& rows);

// Get query statistics
Result<QueryStatistics> get_query_statistics();
```

#### Example

```cpp
// Execute a query
auto result = adapter->execute_query(
    "SELECT * FROM users WHERE age > ?",
    {Scalar{int64_t{18}}}
);

if (result.is_ok()) {
    auto table = result.value.value();
    std::cout << "Returned " << table.rows.size() << " rows\n";
}

// Batch insert
std::vector<RelationalRow> rows;
RelationalRow row1;
row1.columns["name"] = Scalar{"Alice"};
row1.columns["age"] = Scalar{int64_t{30}};
rows.push_back(row1);

auto insert_result = adapter->batch_insert("users", rows);
```

---

### IVectorAdapter

**Purpose**: Vector similarity search for embeddings

#### Methods

```cpp
// Insert a vector
Result<std::string> insert_vector(
    const std::string& collection,
    const Vector& vector);

// Batch insert vectors
Result<size_t> batch_insert_vectors(
    const std::string& collection,
    const std::vector<Vector>& vectors);

// Search for similar vectors
Result<std::vector<std::pair<Vector, double>>> search_vectors(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& filters = {});

// Create vector index
Result<bool> create_index(
    const std::string& collection,
    size_t dimensions,
    const std::map<std::string, Scalar>& index_params = {});
```

#### Example

```cpp
// Create vector
Vector vec;
vec.data = {0.1, 0.2, 0.3, 0.4};  // 4D vector
vec.metadata["category"] = Scalar{"products"};

// Insert vector
auto id_result = adapter->insert_vector("embeddings", vec);

// Search for similar vectors
Vector query_vec;
query_vec.data = {0.15, 0.25, 0.35, 0.45};

auto search_result = adapter->search_vectors("embeddings", query_vec, 10);
if (search_result.is_ok()) {
    for (const auto& [similar_vec, distance] : search_result.value.value()) {
        std::cout << "Distance: " << distance << "\n";
    }
}
```

---

### IGraphAdapter

**Purpose**: Graph traversal and pattern matching

#### Methods

```cpp
// Insert graph node
Result<std::string> insert_node(const GraphNode& node);

// Insert graph edge
Result<std::string> insert_edge(const GraphEdge& edge);

// Find shortest path
Result<GraphPath> shortest_path(
    const std::string& source_id,
    const std::string& target_id,
    size_t max_depth = 10);

// Traverse graph
Result<std::vector<GraphNode>> traverse(
    const std::string& start_id,
    size_t max_depth,
    const std::vector<std::string>& edge_labels = {});

// Execute graph query (Cypher/Gremlin/etc)
Result<std::vector<GraphPath>> execute_graph_query(
    const std::string& query,
    const std::map<std::string, Scalar>& params = {});
```

#### Example

```cpp
// Create nodes
GraphNode person1;
person1.id = "person_1";
person1.label = "Person";
person1.properties["name"] = Scalar{"Alice"};

GraphNode person2;
person2.id = "person_2";
person2.label = "Person";
person2.properties["name"] = Scalar{"Bob"};

adapter->insert_node(person1);
adapter->insert_node(person2);

// Create edge
GraphEdge friendship;
friendship.source_id = "person_1";
friendship.target_id = "person_2";
friendship.label = "KNOWS";
friendship.weight = 1.0;

adapter->insert_edge(friendship);

// Find shortest path
auto path_result = adapter->shortest_path("person_1", "person_2");
```

---

### IDocumentAdapter

**Purpose**: Schema-flexible document storage

#### Methods

```cpp
// Insert document
Result<std::string> insert_document(
    const std::string& collection,
    const Document& doc);

// Batch insert documents
Result<size_t> batch_insert_documents(
    const std::string& collection,
    const std::vector<Document>& docs);

// Find documents
Result<std::vector<Document>> find_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    size_t limit = 100);

// Update documents
Result<size_t> update_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    const std::map<std::string, Scalar>& updates);
```

#### Example

```cpp
// Create document
Document doc;
doc.id = "doc_123";
doc.fields["title"] = Scalar{"CHIMERA Documentation"};
doc.fields["author"] = Scalar{"System Team"};
doc.fields["views"] = Scalar{int64_t{1000}};

// Insert
auto insert_result = adapter->insert_document("articles", doc);

// Find documents
std::map<std::string, Scalar> filter;
filter["author"] = Scalar{"System Team"};

auto find_result = adapter->find_documents("articles", filter, 10);
```

---

### ITransactionAdapter

**Purpose**: ACID transaction management

#### Methods

```cpp
// Begin transaction
Result<std::string> begin_transaction(
    const TransactionOptions& options = {});

// Commit transaction
Result<bool> commit_transaction(const std::string& transaction_id);

// Rollback transaction
Result<bool> rollback_transaction(const std::string& transaction_id);
```

#### Example

```cpp
// Configure transaction
TransactionOptions opts;
opts.isolation_level = TransactionOptions::IsolationLevel::SERIALIZABLE;
opts.timeout = std::chrono::milliseconds(5000);

// Begin transaction
auto txn_result = adapter->begin_transaction(opts);
if (!txn_result.is_ok()) {
    return;
}

std::string txn_id = txn_result.value.value();

// Perform operations...
// If success:
adapter->commit_transaction(txn_id);

// If error:
adapter->rollback_transaction(txn_id);
```

---

### ISystemInfoAdapter

**Purpose**: System metadata and capability discovery

#### Methods

```cpp
// Get system information
Result<SystemInfo> get_system_info();

// Get runtime metrics
Result<SystemMetrics> get_metrics();

// Check capability support
bool has_capability(Capability cap);

// Get all capabilities
std::vector<Capability> get_capabilities();
```

#### Example

```cpp
// Check capabilities
if (adapter->has_capability(Capability::VECTOR_SEARCH)) {
    std::cout << "Vector search supported\n";
}

// Get all capabilities
auto caps = adapter->get_capabilities();
for (auto cap : caps) {
    // Process capabilities
}

// Get system info
auto info_result = adapter->get_system_info();
if (info_result.is_ok()) {
    auto info = info_result.value.value();
    std::cout << "System: " << info.system_name << " " 
              << info.version << "\n";
}

// Get metrics
auto metrics_result = adapter->get_metrics();
if (metrics_result.is_ok()) {
    auto metrics = metrics_result.value.value();
    std::cout << "Memory used: " 
              << metrics.memory.used_bytes << " bytes\n";
}
```

---

## Implementing Custom Adapters

### Step-by-Step Guide

#### 1. Create Adapter Header

```cpp
#ifndef MY_DATABASE_ADAPTER_HPP
#define MY_DATABASE_ADAPTER_HPP

#include "chimera/database_adapter.hpp"

namespace chimera {

class MyDatabaseAdapter : public IDatabaseAdapter {
public:
    // Implement all pure virtual methods
    // Return NOT_IMPLEMENTED for unsupported features
};

} // namespace chimera

#endif
```

#### 2. Implement Required Methods

```cpp
#include "my_database_adapter.hpp"

namespace chimera {

Result<bool> MyDatabaseAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& options
) {
    // Your connection logic here
    // Return Result<bool>::ok(true) on success
    // Return Result<bool>::err(...) on failure
}

// Implement other methods...

} // namespace chimera
```

#### 3. Register Adapter

```cpp
// In your initialization code
#include "my_database_adapter.hpp"

void register_my_adapter() {
    AdapterFactory::register_adapter("MyDatabase",
        []() { return std::make_unique<MyDatabaseAdapter>(); });
}
```

#### 4. Declare Capabilities

```cpp
bool MyDatabaseAdapter::has_capability(Capability cap) {
    switch (cap) {
        case Capability::RELATIONAL_QUERIES:
            return true;  // Supported
        case Capability::VECTOR_SEARCH:
            return false; // Not supported
        // ... handle all capabilities
        default:
            return false;
    }
}

std::vector<Capability> MyDatabaseAdapter::get_capabilities() {
    return {
        Capability::RELATIONAL_QUERIES,
        Capability::TRANSACTIONS,
        // List only supported capabilities
    };
}
```

#### 5. Handle Unsupported Features

```cpp
Result<std::string> MyDatabaseAdapter::insert_vector(
    const std::string& collection,
    const Vector& vector
) {
    // If vector search is not supported
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector search not supported by MyDatabase"
    );
}
```

---

## UML Diagrams

### Class Hierarchy

```
┌─────────────────────────────┐
│    IDatabaseAdapter         │
│  (Pure Virtual Interface)   │
└──────────┬──────────────────┘
           │
           ├──────────────────────────┐
           │                          │
┌──────────▼──────────┐    ┌─────────▼──────────┐
│ IRelationalAdapter  │    │   IVectorAdapter   │
│  - execute_query    │    │  - insert_vector   │
│  - insert_row       │    │  - search_vectors  │
│  - batch_insert     │    │  - create_index    │
└─────────────────────┘    └────────────────────┘
           │                          │
           ├──────────────────────────┤
           │                          │
┌──────────▼──────────┐    ┌─────────▼──────────┐
│   IGraphAdapter     │    │  IDocumentAdapter  │
│  - insert_node      │    │  - insert_document │
│  - insert_edge      │    │  - find_documents  │
│  - shortest_path    │    │  - update_docs     │
└─────────────────────┘    └────────────────────┘
           │                          │
           ├──────────────────────────┤
           │                          │
┌──────────▼───────────┐   ┌─────────▼──────────┐
│ ITransactionAdapter  │   │ ISystemInfoAdapter │
│  - begin_transaction │   │  - get_system_info │
│  - commit            │   │  - get_metrics     │
│  - rollback          │   │  - has_capability  │
└──────────────────────┘   └────────────────────┘
```

### Factory Pattern

```
┌────────────────────────┐
│   AdapterFactory       │
│                        │
│  + create(name)        │───────┐
│  + register_adapter()  │       │
│  + get_supported()     │       │
└────────────────────────┘       │
                                 │ Creates
                                 ▼
                    ┌────────────────────────┐
                    │  IDatabaseAdapter      │
                    │                        │
                    └────────┬───────────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
   ┌──────────▼─────┐ ┌─────▼────────┐ ┌──▼──────────┐
   │ ThemisDBAdapter│ │PostgresAdapter│ │WeaviateAdptr│
   └────────────────┘ └──────────────┘ └─────────────┘
```

### Data Flow

```
┌──────────┐    create()    ┌──────────────┐
│Benchmark │───────────────>│AdapterFactory│
│  Suite   │                └──────┬───────┘
└────┬─────┘                       │
     │                             │ creates
     │                             ▼
     │                    ┌─────────────────┐
     │                    │   Adapter       │
     │   connect()        │   Instance      │
     ├───────────────────>│                 │
     │                    └────────┬────────┘
     │                             │
     │   execute_query()           │
     ├─────────────────────────────┤
     │                             │
     │<────────────────────────────┤
     │   Result<RelationalTable>   │
     │                             │
```

---

## Examples

### Complete Benchmark Example

```cpp
#include "chimera/database_adapter.hpp"
#include <iostream>
#include <chrono>

int main() {
    // Create adapter
    auto adapter = chimera::AdapterFactory::create("ThemisDB");
    if (!adapter) {
        std::cerr << "System not supported\n";
        return 1;
    }
    
    // Connect
    auto conn_result = adapter->connect("themisdb://localhost:7777");
    if (conn_result.is_err()) {
        std::cerr << "Connection failed\n";
        return 1;
    }
    
    // Check capabilities
    if (!adapter->has_capability(chimera::Capability::VECTOR_SEARCH)) {
        std::cerr << "Vector search not supported\n";
        return 1;
    }
    
    // Create vector index
    adapter->create_index("embeddings", 128);
    
    // Benchmark: Insert vectors
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<chimera::Vector> vectors;
    for (int i = 0; i < 10000; ++i) {
        chimera::Vector vec;
        vec.data.resize(128);
        // Fill with random data...
        vectors.push_back(vec);
    }
    
    auto insert_result = adapter->batch_insert_vectors("embeddings", vectors);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    );
    
    std::cout << "Inserted " << vectors.size() << " vectors in " 
              << duration.count() << " ms\n";
    std::cout << "Throughput: " 
              << (vectors.size() * 1000.0 / duration.count()) 
              << " vectors/sec\n";
    
    // Benchmark: Vector search
    chimera::Vector query_vec;
    query_vec.data.resize(128);
    // Fill query vector...
    
    start = std::chrono::high_resolution_clock::now();
    
    auto search_result = adapter->search_vectors("embeddings", query_vec, 10);
    
    end = std::chrono::high_resolution_clock::now();
    auto search_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start
    );
    
    std::cout << "Search latency: " << search_duration.count() << " µs\n";
    
    // Cleanup
    adapter->disconnect();
    
    return 0;
}
```

### Multi-System Comparison

```cpp
#include "chimera/database_adapter.hpp"
#include <vector>
#include <string>

struct BenchmarkResult {
    std::string system_name;
    double insert_throughput;
    double search_latency_ms;
};

std::vector<BenchmarkResult> benchmark_all_systems() {
    std::vector<BenchmarkResult> results;
    
    // Get all supported systems
    auto systems = chimera::AdapterFactory::get_supported_systems();
    
    for (const auto& system_name : systems) {
        auto adapter = chimera::AdapterFactory::create(system_name);
        if (!adapter) continue;
        
        // Connect
        adapter->connect("connection_string_here");
        
        // Run benchmarks...
        BenchmarkResult result;
        result.system_name = system_name;
        // Measure performance...
        
        results.push_back(result);
        
        adapter->disconnect();
    }
    
    // Results are vendor-neutral, sorted alphabetically
    return results;
}
```

---

## Standards Compliance

### IEEE Standards

This architecture complies with:

- **IEEE Std 730-2014**: Software Quality Assurance Processes
- **IEEE Std 1012-2016**: System, Software, and Hardware Verification and Validation
- **IEEE Std 1016-2009**: Software Design Descriptions

### ISO Standards

- **ISO/IEC 9126**: Software Quality Characteristics
- **ISO/IEC 25010**: Systems and software Quality Requirements and Evaluation (SQuaRE)

### Design Principles

1. **Modularity**: Clear separation of concerns
2. **Extensibility**: Easy to add new adapters
3. **Maintainability**: Well-documented, clean code
4. **Portability**: Cross-platform compatibility
5. **Reliability**: Comprehensive error handling
6. **Testability**: Unit test friendly design

### Error Handling Standards

Following POSIX conventions:
- Standard error codes
- No exceptions (for predictable benchmarking)
- Clear error messages
- Result-based error propagation

---

## Conclusion

The CHIMERA adapter architecture provides a scientifically rigorous, vendor-neutral foundation for database benchmarking. By strictly enforcing neutrality at the interface level and providing comprehensive capability discovery, it enables fair comparison of diverse database systems.

For questions or contributions, please refer to the main CHIMERA documentation or open an issue on GitHub.

---

**Document Version**: 1.0.0  
**Last Updated**: 2026-04-06  
**Authors**: CHIMERA Development Team  
**License**: MIT
