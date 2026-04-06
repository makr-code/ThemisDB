<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Chimera Module - Future Enhancements

- Database adapter/driver abstraction layer for multi-database benchmark scenarios
- Connection pool lifecycle management (acquire, release, health-check, eviction)
- Synchronous and asynchronous query routing to pluggable backend adapters
- Adapter registry supporting version-specific and fallback adapter selection
- Prepared statement caching and batch operation pipelines
- Schema management (DDL) and data-generation helpers for benchmark setup
- Metrics export (Prometheus, OpenTelemetry) from adapter-layer instrumentation
- Distributed query coordination across sharded backends

## Design Constraints

- [ ] All adapter interfaces must be pure-virtual (`IDatabaseAdapter`) — no concrete base classes with side effects
- [ ] Connection pool must be thread-safe; concurrent `acquire`/`release` calls must not deadlock
- [ ] Async API must not block the calling thread; futures must resolve on a configurable executor
- [ ] Adapter registration is global and write-once per process; no runtime unregistration
- [ ] Retry policies must be stateless and composable; no shared mutable retry counters
- [ ] Schema operations must be idempotent (create-if-not-exists semantics)
- [ ] No adapter may store plaintext credentials; connection strings must be treated as secrets

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IDatabaseAdapter` | Benchmark runner, query router | Core CRUD + vector + graph operations |
| `IAsyncDatabaseAdapter` | High-concurrency benchmark | Returns `std::future<Result<T>>` for all ops |
| `ConnectionPool` | All adapter consumers | Provides bounded-pool `acquire`/`release` |
| `IPreparedStatement` | Repeated-query benchmark | Plan-cached parameterized execution |
| `ISchemaAdapter` | Benchmark setup/teardown | DDL: create/drop collections and indexes |
| `IDataGenerator` | Data-loading benchmark | Synthetic row, graph, and vector generation |
| `IMetricsExporter` | Monitoring / CI pipeline | Prometheus + OpenTelemetry metric push |
| `IDistributedAdapter` | Multi-shard benchmark | Cross-shard query fan-out and aggregation |

## Planned Features

### Production ThemisDB Adapter Integration
**Priority:** High
**Target Version:** v1.1.0

`src/chimera/themisdb_adapter.cpp` has **7 confirmed stubs** (lines 105, 121, 136, 161, 192, 247, and the class-level comment "This is a stub implementation demonstrating the adapter pattern"). All data operations (`query`, `insert`, `update`, `delete`, `vector_search`, `graph_traverse`) return empty results or placeholder IDs without calling the actual ThemisDB API.

Replace stub implementation with full ThemisDB client integration.

**Implementation Notes:**
- `[ ]` Inject a `ThemisDBClient` (or `StorageEngine*`) into `ThemisDBAdapter` constructor; remove the in-process simulation maps.
- `[ ]` Replace stub `query()` (line 105 "Would execute actual query via ThemisDB API") with real AQL execution via `AQLRunner::execute()`.
- `[ ]` Replace stub `vector_search()` (line 192 "Return empty results") with `VectorIndex::search()` dispatch.
- `[ ]` Replace stub `graph_traverse()` (line 247 "Return empty path") with `GraphEngine::traverse()`.
- `[ ]` Replace stub `generateId()` (line 161) with UUID generation via `utils/uuid.h`.
- `[ ]` Add integration tests for the wired adapter against a live (in-process) ThemisDB instance.

**Implementation:**
```cpp
class ThemisDBAdapter : public IDatabaseAdapter {
private:
    std::unique_ptr<ThemisDBClient> client_;
    std::unique_ptr<ConnectionPool> pool_;
    
public:
    Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options
    ) override {
        client_ = std::make_unique<ThemisDBClient>(connection_string);
        
        // Parse options
        size_t pool_size = parse_option(options, "pool_size", 10);
        pool_ = std::make_unique<ConnectionPool>(client_.get(), pool_size);
        
        // Establish connection
        return client_->connect() ? 
            Result<bool>::ok(true) : 
            Result<bool>::err(ErrorCode::CONNECTION_ERROR, "Failed to connect");
    }
    
    Result<RelationalTable> execute_query(
        const std::string& query,
        const std::vector<Scalar>& params
    ) override {
        auto conn = pool_->acquire();
        auto result = conn->executeAQL(query, params);
        return convert_result(result);
    }
};
```

**Expected Performance:**
- Query execution: Match native ThemisDB performance
- Connection pooling: Support 100+ concurrent connections
- Batch operations: 10K-100K inserts/second
- Vector search: 1-10ms for k=10 on 1M vectors

---

### MongoDB / Qdrant / Neo4j: Replace In-Process Simulation with Real Drivers
**Priority:** High
**Target Version:** v1.2.0

`mongodb_adapter.cpp`, `qdrant_adapter.cpp`, and `neo4j_adapter.cpp` all document explicitly that they "operate in an in-process simulation mode backed by `std::unordered_map`" when the respective native driver is absent. The simulation passes tests but does not exercise real network I/O, serialization, or back-pressure. Production benchmarks against these backends are meaningless without real driver integration.

**Implementation Notes:**
- `[ ]` **MongoDB**: Wire `#ifdef THEMIS_ENABLE_MONGODB` blocks in `mongodb_adapter.cpp` to actual `mongocxx::client` calls; add `mongocxx::instance` singleton initialization in `AdapterFactory::create()`.
- `[ ]` **Qdrant**: Wire `#ifdef THEMIS_ENABLE_QDRANT` blocks to real HTTP calls (cpp-httplib or cpr); replace simulation `std::unordered_map` with REST API calls to `POST /collections/{name}/points`.
- `[ ]` **Neo4j**: Wire `#ifdef THEMIS_ENABLE_NEO4J` blocks to Bolt protocol client (`neo4j-cpp-driver` or libneo4j-client); replace simulation path traversal with real Cypher `MATCH` queries.
- `[ ]` All three adapters return `metrics.cpu.thread_count = 0` (hardcoded) — populate from the driver's connection pool stats.
- `[ ]` Add CI integration tests using Docker Compose with real MongoDB/Qdrant/Neo4j containers.

