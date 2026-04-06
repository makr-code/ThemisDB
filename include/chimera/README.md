# Chimera Module - Header Interfaces

## Module Purpose

The Chimera module headers define the vendor-neutral interface contracts for the **CHIMERA** (Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment) benchmark suite. These headers provide abstract interfaces that any database system can implement to participate in fair, unbiased performance comparisons. The architecture ensures complete vendor neutrality through generic types, standardized error handling, and capability-based feature detection.

## Scope

**In Scope:**
- Abstract interface definitions for all database operations
- Generic data types (Result, Scalar, Vector, Document, Graph structures)
- Error codes and error handling patterns
- Capability enumeration and detection interfaces
- Multi-model operation interfaces (relational, vector, graph, document, transaction)
- System information and metrics structures
- Factory pattern for adapter instantiation

**Out of Scope:**
- Concrete adapter implementations (see `src/chimera/` and `adapters/chimera/`)
- Database-specific types and extensions
- Benchmark test implementations
- Performance measurement frameworks
- Database client libraries

## Header Files

### database_adapter.hpp
**Location:** `/include/chimera/database_adapter.hpp`

Core interface definitions for all database adapter implementations.

**Key Components:**

#### 1. Error Handling Infrastructure

**ErrorCode Enumeration:**
```cpp
enum class ErrorCode {
    SUCCESS = 0,              // Operation completed successfully
    NOT_IMPLEMENTED = 1,      // Feature not implemented by adapter
    INVALID_ARGUMENT = 2,     // Invalid input parameter
    NOT_FOUND = 3,            // Resource not found
    ALREADY_EXISTS = 4,       // Resource already exists
    PERMISSION_DENIED = 5,    // Insufficient permissions
    CONNECTION_ERROR = 6,     // Network or connection failure
    TIMEOUT = 7,              // Operation timeout
    RESOURCE_EXHAUSTED = 8,   // Out of resources (memory, disk, etc.)
    INTERNAL_ERROR = 9,       // Internal system error
    UNSUPPORTED = 10,         // Operation not supported
    TRANSACTION_ABORTED = 11, // Transaction was aborted
    CONSTRAINT_VIOLATION = 12 // Data integrity constraint violated
};
```

**Result<T> Template:**
```cpp
template<typename T>
struct Result {
    std::optional<T> value;          // Result value if successful
    ErrorCode error_code;             // Error code if failed
    std::string error_message;        // Human-readable error description
    
    bool is_ok() const;
    bool is_err() const;
    
    static Result<T> ok(T val);
    static Result<T> err(ErrorCode code, std::string message);
};
```

**Design Rationale:**
- Follows Rust/C++ Expected pattern for error handling without exceptions
- Type-safe error propagation
- Supports error chaining and context preservation
- Zero-cost abstraction when optimized

**Usage Pattern:**
```cpp
// Creating results
auto success = Result<int>::ok(42);
auto failure = Result<int>::err(ErrorCode::NOT_FOUND, "Record not found");

// Checking results
if (result.is_ok()) {
    int value = *result.value;
    process(value);
} else {
    std::cerr << "Error: " << result.error_message << std::endl;
}

// Early return pattern
Result<Document> find_document(const std::string& id) {
    if (!is_connected()) {
        return Result<Document>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    
    auto doc = fetch_from_storage(id);
    if (!doc) {
        return Result<Document>::err(
            ErrorCode::NOT_FOUND,
            "Document not found: " + id
        );
    }
    
    return Result<Document>::ok(std::move(*doc));
}
```

#### 2. Generic Data Types

**Scalar Type (Variant):**
```cpp
using Scalar = std::variant<
    std::monostate,        // NULL/None
    bool,                  // Boolean
    int64_t,               // Integer
    double,                // Floating point
    std::string,           // Text/String
    std::vector<uint8_t>   // Binary/Blob
>;
```

