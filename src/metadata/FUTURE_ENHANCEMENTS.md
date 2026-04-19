> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

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

### `IndexRecommender`: Access-Pattern Persistence and ML Model
**Priority:** Medium
**Target Version:** v1.8.0

`index_recommender.cpp` maintains access statistics only in memory (`stats_` map). On restart, all access history is lost and recommendations revert to the "no data" state. The recommendation algorithm uses simple heuristics (sort usage ratio vs. equality ratio) rather than a query-workload-aware model.

**Implementation Notes:**
- `[x]` Persist `stats_` snapshots to RocksDB under key prefix `meta_idx_stats::` on a configurable interval (default 5 min) or on graceful shutdown.
- `[x]` On `IndexRecommender` construction, load the persisted stats snapshot; merge with any post-restart activity.
- `[x]` Replace the threshold-based heuristic in `recommend()` (line 122) with a cost-model estimate: use `StatisticsCollector` cardinality/selectivity to estimate index benefit vs. write amplification.
- `[x]` Emit `metadata.index_recommendation.generated_total` metric per recommendation cycle.

---

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
**Status:** ✅ Implemented

Track and manage schema changes over time.

**Features:**
- `[x]` Schema version numbers – monotonically increasing counter per table, persisted in RocksDB under `config:schema_version:<table>:current`.
- `[x]` Change history tracking – `getChangeHistory()` loads all `SchemaChange` records from RocksDB in ascending version order; empty list (not an error) when no history exists.
- `[x]` Rollback support – `rollbackToVersion()` applies a historical schema snapshot via `SchemaManager::setTableSchema()` and records the rollback as a new version entry to preserve history.
- `[x]` Migration scripts – `generateMigrationScript()` produces ALTER TABLE DDL (ADD COLUMN, DROP COLUMN, ALTER COLUMN TYPE / SET NOT NULL / DROP NOT NULL) from the diff between any two versions.
- `[x]` Compatibility checking / dry-run – `validateMigration()` validates a proposed schema without persisting: checks non-empty name, at least one column, no duplicate column names, and that the new schema differs from the current version.
- `[x]` Audit log integration – `setAuditLog(SchemaAuditLog*)` attaches an optional audit log; `createSchemaVersion()` and `rollbackToVersion()` emit entries when one is set; no crash when `nullptr`.

**Implemented in:**
- `include/metadata/schema_version_manager.h`
- `src/metadata/schema_version_manager.cpp`

**Tests:**
- `tests/test_schema_version_manager.cpp` – `SchemaVersionManagerFocusedTests` (versioning, history, rollback, diff, JSON serialisation, audit integration)
- `tests/test_schema_version_dryrun.cpp` – `SchemaVersionDryRunFocusedTests` (validateMigration edge cases, no-persist guarantee)

**CI:** `.github/workflows/schema-version-manager-ci.yml`

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
**Status:** ✅ Implemented

Analyze query patterns and recommend indexes.

**Features:**
- `[x]` Query log analysis — `IndexRecommender::recordAccess()` records filter and sort access patterns per column per query; `recordQuery()` tracks the total query count for normalisation.
- `[x]` Index usage tracking — `recommend()` accepts an `existing_indexes` list; indexed columns with a low benefit score are returned as `DROP` recommendations.
- `[x]` Missing index detection — columns with a benefit score above `kAddThreshold` (20.0) and no existing index are returned as `ADD` recommendations.
- `[x]` Unused index identification — indexed columns whose benefit score falls below `kDropThreshold` (5.0) are returned as `DROP` recommendations.
- `[x]` Cost-benefit analysis — `computeCostModelBenefit()` uses `StatisticsCollector` cardinality / selectivity estimates and a write-amplification penalty based on table row count (logarithmic, capped at 20 %).

**Implemented in:**
- `include/metadata/index_recommender.h`
- `src/metadata/index_recommender.cpp`

**Tests:**
- `tests/test_index_recommender.cpp`

**REST endpoint:** `GET /api/v1/metadata/index_recommendations[/:table]`

**CLI:** `themisctl index recommend [table]`

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

- Unit test coverage ≥ 80% across `schema_manager.cpp` (including SystemCatalog functionality), `statistics_collector.cpp`
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

- ✅ **Implemented (v1.6.0)**: Pluggable RBAC / access-control interface — `IMetadataSecurityProvider`
  (`include/metadata/imetadata_security_provider.h`) with `NoOpMetadataSecurityProvider` (default;
  permits all) and `InMemoryRbacMetadataSecurityProvider` (thread-safe RBAC: grant/revoke/revokeAll,
  wildcard `"*"` resource, ADMIN-implies-all, `MetadataAccessDeniedException`).
- Column statistics must not expose sample data values; only aggregate statistics (min, max, NDV, histogram bucket boundaries) are stored
- Schema version history stored in-memory must be bounded (default: last 1,000 versions) to prevent unbounded memory growth across long-running instances
- External catalog integration (Apache Atlas, DataHub) must be disabled by default and require explicit opt-in configuration; credentials must not be logged
- Changefeed subscriber list must be bounded (default: 256 subscribers) to prevent resource exhaustion from abandoned subscriptions
- RocksDB key scanning during discovery must operate in read-only snapshot mode to avoid blocking concurrent writes

---

## Scientific References (IEEE)

The following references underpin the planned enhancements and design decisions in this module.

[1] Y. E. Ioannidis and V. Poosala, "Balancing Histogram Optimality and Practicality for Query Result Size Estimation," in *Proc. ACM SIGMOD Int. Conf. Management of Data*, San Jose, CA, USA, 1995, pp. 233–244. https://doi.org/10.1145/223784.223816

[2] S. Chaudhuri and V. R. Narasayya, "An Efficient Cost-Driven Index Selection Tool for Microsoft SQL Server," in *Proc. 23rd Int. Conf. Very Large Data Bases (VLDB)*, Athens, Greece, 1997, pp. 146–155. https://dl.acm.org/doi/10.5555/645923.671506

[3] J. F. Roddick, "A Survey of Schema Versioning Issues for Database Systems," *Information and Software Technology*, vol. 37, no. 7, pp. 383–393, 1995. https://doi.org/10.1016/0950-5849(95)91245-R

[4] P. Buneman, S. Khanna, and W. C. Tan, "Why and Where: A Characterization of Data Provenance," in *Proc. 8th Int. Conf. Database Theory (ICDT)*, London, UK, 2001, pp. 316–330. https://doi.org/10.1007/3-540-44503-X_20

[5] W3C, "PROV-DM: The PROV Data Model," W3C Recommendation, Apr. 2013. https://www.w3.org/TR/prov-dm/

[6] R. Ramakrishnan and J. Gehrke, *Database Management Systems*, 3rd ed. New York, NY, USA: McGraw-Hill, 2003. ISBN: 978-0-07-246563-1

[7] ISO/IEC, *Information Technology – Database Languages – SQL – Part 11: Information and Definition Schemas (SQL/Schemata)*, ISO/IEC 9075-11:2016, 2016. https://www.iso.org/standard/63556.html

## See Also

- [README.md](README.md) - Current module documentation
- [Header Documentation](../../include/metadata/README.md) - Public API

---

*Last Updated: April 2026*
*Module Version: v1.6.0*
*Next Review: v1.9.0 Release*