---


**Priority:** High  
**Target Version:** v1.1.0

Efficient connection pooling for high-throughput benchmarks.

**Features:**
- Configurable pool size
- Connection health checking
- Automatic reconnection on failures
- Connection timeout management
- Pool statistics and monitoring

**Configuration:**
```cpp
ConnectionPoolConfig pool_config;
pool_config.min_connections = 5;
pool_config.max_connections = 50;
pool_config.connection_timeout = std::chrono::seconds(30);
pool_config.idle_timeout = std::chrono::minutes(5);
pool_config.health_check_interval = std::chrono::seconds(60);

auto adapter = std::make_unique<ThemisDBAdapter>(pool_config);
```

**API:**
```cpp
class ConnectionPool {
public:
    ConnectionPool(DatabaseClient* client, const ConnectionPoolConfig& config);
    
    // Acquire connection (blocks if all busy)
    std::unique_ptr<Connection> acquire(std::chrono::milliseconds timeout);
    
    // Return connection to pool
    void release(std::unique_ptr<Connection> conn);
    
    // Pool statistics
    PoolStats get_stats() const;
    
    // Health management
    void health_check();
    void remove_unhealthy_connections();
};
```

**Pool Statistics:**
```cpp
struct PoolStats {
    size_t total_connections;
    size_t active_connections;
    size_t idle_connections;
    size_t total_acquires;
    size_t total_releases;
    size_t failed_acquires;
    std::chrono::milliseconds avg_acquire_time;
    std::chrono::milliseconds avg_query_time;
};
```

---

### Async/Promise-Based API
**Priority:** Medium  
**Target Version:** v1.2.0  
**Status:** ✅ Implemented (v1.8.0)

Non-blocking async operations for high-concurrency benchmarks.