**Design Rationale:**
- Covers all common database primitive types
- Type-safe (std::variant)
- NULL represented by std::monostate
- Binary data support for blobs

**Vector Structure:**
```cpp
struct Vector {
    std::vector<float> data;          // Vector components (float32)
    std::map<std::string, Scalar> metadata; // Optional metadata
    
    size_t dimensions() const { return data.size(); }
};
```

**Features:**
- Float32 for memory efficiency and GPU compatibility
- Metadata for filtering (e.g., category, timestamp)
- Dimension accessor for validation

**Document Structure:**
```cpp
struct Document {
    std::string id;                              // Unique document identifier
    std::map<std::string, Scalar> fields;        // Document fields
    std::optional<int64_t> version;              // Optional document version
    std::optional<std::chrono::system_clock::time_point> timestamp;
};
```

**Features:**
- Schema-flexible key-value pairs
- Optional versioning for optimistic concurrency
- Optional timestamps for time-travel queries

**Graph Structures:**
```cpp
struct GraphNode {
    std::string id;                              // Unique node identifier
    std::string label;                           // Node type/label
    std::map<std::string, Scalar> properties;    // Node properties
};

struct GraphEdge {
    std::string id;                              // Unique edge identifier
    std::string source_id;                       // Source node ID
    std::string target_id;                       // Target node ID
    std::string label;                           // Edge type/label
    std::map<std::string, Scalar> properties;    // Edge properties
    std::optional<double> weight;                // Optional edge weight
};

struct GraphPath {
    std::vector<GraphNode> nodes;                // Nodes in path
    std::vector<GraphEdge> edges;                // Edges in path
    double total_weight;                         // Total path weight
};
```

**Features:**
- Property graph model (nodes and edges have properties)
- Labeled nodes/edges for type classification
- Weighted edges for shortest path algorithms
- Path representation for traversal results

**Relational Structures:**
```cpp
struct RelationalRow {
    std::map<std::string, Scalar> columns;       // Column name to value mapping
};

struct RelationalTable {
    std::vector<std::string> column_names;       // Column names in order
    std::vector<RelationalRow> rows;             // Table rows
};
```

**Features:**
- Ordered column names for CSV/table rendering
- Map-based row access for flexibility
- Compatible with SQL result sets

#### 3. Transaction Support

**TransactionOptions:**
```cpp
struct TransactionOptions {
    enum class IsolationLevel {
        READ_UNCOMMITTED,    // Lowest isolation, highest concurrency
        READ_COMMITTED,      // Prevent dirty reads
        REPEATABLE_READ,     // Prevent dirty and non-repeatable reads
        SERIALIZABLE         // Highest isolation, lowest concurrency
    };
    
    IsolationLevel isolation_level = IsolationLevel::READ_COMMITTED;
    std::optional<std::chrono::milliseconds> timeout;
    bool read_only = false;
};
```

**Design Rationale:**
- Standard SQL isolation levels
- Optional timeout for deadlock prevention
- Read-only flag for optimization opportunities

#### 4. System Information Structures

**QueryStatistics:**
```cpp
struct QueryStatistics {
    std::chrono::microseconds execution_time;    // Query execution time
    size_t rows_read;                            // Rows scanned
    size_t rows_returned;                        // Rows returned
    size_t bytes_read;                           // Bytes read from storage
    std::map<std::string, Scalar> additional_metrics; // System-specific metrics
};
```

**SystemInfo:**
```cpp
struct SystemInfo {
    std::string system_name;                     // System name
    std::string version;                         // Version string
    std::map<std::string, std::string> build_info; // Build information
    std::map<std::string, Scalar> configuration; // Configuration parameters
};
```

**SystemMetrics:**
```cpp
struct SystemMetrics {
    struct MemoryMetrics {
        size_t total_bytes;
        size_t used_bytes;
        size_t available_bytes;
    };
    
    struct StorageMetrics {
        size_t total_bytes;
        size_t used_bytes;
        size_t available_bytes;
    };
    
    struct CPUMetrics {
        double utilization_percent;
        size_t thread_count;
    };
    
    MemoryMetrics memory;
    StorageMetrics storage;
    CPUMetrics cpu;
    std::map<std::string, Scalar> custom_metrics;
};
```

