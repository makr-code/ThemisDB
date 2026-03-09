# Metadata Module - Future Enhancements

## Scope

- Automatic schema discovery via RocksDB key scanning (`SchemaManager`)
- SystemCatalog: table, column, index, and statistics metadata with configurable TTL caching (default 60 s)
- INFORMATION_SCHEMA views (tables, columns, indexes, statistics) compatible with SQL:2003 standard column names
- StatisticsCollector: cardinality, selectivity, equi-height histograms, and data distribution
- Schema version tracking, change history, and diff/migration script generation
- Real-time schema change notifications via changefeeds
- Column lineage and data provenance tracking
- Distributed metadata catalog across shards (planned)

## Design Constraints

- [x] Metadata cache TTL must be configurable per collection; adaptive TTL must track mutation rate with < 5 s lag
- [x] INFORMATION_SCHEMA column names must comply with SQL:2003 standard; no proprietary column names
- [x] Thread-safe access to all metadata APIs (shared_mutex on cache; exclusive lock on version counter)
- [x] Full schema scan on first load must complete in < 30 s for databases with up to 10M keys
- [x] Statistics must be refreshed at most once per TTL period even under concurrent query pressure
- [x] Schema version counter must be monotonically increasing; gaps are not permitted
- [x] Changefeed notifications must be delivered within 500 ms of the schema mutation event
- [x] External catalog integration (Apache Atlas, DataHub) must be optional and guarded by a feature flag

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `SchemaManager::discoverSchema()` | Query planner, API layer | Triggers RocksDB key scan; result cached for TTL duration |
| `SystemCatalog::getTable(name)` | Query optimizer | Returns table metadata including column types and index list |
| `SystemCatalog::getStatistics(table)` | Query optimizer (selectivity) | Returns cardinality, histogram, and selectivity estimates |
| `InformationSchema::query(view, filter)` | AQL, REST `/api/v1/schema` | Returns SQL-standard rows for tables/columns/indexes/statistics |
| `StatisticsCollector::collect(table)` | Background maintenance job | Computes equi-height histograms; stores in SystemCatalog |
| `SchemaVersionTracker::getVersion()` | Replication, changefeed | Monotonic version counter; incremented on every schema change |
| `SchemaChangefeed::subscribe(callback)` | Live schema change consumers | Delivers notifications within 500 ms of mutation |
| `ColumnLineage::getProvenance(column)` | Data governance, REST endpoint | Returns upstream sources and transformation history |

## Planned Features

### Statistics Collector
**Priority:** High  
**Target Version:** v1.6.0

Comprehensive table and index statistics for query optimization.

**Features:**
- Cardinality estimation
- Data distribution histograms
- Index selectivity
- NULL ratio tracking
- Update frequency tracking

**Implementation:**
```cpp
class StatisticsCollector {
public:
    struct TableStats {
        size_t row_count;
        uint64_t total_size_bytes;
        std::map<std::string, ColumnStats> column_stats;
        std::chrono::system_clock::time_point last_updated;
    };
    
    struct ColumnStats {
        size_t distinct_count;
        size_t null_count;
        double selectivity;
        std::optional<Histogram> distribution;
    };
    
    Result<TableStats> collectStats(const std::string& table_name);
    Result<bool> updateStats(const std::string& table_name);
    Result<TableStats> getStats(const std::string& table_name);
};
```

**Expected Performance:**
- Statistics collection: 1-10 seconds per table
- Update overhead: <5% during collection
- Storage overhead: 1-5% of table size

---

### Information Schema Views
**Priority:** High  
**Target Version:** v1.7.0

SQL-standard INFORMATION_SCHEMA views for metadata access.

**Features:**
- `INFORMATION_SCHEMA.TABLES`
- `INFORMATION_SCHEMA.COLUMNS`
- `INFORMATION_SCHEMA.INDEXES`
- `INFORMATION_SCHEMA.STATISTICS`
- `INFORMATION_SCHEMA.KEY_COLUMN_USAGE`

