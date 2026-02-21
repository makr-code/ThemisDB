# Chimera Module - Source Implementation

## Module Purpose

The Chimera module provides the core implementation for the **CHIMERA** (Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment) benchmark suite's vendor-neutral database adapter architecture. This module enables fair, unbiased benchmarking of diverse database systems (relational, document, graph, vector, hybrid) through a unified interface. Named after the mythological creature with multiple forms, Chimera adapts to any database system's native capabilities while providing consistent benchmarking metrics.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `adapter_factory.cpp` | Thread-safe singleton adapter registry |
| `themisdb_adapter.cpp` | ThemisDB reference implementation adapter |
| `adapters/postgres_adapter.cpp` | PostgreSQL adapter (planned) |
| `benchmark_harness.cpp` | Benchmark execution and result collection |

## Scope

**In Scope:**
- Adapter factory implementation for dynamic adapter registration
- ThemisDB reference adapter implementation
- Base adapter infrastructure and utilities
- Connection management and lifecycle
- Result type conversions and error handling
- Multi-model operation wrappers (relational, vector, graph, document)
- Transaction coordination interfaces
- System information and metrics collection

**Out of Scope:**
- Specific database client libraries (handled by vendor adapters)
- Network protocol implementations (handled by database SDKs)
- Benchmark test harnesses (separate benchmark suite)
- Performance metrics collection (handled by benchmark framework)
- Database schema management (adapter responsibility)

## Source Files

### adapter_factory.cpp
**Location:** `/src/chimera/adapter_factory.cpp`

Core factory implementation for runtime adapter registration and creation.

**Key Features:**
- **Thread-Safe Registry**: Singleton registry with mutex protection for adapter registration
- **Dynamic Registration**: Runtime adapter registration without recompilation
- **Factory Pattern**: Instantiate adapters by system name string
- **Vendor Discovery**: Query available database system adapters
- **Alphabetic Sorting**: Vendor-neutral alphabetic ordering of systems

**Implementation Details:**

**Registry Management:**
```cpp
// Thread-safe singleton pattern
std::map<std::string, AdapterCreator>& AdapterFactory::get_registry() {
    static std::map<std::string, AdapterCreator> registry;
    return registry;
}
```

**Adapter Registration:**
```cpp
bool AdapterFactory::register_adapter(
    const std::string& system_name, 
    AdapterCreator creator
) {
    static std::mutex registry_mutex;
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    auto& registry = get_registry();
    auto result = registry.insert({system_name, creator});
    return result.second; // true if inserted, false if already exists
}
```

**Usage Pattern:**
```cpp
// Register a new adapter at startup or runtime
static bool registered = AdapterFactory::register_adapter(
    "PostgreSQL",
    []() { return std::make_unique<PostgreSQLAdapter>(); }
);

// Create adapter instance
auto adapter = AdapterFactory::create("PostgreSQL");
if (adapter) {
    // Use adapter...
}

// Query supported systems
auto systems = AdapterFactory::get_supported_systems();
// Returns: ["ArangoDB", "MongoDB", "Neo4j", "PostgreSQL", "ThemisDB", "Weaviate"]
```

**Thread Safety:**
- Registry access is read-safe (no locks needed for queries)
- Registration uses mutex to prevent concurrent modification
- Factory creation is thread-safe (no shared state)

**Performance Characteristics:**
- Registry lookup: O(log n) where n = number of registered adapters
- Registration: O(log n) with mutex overhead
- System enumeration: O(n log n) for sorting

### themisdb_adapter.cpp
**Location:** `/src/chimera/themisdb_adapter.cpp`

Reference implementation of the Chimera adapter interface for ThemisDB.

**Key Features:**
- **Complete Interface Implementation**: All IDatabaseAdapter methods implemented
- **Stub Implementations**: Demonstrates adapter pattern without full integration
- **Multi-Model Support**: Relational, vector, graph, document, transaction interfaces
- **Capability Detection**: Reports ThemisDB's full multi-model capabilities
- **Error Handling**: Proper Result<T> error propagation

