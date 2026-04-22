> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Temporal Module - Future Enhancements

The temporal module adds SQL:2011 bi-temporal table support to ThemisDB. It covers: system-versioned tables (transaction time), application-versioned tables (valid time), time-travel queries (`AS OF`, `FROM…TO`, `BETWEEN…AND`), temporal indexes (B-tree and interval-tree), automated retention and archival policies, temporal aggregations (tumbling, sliding, session windows), temporal foreign keys with period-aware referential integrity, and CDC streaming of version change events. Affected source files: `temporal_manager.cpp`, `temporal_query_engine.cpp`, `temporal_index_manager.cpp`, `retention_policy_manager.cpp`, and associated headers under `include/temporal/`.

---

## Design Constraints

- [ ] All temporal operations must preserve transaction atomicity; history table writes and current-table writes occur in the same RocksDB `WriteBatch`.
- [ ] System time columns (`sys_start`, `sys_end`) are database-generated and must never be writable by application code; DML attempts to set them directly return `PermissionDenied`.
- [ ] Time-travel queries must be read-only and must not acquire write locks; they operate on immutable history table snapshots.
- [ ] Retention enforcement runs as a background task and must not block foreground read/write operations; incremental deletion caps per-run latency at ≤ 100 ms per batch.
- [ ] The bi-temporal model uses UTC timestamps with nanosecond precision throughout; no timezone conversion is applied at storage time.
- [ ] New SQL syntax extensions (`FOR SYSTEM_TIME`, `FOR APPLICATION_TIME`) are additive and must not break or alter the behavior of existing non-temporal queries.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `SystemVersionedTable::insert(table, doc)` | `TemporalManager`, SQL DML layer | Writes current row + history row atomically in one `WriteBatch` |
| `TemporalQueryEngine::executeTemporalQuery(table, query, spec)` | AQL engine, REST query endpoint | `spec` encodes clause type (`AS_OF`, `FROM_TO`, `BETWEEN_AND`) and timestamps |
| `TemporalIndexManager::createTemporalIndex(table, name, spec)` | DDL handler | Creates B-tree or interval-tree index depending on `spec.type` |
| `RetentionPolicyManager::setRetentionPolicy(table, policy)` | Admin API, `ALTER TABLE … SET RETENTION` DDL | Policy enforced by scheduled background job |
| `TemporalCDC::subscribeToChanges(table, callback)` | Replication module, Kafka connector | Delivers typed `ChangeEvent` per committed version write |

---

## Planned Features

### ~~Full System-Versioned Table Support~~ ✅ Implemented (v1.1.0)
**Priority:** High
**Target Version:** v1.1.0
**Status:** ✅ **Delivered** — see `include/temporal/system_versioned_table.h` and `src/temporal/system_versioned_table.cpp`

Complete implementation of SQL:2011 temporal table standard.

**Implemented Features:**
- ✅ Automatic history table creation and management (`history_table_name` in `Config`)
- ✅ Transparent version tracking on all DML operations (`insert`, `update`, `upsert`, `deleteRow`)
- ✅ System-generated transaction time columns (`sys_time` with auto-assigned `now()`)
- ✅ Efficient storage of historical versions (configurable `retention_period`; `enforceRetentionPolicy()`)
- ✅ Integration with table DDL operations (`createVersionedTable` static factory with JSON schema)
- ✅ User attribution tracking (`track_user_id` / `modified_by`)
- ✅ Compression flag (`compress_history`) stored in config

**Delivered API:**
```cpp
struct SystemVersionedTable::Config {
    std::string history_table_name;          // defaults to "<table>_history"
    bool compress_history = true;
    std::chrono::milliseconds retention_period{365LL * 24 * 3600 * 1000}; // 1 year
    bool track_user_id = true;
};

// DDL factory — schema stored in getStatistics()["schema"]
static SystemVersionedTable createVersionedTable(
    const std::string& table_name,
    const Document& schema,
    Config config,
    const std::string& source_node = "local");

// Atomic insert-or-update (returns true = insert, false = update)
bool upsert(const std::string& key, const Document& doc);

// Apply config.retention_period; purge closed versions older than cutoff
size_t enforceRetentionPolicy();

const Config& getConfig() const noexcept;
```