**Features:**
- Standardized resource metrics
- Custom metrics for system-specific measurements
- Snapshot-based (point-in-time) metrics

#### 5. Capability System

**Capability Enumeration:**
```cpp
enum class Capability {
    RELATIONAL_QUERIES,        // SQL/Relational query support
    VECTOR_SEARCH,             // Vector similarity search
    GRAPH_TRAVERSAL,           // Graph algorithms and traversal
    DOCUMENT_STORE,            // Document storage and queries
    FULL_TEXT_SEARCH,          // Full-text search capabilities
    TRANSACTIONS,              // ACID transaction support
    DISTRIBUTED_QUERIES,       // Distributed query execution
    GEOSPATIAL_QUERIES,        // Geographic/spatial queries
    TIME_SERIES,               // Time-series data handling
    STREAM_PROCESSING,         // Real-time stream processing
    BATCH_OPERATIONS,          // Bulk insert/update operations
    SECONDARY_INDEXES,         // Secondary index support
    MATERIALIZED_VIEWS,        // Materialized view support
    REPLICATION,               // Data replication
    SHARDING                   // Horizontal sharding/partitioning
};
```

**Design Rationale:**
- Allows benchmarks to discover database features
- Skip unsupported tests gracefully
- Report capability coverage in results
- Enable fair comparisons (only compare supported features)

#### 6. Interface Hierarchy

**IRelationalAdapter - Relational Database Operations:**
```cpp
class IRelationalAdapter {
public:
    virtual ~IRelationalAdapter() = default;
    
    // Execute query (SQL or equivalent)
    virtual Result<RelationalTable> execute_query(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;
    
    // Insert single row
    virtual Result<size_t> insert_row(
        const std::string& table_name,
        const RelationalRow& row
    ) = 0;
    
    // Batch insert (optimized)
    virtual Result<size_t> batch_insert(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows
    ) = 0;
    
    // Query execution statistics
    virtual Result<QueryStatistics> get_query_statistics() = 0;
};
```

**IVectorAdapter - Vector Similarity Search:**
```cpp
class IVectorAdapter {
public:
    virtual ~IVectorAdapter() = default;
    
    // Insert vector
    virtual Result<std::string> insert_vector(
        const std::string& collection,
        const Vector& vector
    ) = 0;
    
    // Batch insert vectors
    virtual Result<size_t> batch_insert_vectors(
        const std::string& collection,
        const std::vector<Vector>& vectors
    ) = 0;
    
    // k-NN search
    virtual Result<std::vector<std::pair<Vector, double>>> search_vectors(
        const std::string& collection,
        const Vector& query_vector,
        size_t k,
        const std::map<std::string, Scalar>& filters = {}
    ) = 0;
    
    // Create vector index
    virtual Result<bool> create_index(
        const std::string& collection,
        size_t dimensions,
        const std::map<std::string, Scalar>& index_params = {}
    ) = 0;
};
```

**IGraphAdapter - Graph Database Operations:**
```cpp
class IGraphAdapter {
public:
    virtual ~IGraphAdapter() = default;
    
    // Insert node/vertex
    virtual Result<std::string> insert_node(const GraphNode& node) = 0;
    
    // Insert edge
    virtual Result<std::string> insert_edge(const GraphEdge& edge) = 0;
    
    // Shortest path algorithm
    virtual Result<GraphPath> shortest_path(
        const std::string& source_id,
        const std::string& target_id,
        size_t max_depth = 10
    ) = 0;
    
    // Graph traversal
    virtual Result<std::vector<GraphNode>> traverse(
        const std::string& start_id,
        size_t max_depth,
        const std::vector<std::string>& edge_labels = {}
    ) = 0;
    
    // Execute graph query (Cypher, Gremlin, etc.)
    virtual Result<std::vector<GraphPath>> execute_graph_query(
        const std::string& query,
        const std::map<std::string, Scalar>& params = {}
    ) = 0;
};
```