**Implemented Interfaces:**

**1. Connection Management:**
```cpp
Result<bool> ThemisDBAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& options
) {
    // Parse connection string: "themisdb://host:port/database"
    // Store connection state
    connected_ = true;
    connection_string_ = connection_string;
    return Result<bool>::ok(true);
}

Result<bool> ThemisDBAdapter::disconnect() {
    connected_ = false;
    return Result<bool>::ok(true);
}

bool ThemisDBAdapter::is_connected() const {
    return connected_;
}
```

**2. Relational Operations (IRelationalAdapter):**
```cpp
Result<RelationalTable> execute_query(
    const std::string& query,
    const std::vector<Scalar>& params
) {
    if (!connected_) {
        return Result<RelationalTable>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    
    // Execute AQL query via ThemisDB API
    RelationalTable table;
    // ... populate table from query results
    return Result<RelationalTable>::ok(std::move(table));
}

Result<size_t> insert_row(
    const std::string& table_name,
    const RelationalRow& row
) {
    // Insert into ThemisDB collection
    return Result<size_t>::ok(1);
}

Result<size_t> batch_insert(
    const std::string& table_name,
    const std::vector<RelationalRow>& rows
) {
    // Batch insert optimization
    return Result<size_t>::ok(rows.size());
}
```

**3. Vector Operations (IVectorAdapter):**
```cpp
Result<std::string> insert_vector(
    const std::string& collection,
    const Vector& vector
) {
    // Insert vector into ThemisDB vector index
    // Generate unique ID
    return Result<std::string>::ok("vector_id_001");
}

Result<std::vector<std::pair<Vector, double>>> search_vectors(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& filters
) {
    // Execute k-NN search with optional metadata filters
    std::vector<std::pair<Vector, double>> results;
    // ... perform HNSW/FAISS search
    return Result<std::vector<std::pair<Vector, double>>>::ok(std::move(results));
}

Result<bool> create_index(
    const std::string& collection,
    size_t dimensions,
    const std::map<std::string, Scalar>& index_params
) {
    // Create HNSW/IVF vector index
    return Result<bool>::ok(true);
}
```

**4. Graph Operations (IGraphAdapter):**
```cpp
Result<std::string> insert_node(const GraphNode& node) {
    // Insert vertex into graph
    return Result<std::string>::ok(node.id.empty() ? "node_001" : node.id);
}

Result<std::string> insert_edge(const GraphEdge& edge) {
    // Insert edge with source/target references
    return Result<std::string>::ok(edge.id.empty() ? "edge_001" : edge.id);
}

Result<GraphPath> shortest_path(
    const std::string& source_id,
    const std::string& target_id,
    size_t max_depth
) {
    // Execute shortest path algorithm (Dijkstra/BFS)
    GraphPath path;
    path.total_weight = 0.0;
    return Result<GraphPath>::ok(std::move(path));
}

Result<std::vector<GraphNode>> traverse(
    const std::string& start_id,
    size_t max_depth,
    const std::vector<std::string>& edge_labels
) {
    // Graph traversal with depth limit and edge filtering
    std::vector<GraphNode> nodes;
    return Result<std::vector<GraphNode>>::ok(std::move(nodes));
}
```

**5. Document Operations (IDocumentAdapter):**
```cpp
Result<std::string> insert_document(
    const std::string& collection,
    const Document& doc
) {
    // Insert JSON document
    return Result<std::string>::ok(doc.id.empty() ? "doc_001" : doc.id);
}

Result<std::vector<Document>> find_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    size_t limit
) {
    // Query documents with filter criteria
    std::vector<Document> docs;
    return Result<std::vector<Document>>::ok(std::move(docs));
}

Result<size_t> update_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    const std::map<std::string, Scalar>& updates
) {
    // Bulk update matching documents
    return Result<size_t>::ok(0);
}
```