**Features:**
- Future/Promise-based async operations
- Callback-based completion notifications
- Event loop integration
- Concurrent query execution
- Backpressure handling

**API:**
```cpp
class IAsyncDatabaseAdapter {
public:
    // Async query execution
    virtual std::future<Result<RelationalTable>> execute_query_async(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;
    
    // Async batch insert
    virtual std::future<Result<size_t>> batch_insert_async(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows
    ) = 0;
    
    // Async vector search
    virtual std::future<Result<std::vector<std::pair<Vector, double>>>> search_vectors_async(
        const std::string& collection,
        const Vector& query_vector,
        size_t k
    ) = 0;
};
```

**Usage Example:**
```cpp
ThemisDBAsyncAdapter adapter;
adapter.connect("themisdb://localhost:8529/db");

// Launch multiple queries concurrently
std::vector<std::future<Result<RelationalTable>>> futures;
for (const auto& query : queries) {
    futures.push_back(adapter.execute_query_async(query));
}

// Wait for all results
for (auto& future : futures) {
    auto result = future.get();
    process(result);
}
```

---

### Streaming Result Sets
**Priority:** Medium  
**Target Version:** v1.2.0

Stream large result sets without loading into memory.

**Features:**
- Iterator-based result streaming
- Configurable batch size
- Backpressure handling
- Memory-efficient processing
- Cursor-based pagination

**API:**
```cpp
class ResultStream {
public:
    // Check if more data available
    bool has_next() const;
    
    // Fetch next batch
    std::vector<RelationalRow> next_batch(size_t batch_size = 100);
    
    // Stream statistics
    StreamStats get_stats() const;
};

// Usage
auto stream = adapter->execute_query_stream(query);
while (stream.has_next()) {
    auto batch = stream.next_batch(1000);
    for (const auto& row : batch) {
        process(row);
    }
}
```

---

### Prepared Statement Support
**Priority:** Medium  
**Target Version:** v1.2.0

Prepared statements for repeated query execution with different parameters.

**Features:**
- Query plan caching
- Parameter binding
- Execution plan reuse
- Type-safe parameter binding
- Batch parameter execution

**API:**
```cpp
class IPreparedStatement {
public:
    virtual ~IPreparedStatement() = default;
    
    // Bind parameters
    virtual Result<bool> bind(const std::string& name, const Scalar& value) = 0;
    
    // Execute with bound parameters
    virtual Result<RelationalTable> execute() = 0;
    
    // Reset parameters
    virtual void reset() = 0;
};

// Usage
auto stmt = adapter->prepare("FOR u IN users FILTER u.age > @age RETURN u");
for (int age : {20, 30, 40, 50}) {
    stmt->bind("age", age);
    auto result = stmt->execute();
    process(result);
}
```

---

### Transaction Management Enhancements
**Priority:** High  
**Target Version:** v1.1.0

Full ACID transaction support with advanced features.

**Features:**
- Nested transactions
- Savepoints
- Transaction isolation levels
- Deadlock detection and retry
- Transaction statistics

**API Extensions:**
```cpp
class ITransactionAdapter {
public:
    // Create savepoint
    virtual Result<std::string> create_savepoint(
        const std::string& transaction_id,
        const std::string& savepoint_name
    ) = 0;
    
    // Rollback to savepoint
    virtual Result<bool> rollback_to_savepoint(
        const std::string& transaction_id,
        const std::string& savepoint_name
    ) = 0;
    
    // Get transaction statistics
    virtual Result<TransactionStats> get_transaction_stats(
        const std::string& transaction_id
    ) = 0;
    
    // Automatic retry on deadlock
    virtual Result<T> execute_with_retry(
        std::function<Result<T>()> operation,
        size_t max_retries = 3
    ) = 0;
};
```

**Deadlock Retry Example:**
```cpp
auto result = adapter->execute_with_retry([&]() -> Result<size_t> {
    auto txn_id = adapter->begin_transaction().value();
    
    // Perform operations that might deadlock
    auto r1 = adapter->insert_row("table1", row1);
    auto r2 = adapter->insert_row("table2", row2);
    
    adapter->commit_transaction(txn_id);
    return Result<size_t>::ok(2);
}, 5); // Retry up to 5 times
```

