# Temporal Module API - Future Enhancements

## Scope

- Public API enhancements for `include/temporal/` headers
- Bi-temporal query interface using explicit `SystemTime` + `ValidTime` parameters
- Time-travel query builder API (`queryAsOf`, `queryFromTo`, builder pattern)
- Temporal index registration API (`TemporalIndexManager`)
- Retention policy API (`RetentionManager`, async enforcement)

## Design Constraints

- [ ] Bi-temporal queries MUST accept both `SystemTime` and `ValidTime` — no ambiguous single-time overload
- [ ] Time-travel API is immutable (read-only, no side effects on historical data)
- [ ] Retention API is async; synchronous `enforceRetention()` is a manual-trigger only
- [ ] All temporal timestamps use `Timestamp` (strong type); raw integers not accepted in public API
- [ ] `ValidTimePeriod` operations (`overlaps`, `contains`, `precedes`) are `const` and `noexcept`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `BiTemporalTable::queryBiTemporal(query, SystemTime, ValidTimePeriod)` | AQL engine, REST API | Both time dimensions required |
| `TemporalQueryEngine::queryAsOf(table, query, Timestamp)` | Query planner | Read-only, no write side effects |
| `TemporalIndexManager::createIndex(table, name, TemporalIndexSpec)` | Schema manager | Returns index name |
| `RetentionManager::setPolicy(table, RetentionPolicy)` | Admin API | Async enforcement by default |
| `SnapshotManager::createSnapshot(tables)` | Transaction module | RAII handle |

## Planned API Extensions

### ~~System-Versioned Table API~~ ✅ Implemented (v1.1.0)
**Priority:** High
**Target Version:** v1.1.0
**Status:** ✅ **Delivered** — `include/temporal/system_versioned_table.h`

Public API for system-versioned temporal tables.

**Implemented API (see system_versioned_table.h for full docs):**
```cpp
// Config struct
struct SystemVersionedTable::Config {
    std::string history_table_name;              // defaults to "<table>_history"
    bool compress_history = true;
    std::chrono::milliseconds retention_period{365LL * 24 * 3600 * 1000};
    bool track_user_id = true;
};

// DDL factory with JSON schema
static SystemVersionedTable createVersionedTable(
    const std::string& table_name,
    const Document& schema,
    Config config,
    const std::string& source_node = "local");

// DML
bool insert(const std::string& key, const Document& doc);
bool update(const std::string& key, const Document& updates);
bool upsert(const std::string& key, const Document& doc);
bool deleteRow(const std::string& key);

// Retention
size_t enforceRetentionPolicy();

// Config accessor
const Config& getConfig() const noexcept;
```

---

### Time-Travel Query API
**Priority:** High
**Target Version:** v1.2.0

API for temporal queries with time-travel capabilities.

**New Headers:**
```cpp
// temporal/temporal_query_engine.h
namespace themisdb {
namespace temporal {

class TemporalQueryEngine {
public:
    explicit TemporalQueryEngine(RocksDBWrapper* storage);

    // Query as of specific time
    Result<std::vector<Document>> queryAsOf(
        const std::string& table_name,
        const std::string& query,
        const Timestamp& as_of_time
    );

    // Query all versions in range
    Result<std::vector<VersionedDocument>> queryFromTo(
        const std::string& table_name,
        const std::string& query,
        const Timestamp& from_time,
        const Timestamp& to_time
    );

    // Query with temporal predicates
    Result<std::vector<Document>> queryWithTemporal(
        const std::string& table_name,
        const std::string& query,
        const TemporalPredicate& predicate
    );

    // Temporal join
    Result<std::vector<Document>> temporalJoin(
        const std::string& left_table,
        const std::string& right_table,
        const std::string& join_condition,
        const TemporalClause& left_temporal,
        const TemporalClause& right_temporal
    );
};

enum class TemporalOperator {
    AS_OF,
    FROM_TO,
    BETWEEN_AND,
    OVERLAPS,
    CONTAINS,
    PRECEDES,
    SUCCEEDS
};

struct TemporalPredicate {
    TemporalOperator op;
    Timestamp start;
    Timestamp end;
};

}}
```