**Query Examples:**
```sql
SELECT table_name, table_type, row_count
FROM INFORMATION_SCHEMA.TABLES
WHERE table_schema = 'main';

SELECT column_name, data_type, is_nullable
FROM INFORMATION_SCHEMA.COLUMNS
WHERE table_name = 'users';
```

---

### Schema Versioning
**Priority:** Medium  
**Target Version:** v1.8.0

Track and manage schema changes over time.

**Features:**
- Schema version numbers
- Change history tracking
- Rollback support
- Migration scripts
- Compatibility checking

**Implementation:**
```cpp
class SchemaVersionManager {
public:
    Result<uint64_t> getCurrentVersion(const std::string& table_name);
    Result<bool> createSchemaVersion(const std::string& table_name);
    Result<std::vector<SchemaChange>> getChangeHistory(
        const std::string& table_name
    );
    Result<bool> rollbackToVersion(
        const std::string& table_name,
        uint64_t version
    );
};
```

---

### Schema Constraints
**Priority:** Medium  
**Target Version:** v1.8.0

Enforce schema constraints and validation.

**Features:**
- NOT NULL constraints
- UNIQUE constraints
- CHECK constraints
- DEFAULT values
- Foreign key constraints

---

### Automatic Indexing Recommendations
**Priority:** Low  
**Target Version:** v1.9.0

Analyze query patterns and recommend indexes.

**Features:**
- Query log analysis
- Index usage tracking
- Missing index detection
- Unused index identification
- Cost-benefit analysis

---

## Performance Roadmap

### v1.6.0 Targets
- Statistics collection: <10 seconds per table
- Cache hit rate: >95%
- Metadata query latency: <1ms (cached)

### v1.7.0 Targets
- Information schema queries: <10ms
- Schema export: <100ms for 1000 tables
- Memory usage: <100 MB for 1000 tables

## Test Strategy

- Unit test coverage ≥ 80% across `schema_manager.cpp`, `system_catalog.cpp`, `statistics_collector.cpp`
- Integration tests: schema discovery after bulk insert, INFORMATION_SCHEMA queries matching expected column names, changefeed notification delivery latency < 500 ms
- Property-based tests: schema version counter must be strictly monotonically increasing under concurrent schema mutations (100 concurrent writers)
- Statistics accuracy test: estimated cardinality must be within 20% of true cardinality for uniform and skewed distributions
- Cache TTL test: stale cache entries must not be served after 2× TTL; adaptive TTL must converge within 5 mutation events
- Migration script test: generated diff script applied to a test schema must produce the target schema with zero manual edits

## Performance Targets

- Schema cache lookup (cache hit): < 1 ms p99
- Full schema scan (first load, 10M-key database): < 30 s
- INFORMATION_SCHEMA query (1,000 tables): < 50 ms p99
- StatisticsCollector full collection (1M-row table): < 60 s background job; no foreground blocking
- Changefeed notification delivery latency: < 500 ms p99 from schema mutation to subscriber callback
- Adaptive TTL convergence: TTL updated within 5 s of a 10× change in table mutation rate

## Security / Reliability

- Metadata API endpoints must enforce the same RBAC permissions as data APIs; unauthenticated access to INFORMATION_SCHEMA must be denied
- Column statistics must not expose sample data values; only aggregate statistics (min, max, NDV, histogram bucket boundaries) are stored
- Schema version history stored in-memory must be bounded (default: last 1,000 versions) to prevent unbounded memory growth across long-running instances
- External catalog integration (Apache Atlas, DataHub) must be disabled by default and require explicit opt-in configuration; credentials must not be logged
- Changefeed subscriber list must be bounded (default: 256 subscribers) to prevent resource exhaustion from abandoned subscriptions
- RocksDB key scanning during discovery must operate in read-only snapshot mode to avoid blocking concurrent writes

---

## See Also

- [README.md](README.md) - Current module documentation
- [Header Documentation](../../include/metadata/README.md) - Public API

---

*Last Updated: February 2026*  
*Module Version: v1.5.0*  
*Next Review: v1.6.0 Release*
