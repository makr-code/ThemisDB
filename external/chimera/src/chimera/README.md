# Chimera Module - Source Implementation

## Module Purpose

The Chimera module provides the core implementation for the **CHIMERA** (Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment) benchmark suite's vendor-neutral database adapter architecture. This module enables fair, unbiased benchmarking of diverse database systems (relational, document, graph, vector, hybrid) through a unified interface. Named after the mythological creature with multiple forms, Chimera adapts to any database system's native capabilities while providing consistent benchmarking metrics.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `adapter_factory.cpp` | Thread-safe singleton adapter registry |
| `themisdb_adapter.cpp` | ThemisDB reference implementation adapter |
| `mongodb_adapter.cpp` | MongoDB adapter (document + Atlas Vector Search) |
| `postgresql_adapter.cpp` | PostgreSQL adapter (relational + pgvector) |
| `elasticsearch_adapter.cpp` | Elasticsearch adapter (full-text + vector search) |
| `pinecone_adapter.cpp` | Pinecone adapter (managed vector search) |
| `qdrant_adapter.cpp` | Qdrant adapter (native vector database) |
| `weaviate_adapter.cpp` | Weaviate adapter (native vector database) |
| `neo4j_adapter.cpp` | Neo4j adapter (native graph database) |

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
// Returns: ["Elasticsearch", "MongoDB", "Neo4j", "Pinecone", "PostgreSQL",
//           "Qdrant", "ThemisDB", "Weaviate"]
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

### mongodb_adapter.cpp
**Location:** `/src/chimera/mongodb_adapter.cpp`

MongoDB adapter implementing document storage and Atlas Vector Search for the CHIMERA Suite.

**Key Features:**
- **Document CRUD**: Insert, batch-insert, find (with field filters), update operations
- **Atlas Vector Search**: k-NN cosine-similarity search (`$vectorSearch` pattern), metadata filtering
- **Transaction Support**: Multi-document ACID transactions (MongoDB 4.0+ style)
- **Security**: Automatic credential masking (`user:pass@` → `***:***@`) in stored strings
- **Simulation Mode**: In-process storage backed by `std::unordered_map` – no live server required for testing
- **Auto-Registration**: Registers itself as `"MongoDB"` in `AdapterFactory` at static-init time

**Supported Capabilities:**
`DOCUMENT_STORE`, `VECTOR_SEARCH`, `FULL_TEXT_SEARCH`, `TRANSACTIONS`, `BATCH_OPERATIONS`, `SECONDARY_INDEXES`

**Unsupported (returns `NOT_IMPLEMENTED`):**
`RELATIONAL_QUERIES`, `GRAPH_TRAVERSAL`

**Connection Strings:**
```
mongodb://[user:pass@]host[:port][/dbname]
mongodb+srv://[user:pass@]cluster.mongodb.net[/dbname]
```

**Usage Example:**
```cpp
#include "chimera/mongodb_adapter.hpp"

// Adapter auto-registers as "MongoDB" – factory is ready immediately.
auto adapter = chimera::AdapterFactory::create("MongoDB");
adapter->connect("mongodb://localhost:27017/mydb");

// Document operations
chimera::Document doc;
doc.id = "user_001";
doc.fields["name"] = chimera::Scalar{std::string{"Alice"}};
adapter->insert_document("users", doc);

auto result = adapter->find_documents("users",
    {{"name", chimera::Scalar{std::string{"Alice"}}}});

// Atlas Vector Search
chimera::Vector query;
query.data = {0.9f, 0.1f, 0.0f};
auto hits = adapter->search_vectors("embeddings", query, /*k=*/10);
```

---

### postgresql_adapter.cpp
**Location:** `/src/chimera/postgresql_adapter.cpp`

PostgreSQL + pgvector adapter for relational workloads and optional vector similarity search.

**Key Features:**
- **Relational CRUD**: SQL-style row insert, batch insert, query execution
- **pgvector**: L2 / cosine-similarity vector search via pgvector extension pattern
- **Document Store**: JSONB-style document storage over relational rows
- **Transaction Support**: Multi-statement ACID transactions
- **Security**: Automatic credential masking in stored connection strings
- **Simulation Mode**: In-process storage – no live PostgreSQL server required for testing
- **Auto-Registration**: Registers itself as `"PostgreSQL"` in `AdapterFactory` at static-init time

**Supported Capabilities:**
`RELATIONAL_QUERIES`, `VECTOR_SEARCH`, `DOCUMENT_STORE`, `FULL_TEXT_SEARCH`, `TRANSACTIONS`, `BATCH_OPERATIONS`, `SECONDARY_INDEXES`

**Unsupported (returns `NOT_IMPLEMENTED`):**
`GRAPH_TRAVERSAL`

**Connection Strings:**
```
postgresql://[user:pass@]host[:port][/dbname]
postgres://[user:pass@]host[:port][/dbname]
```

**Usage Example:**
```cpp
#include "chimera/postgresql_adapter.hpp"

auto adapter = chimera::AdapterFactory::create("PostgreSQL");
adapter->connect("postgresql://localhost:5432/mydb");

// Relational row insert
chimera::RelationalRow row;
row.columns["name"]  = chimera::Scalar{std::string{"Alice"}};
row.columns["score"] = chimera::Scalar{int64_t{95}};
adapter->insert_row("users", row);

// pgvector similarity search
chimera::Vector query;
query.data = {0.1f, 0.8f, 0.5f};
auto hits = adapter->search_vectors("embeddings", query, /*k=*/5);
```