**Example Usage:**
```cpp
TemporalQueryEngine engine(db_storage);

// Query as of specific time
auto employees = engine.queryAsOf(
    "employees",
    "SELECT * WHERE department = 'Engineering'",
    Timestamp::fromString("2024-01-01 00:00:00")
);

// Query all changes in 2024
auto changes = engine.queryFromTo(
    "employees",
    "SELECT * WHERE id = 123",
    Timestamp::fromString("2024-01-01"),
    Timestamp::fromString("2024-12-31")
);

// Temporal join
auto assignments = engine.temporalJoin(
    "employees",
    "departments",
    "employees.dept_id = departments.id",
    TemporalClause::asOf("2024-06-01"),
    TemporalClause::asOf("2024-06-01")
);
```

---

### Retention Policy API
**Priority:** Medium
**Target Version:** v1.3.0

Configurable retention policies for historical data.

**New Headers:**
```cpp
// temporal/retention_manager.h
namespace themisdb {
namespace temporal {

class RetentionManager {
public:
    explicit RetentionManager(RocksDBWrapper* storage);

    // Set retention policy
    Result<bool> setPolicy(
        const std::string& table_name,
        const RetentionPolicy& policy
    );

    // Get current policy
    Result<RetentionPolicy> getPolicy(
        const std::string& table_name
    ) const;

    // Enforce retention (manual trigger)
    Result<RetentionStats> enforceRetention(
        const std::string& table_name
    );

    // Schedule automatic enforcement
    Result<bool> scheduleEnforcement(
        const std::string& table_name,
        std::chrono::seconds interval
    );

    // Archive before deletion
    Result<bool> archiveHistory(
        const std::string& table_name,
        const TimeRange& range,
        const std::string& archive_location
    );
};

enum class RetentionType {
    TIME_BASED,
    STORAGE_BASED,
    VERSION_COUNT_BASED,
    CUSTOM
};

struct RetentionPolicy {
    RetentionType type;

    // Time-based retention
    std::chrono::seconds retention_period;

    // Storage-based retention
    uint64_t max_storage_bytes;

    // Version count-based retention
    size_t max_versions_per_key;

    // Archiving options
    bool archive_before_delete = false;
    std::string archive_location;

    // Custom predicate
    std::function<bool(const VersionedDocument&)> should_keep;
};

struct RetentionStats {
    size_t versions_deleted;
    size_t versions_archived;
    uint64_t bytes_freed;
    std::chrono::milliseconds execution_time;
    std::vector<std::string> errors;
};

}}
```

**Example Usage:**
```cpp
RetentionManager retention(db_storage);

// Time-based retention
RetentionPolicy policy;
policy.type = RetentionType::TIME_BASED;
policy.retention_period = std::chrono::days(365);
policy.archive_before_delete = true;
policy.archive_location = "s3://archive-bucket/employees/";

retention.setPolicy("employees", policy);

// Schedule automatic enforcement (daily)
retention.scheduleEnforcement("employees", std::chrono::hours(24));

// Manual enforcement
auto stats = retention.enforceRetention("employees");
std::cout << "Deleted " << stats.versions_deleted << " versions, "
          << "Freed " << stats.bytes_freed << " bytes" << std::endl;
```

---

### Temporal Index API
**Priority:** High
**Target Version:** v1.2.0

Specialized indexes for temporal queries.

**New Headers:**
```cpp
// temporal/temporal_index.h
namespace themisdb {
namespace temporal {

class TemporalIndexManager {
public:
    explicit TemporalIndexManager(RocksDBWrapper* storage);

    // Create temporal index
    Result<std::string> createIndex(
        const std::string& table_name,
        const std::string& index_name,
        const TemporalIndexSpec& spec
    );

    // Drop temporal index
    Result<bool> dropIndex(
        const std::string& table_name,
        const std::string& index_name
    );

    // Query using temporal index
    Result<std::vector<VersionedDocument>> queryIndex(
        const std::string& index_name,
        const TimeRange& range,
        const std::vector<Filter>& filters = {}
    );

    // Get index statistics
    Result<IndexStats> getIndexStats(
        const std::string& index_name
    ) const;
};

enum class TemporalIndexType {
    TIME_RANGE,     // Optimized for FROM...TO queries
    POINT_IN_TIME,  // Optimized for AS OF queries
    BI_TEMPORAL,    // Transaction + Valid time
    INTERVAL_TREE   // Overlapping period detection
};

struct TemporalIndexSpec {
    TemporalIndexType type;
    std::vector<std::string> indexed_columns;
    bool include_current_data = true;
    bool include_history = true;

    // Index options
    bool create_covering_index = false;
    std::vector<std::string> included_columns;
};

struct IndexStats {
    std::string index_name;
    size_t total_entries;
    uint64_t index_size_bytes;
    std::chrono::microseconds avg_lookup_time;
    double selectivity;
};

}}
```