**IDocumentAdapter - Document Store Operations:**
```cpp
class IDocumentAdapter {
public:
    virtual ~IDocumentAdapter() = default;
    
    // Insert document
    virtual Result<std::string> insert_document(
        const std::string& collection,
        const Document& doc
    ) = 0;
    
    // Batch insert documents
    virtual Result<size_t> batch_insert_documents(
        const std::string& collection,
        const std::vector<Document>& docs
    ) = 0;
    
    // Find documents (filtered query)
    virtual Result<std::vector<Document>> find_documents(
        const std::string& collection,
        const std::map<std::string, Scalar>& filter,
        size_t limit = 100
    ) = 0;
    
    // Update documents
    virtual Result<size_t> update_documents(
        const std::string& collection,
        const std::map<std::string, Scalar>& filter,
        const std::map<std::string, Scalar>& updates
    ) = 0;
};
```

**ITransactionAdapter - Transaction Management:**
```cpp
class ITransactionAdapter {
public:
    virtual ~ITransactionAdapter() = default;
    
    // Begin transaction
    virtual Result<std::string> begin_transaction(
        const TransactionOptions& options = {}
    ) = 0;
    
    // Commit transaction
    virtual Result<bool> commit_transaction(const std::string& transaction_id) = 0;
    
    // Rollback transaction
    virtual Result<bool> rollback_transaction(const std::string& transaction_id) = 0;
};
```

**ISystemInfoAdapter - System Information:**
```cpp
class ISystemInfoAdapter {
public:
    virtual ~ISystemInfoAdapter() = default;
    
    // Get system info
    virtual Result<SystemInfo> get_system_info() = 0;
    
    // Get runtime metrics
    virtual Result<SystemMetrics> get_metrics() = 0;
    
    // Check capability
    virtual bool has_capability(Capability cap) = 0;
    
    // Get all capabilities
    virtual std::vector<Capability> get_capabilities() = 0;
};
```

**IDatabaseAdapter - Complete Interface:**
```cpp
class IDatabaseAdapter : public IRelationalAdapter,
                          public IVectorAdapter,
                          public IGraphAdapter,
                          public IDocumentAdapter,
                          public ITransactionAdapter,
                          public ISystemInfoAdapter {
public:
    virtual ~IDatabaseAdapter() = default;
    
    // Connection management
    virtual Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options = {}
    ) = 0;
    
    virtual Result<bool> disconnect() = 0;
    
    virtual bool is_connected() const = 0;
};
```

**Design Rationale:**
- Multiple inheritance combines all interfaces
- Adapters can return NOT_IMPLEMENTED for unsupported features
- Single entry point for all database operations
- Capability system allows feature discovery

#### 7. Factory Pattern

**AdapterFactory:**
```cpp
class AdapterFactory {
public:
    using AdapterCreator = std::function<std::unique_ptr<IDatabaseAdapter>()>;
    
    // Create adapter by system name
    static std::unique_ptr<IDatabaseAdapter> create(const std::string& system_name);
    
    // Register adapter creator
    static bool register_adapter(const std::string& system_name, AdapterCreator creator);
    
    // Query supported systems
    static std::vector<std::string> get_supported_systems();
    
    // Check if system is supported
    static bool is_supported(const std::string& system_name);

private:
    static std::map<std::string, AdapterCreator>& get_registry();
};
```