**DDL Syntax (planned for AQL layer — Phase 3b):**
```sql
CREATE TABLE employees (
    id INTEGER PRIMARY KEY,
    name VARCHAR(100),
    salary DECIMAL(10,2),
    department VARCHAR(50),
    sys_start TIMESTAMP GENERATED ALWAYS AS ROW START,
    sys_end TIMESTAMP GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME (sys_start, sys_end)
)
WITH SYSTEM VERSIONING;
```

**Performance Notes:**
- History table write overhead: <15% (single in-memory append per DML operation)
- History table storage: 2-3x with compression (compress_history flag ready; codec integration is Phase 4)
- Time-travel query performance: 80-95% of current table queries (O(n) scan; temporal index integration in Phase 3b)

---

### ~~Application-Versioned Tables (Bi-Temporal)~~ ✅ Implemented (v1.2.0)
**Priority:** High
**Target Version:** v1.2.0
**Status:** ✅ **Delivered** — see `include/temporal/bi_temporal.h` and `src/temporal/bi_temporal.cpp`

Support for user-defined valid time periods alongside system transaction time.

**Implemented Features:**
- ✅ User-controlled valid time periods (`BiTemporalTable::insertWithValidTime`, `updateForValidTime`)
- ✅ Bi-temporal queries (transaction time + valid time) (`BiTemporalTable::queryBiTemporal`, `scanBiTemporal`)
- ✅ Temporal foreign keys (`TemporalForeignKey::validate()` — period-aware referential integrity check)
- ✅ Temporal uniqueness constraints (`BiTemporalTable::hasUniquenessConflict` + overlap rejection on insert)
- ✅ Gap and overlap detection (`BiTemporalTable::findGaps`, `BiTemporalTable::findOverlaps`)

**Delivered API:**
```cpp
// Temporal uniqueness constraint check
bool BiTemporalTable::hasUniquenessConflict(
    const std::string& key,
    const TimeRange& period) const;

// Gap detection in valid-time coverage
std::vector<TimeRange> BiTemporalTable::findGaps(
    const std::string& key,
    Timestamp from,
    Timestamp to) const;

// Period-aware referential integrity
struct TemporalForeignKey {
    std::string parent_table_name;
    bool validate(const BiTemporalTable& parent_table,
                  const std::string& parent_key,
                  const TimeRange& child_period) const;
};
```

**DDL Syntax (planned for AQL layer — Phase 3b):**
```sql
CREATE TABLE contracts (
    contract_id INTEGER PRIMARY KEY,
    customer_id INTEGER,
    amount DECIMAL(10,2),
    valid_from DATE,
    valid_to DATE,
    PERIOD FOR APPLICATION_TIME (valid_from, valid_to),
    sys_start TIMESTAMP GENERATED ALWAYS AS ROW START,
    sys_end TIMESTAMP GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME (sys_start, sys_end)
)
WITH SYSTEM VERSIONING;
```

---

### ~~Time-Travel Query Engine~~ ✅ Implemented (v1.2.0)
**Priority:** High
**Target Version:** v1.2.0
**Status:** ✅ **Delivered** — see `include/temporal/temporal_query_engine.h` and `src/temporal/temporal_query_engine.cpp`

Full SQL:2011 temporal query support with optimization.

**Features:**
- FOR SYSTEM_TIME AS OF queries
- FOR SYSTEM_TIME BETWEEN...AND queries
- FOR SYSTEM_TIME FROM...TO queries
- FOR APPLICATION_TIME queries
- Temporal predicates (OVERLAPS, CONTAINS, etc.)
- Query optimization for temporal operations

**Implementation:**
```cpp
class TemporalQueryEngine {
public:
    enum class TemporalClause {
        AS_OF,
        FROM_TO,
        BETWEEN_AND,
        CONTAINED_IN,
        OVERLAPS
    };

    struct TemporalQuery {
        TemporalClause clause;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        bool include_deleted = false;
    };

    // Execute temporal query
    Result<std::vector<Document>> executeTemporalQuery(
        const std::string& table_name,
        const std::string& base_query,
        const TemporalQuery& temporal_spec
    );

    // Temporal join
    Result<std::vector<Document>> temporalJoin(
        const std::string& left_table,
        const std::string& right_table,
        const std::string& join_condition,
        const TemporalQuery& left_temporal,
        const TemporalQuery& right_temporal
    );
};
```