---

### Query Statistics and Profiling
**Priority:** Medium  
**Target Version:** v1.3.0

Detailed query execution metrics for benchmarking.

**Features:**
- Query execution time breakdown
- I/O statistics (bytes read/written)
- CPU usage per query
- Memory usage tracking
- Network latency measurement
- Query plan visualization

**API:**
```cpp
struct QueryProfile {
    std::chrono::microseconds total_time;
    std::chrono::microseconds parse_time;
    std::chrono::microseconds optimize_time;
    std::chrono::microseconds execute_time;
    
    size_t rows_scanned;
    size_t rows_returned;
    size_t bytes_read;
    size_t bytes_written;
    
    double cpu_usage_percent;
    size_t peak_memory_bytes;
    
    std::chrono::microseconds network_latency;
    
    std::string query_plan;
    std::map<std::string, Scalar> custom_metrics;
};

class ISystemInfoAdapter {
public:
    // Get query profile
    virtual Result<QueryProfile> profile_query(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) = 0;
    
    // Enable profiling
    virtual Result<bool> enable_profiling(bool enabled) = 0;
};
```

---

### Multi-Database Adapter Registration
**Priority:** Low  
**Target Version:** v1.3.0

Support for multiple database backends in a single benchmark run.

**Features:**
- Register multiple implementations of same system
- Version-specific adapters
- Fallback adapter selection
- Adapter capability negotiation

**API:**
```cpp
// Register versioned adapters
AdapterFactory::register_adapter(
    "PostgreSQL:14",
    []() { return std::make_unique<PostgreSQL14Adapter>(); }
);

AdapterFactory::register_adapter(
    "PostgreSQL:15",
    []() { return std::make_unique<PostgreSQL15Adapter>(); }
);

AdapterFactory::register_adapter(
    "PostgreSQL:16",
    []() { return std::make_unique<PostgreSQL16Adapter>(); }
);

// Create specific version
auto pg14 = AdapterFactory::create("PostgreSQL:14");

// Create with fallback
auto pg_latest = AdapterFactory::create_with_fallback(
    {"PostgreSQL:16", "PostgreSQL:15", "PostgreSQL:14"}
);
```

---

### Adapter Configuration Validation
**Priority:** Medium  
**Target Version:** v1.2.0  
**Status:** ✅ Implemented (v1.8.0)

Validate adapter configuration before connection.

**Features:**
- Configuration schema validation
- Required parameter checking
- Type validation
- Range checking
- Connection string parsing

**API:**
```cpp
struct AdapterConfig {
    std::string connection_string;
    std::map<std::string, Scalar> options;
    
    // Validate configuration
    Result<bool> validate() const;
    
    // Get validation errors
    std::vector<std::string> get_validation_errors() const;
};

// Usage
AdapterConfig config;
config.connection_string = "themisdb://localhost:8529/db";
config.options["pool_size"] = 50;
config.options["timeout_ms"] = 30000;

if (!config.validate().is_ok()) {
    for (const auto& error : config.get_validation_errors()) {
        std::cerr << "Config error: " << error << std::endl;
    }
}
```

---

### Distributed Query Coordination
**Priority:** Low  
**Target Version:** v1.4.0

Coordinate distributed queries across multiple database nodes.

**Features:**
- Multi-shard query execution
- Result aggregation across nodes
- Distributed transaction coordination
- Partition-aware query routing
- Cross-datacenter query support

**API:**
```cpp
class IDistributedAdapter {
public:
    // Execute query across multiple shards
    virtual Result<RelationalTable> execute_distributed_query(
        const std::string& query,
        const std::vector<std::string>& shard_urls
    ) = 0;
    
    // Get shard topology
    virtual Result<ShardTopology> get_shard_topology() = 0;
    
    // Distributed aggregation
    virtual Result<AggregationResult> distributed_aggregate(
        const std::string& query,
        const AggregationSpec& spec
    ) = 0;
};
```