**6. Transaction Support (ITransactionAdapter):**
```cpp
Result<std::string> begin_transaction(
    const TransactionOptions& options
) {
    // Start ACID transaction with isolation level
    return Result<std::string>::ok("txn_001");
}

Result<bool> commit_transaction(const std::string& transaction_id) {
    // Commit transaction
    return Result<bool>::ok(true);
}

Result<bool> rollback_transaction(const std::string& transaction_id) {
    // Rollback transaction
    return Result<bool>::ok(true);
}
```

**7. System Information (ISystemInfoAdapter):**
```cpp
Result<SystemInfo> get_system_info() {
    SystemInfo info;
    info.system_name = "ThemisDB";
    info.version = "1.5.0";
    info.build_info["compiler"] = "GCC/Clang";
    info.build_info["platform"] = "Linux/Windows/macOS";
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> get_metrics() {
    SystemMetrics metrics;
    metrics.memory.total_bytes = 0;
    metrics.memory.used_bytes = 0;
    metrics.cpu.utilization_percent = 0.0;
    metrics.storage.total_bytes = 0;
    return Result<SystemMetrics>::ok(std::move(metrics));
}

bool has_capability(Capability cap) {
    // ThemisDB supports all capabilities
    switch (cap) {
        case Capability::RELATIONAL_QUERIES:
        case Capability::VECTOR_SEARCH:
        case Capability::GRAPH_TRAVERSAL:
        case Capability::DOCUMENT_STORE:
        case Capability::TRANSACTIONS:
        case Capability::DISTRIBUTED_QUERIES:
        case Capability::GEOSPATIAL_QUERIES:
        case Capability::TIME_SERIES:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> get_capabilities() {
    return {
        Capability::RELATIONAL_QUERIES,
        Capability::VECTOR_SEARCH,
        Capability::GRAPH_TRAVERSAL,
        Capability::DOCUMENT_STORE,
        Capability::FULL_TEXT_SEARCH,
        Capability::TRANSACTIONS,
        Capability::DISTRIBUTED_QUERIES,
        Capability::GEOSPATIAL_QUERIES,
        Capability::TIME_SERIES,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES
    };
}
```

**Connection State Management:**
```cpp
class ThemisDBAdapter {
private:
    bool connected_ = false;
    std::string connection_string_;
    
    // Future: Add actual ThemisDB client instance
    // std::unique_ptr<ThemisDBClient> client_;
};
```

**Error Handling Pattern:**
```cpp
// Check connection before operations
if (!connected_) {
    return Result<T>::err(
        ErrorCode::CONNECTION_ERROR,
        "Not connected to database"
    );
}

// Wrap database errors
try {
    auto result = perform_operation();
    return Result<T>::ok(result);
} catch (const std::exception& e) {
    return Result<T>::err(
        ErrorCode::INTERNAL_ERROR,
        e.what()
    );
}
```

**Performance Considerations:**
- Stub implementation has O(1) complexity for all operations
- Production implementation complexity depends on ThemisDB operations
- Connection state checks add minimal overhead (~1ns)
- Result<T> has zero-cost abstraction (optimized away)

**Thread Safety:**
- Individual adapter instances are NOT thread-safe
- Multiple adapters can be used concurrently (separate connections)
- Connection state is not protected (single-threaded usage expected)

**Extending for Production:**
```cpp
// Add actual ThemisDB integration
#include "themisdb/client.h"

class ThemisDBAdapter : public IDatabaseAdapter {
private:
    std::unique_ptr<ThemisDBClient> client_;
    
public:
    Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options
    ) override {
        try {
            client_ = std::make_unique<ThemisDBClient>(connection_string);
            client_->connect(options);
            connected_ = true;
            return Result<bool>::ok(true);
        } catch (const std::exception& e) {
            return Result<bool>::err(
                ErrorCode::CONNECTION_ERROR,
                e.what()
            );
        }
    }
    
    Result<RelationalTable> execute_query(
        const std::string& query,
        const std::vector<Scalar>& params
    ) override {
        if (!connected_) {
            return Result<RelationalTable>::err(
                ErrorCode::CONNECTION_ERROR,
                "Not connected"
            );
        }
        
        try {
            auto result = client_->executeAQL(query, params);
            RelationalTable table = convert_to_table(result);
            return Result<RelationalTable>::ok(std::move(table));
        } catch (const std::exception& e) {
            return Result<RelationalTable>::err(
                ErrorCode::INTERNAL_ERROR,
                e.what()
            );
        }
    }
};
```