**Usage Pattern:**
```cpp
// Registration (typically in static initializer)
static bool registered = AdapterFactory::register_adapter(
    "MyDatabase",
    []() { return std::make_unique<MyDatabaseAdapter>(); }
);

// Creation
auto adapter = AdapterFactory::create("MyDatabase");
if (adapter) {
    adapter->connect("mydb://localhost:5432/db");
    // Use adapter...
}

// Discovery
auto systems = AdapterFactory::get_supported_systems();
for (const auto& name : systems) {
    std::cout << "Supported: " << name << std::endl;
}
```

### mongodb_adapter.hpp
**Location:** `/include/chimera/mongodb_adapter.hpp`

Header for the MongoDB adapter targeting MongoDB 4.4+ and Atlas Vector Search.

**Key Components:**

**MongoDBAdapter Class:**
```cpp
class MongoDBAdapter : public IDatabaseAdapter {
public:
    MongoDBAdapter();
    ~MongoDBAdapter() override;

    // Connection Management
    Result<bool> connect(const std::string& connection_string, ...) override;
    Result<bool> disconnect() override;
    bool is_connected() const override;

    // IVectorAdapter – Atlas Vector Search
    Result<std::string> insert_vector(const std::string& collection, const Vector& v) override;
    Result<std::vector<std::pair<Vector,double>>> search_vectors(...) override;

    // IDocumentAdapter
    Result<std::string> insert_document(const std::string& collection, const Document& doc) override;
    Result<std::vector<Document>> find_documents(...) override;
    Result<size_t> update_documents(...) override;

    // ITransactionAdapter, ISystemInfoAdapter, …
};
```

**Private helpers:**
- `mask_credentials()` – strips `user:pass@` before storing connection string
- `parse_database_name()` – extracts database from MongoDB URI
- `cosine_similarity()` – BLAS-free dot-product / magnitude calculation
- `document_matches()` – field-by-field filter evaluation, `"id"` key maps to `doc.id`

---

### postgresql_adapter.hpp
**Location:** `/include/chimera/postgresql_adapter.hpp`

Header for the PostgreSQL + pgvector adapter targeting PostgreSQL 14+.

**Key Components:**

**PostgreSQLAdapter Class:**
```cpp
class PostgreSQLAdapter : public IDatabaseAdapter {
public:
    PostgreSQLAdapter();
    ~PostgreSQLAdapter() override;

    // Connection Management
    Result<bool> connect(const std::string& connection_string, ...) override;

    // IRelationalAdapter – SQL operations
    Result<RelationalTable> execute_query(const std::string& query, ...) override;
    Result<size_t> insert_row(const std::string& table_name, const RelationalRow& row) override;

    // IVectorAdapter – pgvector similarity search
    Result<std::vector<std::pair<Vector,double>>> search_vectors(...) override;

    // IDocumentAdapter, ITransactionAdapter, ISystemInfoAdapter, …
};
```

---

### themisdb_adapter.hpp
**Location:** `/include/chimera/themisdb_adapter.hpp`

Reference adapter header for ThemisDB integration.

**Key Components:**

**ThemisDBAdapter Class:**
```cpp
class ThemisDBAdapter : public IDatabaseAdapter {
public:
    ThemisDBAdapter() = default;
    ~ThemisDBAdapter() override = default;
    
    // All IDatabaseAdapter methods declared
    // (See database_adapter.hpp for complete interface)
    
private:
    bool connected_ = false;
    std::string connection_string_;
};
```

**Features:**
- Complete implementation of all 6 adapter interfaces
- Private connection state management
- Example of proper adapter structure

**Usage:**
```cpp
#include "chimera/themisdb_adapter.hpp"

chimera::ThemisDBAdapter adapter;
auto result = adapter.connect("themisdb://localhost:8529/benchmark");

if (result.is_ok()) {
    // Execute operations
    auto query_result = adapter.execute_query(
        "FOR u IN users FILTER u.age > @age RETURN u",
        {Scalar(int64_t(30))}
    );
}
```

## Architecture

### Type System Hierarchy