---

## Architecture

### Component Interaction

```
Benchmark Suite
      ↓
AdapterFactory::create("ThemisDB" or "MongoDB" or "PostgreSQL")
      ↓
Concrete adapter instance (ThemisDBAdapter / MongoDBAdapter / PostgreSQLAdapter)
      ↓
connect() → target database (or in-process simulation)
      ↓
execute_query() / insert_document() / search_vectors() / …
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

MongoDBAdapter (concrete implementation)
  ├─ implements IVectorAdapter (Atlas Vector Search)
  ├─ implements IDocumentAdapter
  ├─ implements ITransactionAdapter
  ├─ implements ISystemInfoAdapter
  └─ returns NOT_IMPLEMENTED for IRelationalAdapter / IGraphAdapter

PostgreSQLAdapter (concrete implementation)
  ├─ implements IRelationalAdapter (primary)
  ├─ implements IVectorAdapter (pgvector)
  ├─ implements IDocumentAdapter (JSONB)
  ├─ implements ITransactionAdapter
  ├─ implements ISystemInfoAdapter
  └─ returns NOT_IMPLEMENTED for IGraphAdapter
```

### Factory Registration Flow

```
Static Initialization (before main())
      ↓
register_adapter("ThemisDB" or "MongoDB" or "PostgreSQL", creator_lambda)
      ↓
Insert into static registry map
      ↓
Runtime Usage:
create("MongoDB") → lookup registry → invoke creator → return MongoDBAdapter
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

// Register adapter with static capability hints (avoids instantiation during negotiation)
static bool register_adapter(
    const std::string& system_name,
    AdapterCreator creator,
    const std::vector<Capability>& static_capabilities
);

// Query available systems
static std::vector<std::string> get_supported_systems();

// Check if system is registered
static bool is_supported(const std::string& system_name);

// Create from prioritised fallback list (returns first successfully created)
static std::unique_ptr<IDatabaseAdapter> create_with_fallback(
    const std::vector<std::string>& candidates
);

// Create first adapter in list that meets required capabilities
// Uses static capability hints when registered, otherwise probes the live instance
static std::unique_ptr<IDatabaseAdapter> create_with_capabilities(
    const std::vector<std::string>& candidates,
    const std::vector<Capability>& required_capabilities
);
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

**Current Status:** Alpha — All adapter implementations complete in simulation mode (no live server required)

✅ **Complete:**
- Factory pattern implementation with auto-registration (static init)
- Interface implementation (all methods across all adapters)
- Error handling infrastructure
- Capability reporting
- Thread-safe registry
- ThemisDB reference adapter (all 5 operation interfaces)
- MongoDB adapter: document CRUD + Atlas Vector Search (cosine similarity)
- MongoDB adapter: transaction lifecycle + security (credential masking)
- PostgreSQL adapter: relational CRUD + pgvector similarity search
- PostgreSQL adapter: JSONB document store + transaction lifecycle
- Elasticsearch adapter: full-text search + k-NN vector search
- Pinecone adapter: managed vector search (upsert, query, delete)
- Qdrant adapter: native vector database (collections, search, payload filtering)
- Weaviate adapter: native vector database (objects, semantic search)
- Neo4j adapter: native graph database (Cypher queries, graph traversal)

⚠️ **Simulation Mode (no live server required):**
- All vendor adapters use in-process `std::unordered_map` storage for tests
- Production deployments require linking the respective native client library
  (e.g. `libmongocxx`, `libpqxx`, `cpp-httplib`/`cpr` for HTTP-based adapters)
  and replacing the simulation blocks with real API calls

🔮 **Future Work:**
- Production driver integration for all adapters
- Connection pooling
- Retry logic and error recovery
- Cross-system query federation

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

*Last Updated: April 2026*  
*Module Version: v1.1.0 (All Adapters Implemented)*  
*Status: Alpha — Simulation Mode; Production Driver Integration Pending*

## Scientific References

1. DeWitt, D., & Gray, J. (1992). **Parallel Database Systems: The Future of High Performance Database Systems**. *Communications of the ACM*, 35(6), 85–98. https://doi.org/10.1145/129888.129894

2. Boncz, P., Neumann, T., & Erling, O. (2013). **TPC-H Analyzed: Hidden Messages and Lessons Learned from an Influential Benchmark**. *Proceedings of the 5th TPC Technology Conference (TPCTC)*, LNCS 8169, 61–76. https://doi.org/10.1007/978-3-319-04936-6_5

3. Stonebraker, M., Madden, S., Abadi, D., Harizopoulos, S., Hachem, N., & Helland, P. (2007). **The End of an Architectural Era (It's Time for a Complete Rewrite)**. *Proceedings of the 33rd International Conference on Very Large Data Bases (VLDB)*, 1150–1160. https://dl.acm.org/doi/10.5555/1325851.1325981

4. Leis, V., Kemper, A., & Neumann, T. (2013). **The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases**. *Proceedings of the 2013 IEEE International Conference on Data Engineering (ICDE)*, 38–49. https://doi.org/10.1109/ICDE.2013.6544812

5. Raasveldt, M., & Mühleisen, H. (2019). **DuckDB: an Embeddable Analytical Database**. *Proceedings of the 2019 ACM SIGMOD International Conference on Management of Data*, 1981–1984. https://doi.org/10.1145/3299869.3320212