**Query Examples:**
```sql
-- As of specific time
SELECT * FROM employees
FOR SYSTEM_TIME AS OF TIMESTAMP '2024-01-01 00:00:00'
WHERE department = 'Engineering';

-- All versions in range
SELECT * FROM employees
FOR SYSTEM_TIME FROM TIMESTAMP '2024-01-01'
                  TO TIMESTAMP '2024-12-31'
WHERE employee_id = 12345;

-- Temporal join
SELECT e.name, d.dept_name
FROM employees FOR SYSTEM_TIME AS OF TIMESTAMP '2024-06-01' e
JOIN departments FOR SYSTEM_TIME AS OF TIMESTAMP '2024-06-01' d
  ON e.department_id = d.dept_id;
```

**Optimization:**
- Temporal index utilization
- Version pruning for AS OF queries
- Batch fetching for range queries
- Result caching for frequently accessed historical data

---

### ~~Temporal Indexes~~ ✅ Implemented (v1.2.0)
**Priority:** High
**Target Version:** v1.2.0
**Status:** ✅ **Delivered** — see `include/temporal/temporal_index.h` and `src/temporal/temporal_index.cpp`

Specialized indexes for efficient temporal queries.

**Features:**
- Time-range indexes for efficient period queries
- Bi-temporal indexes (transaction + valid time)
- Covering indexes for common temporal patterns
- Index-only scans for temporal queries
- Automatic index selection

**Implementation:**
```cpp
class TemporalIndexManager {
public:
    enum class TemporalIndexType {
        TIME_RANGE,        // Optimized for range queries
        BI_TEMPORAL,       // Transaction + Valid time
        SNAPSHOT,          // Point-in-time queries
        INTERVAL_TREE      // Overlapping period detection
    };

    struct TemporalIndexSpec {
        TemporalIndexType type;
        std::vector<std::string> columns;
        std::string time_column;
        bool include_current = true;
        bool include_history = true;
    };

    // Create temporal index
    Result<std::string> createTemporalIndex(
        const std::string& table_name,
        const std::string& index_name,
        const TemporalIndexSpec& spec
    );

    // Query using temporal index
    Result<std::vector<VersionedDocument>> queryWithTemporalIndex(
        const std::string& index_name,
        const TimeRange& range
    );
};
```

**Index Types:**
1. **Time-Range Index**: B-tree on (start_time, end_time) for range queries
2. **Bi-Temporal Index**: Composite index on transaction time + valid time
3. **Snapshot Index**: Optimized for AS OF queries
4. **Interval Tree**: Efficient overlap detection for period queries

**Performance Impact:**
- AS OF queries: 10-100x speedup with snapshot index
- Range queries: 5-50x speedup with time-range index
- Overlap detection: 50-1000x speedup with interval tree

---

### Automated Retention Policies
**Priority:** Medium
**Target Version:** v1.3.0

Automated lifecycle management for historical data.

**Features:**
- Time-based retention policies
- Storage-based retention policies
- Selective retention by table or column
- Archived data migration to cold storage
- Compliance-aware retention rules

**Implementation:**
```cpp
class RetentionPolicyManager {
public:
    enum class RetentionType {
        TIME_BASED,        // Keep data for N days/months/years
        STORAGE_BASED,     // Keep latest N GB of history
        COUNT_BASED,       // Keep latest N versions
        CUSTOM             // User-defined policy
    };

    struct RetentionPolicy {
        RetentionType type;
        std::chrono::seconds retention_period;
        uint64_t max_storage_bytes;
        size_t max_version_count;
        bool archive_before_delete = true;
        std::string archive_location;
        std::function<bool(const Document&)> custom_predicate;
    };

    // Set retention policy for table
    Result<bool> setRetentionPolicy(
        const std::string& table_name,
        const RetentionPolicy& policy
    );

    // Execute retention policy
    Result<RetentionStats> enforceRetention(
        const std::string& table_name
    );

    // Archive historical data
    Result<bool> archiveHistory(
        const std::string& table_name,
        const TimeRange& range,
        const std::string& archive_location
    );
};

struct RetentionStats {
    size_t versions_deleted;
    size_t versions_archived;
    uint64_t space_freed_bytes;
    std::chrono::milliseconds execution_time;
};
```

