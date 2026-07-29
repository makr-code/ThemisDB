# Chimera Module Headers - Future Enhancements

## Scope

- API-level enhancements to `include/chimera/` headers — connection pool interface (`IConnectionPoolAdapter`)
- Async / promise-based query API (`IAsyncDatabaseAdapter`) returning `std::future<Result<T>>`
- Streaming result cursor interface (`IResultStream`, `IStreamingAdapter`) for large result sets
- Prepared statement API (`IPreparedStatement`, `IPreparedStatementAdapter`) for repeated query execution
- Transaction savepoint extensions and distributed query interfaces (`ITransactionAdapter`, `IDistributedAdapter`)
- Schema management, geospatial, and time-series adapter interfaces for future capability tiers

## Design Constraints

- [ ] Async API uses `std::future<Result<T>>` return type; `progress_callback` is supplementary and must not block
- [ ] Connection pool must be thread-safe; `initialize_pool()`, `borrow`, and `return` safe to call from any thread
- [ ] Result streaming uses pull-based cursor pattern (`IResultStream`); implementations must not push without consumer request
- [ ] New interfaces are capability-gated via `Capability` / `ExtensionCapability` enum; unsupported methods return `NOT_IMPLEMENTED`
- [ ] Prepared statement API prevents SQL injection by design — parameters bound via typed `Scalar`, never string-concatenated
- [ ] Connection strings and credentials must not be stored in public header types; callers receive opaque connection handles

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IConnectionPoolAdapter` | Connection pool consumers | Thread-safe; exposes `ConnectionPoolStats` |
| `IAsyncDatabaseAdapter` | Non-blocking query dispatchers | Returns `std::future<Result<T>>` |
| `IResultStream` | Large result set consumers | Pull-based cursor: `has_more()` / `next_batch()` |
| `IPreparedStatement` | Repeated query executors | Typed `bind()` by name or position |
| `ITransactionAdapter` (savepoints) | Complex transaction consumers | `create_savepoint()` / `rollback_to_savepoint()` |
| `ISchemaAdapter` | Schema management tooling | DDL: `create_collection()`, `create_index()` |

## Planned Interface Extensions

### Async Operation Interfaces
**Priority:** High  
**Target Version:** v1.2.0  
**Status:** ✅ Implemented (v1.8.0)

Add async/await style interfaces for non-blocking operations.

**New Interface:**
```cpp
class IAsyncDatabaseAdapter {
public:
    virtual ~IAsyncDatabaseAdapter() = default;
    
    // Async query execution
    virtual std::future<Result<RelationalTable>> execute_query_async(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;
    
    // Async batch insert with progress
    virtual std::future<Result<size_t>> batch_insert_async(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows,
        std::function<void(size_t processed)> progress_callback = nullptr
    ) = 0;
    
    // Async vector search
    virtual std::future<Result<std::vector<std::pair<Vector, double>>>> 
    search_vectors_async(
        const std::string& collection,
        const Vector& query_vector,
        size_t k,
        const std::map<std::string, Scalar>& filters = {}
    ) = 0;
    
    // Cancel async operation
    virtual Result<bool> cancel_async(const std::string& operation_id) = 0;
};
```

**Backward Compatibility:**
- New interface, doesn't break existing code
- Optional implementation (return NOT_IMPLEMENTED)
- Coexists with synchronous IDatabaseAdapter

---

### Streaming Result Interfaces
**Priority:** High  
**Target Version:** v1.2.0

Support incremental result streaming for large result sets.

**New Types:**
```cpp
class IResultStream {
public:
    virtual ~IResultStream() = default;
    
    // Check if more data available
    virtual bool has_more() const = 0;
    
    // Fetch next batch
    virtual Result<std::vector<RelationalRow>> next_batch(size_t batch_size = 100) = 0;
    
    // Get current position
    virtual size_t position() const = 0;
    
    // Get total size (if known)
    virtual std::optional<size_t> total_size() const = 0;
    
    // Close stream
    virtual Result<bool> close() = 0;
};

class IStreamingAdapter {
public:
    virtual ~IStreamingAdapter() = default;
    