**Example Usage:**
```cpp
TemporalIndexManager index_mgr(db_storage);

// Create time-range index for efficient history queries
TemporalIndexSpec spec;
spec.type = TemporalIndexType::TIME_RANGE;
spec.indexed_columns = {"department", "role"};
spec.create_covering_index = true;
spec.included_columns = {"name", "salary"};

auto index_name = index_mgr.createIndex(
    "employees",
    "idx_employees_temporal",
    spec
);

// Query using index (much faster than full scan)
auto results = index_mgr.queryIndex(
    *index_name,
    TimeRange::fromTo("2024-01-01", "2024-12-31"),
    {Filter{"department", "Engineering"}}
);

// Check index performance
auto stats = index_mgr.getIndexStats(*index_name);
std::cout << "Avg lookup: " << stats.avg_lookup_time.count() << "μs" << std::endl;
```

---

### Snapshot Isolation API
**Priority:** High
**Target Version:** v1.1.0

Transaction-level snapshot isolation for temporal queries.

**New Headers:**
```cpp
// temporal/snapshot_manager.h
namespace themisdb {
namespace temporal {

class SnapshotManager {
public:
    explicit SnapshotManager(RocksDBWrapper* storage);

    // Create consistent snapshot
    Result<SnapshotHandle> createSnapshot(
        const std::vector<std::string>& tables = {}
    );

    // Query using snapshot
    Result<std::vector<Document>> querySnapshot(
        const SnapshotHandle& snapshot,
        const std::string& table_name,
        const std::string& query
    );

    // Release snapshot (free resources)
    Result<bool> releaseSnapshot(
        const SnapshotHandle& snapshot
    );

    // Get snapshot metadata
    Result<SnapshotMetadata> getSnapshotMetadata(
        const SnapshotHandle& snapshot
    ) const;
};

struct SnapshotHandle {
    std::string snapshot_id;
    Timestamp creation_time;
    uint64_t version_number;
    std::vector<std::string> included_tables;

    // Comparison for ordering
    bool operator<(const SnapshotHandle& other) const;
};

struct SnapshotMetadata {
    SnapshotHandle handle;
    size_t total_tables;
    uint64_t snapshot_size_bytes;
    std::chrono::system_clock::time_point created_at;
    std::chrono::seconds ttl;
    bool is_valid;
};

}}
```

**Example Usage:**
```cpp
SnapshotManager snapshot_mgr(db_storage);

// Create snapshot of all employee-related tables
auto snapshot = snapshot_mgr.createSnapshot({
    "employees", "departments", "salaries"
});

// All queries see consistent view as of snapshot creation
auto employees = snapshot_mgr.querySnapshot(
    *snapshot,
    "employees",
    "SELECT * WHERE department = 'Engineering'"
);

auto departments = snapshot_mgr.querySnapshot(
    *snapshot,
    "departments",
    "SELECT * WHERE budget > 100000"
);

// Queries are isolated from concurrent writes

// Release when done
snapshot_mgr.releaseSnapshot(*snapshot);
```

---

### Bi-Temporal API
**Priority:** Medium
**Target Version:** v1.2.0

Support for both transaction time and application-defined valid time.