**Policy Examples:**
```sql
-- Time-based retention
ALTER TABLE employees
SET RETENTION_PERIOD = INTERVAL '2 YEARS';

-- Storage-based retention
ALTER TABLE audit_log
SET MAX_HISTORY_SIZE = '100 GB';

-- Custom retention with archiving
ALTER TABLE transactions
SET RETENTION_POLICY = (
    PERIOD = INTERVAL '1 YEAR',
    ARCHIVE_BEFORE_DELETE = TRUE,
    ARCHIVE_LOCATION = 's3://archive-bucket/transactions/'
);
```

**Background Processing:**
- Scheduled retention enforcement (daily/weekly)
- Incremental cleanup to avoid performance impact
- Progress tracking and logging
- Automatic retry on failures

---

### Temporal Aggregations
**Priority:** Medium
**Target Version:** v1.3.0

Advanced aggregation operations over temporal data.

**Features:**
- Time-window aggregations
- Sliding window calculations
- Temporal GROUP BY operations
- Snapshot aggregations at regular intervals
- Trend analysis functions

**Implementation:**
```cpp
class TemporalAggregator {
public:
    enum class WindowType {
        TUMBLING,      // Non-overlapping windows
        SLIDING,       // Overlapping windows
        SESSION        // Gap-based windows
    };

    struct AggregationSpec {
        WindowType window_type;
        std::chrono::seconds window_size;
        std::chrono::seconds slide_interval;  // For sliding windows
        std::string aggregation_function;  // SUM, AVG, COUNT, etc.
        std::vector<std::string> group_by_columns;
    };

    // Aggregate over time windows
    Result<std::vector<AggregateResult>> aggregateOverTime(
        const std::string& table_name,
        const std::string& measure_column,
        const AggregationSpec& spec,
        const TimeRange& range
    );

    // Calculate trends
    Result<TrendAnalysis> analyzeTrend(
        const std::string& table_name,
        const std::string& measure_column,
        const TimeRange& range
    );
};

struct AggregateResult {
    std::chrono::system_clock::time_point window_start;
    std::chrono::system_clock::time_point window_end;
    double aggregate_value;
    size_t record_count;
    std::map<std::string, std::string> group_values;
};
```

**Query Examples:**
```sql
-- Monthly sales aggregation
SELECT
    YEAR(sys_start) as year,
    MONTH(sys_start) as month,
    SUM(amount) as total_sales
FROM sales
FOR SYSTEM_TIME FROM '2024-01-01' TO '2024-12-31'
GROUP BY YEAR(sys_start), MONTH(sys_start);

-- Moving average over time
SELECT
    sys_start,
    AVG(price) OVER (
        ORDER BY sys_start
        ROWS BETWEEN 29 PRECEDING AND CURRENT ROW
    ) as moving_avg_30day
FROM products
FOR SYSTEM_TIME FROM '2024-01-01' TO '2024-12-31';
```

---

### Temporal Foreign Keys
**Priority:** Medium
**Target Version:** v1.4.0

Referential integrity for temporal relationships.

**Features:**
- Temporal foreign key constraints
- Period-aware referential integrity
- Cascade operations for temporal updates
- Temporal referential actions (CASCADE, RESTRICT, SET NULL)

**Implementation:**
```cpp
class TemporalConstraintManager {
public:
    struct TemporalForeignKey {
        std::string constraint_name;
        std::string source_table;
        std::string target_table;
        std::vector<std::string> source_columns;
        std::vector<std::string> target_columns;
        bool check_temporal_overlap = true;
        std::string on_delete_action;  // CASCADE, RESTRICT, SET NULL
        std::string on_update_action;
    };

    // Add temporal foreign key
    Result<bool> addTemporalForeignKey(
        const TemporalForeignKey& fk_spec
    );

    // Validate temporal referential integrity
    Result<std::vector<IntegrityViolation>> validateIntegrity(
        const std::string& table_name
    );
};
```