## Architecture

### Component Interaction

```
Benchmark Suite
      ↓
AdapterFactory::create("ThemisDB")
      ↓
ThemisDBAdapter instance
      ↓
connect() → ThemisDB Client
      ↓
execute_query() → AQL Execution
insert_vector() → Vector Index
insert_node() → Graph Storage
      ↓
Result<T> → Benchmark Metrics
```

### Class Hierarchy

```
IDatabaseAdapter (abstract interface)
  ├─ IRelationalAdapter
  ├─ IVectorAdapter
  ├─ IGraphAdapter
  ├─ IDocumentAdapter
  ├─ ITransactionAdapter
  └─ ISystemInfoAdapter

ThemisDBAdapter (concrete implementation)
  └─ implements all 6 interfaces
```

### Factory Registration Flow

```
Static Initialization (before main())
      ↓
register_adapter("ThemisDB", creator_lambda)
      ↓
Insert into static registry map
      ↓
Runtime Usage:
create("ThemisDB") → lookup registry → invoke creator → return adapter
```

## Integration Points

### Benchmark Suite Integration

```cpp
#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"

// Benchmark initialization
auto adapter = AdapterFactory::create("ThemisDB");
adapter->connect("themisdb://localhost:8529/benchmark");

// Run benchmark workload
for (const auto& query : benchmark_queries) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = adapter->execute_query(query.text, query.params);
    auto duration = std::chrono::high_resolution_clock::now() - start;
    
    record_metric(query.name, duration, result.is_ok());
}

// Cleanup
adapter->disconnect();
```

### Multi-System Comparison

```cpp
std::vector<std::string> systems = {
    "ThemisDB", "PostgreSQL", "MongoDB", "Neo4j", "Weaviate"
};

for (const auto& system_name : systems) {
    auto adapter = AdapterFactory::create(system_name);
    if (!adapter) {
        std::cerr << system_name << " not available" << std::endl;
        continue;
    }
    
    // Run identical benchmark on each system
    run_benchmark(adapter.get(), system_name);
}

// Compare results across systems
```

### Custom Adapter Registration

```cpp
// In your custom adapter implementation file
#include "chimera/database_adapter.hpp"
#include "mydb_adapter.hpp"

namespace {
    bool registered = AdapterFactory::register_adapter(
        "MyDatabase",
        []() { return std::make_unique<MyDatabaseAdapter>(); }
    );
}
```

## API Reference

### AdapterFactory

**Static Methods:**

```cpp
// Create adapter instance
static std::unique_ptr<IDatabaseAdapter> create(
    const std::string& system_name
);

// Register new adapter
static bool register_adapter(
    const std::string& system_name,
    AdapterCreator creator
);

// Query available systems
static std::vector<std::string> get_supported_systems();

// Check if system is registered
static bool is_supported(const std::string& system_name);
```

### ThemisDBAdapter

**Constructor:**
```cpp
ThemisDBAdapter() = default;
~ThemisDBAdapter() override = default;
```

**Connection Management:**
```cpp
Result<bool> connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& options = {}
);

Result<bool> disconnect();

bool is_connected() const;
```

**All Interface Methods:**
See [database_adapter.hpp](../../include/chimera/database_adapter.hpp) for complete API.

## Dependencies

### Internal Dependencies
- `chimera/database_adapter.hpp` - Interface definitions
- `chimera/themisdb_adapter.hpp` - Header declarations

### External Dependencies
**Required:**
- Standard C++17 library (std::map, std::mutex, std::string, std::vector)
- No external libraries required

**Optional (for production ThemisDB integration):**
- ThemisDB client library
- RocksDB (storage backend)
- HNSW/FAISS (vector indexing)