**New Headers:**
```cpp
// temporal/bi_temporal.h
namespace themisdb {
namespace temporal {

class BiTemporalTable {
public:
    BiTemporalTable(
        const std::string& table_name,
        RocksDBWrapper* storage
    );

    // Insert with valid time period
    Result<bool> insertWithValidTime(
        const Document& doc,
        const ValidTimePeriod& valid_time
    );

    // Update for specific valid time period
    Result<bool> updateForPeriod(
        const std::string& key,
        const Document& updates,
        const ValidTimePeriod& period
    );

    // Query with both time dimensions
    Result<std::vector<Document>> queryBiTemporal(
        const std::string& query,
        const Timestamp& transaction_time,
        const ValidTimePeriod& valid_time
    );

    // Detect overlapping valid time periods
    Result<std::vector<Document>> findOverlaps(
        const std::string& key,
        const ValidTimePeriod& period
    );
};

struct ValidTimePeriod {
    Timestamp valid_from;
    Timestamp valid_to;

    bool overlaps(const ValidTimePeriod& other) const;
    bool contains(const Timestamp& t) const;
    bool precedes(const ValidTimePeriod& other) const;
    bool meets(const ValidTimePeriod& other) const;
};

}}
```

**Example Usage:**
```cpp
BiTemporalTable contracts("contracts", db_storage);

// Insert contract valid for specific period
ValidTimePeriod contract_period{
    Timestamp::fromString("2024-01-01"),
    Timestamp::fromString("2024-12-31")
};

contracts.insertWithValidTime(
    {{"contract_id", 123}, {"amount", 50000}},
    contract_period
);

// Query: "What did we know on 2024-06-01 about contracts
// valid on 2024-08-15?"
auto results = contracts.queryBiTemporal(
    "SELECT * WHERE amount > 10000",
    Timestamp::fromString("2024-06-01"),  // Transaction time
    ValidTimePeriod::at("2024-08-15")     // Valid time
);
```

---

### Temporal Aggregation API
**Priority:** Medium
**Target Version:** v1.3.0

Aggregations over temporal data with windowing.

**New Headers:**
```cpp
// temporal/aggregator.h
namespace themisdb {
namespace temporal {

class TemporalAggregator {
public:
    explicit TemporalAggregator(RocksDBWrapper* storage);

    // Aggregate over time windows
    Result<std::vector<AggregateResult>> aggregateOverWindows(
        const std::string& table_name,
        const std::string& measure_column,
        const std::string& aggregate_function,
        const WindowSpec& window_spec,
        const TimeRange& range
    );

    // Calculate trends
    Result<TrendAnalysis> analyzeTrend(
        const std::string& table_name,
        const std::string& measure_column,
        const TimeRange& range,
        TrendMethod method = TrendMethod::LINEAR_REGRESSION
    );

    // Moving average
    Result<std::vector<MovingAveragePoint>> movingAverage(
        const std::string& table_name,
        const std::string& measure_column,
        size_t window_size,
        const TimeRange& range
    );
};

enum class WindowType {
    TUMBLING,   // Non-overlapping fixed windows
    SLIDING,    // Overlapping windows
    SESSION     // Dynamic windows based on gaps
};

struct WindowSpec {
    WindowType type;
    std::chrono::seconds window_size;
    std::chrono::seconds slide_interval;  // For SLIDING
    std::chrono::seconds gap_duration;     // For SESSION
};

struct AggregateResult {
    Timestamp window_start;
    Timestamp window_end;
    double value;
    size_t count;
    std::map<std::string, std::string> group_values;
};

}}
```

**Example Usage:**
```cpp
TemporalAggregator aggregator(db_storage);

// Monthly sales aggregation
WindowSpec monthly_windows{
    WindowType::TUMBLING,
    std::chrono::hours(24 * 30),  // ~30 days
    std::chrono::seconds(0)
};

auto monthly_sales = aggregator.aggregateOverWindows(
    "sales",
    "amount",
    "SUM",
    monthly_windows,
    TimeRange::year(2024)
);

for (const auto& result : monthly_sales) {
    std::cout << "Month starting " << result.window_start
              << ": $" << result.value << std::endl;
}

// Analyze trend
auto trend = aggregator.analyzeTrend(
    "sales",
    "amount",
    TimeRange::year(2024)
);
std::cout << "Sales trend slope: " << trend.slope << std::endl;
```

---

## API Design Principles

### Consistency with Existing APIs
All temporal APIs follow established ThemisDB patterns:
- `Result<T>` for operations that can fail
- Const correctness for read-only operations
- RAII resource management (snapshots, indexes)
- Builder pattern for complex configuration

