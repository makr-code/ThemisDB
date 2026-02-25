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

## Column Lineage and Data Provenance

`ColumnLineageTracker` records how each column was derived from its source columns
and exposes a directed acyclic graph (DAG) for transitive upstream/downstream
traversal.  It operates entirely in-memory and is append-only: recorded entries
are never modified or deleted.

### Record a Derivation

```cpp
#include "metadata/column_lineage.h"

using namespace themis::metadata;

ColumnLineageTracker tracker;

// ETL: full_name is computed from first_name + last_name
ColumnLineageEntry entry;
entry.target_column  = {"users", "full_name"};
entry.source_columns = {{"users", "first_name"}, {"users", "last_name"}};
entry.transformation = TransformationType::COMPUTED;
entry.transformation_expression = "first_name || ' ' || last_name";
entry.performed_by   = "etl-service";
tracker.recordDerivation(entry);   // entry_id and timestamp auto-assigned

// Type-cast: price_cents → price_eur
ColumnLineageEntry cast_entry;
cast_entry.target_column  = {"orders", "price_eur"};
cast_entry.source_columns = {{"orders", "price_cents"}};
cast_entry.transformation = TransformationType::CAST;
cast_entry.transformation_expression = "price_cents / 100.0";
tracker.recordDerivation(cast_entry);
```

### Query Upstream Sources (Transitive)

```cpp
// All columns that contributed to full_name, directly or transitively
auto upstream = tracker.getUpstreamColumns({"users", "full_name"});
for (const auto& ref : upstream) {
    std::cout << ref.table_name << "." << ref.column_name << "\n";
}
```

### Query Downstream Dependents (Transitive)

```cpp
// All columns derived from first_name, directly or transitively
auto downstream = tracker.getDownstreamColumns({"users", "first_name"});
```

### Full Provenance Record (JSON)

```cpp
nlohmann::json prov = tracker.getColumnProvenance({"users", "full_name"});
// {
//   "column":             {"table": "users", "column": "full_name"},
//   "entries":            [...],          // direct derivation entries
//   "upstream_columns":   [...],          // transitive sources
//   "downstream_columns": [...]           // transitive dependents
// }
```

### Export Table or Full Lineage

```cpp
// All lineage entries for the "orders" table
nlohmann::json tbl = tracker.exportTableLineage("orders");

// Full lineage graph — {"entries": [...], "total_entries": N}
nlohmann::json all = tracker.exportAllLineage();
```

### TransformationType Values

| Value | Description |
|-------|-------------|
| `DIRECT_COPY` | Verbatim copy from a single source column |
| `RENAME` | Column renamed; content identical to source |
| `CAST` | Type cast applied (e.g. `INTEGER → DOUBLE`) |
| `COMPUTED` | Arithmetic / string expression over one or more sources |
| `AGGREGATION` | Aggregation function (SUM, AVG, COUNT, …) |
| `ANONYMIZATION` | PII/PHI was anonymized or pseudonymized |
| `ENRICHMENT` | Source enriched with data from an external source |
| `CUSTOM` | Any other transformation; detail in `transformation_expression` |

### Notes

- `entry_id` and `timestamp_ms` are auto-assigned if left at their zero values.
- The tracker is thread-safe: all public methods acquire a `std::mutex`.
- No persistence is built in; use `exportAllLineage()` and store the JSON in
  RocksDB (under a custom prefix, e.g. `lineage:col:`) for durability across restarts.
## External Catalog Integration (Apache Atlas & DataHub)

`CatalogExporter` (`include/metadata/catalog_exporter.h`) publishes ThemisDB schema
metadata to external data governance catalogs.

### Apache Atlas

Entities published: `rdbms_db` (one per call) + `rdbms_table` + `rdbms_column` per table.  
Endpoint: `POST /api/atlas/v2/entity/bulk`

```cpp
#include "metadata/catalog_exporter.h"

CatalogExporter::Config cfg;
cfg.type          = CatalogExporter::CatalogType::APACHE_ATLAS;
cfg.endpoint      = "http://atlas-host:21000";
cfg.username      = "admin";
cfg.password      = "admin";
cfg.database_name = "production";   // logical catalog name

CatalogExporter exporter(cfg);
auto result = exporter.publishSchema(schema_mgr.getAllTables());
if (!result.success) {
    spdlog::error("Atlas sync failed: {}", result.error);
} else {
    spdlog::info("Published {} entities to Atlas", result.entities_published);
}
```

### DataHub

Two `MetadataChangeProposal` objects are emitted per table:
- `datasetProperties` aspect – name, description, custom properties
- `schemaMetadata` aspect – field list with native types

Endpoint: `POST /aspects?action=ingestProposal`

```cpp
CatalogExporter::Config cfg;
cfg.type          = CatalogExporter::CatalogType::DATAHUB;
cfg.endpoint      = "http://datahub-gms:8080";
cfg.token         = "eyJ...";   // GMS access token
cfg.database_name = "production";

CatalogExporter exporter(cfg);
auto result = exporter.publishSchema(schema_mgr.getAllTables());
```

### Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| HTTP 401 | Wrong credentials / token | Verify `username`/`password` (Atlas) or `token` (DataHub) |
| HTTP 404 | Wrong endpoint URL | Check `cfg.endpoint` (no trailing slash) |
| HTTP 500 | Atlas entity type not registered | Import Atlas RDBMS typedefs before first publish |
| `result.success == false` with `entities_published > 0` (DataHub) | Partial failure mid-batch | Re-run; each proposal is idempotent (UPSERT) |

---

## See Also

- [`docs/metadata/schema_migration_runbook.md`](./schema_migration_runbook.md)
- [`docs/metadata/troubleshooting.md`](./troubleshooting.md)
- [`docs/metadata/recovery_runbook.md`](./recovery_runbook.md)
- [`docs/metadata/metadata_roadmap.md`](./metadata_roadmap.md)