---

### Error Recovery and Retry Logic
**Priority:** High  
**Target Version:** v1.1.0  
**Status:** ✅ Implemented (v1.8.0)

Automatic error recovery for transient failures.

**Features:**
- Exponential backoff retry
- Circuit breaker pattern
- Health check on retry
- Configurable retry policies
- Error classification (transient vs permanent)

**API:**
```cpp
struct RetryPolicy {
    size_t max_retries = 3;
    std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100);
    double backoff_multiplier = 2.0;
    std::chrono::milliseconds max_delay = std::chrono::seconds(10);
    
    // Error classification
    std::function<bool(ErrorCode)> is_transient;
};

class ConnectionWithRetry {
public:
    ConnectionWithRetry(
        std::unique_ptr<IDatabaseAdapter> adapter,
        const RetryPolicy& policy
    );
    
    // Execute with automatic retry
    template<typename T>
    Result<T> execute_with_retry(std::function<Result<T>()> operation);
};
```

**Usage:**
```cpp
RetryPolicy policy;
policy.max_retries = 5;
policy.is_transient = [](ErrorCode code) {
    return code == ErrorCode::TIMEOUT || 
           code == ErrorCode::CONNECTION_ERROR;
};

ConnectionWithRetry conn(std::move(adapter), policy);

// Automatically retries on transient errors
auto result = conn.execute_with_retry([&]() {
    return adapter->execute_query(query);
});
```

---

### Benchmark-Specific Optimizations
**Priority:** Medium  
**Target Version:** v1.3.0

Optimizations specifically for benchmark workloads.

**Features:**
- Warm-up query execution
- Cache priming
- Result validation
- Baseline performance measurement
- Outlier detection

**API:**
```cpp
class BenchmarkAdapter {
public:
    // Warm up caches
    Result<bool> warmup(const std::vector<std::string>& queries);
    
    // Execute with timing
    Result<BenchmarkResult> execute_benchmark(
        const std::string& query,
        size_t iterations = 100
    );
    
    // Clear all caches
    Result<bool> clear_caches();
    
    // Get baseline performance
    Result<SystemBaseline> measure_baseline();
};

struct BenchmarkResult {
    std::chrono::microseconds min_time;
    std::chrono::microseconds max_time;
    std::chrono::microseconds median_time;
    std::chrono::microseconds p95_time;
    std::chrono::microseconds p99_time;
    double std_dev;
    size_t outlier_count;
};
```

---

### Schema Management
**Priority:** Low  
**Target Version:** v1.4.0

Database schema creation and management for benchmarks.

**Features:**
- Schema creation from DDL
- Index creation and management
- Data type mapping
- Schema migration
- Schema validation

**API:**
```cpp
class ISchemaAdapter {
public:
    // Create collection/table
    virtual Result<bool> create_collection(
        const std::string& name,
        const CollectionSchema& schema
    ) = 0;
    
    // Create index
    virtual Result<bool> create_index(
        const std::string& collection,
        const std::string& index_name,
        const IndexSpec& spec
    ) = 0;
    
    // Drop collection
    virtual Result<bool> drop_collection(const std::string& name) = 0;
    
    // Get schema
    virtual Result<CollectionSchema> get_schema(const std::string& name) = 0;
};
```

---

### Data Generation Integration
**Priority:** Low  
**Target Version:** v1.4.0

Generate benchmark data directly through adapters.

**Features:**
- Configurable data generation
- Realistic data distributions
- Relationship generation (for graphs)
- Vector embedding generation
- Bulk data loading

**API:**
```cpp
class IDataGenerator {
public:
    // Generate synthetic data
    virtual Result<size_t> generate_data(
        const std::string& collection,
        size_t count,
        const DataGenerationSpec& spec
    ) = 0;
    
    // Generate graph structure
    virtual Result<GraphStats> generate_graph(
        const std::string& graph_name,
        size_t node_count,
        double edge_density
    ) = 0;
    
    // Generate vectors
    virtual Result<size_t> generate_vectors(
        const std::string& collection,
        size_t count,
        size_t dimensions
    ) = 0;
};
```