```
Scalar (std::variant)
  ├─ std::monostate (NULL)
  ├─ bool
  ├─ int64_t
  ├─ double
  ├─ std::string
  └─ std::vector<uint8_t>

Complex Types
  ├─ Vector (embeddings)
  ├─ Document (JSON-like)
  ├─ GraphNode (vertices)
  ├─ GraphEdge (edges)
  ├─ GraphPath (traversal results)
  ├─ RelationalRow (SQL row)
  └─ RelationalTable (SQL result set)
```

### Interface Composition

```
IDatabaseAdapter
  ├─ IRelationalAdapter
  │    ├─ execute_query()
  │    ├─ insert_row()
  │    └─ batch_insert()
  │
  ├─ IVectorAdapter
  │    ├─ insert_vector()
  │    ├─ search_vectors()
  │    └─ create_index()
  │
  ├─ IGraphAdapter
  │    ├─ insert_node()
  │    ├─ insert_edge()
  │    ├─ shortest_path()
  │    └─ traverse()
  │
  ├─ IDocumentAdapter
  │    ├─ insert_document()
  │    ├─ find_documents()
  │    └─ update_documents()
  │
  ├─ ITransactionAdapter
  │    ├─ begin_transaction()
  │    ├─ commit_transaction()
  │    └─ rollback_transaction()
  │
  └─ ISystemInfoAdapter
       ├─ get_system_info()
       ├─ get_metrics()
       └─ has_capability()
```

### Error Handling Flow

```
Operation Call
      ↓
Check Preconditions
      ↓
  [Valid?] ──No──> Result<T>::err(ErrorCode, "message")
      │
     Yes
      ↓
Execute Operation
      ↓
  [Success?] ──No──> Result<T>::err(ErrorCode, "message")
      │
     Yes
      ↓
Result<T>::ok(value)
```

## Design Principles

### 1. Vendor Neutrality

**Principles:**
- No vendor-specific names in types or interfaces
- No performance assumptions or optimizations favoring specific systems
- Alphabetic ordering of systems in enumerations
- Generic error codes and messages

**Examples:**
```cpp
// ✅ Good: Generic, vendor-neutral
Result<RelationalTable> execute_query(const std::string& query);

// ❌ Bad: Vendor-specific
Result<PostgreSQLResultSet> execute_postgres_query(const std::string& sql);
```

### 2. Capability-Based Design

**Principles:**
- All features are optional
- Adapters report capabilities
- Benchmarks check capabilities before testing
- Graceful degradation for unsupported features

**Example:**
```cpp
auto adapter = AdapterFactory::create("Redis");

if (adapter->has_capability(Capability::GRAPH_TRAVERSAL)) {
    // Run graph benchmarks
    run_graph_benchmark(adapter.get());
} else {
    // Skip graph benchmarks for Redis
    std::cout << "Skipping graph tests (not supported)" << std::endl;
}
```

### 3. Error Handling Without Exceptions

**Principles:**
- Result<T> for all fallible operations
- Explicit error checking required
- Error context preserved
- Zero exception overhead

**Example:**
```cpp
Result<RelationalTable> execute_multi_step() {
    auto conn_result = adapter->connect("...");
    if (conn_result.is_err()) {
        return Result<RelationalTable>::err(
            conn_result.error_code,
            "Connection failed: " + conn_result.error_message
        );
    }
    
    auto query_result = adapter->execute_query("...");
    if (query_result.is_err()) {
        return query_result; // Forward error
    }
    
    return query_result; // Success
}
```

### 4. Type Safety

**Principles:**
- std::variant for type-safe unions
- Templates for generic operations
- const correctness
- Move semantics for efficiency

**Example:**
```cpp
// Type-safe scalar access
Scalar age = int64_t(30);

// Compile-time type checking
if (std::holds_alternative<int64_t>(age)) {
    int64_t value = std::get<int64_t>(age);
    process_age(value);
}

// Move semantics for efficiency
Result<RelationalTable> query(const std::string& sql) {
    RelationalTable table = fetch_from_db(sql);
    return Result<RelationalTable>::ok(std::move(table)); // No copy
}
```