**DDL Syntax:**
```sql
ALTER TABLE employee_assignments
ADD CONSTRAINT fk_temporal_employee
FOREIGN KEY (employee_id, PERIOD FOR valid_time)
REFERENCES employees (employee_id, PERIOD FOR valid_time);
```

---

### ~~Interval-Tree Index~~ ✅ Implemented (v1.6.0)
**Priority:** High
**Target Version:** v1.4.0
**Status:** Delivered in `src/temporal/interval_tree_index.cpp` + `include/temporal/interval_tree_index.h`

Augmented BST-based interval tree providing O(log n) insert/remove and O(log n + k) overlap detection for valid-time period predicates and period-aware foreign-key enforcement.

**Delivered Features:**
- ✅ Max-end tracking per node for O(log n + k) overlap queries
- ✅ Half-open interval semantics `[start, end)`
- ✅ `insert()`, `remove()`, `removeKey()`, `clear()` operations
- ✅ `queryPoint(t)`, `queryOverlap(from, to)`, `queryKey(key, range)` queries
- ✅ `stats()` snapshot for observability (total entries, query counters, tree height)
- ✅ Thread-safe via `std::mutex`

---

### ~~Temporal Change Data Capture (CDC)~~ ✅ Implemented (v1.6.0)
**Priority:** Medium
**Target Version:** v1.4.0
**Status:** Delivered in `src/temporal/temporal_cdc.cpp` + `include/temporal/temporal_cdc.h`

Stream temporal change events for real-time processing.

**Features:**
- Real-time change event streaming
- Version-aware CDC
- Temporal diff generation
- Integration with event systems (Kafka, etc.)
- Replay historical changes

**Actual implementation — see `include/temporal/temporal_cdc.h`:**
```cpp
std::string subscribeToChanges(
    const std::string& table_name,
    std::function<void(const ChangeEvent&)> callback);

bool unsubscribe(const std::string& sub_id);
void publishEvent(const ChangeEvent& event);

std::vector<ChangeEvent> replayChanges(
    const std::string& table_name,
    const TimeRange& range) const;

size_t logSize() const;
uint64_t totalPublished() const noexcept;
void clearLog();
```

---

### ~~Temporal Conflict Detection and Resolution~~ ✅ Implemented (v1.1.0)
**Priority:** High
**Target Version:** v1.1.0
**Status:** Delivered in `src/temporal/temporal_conflict_resolver.cpp` + `include/temporal/temporal_conflict_resolver.h`

Enhanced conflict detection for distributed temporal databases.

**Features:**
- ✅ Multi-version concurrency control (MVCC) — concurrent HLC detection via `detectConcurrentUpdate`
- ✅ Optimistic locking for temporal updates — CONCURRENT_UPDATE conflict type
- ✅ Automatic conflict resolution strategies — `autoResolveConflict` with configurable `ConflictPolicy`
- ✅ Manual conflict resolution interface — `queueForManualResolution`, `getQueuedConflicts`, `clearQueue`
- ✅ Conflict audit trail — inherited from `TemporalConflictResolver::exportAuditLog`

**Implementation:**
```cpp
class TemporalConflictDetector {
public:
    enum class ConflictType {
        CONCURRENT_UPDATE,
        OVERLAPPING_PERIODS,
        REFERENTIAL_INTEGRITY,
        UNIQUENESS_VIOLATION
    };

    struct Conflict {
        ConflictType type;
        std::string entity_id;
        TemporalSnapshot local_version;
        TemporalSnapshot remote_version;
        std::vector<std::string> affected_columns;
    };

    // Detect conflicts
    Result<std::vector<Conflict>> detectConflicts(
        const std::string& table_name,
        const TemporalSnapshot& local,
        const TemporalSnapshot& remote
    );

    // Auto-resolve conflicts
    Result<TemporalSnapshot> autoResolveConflict(
        const Conflict& conflict,
        ConflictPolicy policy
    );

    // Queue for manual resolution
    Result<bool> queueForManualResolution(
        const Conflict& conflict
    );
};
```

---

### ~~Snapshot Isolation~~ ✅ Implemented (v1.3.0)
**Priority:** High
**Target Version:** v1.1.0
**Status:** ✅ **Delivered** — see `include/temporal/snapshot_manager.h` and `src/temporal/snapshot_manager.cpp`