    // Execute query with streaming results
    virtual Result<std::unique_ptr<IResultStream>> execute_query_stream(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;
    
    // Stream configuration
    virtual Result<bool> set_stream_config(const StreamConfig& config) = 0;
};

struct StreamConfig {
    size_t default_batch_size = 1000;
    size_t max_buffer_size = 10 * 1024 * 1024; // 10MB
    std::chrono::milliseconds fetch_timeout = std::chrono::seconds(30);
    bool prefetch_enabled = true;
};
```

**Usage Example:**
```cpp
// Execute streaming query
auto stream_result = adapter->execute_query_stream(
    "FOR doc IN large_collection RETURN doc"
);

if (stream_result.is_ok()) {
    auto stream = std::move(stream_result.value());
    
    // Process batches
    while (stream->has_more()) {
        auto batch = stream->next_batch(1000);
        if (batch.is_ok()) {
            for (const auto& row : batch.value()) {
                process(row);
            }
        }
    }
    
    stream->close();
}
```

---

### Prepared Statement Interfaces
**Priority:** Medium  
**Target Version:** v1.2.0

Add prepared statement support for repeated query execution.

**New Interface:**
```cpp
class IPreparedStatement {
public:
    virtual ~IPreparedStatement() = default;
    
    // Get statement ID
    virtual std::string get_id() const = 0;
    
    // Get query text
    virtual std::string get_query() const = 0;
    
    // Bind named parameter
    virtual Result<bool> bind(const std::string& name, const Scalar& value) = 0;
    
    // Bind positional parameter
    virtual Result<bool> bind(size_t position, const Scalar& value) = 0;
    
    // Bind all parameters at once
    virtual Result<bool> bind_all(const std::map<std::string, Scalar>& params) = 0;
    
    // Execute with current bindings
    virtual Result<RelationalTable> execute() = 0;
    
    // Execute asynchronously
    virtual std::future<Result<RelationalTable>> execute_async() = 0;
    
    // Reset bindings
    virtual Result<bool> reset() = 0;
    
    // Get execution statistics
    virtual Result<QueryStatistics> get_statistics() const = 0;
};

class IPreparedStatementAdapter {
public:
    virtual ~IPreparedStatementAdapter() = default;
    
    // Prepare statement
    virtual Result<std::unique_ptr<IPreparedStatement>> prepare(
        const std::string& query
    ) = 0;
    
    // Unprepare statement
    virtual Result<bool> unprepare(const std::string& statement_id) = 0;
    
    // Get all prepared statements
    virtual Result<std::vector<std::string>> list_prepared() = 0;
};
```

**Benefits:**
- Query plan caching
- Reduced parse overhead
- Parameter type checking
- Better performance for repeated queries

---

### Transaction Savepoint Support
**Priority:** Medium  
**Target Version:** v1.3.0

Extend transaction interfaces with savepoint support.

**Interface Extensions:**
```cpp
class ITransactionAdapter {
public:
    // Existing methods...
    
    // Create savepoint within transaction
    virtual Result<std::string> create_savepoint(
        const std::string& transaction_id,
        const std::string& savepoint_name
    ) = 0;
    
    // Rollback to savepoint
    virtual Result<bool> rollback_to_savepoint(
        const std::string& transaction_id,
        const std::string& savepoint_name
    ) = 0;
    
    // Release savepoint
    virtual Result<bool> release_savepoint(
        const std::string& transaction_id,
        const std::string& savepoint_name
    ) = 0;
    
    // Get transaction state
    virtual Result<TransactionState> get_transaction_state(
        const std::string& transaction_id
    ) = 0;
};

struct TransactionState {
    std::string transaction_id;
    TransactionOptions::IsolationLevel isolation_level;
    std::chrono::system_clock::time_point start_time;
    std::vector<std::string> savepoints;
    bool is_read_only;
    std::optional<std::chrono::milliseconds> elapsed_time;
};
```

---

### Schema Management Interfaces
**Priority:** Low  
**Target Version:** v1.4.0

Add DDL and schema management operations.

**New Interfaces:**
```cpp
class ISchemaAdapter {
public:
    virtual ~ISchemaAdapter() = default;
    
    // Create collection/table
    virtual Result<bool> create_collection(
        const std::string& name,
        const CollectionSchema& schema
    ) = 0;
    
    // Drop collection/table
    virtual Result<bool> drop_collection(
        const std::string& name,
        bool if_exists = true
    ) = 0;
    
    // Alter collection schema
    virtual Result<bool> alter_collection(
        const std::string& name,
        const SchemaChange& change
    ) = 0;
    