## Integration Patterns

### 1. Adapter Implementation Pattern

```cpp
// Step 1: Inherit from IDatabaseAdapter
class MyDatabaseAdapter : public chimera::IDatabaseAdapter {
public:
    // Step 2: Implement connection management
    Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options
    ) override {
        // Parse connection_string
        // Establish connection
        // Store state
        return Result<bool>::ok(true);
    }
    
    // Step 3: Implement each interface method
    Result<RelationalTable> execute_query(
        const std::string& query,
        const std::vector<Scalar>& params
    ) override {
        // Execute using native client
        // Convert results to RelationalTable
        // Return Result
    }
    
    // Step 4: Report capabilities
    bool has_capability(Capability cap) override {
        switch (cap) {
            case Capability::RELATIONAL_QUERIES:
                return true; // Supported
            case Capability::GRAPH_TRAVERSAL:
                return false; // Not supported
            // ... other capabilities
        }
    }
    
    // Step 5: Return NOT_IMPLEMENTED for unsupported features
    Result<std::string> insert_node(const GraphNode& node) override {
        return Result<std::string>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "Graph operations not supported by MyDatabase"
        );
    }
};

// Step 6: Register adapter
namespace {
    bool registered = chimera::AdapterFactory::register_adapter(
        "MyDatabase",
        []() { return std::make_unique<MyDatabaseAdapter>(); }
    );
}
```

### 2. Benchmark Integration Pattern

```cpp
#include "chimera/database_adapter.hpp"
using namespace chimera;

void run_benchmark(const std::string& system_name) {
    // Create adapter
    auto adapter = AdapterFactory::create(system_name);
    if (!adapter) {
        std::cerr << "Adapter not found: " << system_name << std::endl;
        return;
    }
    
    // Connect
    auto conn_result = adapter->connect("...");
    if (!conn_result.is_ok()) {
        std::cerr << "Connection failed: " << conn_result.error_message << std::endl;
        return;
    }
    
    // Check capabilities
    if (!adapter->has_capability(Capability::VECTOR_SEARCH)) {
        std::cout << "Skipping vector benchmarks (not supported)" << std::endl;
    } else {
        // Run vector benchmarks
        run_vector_benchmark(adapter.get());
    }
    
    // Cleanup
    adapter->disconnect();
}
```

### 3. Type Conversion Pattern

```cpp
// Convert native type to Scalar
Scalar to_scalar(const MyNativeType& native) {
    if (native.is_null()) {
        return std::monostate{};
    } else if (native.is_int()) {
        return int64_t(native.as_int());
    } else if (native.is_string()) {
        return std::string(native.as_string());
    }
    // ... other types
}

// Convert Scalar to native type
MyNativeType from_scalar(const Scalar& scalar) {
    return std::visit([](auto&& value) -> MyNativeType {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return MyNativeType::null();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return MyNativeType::from_int(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return MyNativeType::from_string(value);
        }
        // ... other types
    }, scalar);
}
```

## API Reference

### Core Types

```cpp
// Error handling
template<typename T> struct Result;
enum class ErrorCode;

// Generic data types
using Scalar = std::variant<...>;
struct Vector;
struct Document;
struct GraphNode;
struct GraphEdge;
struct GraphPath;
struct RelationalRow;
struct RelationalTable;

// Transaction types
struct TransactionOptions;

// System types
struct QueryStatistics;
struct SystemInfo;
struct SystemMetrics;
enum class Capability;
```

### Interfaces

```cpp
class IRelationalAdapter;
class IVectorAdapter;
class IGraphAdapter;
class IDocumentAdapter;
class ITransactionAdapter;
class ISystemInfoAdapter;
class IDatabaseAdapter;
```

### Factory