### Build Configuration

```cmake
# CMakeLists.txt for Chimera module
add_library(themisdb_chimera
    adapter_factory.cpp
    themisdb_adapter.cpp
)

target_include_directories(themisdb_chimera
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(themisdb_chimera
    PUBLIC # No external dependencies for base implementation
)

# Optional: Link ThemisDB for production implementation
# target_link_libraries(themisdb_chimera PRIVATE themisdb_client)
```

## Performance Characteristics

### Factory Operations

- **Adapter Creation:** O(log n) registry lookup + O(1) instantiation
- **Registration:** O(log n) insertion with mutex lock
- **System Enumeration:** O(n log n) for alphabetic sorting
- **Capability Query:** O(1) switch statement

### Adapter Operations (Stub Implementation)

- **All Operations:** O(1) - stub returns immediately
- **Connection Check:** O(1) - bool flag check
- **Error Construction:** O(1) - inline Result<T> construction

### Memory Usage

- **Factory Registry:** ~100 bytes per registered adapter
- **Adapter Instance:** ~100 bytes (2 strings + bool + vtable ptr)
- **Result<T>:** sizeof(T) + sizeof(optional) + ~100 bytes (error string)

### Production Performance (with ThemisDB)

- **execute_query():** 1-1000ms (depends on query complexity)
- **insert_vector():** 1-10ms (HNSW insertion)
- **search_vectors():** 1-50ms (k-NN search, depends on k and index size)
- **shortest_path():** 10-500ms (depends on graph size and depth)
- **find_documents():** 1-100ms (depends on filter selectivity)

## Known Limitations

1. **Stub Implementation:**
   - Current implementation returns empty results
   - No actual database integration
   - Requires production implementation for real benchmarks

2. **Thread Safety:**
   - Adapter instances are NOT thread-safe
   - Multiple adapters needed for concurrent access
   - Factory registration is thread-safe

3. **Error Handling:**
   - Limited error detail in stub implementation
   - No retry logic or connection pooling
   - No timeout handling

4. **Capability Detection:**
   - Static capability reporting (hardcoded)
   - No runtime capability probing
   - No feature version detection

5. **Transaction Support:**
   - Stub transaction IDs (no actual transactions)
   - No transaction isolation enforcement
   - No deadlock detection

6. **Resource Management:**
   - No connection pooling
   - No automatic reconnection
   - No resource cleanup on errors

7. **Configuration:**
   - Limited connection string parsing
   - No advanced configuration options
   - No SSL/TLS support documented

8. **Batch Operations:**
   - No batch size limits
   - No memory management for large batches
   - No progress reporting

## Status

**Current Status:** Reference Implementation (Stub)

✅ **Complete:**
- Factory pattern implementation
- Interface implementation (all methods)
- Error handling infrastructure
- Capability reporting
- Thread-safe registry

⚠️ **Incomplete (Stub):**
- ThemisDB client integration
- Actual query execution
- Vector index operations
- Graph algorithms
- Transaction management

🔮 **Future Work:**
- Production ThemisDB integration
- Connection pooling
- Retry logic and error recovery
- Performance optimizations
- Comprehensive testing

## Related Documentation

- [Header Documentation](../../include/chimera/README.md) - Interface definitions and contracts
- [Adapter Templates](../../adapters/chimera/README.md) - Creating custom adapters
- [CHIMERA Benchmark Suite](../../benchmarks/chimera/README.md) - Benchmark framework
- [Database Adapter Tests](../../tests/chimera/README.md) - Testing infrastructure

## Contributing

To implement production-ready adapters:

1. **Replace Stub Logic:** Integrate actual database clients
2. **Add Error Handling:** Comprehensive error cases and retry logic
3. **Optimize Performance:** Connection pooling, batch operations
4. **Add Tests:** Unit tests and integration tests
5. **Document:** Usage examples and performance characteristics

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

---

*Last Updated: February 2026*  
*Module Version: v1.0.0 (Reference Implementation)*  
*Status: Stub/Template - Production Implementation Pending*