    // Create index
    virtual Result<bool> create_index(
        const std::string& collection,
        const std::string& index_name,
        const IndexSpec& spec
    ) = 0;
    
    // Drop index
    virtual Result<bool> drop_index(
        const std::string& collection,
        const std::string& index_name
    ) = 0;
    
    // Get collection schema
    virtual Result<CollectionSchema> get_schema(const std::string& name) = 0;
    
    // List all collections
    virtual Result<std::vector<std::string>> list_collections() = 0;
};

struct CollectionSchema {
    std::string name;
    std::map<std::string, FieldSpec> fields;
    std::vector<std::string> primary_key;
    std::map<std::string, IndexSpec> indexes;
};

struct FieldSpec {
    enum class Type {
        INTEGER, FLOAT, STRING, BOOLEAN, BINARY, VECTOR, JSON, TIMESTAMP
    };
    
    Type type;
    bool nullable = true;
    std::optional<Scalar> default_value;
    std::map<std::string, Scalar> constraints; // min, max, pattern, etc.
};

struct IndexSpec {
    enum class Type {
        BTREE, HASH, VECTOR_HNSW, VECTOR_IVF, FULLTEXT, GEOSPATIAL
    };
    
    Type type;
    std::vector<std::string> fields;
    std::map<std::string, Scalar> parameters;
    bool unique = false;
};

struct SchemaChange {
    enum class Operation {
        ADD_FIELD, DROP_FIELD, RENAME_FIELD, CHANGE_TYPE
    };
    
    Operation operation;
    std::string field_name;
    std::optional<std::string> new_name;
    std::optional<FieldSpec> new_spec;
};
```

---

### Extended Metrics and Profiling
**Priority:** Medium  
**Target Version:** v1.3.0

Add detailed profiling and metrics structures.

**New Types:**
```cpp
struct QueryProfile {
    // Timing breakdown
    std::chrono::microseconds parse_time;
    std::chrono::microseconds optimize_time;
    std::chrono::microseconds execution_time;
    std::chrono::microseconds total_time;
    
    // Resource usage
    size_t peak_memory_bytes;
    size_t total_memory_allocated;
    double cpu_time_seconds;
    
    // I/O statistics
    size_t disk_reads;
    size_t disk_writes;
    size_t bytes_read;
    size_t bytes_written;
    size_t network_bytes_sent;
    size_t network_bytes_received;
    
    // Query execution details
    size_t rows_scanned;
    size_t rows_filtered;
    size_t rows_returned;
    size_t index_seeks;
    size_t index_scans;
    
    // Operator breakdown
    std::vector<OperatorProfile> operators;
    
    // Query plan
    std::string query_plan;
    std::map<std::string, Scalar> optimizer_hints;
};

struct OperatorProfile {
    std::string operator_type; // "IndexScan", "Filter", "Join", etc.
    std::chrono::microseconds execution_time;
    size_t rows_input;
    size_t rows_output;
    size_t memory_used;
    std::map<std::string, Scalar> operator_metrics;
};

class IProfilingAdapter {
public:
    virtual ~IProfilingAdapter() = default;
    
    // Enable/disable profiling
    virtual Result<bool> enable_profiling(bool enabled) = 0;
    
