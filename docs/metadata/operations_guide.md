# Operations Guide: Metadata Module

**Module:** `src/metadata`  
**Version:** 2026 Q1  
**Maintained by:** ThemisDB Contributors

---

## Overview

The metadata module is the authoritative source of schema knowledge for ThemisDB.  It powers:

- SQL-standard `INFORMATION_SCHEMA` views
- Query-cost estimation via `StatisticsCollector`
- Write-time constraint enforcement via `SchemaConstraints`
- Schema change tracking and rollback via `SchemaVersionManager`
- Auto index recommendations via `IndexRecommender`
- Audit trail via `SchemaAuditLog`

All metadata is persisted in the same RocksDB instance as the data, under reserved key prefixes (see **Key Prefixes** below).

---

## Key Prefixes

| Prefix | Purpose |
|--------|---------|
| `stats:<table>` | Serialised `TableStats` blob |
| `config:constraints:<table>` | Serialised `ColumnConstraint` list |
| `config:schema_version:<table>:<N>` | `SchemaChange` snapshot at version N |
| `config:schema_version:<table>:current` | Current version counter |
| `audit:schema:<table>:<ns>` | `SchemaAuditEntry` sorted by nanosecond timestamp |

---

## Tuning the Statistics Collector

### Sample Size

The default sample size is **1 000 rows** (`kDefaultSampleSize`).

- Increase for better cardinality estimates on high-cardinality columns.
- Decrease on very large tables where full sampling is too slow.

Override at call-site:

```cpp
stats_collector.collectStats("orders", /*sample_size=*/5000);
```

REST:
```
POST /api/v1/metadata/stats/orders
Content-Type: application/json
{ "sample_size": 5000 }
```

### Refresh Schedule

Statistics are not automatically refreshed by default (configurable schedule planned for v2.0).  
Manually trigger a refresh when:

- A bulk import completes
- Table row-count has grown > 20 %
- Query planner starts returning poor plans

```bash
curl -X POST http://localhost:8080/api/v1/metadata/stats/orders
```

### Histogram Buckets

Default: **20 buckets**.  Increase for more granular selectivity estimates:

```cpp
// Increase before collecting
stats_collector.collectStats("sales");  // uses kDefaultHistogramBuckets=20 internally
```

For custom bucket counts, call `buildColumnStats()` directly (internal API).

### Metrics Hook (Prometheus / OTel)

Implement `StatisticsCollector::IMetricsHook` and call `setMetricsHook()`:

```cpp
class MyMetricsSink : public themis::StatisticsCollector::IMetricsHook {
public:
    void onCollect(std::string_view table, double ms, size_t rows, bool ok) override {
        prometheus_histogram.Observe(ms);  // themis_stats_collection_duration_ms
    }
    void onCacheHit(std::string_view table)  override { cache_hits_.Inc(); }
    void onCacheMiss(std::string_view table) override { cache_misses_.Inc(); }
    void onError(std::string_view table, int code) override { errors_.Inc(); }
};

MyMetricsSink sink;
stats_collector.setMetricsHook(&sink);
```

Suggested metric names:
- `themis_stats_collection_duration_ms` – histogram
- `themis_stats_cache_hits_total` – counter
- `themis_stats_cache_misses_total` – counter
- `themis_stats_errors_total` – counter

---

## Schema Constraints

### Registering Constraints

```cpp
#include "metadata/schema_constraints.h"
using namespace themis;

SchemaConstraints sc;

// NOT NULL on column "email"
sc.addConstraint("users", "email",
    ColumnConstraint::makeNotNull("nn_users_email"));

// UNIQUE on column "sku"
sc.addConstraint("products", "sku",
    ColumnConstraint::makeUnique("uq_products_sku"));

// DEFAULT for "status"
sc.addConstraint("orders", "status",
    ColumnConstraint::makeDefault("df_orders_status", ColumnValue{"pending"}));

// Persist to RocksDB
sc.persistTo(db);
```

Constraints are automatically reloaded from RocksDB on server start (`loadFrom(db)` is called during `HttpServer::init()`).