### Type Safety
```cpp
// Strong typing for timestamps
Timestamp t1 = Timestamp::now();
Timestamp t2 = Timestamp::fromString("2024-01-01");
Timestamp t3 = Timestamp::fromUnixMillis(1704067200000);

// Type-safe time ranges
TimeRange range = TimeRange::fromTo(t1, t2);
TimeRange year = TimeRange::year(2024);
TimeRange month = TimeRange::month(2024, 6);

// Type-safe periods
ValidTimePeriod period{start, end};
```

### Error Handling
```cpp
// All fallible operations return Result<T>
auto result = table.insert(doc);
if (!result.is_ok()) {
    std::cerr << "Error: " << result.error().message << std::endl;
    std::cerr << "Code: " << static_cast<int>(result.error().code) << std::endl;
}

// Or use exceptions if enabled
try {
    auto table = SystemVersionedTable::createOrThrow(...);
} catch (const TemporalException& e) {
    // Handle error
}
```

### Resource Management
```cpp
// RAII for snapshots
{
    auto snapshot = snapshot_mgr.createSnapshot();
    // Use snapshot
} // Automatically released

// Or explicit control
auto snapshot = snapshot_mgr.createSnapshot();
// ... use snapshot ...
snapshot_mgr.releaseSnapshot(*snapshot);
```

---

## Backward Compatibility

### API Versioning
```cpp
// v1.0 API remains available
namespace themisdb::temporal::v1 {
    class TemporalConflictResolver { /* v1 API */ };
}

// v2.0 API with extensions
namespace themisdb::temporal::v2 {
    class TemporalConflictResolver : public v1::TemporalConflictResolver {
        /* Extended API */
    };
}

// Default to latest
namespace themisdb::temporal {
    using TemporalConflictResolver = v2::TemporalConflictResolver;
}
```

### Deprecation Process
1. Feature marked deprecated in version N
2. Deprecation warning issued for 2 minor versions
3. Feature removed in version N+2

### Migration Support
```cpp
// Migration helper API (v1.1.0+)
namespace themisdb::temporal {

class LegacyMigrator {
public:
    // Migrate v1.0 snapshots to v1.1 format
    Result<bool> migrateSnapshots();

    // Convert manual history to system-versioned
    Result<bool> convertToSystemVersioned(
        const std::string& table_name,
        const std::string& history_table_name
    );
};

}
```

---

## Test Strategy

- Unit tests for each public API class (`SystemVersionedTable`, `TemporalQueryEngine`, `BiTemporalTable`, etc.)
- Integration tests: bi-temporal round-trip (insert → update → `queryBiTemporal` asserts correct version)
- Boundary tests: `ValidTimePeriod` edge cases (zero-length periods, open-ended periods)
- Regression tests: time-travel queries return consistent snapshots under concurrent writes
- Compile-time tests: verify no ambiguous single-time overloads are introduced

## Performance Targets

- Time-travel query setup (`queryAsOf` call overhead before result iteration): ≤ 5 ms
- Temporal index lookup (`queryIndex` for single key): ≤ 200 µs
- Retention enforcement trigger (`enforceRetention` call latency before async work begins): ≤ 10 ms per collection
- `SnapshotManager::createSnapshot` (empty snapshot handle creation): ≤ 1 ms

## Security / Reliability

- Time-travel queries respect row-level security; unauthorized rows MUST NOT appear in historical results
- Retention deletes are audit-logged with user identity, timestamp, and version count
- `archiveHistory` validates archive destination path before writing
- `SnapshotHandle` is non-copyable to prevent accidental resource duplication
- Bi-temporal inserts validate that `valid_from < valid_to`; malformed periods are rejected with `Result` error

## See Also

- [Implementation FUTURE_ENHANCEMENTS](../../src/temporal/FUTURE_ENHANCEMENTS.md) - Detailed implementation plans
- [Current API](README.md) - v1.0.0 API documentation
- [Migration Guide](../../docs/migration/temporal.md) - Version migration guides
- [Examples](../../examples/temporal/) - Usage examples

---

*Last Updated: April 2026*
*Target API Version: v2.0.0*
*Current Stable: v1.0.0*