---

### Metric Export Integration
**Priority:** Medium  
**Target Version:** v1.3.0

Export metrics to monitoring systems.

**Features:**
- Prometheus metrics export
- OpenTelemetry integration
- Custom metric collectors
- Real-time metric streaming
- Metric aggregation

**API:**
```cpp
class IMetricsExporter {
public:
    // Export to Prometheus
    virtual Result<bool> export_prometheus(
        const std::string& endpoint,
        const SystemMetrics& metrics
    ) = 0;
    
    // Export to OpenTelemetry
    virtual Result<bool> export_opentelemetry(
        const std::string& endpoint,
        const QueryProfile& profile
    ) = 0;
    
    // Custom exporter
    virtual Result<bool> export_custom(
        const std::string& format,
        const std::map<std::string, Scalar>& data
    ) = 0;
};
```

---

## API Extensions

### Batch Operation Enhancements
**Priority:** High  
**Target Version:** v1.1.0  
**Status:** ✅ Implemented (v1.8.0)

Enhanced batch operations with progress tracking.

**Features:**
- Progress callbacks
- Partial success handling
- Batch size optimization
- Memory management
- Error aggregation

**API:**
```cpp
struct BatchOptions {
    size_t batch_size = 1000;
    bool stop_on_error = false;
    std::function<void(size_t processed, size_t total)> progress_callback;
    std::function<void(size_t batch_index, const Result<size_t>&)> batch_callback;
};

class IBatchAdapter {
public:
    // Batch insert with options
    virtual Result<BatchResult> batch_insert_advanced(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows,
        const BatchOptions& options
    ) = 0;
};

struct BatchResult {
    size_t total_processed;
    size_t successful;
    size_t failed;
    std::vector<Result<size_t>> batch_results;
    std::chrono::milliseconds total_time;
};
```

---

### Capability Negotiation
**Priority:** Medium  
**Target Version:** v1.2.0

Runtime capability discovery and negotiation.

**Features:**
- Version detection
- Feature probing
- Capability fallback
- Compatibility checking

**API:**
```cpp
class ICapabilityNegotiator {
public:
    // Probe capabilities
    virtual Result<std::set<Capability>> probe_capabilities() = 0;
    
    // Check compatibility
    virtual Result<bool> is_compatible_with(
        const std::set<Capability>& required
    ) = 0;
    
    // Get capability details
    virtual Result<CapabilityInfo> get_capability_info(
        Capability cap
    ) = 0;
};

struct CapabilityInfo {
    Capability capability;
    std::string version;
    bool enabled;
    std::map<std::string, Scalar> parameters;
};
```

---

## Performance Roadmap

### v1.1.0 Performance Targets
- Connection pooling: 100+ concurrent connections
- Query throughput: 10K+ queries/second
- Batch insert: 100K+ rows/second
- Vector search: <10ms for k=10 on 1M vectors
- Graph traversal: <100ms for depth 5 on 1M nodes

### v1.2.0 Performance Targets
- Async operations: 50K+ concurrent queries
- Streaming: 100MB/s result throughput
- Prepared statements: 2x speedup for repeated queries
- Connection acquire: <1ms from pool

### v1.3.0 Performance Targets
- Query profiling overhead: <5%
- Metric export: <100μs per metric
- Schema operations: <100ms for index creation
- Benchmark warmup: <1s for typical workload

---

## Backward Compatibility

### API Versioning
**Target Version:** v1.2.0

Versioned interfaces for backward compatibility.

**Strategy:**
```cpp
namespace chimera::v1 {
    class IDatabaseAdapter { /* v1 API */ };
}

namespace chimera::v2 {
    class IDatabaseAdapter : public chimera::v1::IDatabaseAdapter {
        /* v2 API with extensions */
    };
}

// Default to latest
namespace chimera {
    using IDatabaseAdapter = v2::IDatabaseAdapter;
}
```