Transactional isolation for temporal queries.

**Features:**
- Consistent snapshot creation
- Repeatable read isolation for temporal queries
- Snapshot versioning
- Snapshot garbage collection
- Distributed snapshot coordination

**Implementation:**
```cpp
class TemporalSnapshotManager {
public:
    // Create consistent snapshot
    Result<SnapshotHandle> createSnapshot(
        const std::vector<std::string>& tables
    );

    // Query using snapshot
    Result<std::vector<Document>> querySnapshot(
        const SnapshotHandle& snapshot,
        const std::string& query
    );

    // Release snapshot
    Result<bool> releaseSnapshot(
        const SnapshotHandle& snapshot
    );
};

struct SnapshotHandle {
    std::string snapshot_id;
    std::chrono::system_clock::time_point creation_time;
    std::vector<std::string> included_tables;
    uint64_t version_number;
};
```

---

### ~~Temporal Data Compression~~ ✅ Implemented (v1.6.0)
**Priority:** Medium
**Target Version:** v1.3.0
**Status:** Delivered in `src/temporal/temporal_compressor.cpp` + `include/temporal/temporal_compressor.h`

Efficient compression for historical data.

**Features:**
- Delta compression between versions
- Time-series specific compression algorithms
- Selective column compression
- Transparent decompression on query
- Compression ratio monitoring

**Implementation:**
```cpp
class TemporalCompressor {
public:
    enum class CompressionAlgorithm {
        DELTA,          // Store deltas between versions
        ZSTD,           // General-purpose compression
        GORILLA,        // Time-series optimized
        DICTIONARY      // Dictionary encoding
    };

    struct CompressionConfig {
        CompressionAlgorithm algorithm;
        int compression_level = 3;
        bool compress_immediately = false;
        std::chrono::seconds delay_before_compression{24 * 3600}; // 1 day
    };

    // Compress historical versions
    CompressionStats compressHistory(
        const SystemVersionedTable& table,
        const TimeRange& range,
        const CompressionConfig& config
    );
};

struct CompressionStats {
    size_t versions_processed;
    size_t versions_compressed;
    size_t versions_skipped;
    size_t original_size_bytes;
    size_t compressed_size_bytes;
    double compression_ratio;
    std::chrono::milliseconds compression_time;
};
```

**Expected Compression Ratios:**
- Delta compression: 5-10x for similar versions
- ZSTD: 2-5x for general data
- Gorilla: 10-20x for time-series numeric data
- Dictionary: 3-8x for high-cardinality string data

---

## Performance Roadmap

### v1.1.0 Performance Targets
- System-versioned insert overhead: <15%
- AS OF query performance: 80-95% of current table
- History table compression: 3-5x space savings
- Conflict resolution latency: <10ms

### v1.2.0 Performance Targets
- Time-travel query optimization: 10-100x speedup with indexes
- Bi-temporal query support: <2x overhead vs single time dimension
- Temporal join performance: Within 50% of non-temporal joins

### v1.3.0 Performance Targets
- Retention enforcement: Process 1M versions/minute
- Temporal aggregation: 100K records/second
- Data compression: Achieve 5x average compression ratio

---

## Backward Compatibility

### Migration Strategy
**Target Version:** v1.1.0

Migration path for existing tables to system-versioned.

**Steps:**
1. Analyze existing table structure
2. Create history table with matching schema
3. Add system time columns (sys_start, sys_end)
4. Backfill history from audit logs or change tracking
5. Enable system versioning
6. Verify data integrity

**Tools:**
```cpp
class TemporalMigrator {
public:
    // Analyze table for temporal migration
    Result<MigrationPlan> analyzeMigration(
        const std::string& table_name
    );

    // Execute migration
    Result<bool> migrateToTemporal(
        const std::string& table_name,
        const MigrationPlan& plan
    );

    // Verify migration
    Result<MigrationReport> verifyMigration(
        const std::string& table_name
    );
};
```

---

## Known Limitations & Workarounds

### Limitation #1: No Automatic History Table
**Severity:** High
**Versions:** v1.0.x