```cpp
class AdapterFactory {
    static std::unique_ptr<IDatabaseAdapter> create(const std::string& system_name);
    static bool register_adapter(const std::string& system_name, AdapterCreator creator);
    static std::vector<std::string> get_supported_systems();
    static bool is_supported(const std::string& system_name);
};
```

## Dependencies

### Internal Dependencies
- None (self-contained headers)

### External Dependencies

**Required:**
- C++17 standard library:
  - `<cstdint>` - Fixed-width integer types
  - `<memory>` - std::unique_ptr, std::shared_ptr
  - `<string>` - std::string
  - `<vector>` - std::vector
  - `<map>` - std::map
  - `<optional>` - std::optional
  - `<variant>` - std::variant
  - `<chrono>` - std::chrono
  - `<functional>` - std::function

**Optional:**
- None

### Build Configuration

```cmake
# Header-only (interface definitions)
add_library(themisdb_chimera_interface INTERFACE)

target_include_directories(themisdb_chimera_interface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(themisdb_chimera_interface INTERFACE
    cxx_std_17
)
```

## Performance Characteristics

### Type Sizes

- `Result<T>`: sizeof(T) + sizeof(std::optional<T>) + ~128 bytes (error string + error code)
- `Scalar`: 32 bytes (std::variant size)
- `Vector`: 24 bytes + vector data (16 bytes + dimensions * 4 bytes)
- `Document`: ~100 bytes + field data
- `GraphNode`: ~100 bytes + property data
- `GraphEdge`: ~150 bytes + property data

### Overhead

- Virtual function call: ~5-10ns
- Result<T> construction: ~1ns (optimized away)
- Error propagation: ~10ns (string construction)
- Scalar type check: ~1ns (variant index check)
- Capability check: ~1ns (enum comparison or switch)

### Memory Usage

- Interface instances: 8 bytes (vtable pointer only)
- Adapter instances: ~200 bytes (varies by implementation)
- Factory registry: ~100 bytes per registered adapter

## Known Limitations

1. **Single-Threaded Adapter Instances:**
   - Adapter instances are NOT thread-safe
   - Use separate adapters per thread or external synchronization

2. **No Async Operations:**
   - All operations are synchronous
   - Blocking I/O model
   - Planned for v1.2.0

3. **Limited Streaming:**
   - Results returned as complete sets
   - No incremental result streaming
   - Planned for v1.2.0

4. **Static Capability Reporting:**
   - Capabilities reported at adapter creation
   - No runtime feature probing
   - No version-specific capabilities

5. **Generic Metrics:**
   - SystemMetrics provides basic counters
   - Database-specific metrics require custom_metrics map
   - No standardized advanced metrics

6. **No Connection Pooling:**
   - Each adapter has single connection
   - No built-in pooling support
   - Planned for v1.1.0

7. **Limited Error Context:**
   - Single error message string
   - No error chaining or stack traces
   - No structured error data

8. **No Schema Management:**
   - No DDL operations defined
   - Schema creation adapter-specific
   - Planned for v1.4.0

## Status

**Current Status:** Stable Interface (v1.0.0)

✅ **Complete:**
- All interface definitions
- Error handling infrastructure
- Generic data types
- Capability system
- Factory pattern
- Documentation

🔒 **Frozen (v1.0.0):**
- Core interface signatures
- Base data types
- ErrorCode enumeration
- Capability enumeration

🔄 **Extensible:**
- Additional capabilities (backward compatible)
- Custom metrics in SystemMetrics
- Additional error codes (backward compatible)

## Related Documentation

- [Source Implementation](../../src/chimera/README.md) - Concrete implementations
- [Adapter Templates](../../adapters/chimera/README.md) - Creating new adapters
- [Benchmark Suite](../../benchmarks/chimera/README.md) - CHIMERA benchmarks
- [Tests](../../tests/chimera/README.md) - Test framework

---

*Last Updated: April 2026*  
*Interface Version: v1.0.0*  
*Status: Stable - Backward Compatibility Guaranteed*