---

### Deprecation Policy

**Rules:**
1. Features deprecated in v1.x removed in v2.0
2. Minimum 2 minor versions deprecation period
3. Clear deprecation warnings
4. Migration guides provided

---

## Known Limitations & Workarounds

### Limitation #1: No Connection Pooling
**Severity:** High  
**Versions:** v1.0.x

Current implementation creates new connection per adapter.

**Workaround:**
- Reuse adapter instances
- Limit concurrent adapter count
- Use async operations when available

**Planned Fix:** v1.1.0 - Connection pooling

---

### Limitation #2: Synchronous Only
**Severity:** Medium  
**Versions:** v1.0.x, v1.1.x

Only synchronous operations supported.

**Workaround:**
- Use thread pool for parallelism
- Batch operations where possible
- Use multiple adapters

**Planned Fix:** v1.2.0 - Async API

---

### Limitation #3: Limited Error Recovery
**Severity:** Medium  
**Versions:** v1.0.x

No automatic retry on transient failures.

**Workaround:**
- Implement retry logic in benchmark code
- Manual connection health checking
- Catch and handle errors explicitly

**Planned Fix:** v1.1.0 - Automatic retry logic

---

## Contributing to Chimera Module

### Priority Areas for Contribution

**High Priority:**
1. Production ThemisDB integration
2. Connection pooling
3. Transaction enhancements
4. Error recovery
5. Batch operation improvements

**Medium Priority:**
1. Async API
2. Streaming results
3. Prepared statements
4. Query profiling
5. Configuration validation

**Low Priority:**
1. Schema management
2. Data generation
3. Distributed queries
4. Metric export

### Contribution Guidelines

1. **Follow Vendor-Neutral Design**: No branding, no bias
2. **Add Tests**: Unit and integration tests required
3. **Benchmark**: Include performance benchmarks
4. **Document**: Update README and API docs
5. **Backward Compatibility**: Maintain existing interfaces

For detailed guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

## Test Strategy

- Unit tests: ≥ 80% line coverage on `ConnectionPool`, `RetryPolicy`, and all adapter classes
- Mock adapter for unit tests: `MockDatabaseAdapter` returning configurable results/errors
- Integration tests: run against a live ThemisDB testcontainer for each supported version
- Async API: concurrent stress tests launching ≥ 1,000 parallel `execute_query_async` calls; verify no deadlocks or lost futures
- Connection pool: chaos tests that randomly kill backend connections; verify pool self-heals within 5 s
- Retry logic: inject transient `CONNECTION_ERROR` and `TIMEOUT` at configurable rates; verify max-retry limit is respected
- Schema idempotency: run `create_collection` twice and assert no error on second call
- Performance regression gate: CI rejects if batch-insert throughput drops > 10% from baseline

## Security / Reliability

- Connection strings must never appear in log output; mask with `***` before any logging call
- Pool `acquire` must enforce a configurable timeout (default: 30 s) to prevent indefinite thread starvation
- Circuit breaker trips after ≥ 5 consecutive failures within 10 s; half-open probe every 30 s
- All error codes must be classified as `transient` or `permanent`; permanent errors must not trigger retries
- Distributed query coordinator must not accept shard URLs from untrusted user input without allowlist validation
- Adapter factory must reject unknown adapter names with a hard error, not a null pointer
- Metric export endpoints must require authenticated access; unauthenticated export is disabled by default

## See Also

- [README.md](README.md) - Current module documentation
- [Header Documentation](../../include/chimera/README.md) - Interface definitions
- [Adapter Implementations](../../adapters/chimera/README.md) - Concrete adapters
- [Benchmark Suite](../../benchmarks/chimera/README.md) - CHIMERA benchmarks

---

*Last Updated: April 2026*  
*Module Version: v1.0.0*  
*Next Review: v1.1.0 Release*