### Enforcement

Call `enforce()` before writing a row:

```cpp
std::map<std::string, std::string> row = {{"email", ""}, {"status", "active"}};
auto violations = sc.enforce("users", row);
if (!violations.empty()) {
    // violations[0].constraint_type == "NOT_NULL", .column_name == "email"
}
```

Apply defaults first:

```cpp
sc.applyDefaults("orders", row);  // fills missing "status" = "pending"
sc.enforce("orders", row);
```

### Batch Validation (REST)

```bash
curl -s -X POST http://localhost:8080/api/v1/metadata/constraints/validate/users \
  -H 'Content-Type: application/json' \
  -d '{
    "rows": [
      {"email": "alice@example.com"},
      {"email": ""}
    ]
  }' | jq .
```

Response contains `invalid_rows` with `violations` arrays and HTTP 422 when violations exist.

### Prometheus Metric

Emit `themis_constraint_violations_total{type, table}` from a `SchemaConstraints::IViolationHook` (planned; see roadmap v2.0).

---

## Schema Versioning

Every schema mutation should be followed by a version snapshot:

```bash
# Snapshot before migration
curl -X POST http://localhost:8080/api/v1/schema/versions/users \
  -d '{"author":"ops", "description":"pre-migration baseline"}'

# Apply migration …

# Snapshot after
curl -X POST http://localhost:8080/api/v1/schema/versions/users \
  -d '{"author":"ops", "description":"added phone column"}'
```

Full lifecycle: see [`docs/metadata/schema_migration_runbook.md`](./schema_migration_runbook.md).

---

## Audit Log

Every `createSchemaVersion()` and `rollbackToVersion()` automatically writes an entry to `audit:schema:`.

Query audit history:

```bash
# All tables
curl http://localhost:8080/api/v1/metadata/audit | jq .

# Single table
curl http://localhost:8080/api/v1/metadata/audit/users | jq .
```

Each entry includes `operation`, `author`, `description`, `version`, and `timestamp`.

---

## Schema Import (Bulk)

Import multiple table schemas in one request:

```bash
curl -X PUT http://localhost:8080/api/v1/metadata/schema_import \
  -H 'Content-Type: application/json' \
  -d '{
    "tables": [
      {"name": "users",    "type": "relational", "properties": [...]},
      {"name": "products", "type": "relational", "properties": [...]}
    ]
  }' | jq .
```

Response contains `imported`, `errors`, `imported_count`, `error_count`.  
Partial success returns HTTP 207.

---

## Index Recommendations

```bash
# All tables
curl http://localhost:8080/api/v1/metadata/index_recommendations | jq .

# Single table
curl http://localhost:8080/api/v1/metadata/index_recommendations/users | jq .
```

Recommendations are based on recorded query access patterns (`IndexRecommender::recordAccess()`).  
Wire `recordAccess()` into the AQL query execution path for production usefulness.

---

## INFORMATION_SCHEMA

```bash
# All views
curl http://localhost:8080/api/v1/information_schema | jq .

# Tables only
curl http://localhost:8080/api/v1/information_schema/tables | jq .

# Columns for a specific table
curl http://localhost:8080/api/v1/information_schema/columns/users | jq .

# Statistics
curl http://localhost:8080/api/v1/information_schema/statistics/users | jq .
```

---

## Cache TTL

The in-memory statistics cache has no automatic TTL eviction (planned for v2.0).  
Clear stale statistics manually:

```bash
curl -X DELETE http://localhost:8080/api/v1/metadata/stats/users  # (planned endpoint)
```

Or via C++ API:

```cpp
stats_collector.clearStats("users");
```

---

## See Also

- [`docs/metadata/schema_migration_runbook.md`](./schema_migration_runbook.md)
- [`docs/metadata/troubleshooting.md`](./troubleshooting.md)
- [`docs/metadata/recovery_runbook.md`](./recovery_runbook.md)
- [`docs/metadata/metadata_roadmap.md`](./metadata_roadmap.md)