    // Execute query with profiling
    virtual Result<std::pair<RelationalTable, QueryProfile>> 
    execute_with_profile(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;
    
    // Get explain plan without execution
    virtual Result<QueryProfile> explain(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;
};
```

---

### Distributed Query Interfaces
**Priority:** Low  
**Target Version:** v1.4.0

Support for distributed/federated query execution.

**New Interfaces:**
```cpp
struct ShardInfo {
    std::string shard_id;
    std::string endpoint;
    std::vector<std::string> partition_keys;
    std::map<std::string, Scalar> metadata;
};

struct ClusterTopology {
    std::string cluster_name;
    std::vector<ShardInfo> shards;
    std::map<std::string, std::string> shard_assignments;
    std::string coordinator_endpoint;
};

class IDistributedAdapter {
public:
    virtual ~IDistributedAdapter() = default;
    
    // Get cluster topology
    virtual Result<ClusterTopology> get_topology() = 0;
    
    // Execute distributed query
    virtual Result<RelationalTable> execute_distributed(
        const std::string& query,
        const std::vector<Scalar>& params = {},
        const DistributedQueryOptions& options = {}
    ) = 0;
    
    // Get shard for key
    virtual Result<std::string> get_shard_for_key(
        const std::string& collection,
        const Scalar& partition_key
    ) = 0;
    
    // Distributed aggregation
    virtual Result<RelationalTable> aggregate_across_shards(
        const std::string& query,
        const std::vector<std::string>& shard_ids = {}
    ) = 0;
};

struct DistributedQueryOptions {
    std::vector<std::string> target_shards; // Empty = all shards
    bool parallel_execution = true;
    size_t max_parallel_requests = 10;
    std::chrono::milliseconds per_shard_timeout = std::chrono::seconds(30);
    bool fail_fast = false; // Abort on first shard failure
};
```

---

### Batch Operation Configuration
**Priority:** Medium  
**Target Version:** v1.2.0  
**Status:** ✅ Implemented (v1.8.0)

Enhanced batch operation control.

**New Types:**
```cpp
struct BatchOptions {
    size_t batch_size = 1000;
    bool stop_on_first_error = false;
    bool return_failed_items = false;
    std::function<void(size_t processed, size_t total)> progress_callback;
    std::optional<std::chrono::milliseconds> timeout_per_batch;
};

struct BatchResult {
    size_t total_items;
    size_t successful_items;
    size_t failed_items;
    std::chrono::milliseconds total_time;
    std::vector<size_t> failed_indices; // If return_failed_items = true
    std::vector<ErrorCode> error_codes;
    std::vector<std::string> error_messages;
};

class IBatchAdapter {
public:
    virtual ~IBatchAdapter() = default;
    
    // Enhanced batch insert
    virtual Result<BatchResult> batch_insert_advanced(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows,
        const BatchOptions& options
    ) = 0;
    
    // Batch update
    virtual Result<BatchResult> batch_update(
        const std::string& table_name,
        const std::vector<std::pair<Scalar, RelationalRow>>& key_value_pairs,
        const BatchOptions& options
    ) = 0;
    
    // Batch delete
    virtual Result<BatchResult> batch_delete(
        const std::string& table_name,
        const std::vector<Scalar>& keys,
        const BatchOptions& options
    ) = 0;
};
```

---

### Connection Pool Configuration
**Priority:** High  
**Target Version:** v1.1.0

Connection pooling support in adapter interface.

**New Types:**
```cpp
struct ConnectionPoolConfig {
    size_t min_connections = 2;
    size_t max_connections = 10;
    std::chrono::seconds connection_timeout = std::chrono::seconds(30);
    std::chrono::minutes idle_timeout = std::chrono::minutes(5);
    std::chrono::seconds health_check_interval = std::chrono::seconds(60);
    bool test_on_borrow = true;
    size_t max_connection_age_minutes = 30;
};

struct ConnectionPoolStats {
    size_t total_connections;
    size_t active_connections;
    size_t idle_connections;
    size_t total_created;
    size_t total_destroyed;
    size_t total_borrowed;
    size_t total_returned;
    size_t failed_borrows;
    std::chrono::milliseconds avg_wait_time;
};

class IConnectionPoolAdapter {
public:
    virtual ~IConnectionPoolAdapter() = default;
    
    // Initialize connection pool
    virtual Result<bool> initialize_pool(const ConnectionPoolConfig& config) = 0;
    
    // Get pool statistics
    virtual Result<ConnectionPoolStats> get_pool_stats() = 0;
    
    // Resize pool
    virtual Result<bool> resize_pool(size_t new_min, size_t new_max) = 0;
    
    // Close all idle connections
    virtual Result<size_t> close_idle_connections() = 0;
    
    // Health check all connections
    virtual Result<size_t> health_check_connections() = 0;
};
```

---

### Capability Versioning
**Priority:** Medium  
**Target Version:** v1.3.0

Version-aware capability system.

**Extended Types:**
```cpp
struct CapabilityVersion {
    Capability capability;
    std::string version;
    bool enabled;
    std::map<std::string, Scalar> parameters;
    std::vector<std::string> deprecation_notes;
};

class ISystemInfoAdapter {
public:
    // Existing methods...
    
    // Get capability with version info
    virtual Result<CapabilityVersion> get_capability_version(
        Capability cap
    ) = 0;
    
    // Check minimum capability version
    virtual bool has_capability_version(
        Capability cap,
        const std::string& min_version
    ) = 0;
    
    // Get all capabilities with versions
    virtual std::vector<CapabilityVersion> get_capabilities_detailed() = 0;
};
```

---

### Time Series Data Types
**Priority:** Low  
**Target Version:** v1.4.0

Specialized time series support.

**New Types:**
```cpp
struct TimeSeriesPoint {
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, Scalar> tags; // Dimensions
    std::map<std::string, Scalar> fields; // Measurements
};

struct TimeSeriesRange {
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
};

struct TimeSeriesAggregation {
    enum class Function {
        MEAN, MEDIAN, MIN, MAX, SUM, COUNT, STDDEV, PERCENTILE
    };
    
    Function function;
    std::chrono::milliseconds window_size;
    std::string field;
    std::optional<double> percentile; // For PERCENTILE function
};

class ITimeSeriesAdapter {
public:
    virtual ~ITimeSeriesAdapter() = default;
    
    // Insert time series point
    virtual Result<bool> insert_point(
        const std::string& measurement,
        const TimeSeriesPoint& point
    ) = 0;
    
    // Insert multiple points
    virtual Result<size_t> batch_insert_points(
        const std::string& measurement,
        const std::vector<TimeSeriesPoint>& points
    ) = 0;
    
    // Query time series
    virtual Result<std::vector<TimeSeriesPoint>> query_range(
        const std::string& measurement,
        const TimeSeriesRange& range,
        const std::map<std::string, Scalar>& tag_filters = {}
    ) = 0;
    
    // Aggregate time series
    virtual Result<std::vector<TimeSeriesPoint>> aggregate(
        const std::string& measurement,
        const TimeSeriesRange& range,
        const TimeSeriesAggregation& aggregation,
        const std::map<std::string, Scalar>& tag_filters = {}
    ) = 0;
    
    // Downsample time series
    virtual Result<std::vector<TimeSeriesPoint>> downsample(
        const std::string& measurement,
        const TimeSeriesRange& range,
        std::chrono::milliseconds interval
    ) = 0;
};
```

---

### Geospatial Data Types
**Priority:** Low  
**Target Version:** v1.4.0

Enhanced geospatial support.

**New Types:**
```cpp
struct GeoPoint {
    double latitude;
    double longitude;
    std::optional<double> altitude;
};

struct GeoBoundingBox {
    GeoPoint southwest;
    GeoPoint northeast;
};

struct GeoPolygon {
    std::vector<GeoPoint> exterior_ring;
    std::vector<std::vector<GeoPoint>> interior_rings; // Holes
};

struct GeoCircle {
    GeoPoint center;
    double radius_meters;
};

class IGeoSpatialAdapter {
public:
    virtual ~IGeoSpatialAdapter() = default;
    
    // Find within radius
    virtual Result<std::vector<Document>> find_within_radius(
        const std::string& collection,
        const GeoPoint& center,
        double radius_meters,
        const std::map<std::string, Scalar>& filters = {}
    ) = 0;
    
    // Find within bounding box
    virtual Result<std::vector<Document>> find_within_box(
        const std::string& collection,
        const GeoBoundingBox& box,
        const std::map<std::string, Scalar>& filters = {}
    ) = 0;
    
    // Find within polygon
    virtual Result<std::vector<Document>> find_within_polygon(
        const std::string& collection,
        const GeoPolygon& polygon,
        const std::map<std::string, Scalar>& filters = {}
    ) = 0;
    
    // Nearest neighbors (geo)
    virtual Result<std::vector<std::pair<Document, double>>> find_nearest(
        const std::string& collection,
        const GeoPoint& point,
        size_t k,
        const std::map<std::string, Scalar>& filters = {}
    ) = 0;
    
    // Calculate distance
    virtual Result<double> calculate_distance(
        const GeoPoint& point1,
        const GeoPoint& point2
    ) = 0;
};
```

---

### Error Context and Chaining
**Priority:** Medium  
**Target Version:** v1.3.0

Enhanced error handling with context preservation.

**New Types:**
```cpp
struct ErrorContext {
    ErrorCode code;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    std::optional<ErrorContext> cause; // Chained error
    std::map<std::string, Scalar> metadata;
};

template<typename T>
struct ResultV2 {
    std::optional<T> value;
    std::optional<ErrorContext> error;
    
    bool is_ok() const { return value.has_value(); }
    bool is_err() const { return error.has_value(); }
    
    static ResultV2<T> ok(T val) {
        return ResultV2<T>{std::move(val), std::nullopt};
    }
    
    static ResultV2<T> err(ErrorContext ctx) {
        return ResultV2<T>{std::nullopt, std::move(ctx)};
    }
    
    // Error chaining
    template<typename U>
    ResultV2<U> chain_error(const std::string& message) const {
        if (is_ok()) {
            return ResultV2<U>::err(ErrorContext{
                ErrorCode::INTERNAL_ERROR,
                message
            });
        }
        
        ErrorContext new_ctx = *error;
        new_ctx.message = message;
        new_ctx.cause = error;
        return ResultV2<U>::err(std::move(new_ctx));
    }
};
```

---

## Backward Compatibility Strategy

### Interface Versioning

**Namespace Versioning:**
```cpp
namespace chimera::v1 {
    // Original interfaces (v1.0.0)
    class IDatabaseAdapter { ... };
}

namespace chimera::v2 {
    // Extended interfaces (v2.0.0)
    class IDatabaseAdapter : public chimera::v1::IDatabaseAdapter {
        // New methods only
        ...
    };
}

// Current version alias
namespace chimera {
    using IDatabaseAdapter = v2::IDatabaseAdapter;
}
```

**Adapter Inheritance:**
```cpp
// v1 adapter still works
class MyAdapterV1 : public chimera::v1::IDatabaseAdapter {
    // Implements v1 interface
};

// v2 adapter extends
class MyAdapterV2 : public chimera::v2::IDatabaseAdapter {
    // Implements v1 + v2 interface
};
```

---

### Capability-Based Extension Discovery

**Extension Capabilities:**
```cpp
enum class ExtensionCapability {
    // v1.0 capabilities
    RELATIONAL_QUERIES,
    VECTOR_SEARCH,
    // ... existing ...
    
    // v1.1 extensions
    CONNECTION_POOLING,
    TRANSACTION_SAVEPOINTS,
    
    // v1.2 extensions
    ASYNC_OPERATIONS,
    STREAMING_RESULTS,
    PREPARED_STATEMENTS,
    
    // v1.3 extensions
    DETAILED_PROFILING,
    SCHEMA_MANAGEMENT,
    
    // v1.4 extensions
    DISTRIBUTED_QUERIES,
    TIME_SERIES,
    GEOSPATIAL_EXTENDED
};

// Check if adapter supports extension
if (adapter->has_capability(ExtensionCapability::ASYNC_OPERATIONS)) {
    auto async_adapter = dynamic_cast<IAsyncDatabaseAdapter*>(adapter.get());
    // Use async operations
}
```

---

### Default Implementations

**Optional Interface Pattern:**
```cpp
class IOptionalFeatureAdapter {
public:
    virtual ~IOptionalFeatureAdapter() = default;
    
    // Default implementation returns NOT_IMPLEMENTED
    virtual Result<T> optional_feature() {
        return Result<T>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "Optional feature not supported"
        );
    }
};
```

---

## Migration Guidelines

### v1.0 → v1.1 (Connection Pooling)

**Old Code:**
```cpp
auto adapter = AdapterFactory::create("MyDB");
adapter->connect("mydb://localhost/db");
```

**New Code:**
```cpp
auto adapter = AdapterFactory::create("MyDB");

// Optional: Configure pool
if (auto pool_adapter = dynamic_cast<IConnectionPoolAdapter*>(adapter.get())) {
    ConnectionPoolConfig config;
    config.max_connections = 20;
    pool_adapter->initialize_pool(config);
}

adapter->connect("mydb://localhost/db");
```

---

### v1.1 → v1.2 (Async Operations)

**Old Code:**
```cpp
auto result = adapter->execute_query(query);
```

**New Code:**
```cpp
// Check if async supported
if (adapter->has_capability(ExtensionCapability::ASYNC_OPERATIONS)) {
    auto async_adapter = dynamic_cast<IAsyncDatabaseAdapter*>(adapter.get());
    auto future = async_adapter->execute_query_async(query);
    auto result = future.get(); // Wait for completion
} else {
    // Fallback to synchronous
    auto result = adapter->execute_query(query);
}
```

---

### v1.2 → v1.3 (Profiling)

**Old Code:**
```cpp
auto result = adapter->execute_query(query);
```

**New Code:**
```cpp
if (adapter->has_capability(ExtensionCapability::DETAILED_PROFILING)) {
    auto profiling_adapter = dynamic_cast<IProfilingAdapter*>(adapter.get());
    profiling_adapter->enable_profiling(true);
    auto [result, profile] = profiling_adapter->execute_with_profile(query).value();
    analyze_profile(profile);
} else {
    auto result = adapter->execute_query(query);
}
```

---

## Deprecation Policy

**Rules:**
1. Deprecated in v1.x, removed in v2.0
2. Minimum 2 minor versions (6 months) deprecation period
3. Clear [[deprecated]] attributes
4. Migration guides provided

**Example:**
```cpp
// Deprecated in v1.3
[[deprecated("Use ResultV2 instead")]]
template<typename T>
struct Result { ... };

// Removed in v2.0
// Result<T> no longer exists
```

---

## Performance Impact

### New Type Overhead

- `ErrorContext`: +200 bytes (nested error chain)
- `QueryProfile`: +500 bytes (detailed metrics)
- `StreamConfig`: +50 bytes
- `BatchOptions`: +100 bytes (with callbacks)

### Virtual Function Overhead

- Additional interface methods: ~5-10ns per call
- Dynamic_cast for feature detection: ~50-100ns
- Async futures: ~1-10μs overhead

### Memory Overhead

- Additional vtables: +8 bytes per interface
- Async operation state: ~500 bytes per active future
- Streaming buffers: Configurable (default 10MB)

---

## Contributing

### Adding New Interfaces

1. **Design Review:** Propose interface on GitHub Discussions
2. **Capability Flag:** Add new Capability enum value
3. **Interface Definition:** Create new interface class
4. **Documentation:** Document all methods and types
5. **Example Implementation:** Provide reference implementation
6. **Tests:** Add interface tests
7. **Migration Guide:** Document upgrade path

### Interface Design Principles

1. **Vendor Neutral:** No database-specific names
2. **Optional:** Return NOT_IMPLEMENTED for unsupported
3. **Capability Based:** Add Capability enum value
4. **Backward Compatible:** Don't break existing interfaces
5. **Well Documented:** Complete API documentation
6. **Testable:** Provide test framework hooks

---

## Test Strategy

- Unit tests for each new interface in `tests/chimera/`: verify capability detection, method contracts, and error return paths
- Connection pool tests: verify thread-safety under concurrent borrow/return with TSAN; verify `max_connections` limit enforcement
- Async query tests: verify `std::future` resolves correctly; verify `cancel_async()` unblocks waiting callers cleanly
- Prepared statement injection tests: verify that typed `Scalar` binding cannot produce SQL injection regardless of input value
- `IResultStream` tests: verify `has_more()` / `next_batch()` / `close()` lifecycle including early cancellation
- Migration tests: v1.0 adapters implementing only `IDatabaseAdapter` compile and run without modification against v1.1+ runtime

## Performance Targets

- `IConnectionPoolAdapter::acquire()` (pool has idle connection): ≤ 100 µs p99
- `IAsyncDatabaseAdapter::execute_query_async()` dispatch overhead: ≤ 50 µs before thread hand-off
- `IResultStream::next_batch()` overhead (excluding I/O, 1 000-row batch): ≤ 10 µs per call
- `IPreparedStatement::execute()` overhead vs unprepared equivalent query: ≥ 20% faster on repeated execution
- Capability detection via `has_capability()`: ≤ 50 ns (single hash lookup, no locks)
- `IBatchAdapter` batch insert (1 000 rows, local adapter): throughput ≥ 10 K rows/s

## Security / Reliability

- Prepared statement API prevents SQL injection by design: parameters are always bound as typed `Scalar` values, never concatenated
- Connection strings and credentials must never be stored in public header types; callers receive opaque connection handles only
- `IResultStream::close()` must release all server-side resources even when called before the stream is exhausted
- Async operation cancellation (`cancel_async()`) must not leave server-side state inconsistent
- `IConnectionPoolAdapter` must not expose individual raw connection objects; connections are automatically returned to pool
- Distributed query interfaces must validate shard endpoint authenticity before dispatching any queries

## See Also

- [README.md](README.md) - Current interface documentation
- [Source Implementation](../../src/chimera/README.md) - Implementation details
- [Adapter Templates](../../adapters/chimera/README.md) - Creating adapters
- [CONTRIBUTING.md](../../CONTRIBUTING.md) - Contribution guidelines

---

*Last Updated: April 2026*  
*Interface Version: v1.0.0*  
*Next Review: v1.1.0 Release*