History table must be manually created and managed.

**Workaround:**
- Use TemporalConflictResolver for version tracking
- Manual history table management
- Application-level version tracking

**Planned Fix:** v1.1.0 - Automatic history table creation

---

### Limitation #2: Limited Query Support
**Severity:** High
**Versions:** v1.0.x, v1.1.x

No SQL syntax for time-travel queries.

**Workaround:**
- Use API-level temporal queries
- Manual timestamp filtering
- Application-level time-travel logic

**Planned Fix:** v1.2.0 - Full temporal query syntax

---

### Limitation #3: No Automatic Retention
**Severity:** Medium
**Versions:** v1.0.x, v1.1.x, v1.2.x

Manual cleanup of old historical data required.

**Workaround:**
- Scheduled cleanup jobs
- Manual data purging
- Archive to external storage

**Planned Fix:** v1.3.0 - Automated retention policies

---

## Contributing to Temporal Module

### Priority Areas for Contribution

**High Priority:**
1. System-versioned table implementation
2. Time-travel query engine
3. Temporal indexes
4. Conflict detection enhancements

**Medium Priority:**
1. Retention policy automation
2. Temporal aggregations
3. Data compression
4. CDC integration

**Low Priority:**
1. Temporal foreign keys
2. Advanced temporal predicates
3. Bi-temporal query optimization

### Contribution Guidelines

1. **Follow SQL:2011 Standard**: Maintain compatibility with standard temporal SQL
2. **Add Tests**: Unit, integration, and performance tests required
3. **Document**: Update README and API docs
4. **Benchmark**: Include temporal query benchmarks
5. **Backward Compatibility**: Maintain existing temporal APIs

For detailed guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | ≥ 85% line coverage in `temporal_manager.cpp` and `temporal_query_engine.cpp` | Cover `AS OF`, `FROM…TO`, `BETWEEN…AND`, gap/overlap detection, all four conflict types |
| Integration | All SQL:2011 temporal query clauses produce correct result sets | Run against an in-process RocksDB instance; result sets compared with hand-verified expected outputs |
| Performance | `AS OF` query with snapshot index ≤ 2× baseline non-temporal query latency at 1M rows | Benchmark in `benchmarks/bench_temporal.cpp`; runs in CI for all temporal module PRs |
| Retention | Retention job processes ≥ 1M versions/minute without blocking concurrent foreground writes | Measured under a parallel read/write load during retention enforcement run |
| Bi-temporal | Bi-temporal join returns correct rows for all four period overlap predicates (`OVERLAPS`, `CONTAINS`, `EQUALS`, `PRECEDES`) | Dedicated test fixture with 100 hand-crafted period combinations |
| Regression | Zero existing non-temporal AQL query failures after merging temporal changes | Full AQL test suite runs as a mandatory gate for all temporal PRs |

---

## Security / Reliability

- System time columns (`sys_start`, `sys_end`) are set exclusively by the database engine; any DML attempt to write these columns returns a `PermissionDenied` error and is logged to the audit trail.
- History table rows are append-only; `UPDATE` and `DELETE` on the history table are rejected at the API layer with a descriptive error.
- Retention deletion is irreversible: enforcement jobs require an explicit `RETENTION_POLICY` DDL statement and log every batch deletion (table name, row count, time range) to the audit trail before executing.
- Time-travel queries against sensitive tables are subject to the same `PolicyEngine` RBAC controls as current-data queries; historical access does not bypass column-level redaction rules.
- User-specified archive locations for retention policies are validated against the configured `data_dir` allow-list to prevent directory traversal attacks.
- Snapshot handles are released on transaction commit or abort; a snapshot GC watchdog forcibly releases leaked snapshots older than the configured `snapshot_timeout` to prevent unbounded memory growth.

---

## See Also

- [README.md](README.md) - Current module documentation
- [Header Documentation](../../include/temporal/README.md) - Public API
- [Replication Module](../replication/FUTURE_ENHANCEMENTS.md) - Distributed temporal support
- [Storage Module](../storage/FUTURE_ENHANCEMENTS.md) - Temporal storage optimization

---

*Last Updated: April 2026*
*Module Version: v1.6.0*
*Next Review: v1.7.0 Release*
