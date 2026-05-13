# Metadata Module — Public API
<!-- status: current | validated: 2026-05-13 | commit: HEAD -->

**Module Path:** `include/metadata/`
**Implementation:** `../../src/metadata/`
**Status:** ✅ Production Ready (v1.6.0)

Public C++ header files for the ThemisDB metadata module. All headers are
`#pragma once` guarded. Headers containing only interfaces and inline
implementations are header-only; the remaining classes have `.cpp` counterparts
in `src/metadata/`.

---

## Header Overview

| Header | Key Types | Notes |
|--------|-----------|-------|
| `schema_manager.h` | `SchemaManager`, `TableSchema`, `PropertyInfo`, `IndexInfo` | Primary entry point |
| `statistics_collector.h` | `StatisticsCollector`, `ColumnStats`, `HistogramBucket` | Requires `.cpp` |
| `information_schema.h` | `InformationSchema`, `ISTable`, `ISColumn`, `ISStatistic` | Requires `.cpp` |
| `schema_version_manager.h` | `SchemaVersionManager`, `SchemaChange`, `VersionResult` | Requires `.cpp` |
| `schema_audit_log.h` | `SchemaAuditLog`, `SchemaAuditEntry` | Requires `.cpp` |
| `schema_consistency_checker.h` | `SchemaConsistencyChecker`, `ConsistencyIssue` | Requires `.cpp` |
| `schema_constraints.h` | `SchemaConstraints`, `ColumnConstraint`, `ConstraintViolation` | Requires `.cpp` |
| `schema_diff.h` | `SchemaDiffEngine`, `SchemaDiff`, `ColumnDiff`, `IndexDiff` | Header-only |
| `metadata_snapshot.h` | `MetadataSnapshot`, `IMetadataSnapshotStore`, `InMemoryMetadataSnapshotStore` | Header-only |
| `column_lineage.h` | `ColumnLineageTracker`, `ColumnRef`, `ColumnLineageEntry` | Requires `.cpp` |
| `catalog_exporter.h` | `CatalogExporter`, `Config`, `PublishResult` | Requires `.cpp` |
| `distributed_catalog.h` | `DistributedMetadataCatalog` | Requires `.cpp` |
| `er_diagram_exporter.h` | `ERDiagramExporter` | Requires `.cpp` |
| `index_recommender.h` | `IndexRecommender`, `ColumnAccess`, `IndexRecommendation` | Requires `.cpp` |
| `imetadata_security_provider.h` | `IMetadataSecurityProvider`, `InMemoryRbacMetadataSecurityProvider` | Header-only |
| `imetadata_change_listener.h` | `IMetadataChangeListener`, `RecordingMetadataChangeListener` | Header-only |
| `imetadata_export_policy.h` | `IMetadataExportPolicy`, `AlwaysExportPolicy`, `FilteredExportPolicy` | Header-only |
| `imetadata_encryption_provider.h` | `IMetadataEncryptionProvider`, `FieldSetMetadataEncryptionProvider` | Header-only |
| `aql_schema_bridge.h` | `aql::fromTableSchema()` | Header-only bridge |

---

## Headers

### schema_manager.h

**Purpose:** Database schema introspection and self-awareness — the primary entry
point for the metadata module.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `SchemaManager` | Central schema catalog: auto-discovery, caching, JSON export |
| `SchemaManager::TableSchema` | Complete schema for one table/collection |
| `SchemaManager::PropertyInfo` | Column/property descriptor (name, type, index info) |
| `SchemaManager::IndexInfo` | Index descriptor (name, type, columns, uniqueness) |
| `SchemaManager::RelationshipSchema` | Graph edge / foreign-key relationship descriptor |
| `SchemaManager::DatabaseMetadata` | Database-level metadata (version, table count, capabilities) |
| `AdaptiveTTLConfig` | Parameters for mutation-rate-driven adaptive cache TTL |

**Key API:**

```cpp
// Construction
SchemaManager schema_mgr(db, index_mgr);   // index_mgr may be nullptr

// Schema discovery
std::vector<TableSchema> getAllTables();
std::optional<TableSchema> getTable(std::string_view name);
std::vector<RelationshipSchema> getAllRelationships();
DatabaseMetadata getDatabaseMetadata();

// Cache control
void setCacheTTL(std::chrono::seconds ttl);   // default: 60 s
void refreshCache();
void enableAdaptiveTTL(AdaptiveTTLConfig config = {});
void disableAdaptiveTTL();
std::chrono::seconds getEffectiveTTL() const;

// Mutation tracking (for adaptive TTL)
void recordMutation(std::string_view table_name);

// Change notifications
void setChangefeed(Changefeed* changefeed);  // nullptr disables

// JSON export
json toJSON();
json tableToJSON(std::string_view table_name);
json getCapabilitiesJSON();

// Schema management (PUT/PATCH/DELETE)
bool setTableSchema(std::string_view name, const TableSchema& schema);
bool patchTableSchema(std::string_view name, const json& updates);
bool deleteTableSchema(std::string_view name);
std::string validateSchema(const TableSchema& schema) const;
static TableSchema parseTableSchema(const json& j);
```

**Usage:**
```cpp
#include "metadata/schema_manager.h"
using namespace themis;

SchemaManager schema_mgr(db, &idx_mgr);
schema_mgr.setCacheTTL(std::chrono::seconds(60));

auto tables = schema_mgr.getAllTables();
for (const auto& t : tables) {
    std::cout << t.name << " (" << t.type << ") ~"
              << t.estimated_row_count << " rows\n";
}

auto schema = schema_mgr.getTable("users");
if (schema) {
    nlohmann::json j = schema->toJSON();
}
```

**Thread Safety:** `std::shared_mutex` — many concurrent readers, single writer.
**Performance:** `getAllTables()` < 100 ms; `getTable()` < 1 ms (cached).

---

### statistics_collector.h

**Purpose:** Table and column statistics collection for query optimization —
cardinality, selectivity, equi-height histograms, NULL fractions.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `StatisticsCollector` | Main statistics API; auto-refresh background thread |
| `ColumnStats` | Per-column statistics (cardinality, selectivity, histogram, min/max) |
| `HistogramBucket` | One bucket in an equi-height histogram |
| `TableStats` | Aggregated per-table statistics |

**Key API:**

```cpp
StatisticsCollector stats(db);

// Collect statistics for a table
auto result = stats.collectStats("users");
if (result.ok) {
    const TableStats& ts = result.value;
    for (const auto& [col, cs] : ts.column_stats) {
        std::cout << col << ": distinct=" << cs.distinct_count << "\n";
    }
}

// Auto-refresh in the background
stats.startAutoRefresh(std::chrono::hours(1));
stats.stopAutoRefresh();

// Import statistics from the index manager
stats.importIndexStats("users", index_stats_json);
```

**Thread Safety:** Thread-safe with per-table read-write locks.

---

### information_schema.h

**Purpose:** SQL:2003-compatible `INFORMATION_SCHEMA` views — `TABLES`, `COLUMNS`,
`STATISTICS` — enabling familiar SQL introspection queries.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `InformationSchema` | Provides the three INFORMATION_SCHEMA views |
| `ISTable` | One row of `INFORMATION_SCHEMA.TABLES` |
| `ISColumn` | One row of `INFORMATION_SCHEMA.COLUMNS` |
| `ISStatistic` | One row of `INFORMATION_SCHEMA.STATISTICS` |

**Key API:**

```cpp
#include "metadata/information_schema.h"
using namespace themis;

InformationSchema is(schema_mgr);

auto tables  = is.getTables();           // ISTable per collection
auto columns = is.getColumns("users");   // ISColumn per property
auto stats   = is.getStatistics("users"); // ISStatistic per index

// Export view as JSON
nlohmann::json j = is.tablesToJSON();
```

---

### schema_version_manager.h

**Purpose:** Schema versioning, change history, diff, rollback, and DDL migration
script generation.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `SchemaVersionManager` | Main versioning API |
| `SchemaChange` | Single recorded schema change (version, author, snapshot) |
| `VersionResult<T>` | Typed result with `ok`, `value`, `error`, `error_message` |
| `VersionErrorCode` | Error codes: `OK`, `TABLE_NOT_FOUND`, `VERSION_NOT_FOUND`, … |

**Key API:**

```cpp
#include "metadata/schema_version_manager.h"
using namespace themis;

SchemaVersionManager svm(db, schema_mgr);
svm.setAuditLog(&audit_log);   // optional

// Snapshot current schema as a new version
auto r = svm.createSchemaVersion("users", "alice", "added email column");
if (r.ok) std::cout << "Version " << r.value << "\n";

// List history
auto history = svm.getChangeHistory("users");

// Diff between versions
auto diff = svm.diffVersions("users", 1, 2);

// Generate migration script
auto script = svm.generateMigrationScript("users", 1, 2);

// Rollback
svm.rollbackToVersion("users", 1, "alice");

// Dry-run validation
auto valid = svm.validateMigration("users", proposed_schema);
```

**Thread Safety:** NOT thread-safe. External synchronization required.

---

### schema_audit_log.h

**Purpose:** Durable, append-only audit trail for schema changes (DDL events),
stored in RocksDB.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `SchemaAuditLog` | Persistent audit log backed by RocksDB |
| `SchemaAuditEntry` | One audit record: table, operation, author, timestamp, version |

**Operations recorded:** `create`, `update`, `delete`, `rollback`, `import`

**Key API:**

```cpp
#include "metadata/schema_audit_log.h"
using namespace themis;

SchemaAuditLog audit(db);

// Wire into SchemaVersionManager
svm.setAuditLog(&audit);

// Manual log entry
SchemaAuditEntry entry;
entry.table_name = "users";
entry.operation  = "update";
entry.author     = "ops-team";
entry.description = "added phone column";
audit.append(entry);

// Query history
auto history = audit.getHistory("users");
auto recent  = audit.getHistory("users", 10); // last 10 entries
```

**Storage key format:** `audit:schema:<table_name>:<timestamp_ns>`

---

### schema_consistency_checker.h

**Purpose:** Background health scan that detects orphan keys, stale statistics,
and missing constraints in the metadata catalog.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `SchemaConsistencyChecker` | Periodic background scan |
| `ConsistencyIssue` | One discovered problem (type, table, column, detail) |

**Issue types detected:**
- `orphan_key` — RocksDB keys with no matching registered table
- `stale_stats` — Table statistics not refreshed within max age (default 24 h)
- `missing_constraint` — Table in catalog has no registered constraints

**Key API:**

```cpp
#include "metadata/schema_consistency_checker.h"
using namespace themis;

SchemaConsistencyChecker checker(db, schema_mgr, &stats, &constraints);

// Start periodic background scan
checker.startBackgroundCheck(std::chrono::hours(6));

// On-demand check
auto issues = checker.runCheck();
for (const auto& issue : issues) {
    std::cout << issue.issue_type << " on " << issue.table_name
              << ": " << issue.detail << "\n";
}

// Retrieve last scan results without re-scanning
auto last = checker.getLastCheckResults();

checker.stopBackgroundCheck();
```

---

### schema_constraints.h

**Purpose:** User-defined schema constraint validation — NOT NULL, UNIQUE, CHECK,
DEFAULT, and FOREIGN_KEY constraints.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `SchemaConstraints` | Constraint registry and enforcement engine |
| `ColumnConstraint` | A single constraint (kind, name, optional check_expr / default / FK refs) |
| `ColumnConstraint::Kind` | `NOT_NULL`, `UNIQUE`, `CHECK`, `DEFAULT`, `FOREIGN_KEY` |
| `ConstraintViolation` | Violation details (table, column, constraint name, message) |
| `ColumnValue` | `std::variant<monostate, string, int64_t, double, bool>` |

**Key API:**

```cpp
#include "metadata/schema_constraints.h"
using namespace themis;

SchemaConstraints sc(db);

// Define constraints
sc.addConstraint("users", "email",
    ColumnConstraint::makeNotNull("nn_email"));
sc.addConstraint("users", "age",
    ColumnConstraint::makeCheck("chk_age", "age >= 0"));

// Validate a row (as JSON object)
auto violations = sc.validate("users", row_json);
if (!violations.empty()) {
    for (const auto& v : violations) {
        spdlog::warn("Constraint '{}' violated on {}.{}: {}",
            v.constraint_name, v.table_name, v.column_name, v.message);
    }
}
```

---

### schema_diff.h

**Purpose:** Structural diff engine that compares two `TableSchema` snapshots and
returns a deterministic list of column and index changes. Header-only.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `SchemaDiffEngine` | Stateless diff engine; call `diff()` as many times as needed |
| `SchemaDiff` | Aggregated diff for one table: column diffs + index diffs |
| `ColumnDiff` | One column-level change (type, column_name, old_value, new_value) |
| `ColumnDiffType` | `ADDED`, `REMOVED`, `TYPE_CHANGED`, `NULLABILITY_CHANGED`, `INDEX_CHANGED` |
| `IndexDiff` | One index-level change (diff_type, index_name) |
| `IndexDiffType` | `ADDED`, `REMOVED`, `CHANGED` |

**Key API:**

```cpp
#include "metadata/schema_diff.h"
using namespace themis::metadata;

SchemaDiffEngine engine;
SchemaDiff diff = engine.diff(old_schema, new_schema);

if (!diff.isEmpty()) {
    std::cout << "Added columns: "   << diff.addedColumnCount()    << "\n";
    std::cout << "Removed columns: " << diff.removedColumnCount()  << "\n";
    std::cout << "Modified: "        << diff.modifiedColumnCount() << "\n";
    std::cout << diff.toJSON().dump(2) << "\n";
}
```

---

### metadata_snapshot.h

**Purpose:** Point-in-time capture of the complete schema — supports rollback,
audit, and CI/CD gate checks. Header-only.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `MetadataSnapshot` | Immutable snapshot: snapshot_id, created_at, tables, author, description |
| `IMetadataSnapshotStore` | Abstract persistence interface (save / load / list / remove / size) |
| `InMemoryMetadataSnapshotStore` | Thread-safe in-memory store (testing / embedded use) |
| `MetadataSnapshotException` | Thrown on invalid snapshot_id or I/O error |

**Key API:**

```cpp
#include "metadata/metadata_snapshot.h"
using namespace themis::metadata;

InMemoryMetadataSnapshotStore store;

MetadataSnapshot snap;
snap.snapshot_id = "v1.2.0";
snap.created_at  = std::chrono::system_clock::now();
snap.author      = "ci-pipeline";
snap.tables      = schema_mgr.getAllTables();
store.save(snap);

auto loaded = store.load("v1.2.0");
assert(loaded.has_value());

auto ids = store.listSnapshotIds();  // sorted ascending
```

---

### column_lineage.h

**Purpose:** Column-level lineage and data provenance tracking — append-only DAG
of derivation steps between `table.column` nodes.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `ColumnLineageTracker` | Thread-safe, append-only DAG tracker |
| `ColumnRef` | Typed column identifier `{table_name, column_name}` |
| `ColumnLineageEntry` | One derivation step: target, sources, transformation, timestamp, actor |
| `ColumnLineageRecord` | All entries recorded for one target column |
| `TransformationType` | `DIRECT_COPY`, `RENAME`, `CAST`, `COMPUTED`, `AGGREGATION`, `ANONYMIZATION`, `ENRICHMENT`, `CUSTOM` |

**Key API:**

```cpp
#include "metadata/column_lineage.h"
using namespace themis::metadata;

ColumnLineageTracker tracker;

ColumnLineageEntry entry;
entry.target_column  = {"users", "full_name"};
entry.source_columns = {{"users", "first_name"}, {"users", "last_name"}};
entry.transformation = TransformationType::COMPUTED;
entry.transformation_expression = "first_name || ' ' || last_name";
entry.performed_by   = "etl-service";
tracker.recordDerivation(entry);

// Transitive upstream / downstream traversal
auto upstream   = tracker.getUpstreamColumns({"users", "full_name"});
auto downstream = tracker.getDownstreamColumns({"users", "first_name"});

// Provenance as JSON
nlohmann::json prov = tracker.getColumnProvenance({"users", "full_name"});
```

**Design:** Append-only — `recordDerivation()` never modifies existing entries.
**Thread Safety:** All public methods are thread-safe.

---

### catalog_exporter.h

**Purpose:** Publish ThemisDB schema metadata to external data governance catalogs
(Apache Atlas, DataHub).

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `CatalogExporter` | Main export interface |
| `CatalogExporter::Config` | Connection parameters (type, endpoint, credentials) |
| `CatalogExporter::PublishResult` | Outcome: `success`, `entity_count`, `error` |
| `CatalogExporter::CatalogType` | `APACHE_ATLAS` or `DATAHUB` |

**Key API:**

```cpp
#include "metadata/catalog_exporter.h"
using namespace themis;

// Apache Atlas
CatalogExporter::Config cfg;
cfg.type     = CatalogExporter::CatalogType::APACHE_ATLAS;
cfg.endpoint = "http://atlas-host:21000";
cfg.username = "admin";
cfg.password = "admin";

CatalogExporter exporter(cfg);
auto result = exporter.publishSchema(schema_mgr.getAllTables());
if (!result.success) {
    spdlog::error("Atlas publish failed: {}", result.error);
}

// DataHub
CatalogExporter::Config dh_cfg;
dh_cfg.type     = CatalogExporter::CatalogType::DATAHUB;
dh_cfg.endpoint = "http://datahub-gms:8080";
dh_cfg.token    = "my-token";
CatalogExporter dh(dh_cfg);
dh.publishSchema(schema_mgr.getAllTables());

// Inject custom HTTP function (unit tests, offline mode)
exporter.setHttpPostForTesting([](const std::string&, const std::string&,
                                   const std::string&) { return 200; });
```

**Thread Safety:** No shared mutable state; create one instance per thread or
guard with external synchronization.

---

### distributed_catalog.h

**Purpose:** Distribute schema metadata across a cluster using the
`MetadataShardRouter` so that any shard can answer schema queries.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `DistributedMetadataCatalog` | Bridges local `SchemaManager` with the distributed shard router |

**Key API:**

```cpp
#include "metadata/distributed_catalog.h"
using namespace themis;

DistributedMetadataCatalog catalog(router);

// Publish a single schema to the cluster
catalog.publishSchema(schema_mgr.getTable("users").value());

// Bulk-sync all local schemas to the cluster
catalog.syncFromSchemaManager(schema_mgr);

// Fetch from the cluster (bypasses local cache)
auto remote = catalog.fetchSchema("users");

// Delete
catalog.deleteSchema("archived_table");

// Diagnostics
nlohmann::json stats = catalog.getStats();
```

**Thread Safety:** All public methods are thread-safe (atomic operation counters,
thread-safety delegated to the router).

---

### er_diagram_exporter.h

**Purpose:** Export cross-collection ER diagrams from `SchemaManager` metadata
in Mermaid, DOT (Graphviz), or JSON format.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `ERDiagramExporter` | Stateless exporter; all methods are `const` |

**Key API:**

```cpp
#include "metadata/er_diagram_exporter.h"
using namespace themis;

auto tables        = schema_mgr.getAllTables();
auto relationships = schema_mgr.getAllRelationships();

ERDiagramExporter exporter;

// Mermaid — embed in Markdown, GitHub, Confluence
std::string mermaid = exporter.exportMermaid(tables, relationships);

// DOT — render with: dot -Tsvg schema.dot -o schema.svg
std::string dot = exporter.exportDOT(tables, relationships);

// JSON graph — machine-readable node/edge representation
nlohmann::json graph = exporter.exportJSON(tables, relationships);
```

**Thread Safety:** All export methods are `const` and stateless; safe to call concurrently.

---

### index_recommender.h

**Purpose:** Lightweight auto index recommendation engine — records column access
patterns and suggests indexes that would improve query performance.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `IndexRecommender` | Access tracker + recommendation engine |
| `ColumnAccess` | Observed query access pattern for one column |
| `IndexRecommendation` | A single ADD or DROP recommendation with benefit score (0–100) |
| `IndexRecommendation::Action` | `ADD` or `DROP` |

**Key API:**

```cpp
#include "metadata/index_recommender.h"
using namespace themis;

IndexRecommender recommender(db, &stats);

// Record access patterns (call from query executor)
recommender.recordAccess("users", "email",
    /* filter_count= */ 100, /* sort_count= */ 0,
    /* avg_selectivity= */ 0.01);

// Get recommendations
auto recs = recommender.recommend();
for (const auto& r : recs) {
    std::cout << (r.action == IndexRecommendation::Action::ADD ? "ADD" : "DROP")
              << " INDEX ON " << r.table_name << "." << r.column_name
              << " (score=" << r.benefit_score << "): " << r.rationale << "\n";
}

// Background recommender with periodic analysis
recommender.startBackgroundAnalysis(std::chrono::hours(1));
recommender.stopBackgroundAnalysis();
```

---

### imetadata_security_provider.h

**Purpose:** Pluggable RBAC / access-control interface for all metadata operations.
Header-only; ships three implementations.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `IMetadataSecurityProvider` | Abstract interface; `hasPermission()` + `assertPermission()` |
| `NoOpMetadataSecurityProvider` | Default permit-all (zero overhead) |
| `InMemoryRbacMetadataSecurityProvider` | Thread-safe in-memory RBAC: `grant`/`revoke`/`revokeAll` |
| `MetadataOperation` | `READ_SCHEMA`, `WRITE_SCHEMA`, `READ_STATISTICS`, `WRITE_STATISTICS`, `READ_LINEAGE`, `WRITE_LINEAGE`, `READ_AUDIT_LOG`, `ADMIN` |
| `MetadataAccessDeniedException` | Thrown by `assertPermission()` on denial |

**Design rules:**
- `hasPermission()` is non-blocking; ≤ 1 µs on the hot path.
- Wildcard resource `"*"` grants the operation on every resource.
- `ADMIN` implies all other operations on all resources.

**Key API:**

```cpp
#include "metadata/imetadata_security_provider.h"
using namespace themis::metadata;

InMemoryRbacMetadataSecurityProvider sec;
sec.grant("analyst",  MetadataOperation::READ_SCHEMA,    "*");
sec.grant("dba",      MetadataOperation::WRITE_SCHEMA,   "*");
sec.grant("ops",      MetadataOperation::ADMIN,          "*");

// Enforcement (throws MetadataAccessDeniedException on denial)
sec.assertPermission("analyst", MetadataOperation::READ_SCHEMA, "users");

// Query without throwing
if (!sec.hasPermission("analyst", MetadataOperation::WRITE_SCHEMA, "users")) {
    return Status::AccessDenied;
}

// Revoke
sec.revoke("analyst", MetadataOperation::READ_SCHEMA, "*");
sec.revokeAll("analyst");
```

---

### imetadata_change_listener.h

**Purpose:** Observer interface for schema and metadata change events — allows
components (cache invalidators, exporters, audit hooks) to react without polling.
Header-only; ships two implementations.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `IMetadataChangeListener` | Abstract observer; implement `onMetadataChanged()` |
| `RecordingMetadataChangeListener` | Thread-safe in-memory recorder; useful for tests |
| `MetadataChangeEvent` | Change descriptor: `change_type`, `table_name`, `actor`, `detail`, `timestamp` |
| `MetadataChangeType` | `TABLE_CREATED`, `TABLE_MODIFIED`, `TABLE_DROPPED`, `CONSTRAINT_ADDED`, `CONSTRAINT_DROPPED`, `STATISTICS_UPDATED` |

**Design:** `onMetadataChanged()` must return promptly; heavy work should be
offloaded to a background thread inside the implementation.

**Key API:**

```cpp
#include "metadata/imetadata_change_listener.h"
using namespace themis::metadata;

RecordingMetadataChangeListener rec;
rec.setCallback([](const MetadataChangeEvent& ev) {
    spdlog::info("Schema change: {} on {}", static_cast<int>(ev.change_type),
                 ev.table_name);
});

// Wire into the catalog / schema manager (exact wiring depends on component)
// ...

assert(rec.eventCount() > 0);
auto last = rec.lastEvent();  // std::optional<MetadataChangeEvent>
rec.clear();
```

---

### imetadata_export_policy.h

**Purpose:** Pluggable policy that controls which tables are exported to external
metadata catalogs, and with what batching delay. Header-only; ships three
implementations.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `IMetadataExportPolicy` | Abstract policy: `shouldExport()` + `exportDelay()` |
| `AlwaysExportPolicy` | Export everything immediately (default) |
| `NeverExportPolicy` | Suppress all exports (offline mode, testing) |
| `FilteredExportPolicy` | Exclude explicitly listed tables; configurable uniform delay |
| `MetadataExportTrigger` | `SCHEMA_CREATED`, `SCHEMA_MODIFIED`, `SCHEMA_DROPPED`, `STATISTICS_UPDATED` |

**Design:** `shouldExport()` must be non-blocking; ≤ 1 µs on the hot path.

**Key API:**

```cpp
#include "metadata/imetadata_export_policy.h"
using namespace themis::metadata;

// Exclude internal tables; batch other exports after 500 ms
FilteredExportPolicy policy{std::chrono::milliseconds{500}};
policy.addExclusion("_internal_stats");
policy.addExclusion("_tmp_migration");

bool should = policy.shouldExport("users", MetadataExportTrigger::SCHEMA_MODIFIED);
auto delay  = policy.exportDelay("users", MetadataExportTrigger::SCHEMA_MODIFIED);
```

---

### imetadata_encryption_provider.h

**Purpose:** Pluggable field-level encryption policy for metadata values at rest
or in transit. Header-only; ships three implementations.

**Key Classes:**

| Class / Struct | Description |
|----------------|-------------|
| `IMetadataEncryptionProvider` | Abstract interface: `encrypt()` + `decrypt()` + `shouldEncrypt()` + `algorithm()` |
| `NoOpMetadataEncryptionProvider` | Pass-through (no encryption, default) |
| `FieldSetMetadataEncryptionProvider` | Byte-wise XOR cipher (**demo/testing only** — NOT cryptographically secure) |
| `MetadataEncryptionAlgorithm` | `NONE`, `XOR_BASIC`, `AES_GCM_256`, `CUSTOM` |
| `MetadataEncryptionException` | Thrown on empty encryption key or corrupt ciphertext |

> ⚠️ **Warning:** `FieldSetMetadataEncryptionProvider` uses XOR and is intended
> only for testing. Production deployments requiring encryption at rest must
> supply a custom `IMetadataEncryptionProvider` backed by AES-256-GCM.

**Key API:**

```cpp
#include "metadata/imetadata_encryption_provider.h"
using namespace themis::metadata;

FieldSetMetadataEncryptionProvider enc("my-secret-key");
enc.addEncryptedField("users", "ssn");
enc.addEncryptedField("users", "credit_card");

std::string ciphertext = enc.encrypt("users", "ssn", "123-45-6789");
std::string plaintext  = enc.decrypt("users", "ssn", ciphertext);
```

---

### aql_schema_bridge.h

**Purpose:** Bridge function that converts a `SchemaManager::TableSchema` to the
lightweight `aql::CollectionMetadata` snapshot consumed by the AQL query engine.
Header-only, lives in `namespace themis::aql`.

**Key Function:**

```cpp
namespace themis::aql {
    CollectionMetadata fromTableSchema(const SchemaManager::TableSchema& ts);
}
```

**Usage:**

```cpp
#include "metadata/aql_schema_bridge.h"
using namespace themis;

std::vector<aql::CollectionMetadata> meta;
for (const auto& t : schema_mgr.getAllTables()) {
    meta.push_back(aql::fromTableSchema(t));
}
aql_builder.setSchema(meta);
```

---

## Core Types

### Namespaces

Most classes live in `namespace themis`; the v1.6.0 interface headers
(`imetadata_*.h`, `metadata_snapshot.h`, `schema_diff.h`) use
`namespace themis::metadata`.  `aql_schema_bridge.h` uses `namespace themis::aql`.

### Return Patterns

| Pattern | Used by |
|---------|---------|
| `std::vector<T>` | Collection results (`getAllTables`, …) |
| `std::optional<T>` | Single nullable results (`getTable`, `store.load`, …) |
| `VersionResult<T>` | Schema versioning operations (carries `ok`, `value`, `error_message`) |
| `bool` | Write operations (`publishSchema`, `deleteTableSchema`, …) |
| `json` | JSON export methods |

### Thread Safety Summary

| Header | Thread-Safe? |
|--------|-------------|
| `schema_manager.h` | ✅ `std::shared_mutex` |
| `statistics_collector.h` | ✅ per-table locks |
| `information_schema.h` | ✅ (delegates to SchemaManager) |
| `schema_version_manager.h` | ❌ external synchronization required |
| `schema_audit_log.h` | ✅ append-only |
| `schema_consistency_checker.h` | ✅ `runCheck()`; `start/stop` not reentrant |
| `schema_constraints.h` | ✅ |
| `schema_diff.h` | ✅ stateless |
| `metadata_snapshot.h` | ✅ `std::mutex` |
| `column_lineage.h` | ✅ |
| `catalog_exporter.h` | ❌ no shared state; create per-thread |
| `distributed_catalog.h` | ✅ atomics + router |
| `er_diagram_exporter.h` | ✅ stateless |
| `index_recommender.h` | ✅ `std::mutex` |
| `imetadata_security_provider.h` | ✅ `std::mutex` |
| `imetadata_change_listener.h` | ✅ `std::mutex` |
| `imetadata_export_policy.h` | ✅ `std::mutex` |
| `imetadata_encryption_provider.h` | ✅ `std::mutex` |

---

## Configuration Quick Reference

| Parameter | Default | Description |
|-----------|---------|-------------|
| `SchemaManager::setCacheTTL()` | 60 s | Metadata cache expiry |
| `StatisticsCollector::startAutoRefresh()` | no default | Statistics refresh interval |
| `SchemaConsistencyChecker::kDefaultMaxStatsAge` | 24 h | Age before `stale_stats` issued |
| `FilteredExportPolicy` constructor | 0 ms | Batching delay for export policy |

---

## Runtime Behavior, Errors, and Limits

| Situation | Behavior |
|-----------|----------|
| Cache expired, RocksDB unreachable | `getAllTables()` returns stale cached data; logs warning |
| `SchemaVersionManager` table not found | Returns `VersionResult` with `TABLE_NOT_FOUND` |
| `SchemaAuditLog` write failure | Returns false; logs error; does not throw |
| `CatalogExporter` network failure | Returns `PublishResult{success=false, error=<message>}` |
| `MetadataAccessDeniedException` | Thrown by `assertPermission()`; catch in middleware |
| `MetadataSnapshotException` | Thrown by `store.save()` with empty `snapshot_id` |
| `MetadataEncryptionException` | Thrown by `FieldSetMetadataEncryptionProvider` with empty key |
| First schema discovery (cold cache) | May take up to 30 s for > 10 M RocksDB keys |
| Statistics accuracy | Sample-based; histogram accuracy ±20 % for uniform/skewed data |
| Version history in-memory limit | Last 1,000 versions per table; older versions purged from cache |

---

## Usage Examples

### Full Schema Discovery and Export
```cpp
#include "metadata/schema_manager.h"
using namespace themis;

SchemaManager mgr(db, &idx_mgr);
mgr.setCacheTTL(std::chrono::seconds(120));

// List all tables
for (const auto& t : mgr.getAllTables()) {
    std::cout << t.name << " (" << t.estimated_row_count << " rows)\n";
    for (const auto& p : t.properties) {
        std::cout << "  " << p.name << ": " << p.type
                  << (p.indexed ? " [idx:" + p.index_type + "]" : "") << "\n";
    }
}

// JSON export for REST API
nlohmann::json schema_json = mgr.toJSON();
response->setBody(schema_json.dump(2));
```

### Schema Versioning with Audit
```cpp
#include "metadata/schema_version_manager.h"
#include "metadata/schema_audit_log.h"
using namespace themis;

SchemaAuditLog audit(db);
SchemaVersionManager svm(db, schema_mgr);
svm.setAuditLog(&audit);

auto r = svm.createSchemaVersion("orders", "alice", "added status column");
if (r.ok) std::cout << "Version " << r.value << " created\n";

auto script = svm.generateMigrationScript("orders", 1, 2);
if (script.ok) std::cout << script.value;

for (const auto& e : audit.getHistory("orders")) {
    std::cout << e.timestamp_str << " " << e.operation << " by " << e.author << "\n";
}
```

### RBAC-Gated Statistics Access
```cpp
#include "metadata/imetadata_security_provider.h"
#include "metadata/statistics_collector.h"
using namespace themis;
using namespace themis::metadata;

InMemoryRbacMetadataSecurityProvider sec;
sec.grant("analyst", MetadataOperation::READ_STATISTICS, "*");

try {
    sec.assertPermission(current_user, MetadataOperation::READ_STATISTICS, "users");
    auto result = stats.collectStats("users");
    // use result ...
} catch (const MetadataAccessDeniedException& ex) {
    return http::Forbidden(ex.what());
}
```

---

## Troubleshooting

### `getAllTables()` returns empty list on first call
- **Cause:** Cache is cold; initial RocksDB scan may take several seconds.
- **Fix:** Call `schema_mgr.refreshCache()` at startup and pre-warm the cache.

### Cache returns stale schema after `ALTER TABLE`
- **Cause:** Cache TTL has not expired yet.
- **Fix:** Call `schema_mgr.refreshCache()` immediately after schema changes,
  or enable adaptive TTL with `schema_mgr.enableAdaptiveTTL()` and call
  `schema_mgr.recordMutation(table_name)` on every write.

### `CatalogExporter::publishSchema()` returns `success=false`
- **Cause:** Network connectivity issue, wrong credentials, or catalog API
  version mismatch.
- **Fix:** Verify endpoint URL and credentials in `Config`. Use
  `setHttpPostForTesting()` to inject a mock HTTP function for offline testing.

### `VersionResult` contains `TABLE_NOT_FOUND`
- **Cause:** `SchemaVersionManager::createSchemaVersion()` was never called for
  that table.
- **Fix:** Call `createSchemaVersion()` to snapshot the initial schema before
  querying history or running migrations.

### `MetadataAccessDeniedException` in production
- **Cause:** `InMemoryRbacMetadataSecurityProvider` does not have a grant for
  the requested (principal, operation, resource) triple.
- **Fix:** Call `sec.grant(principal, op, resource)` during initialization, or
  use the wildcard resource `"*"` to grant access to all resources.

### `SchemaConsistencyChecker` reports `stale_stats` on every run
- **Cause:** `StatisticsCollector::startAutoRefresh()` has not been started, or
  the refresh interval is longer than `kDefaultMaxStatsAge` (24 h).
- **Fix:** Start the auto-refresh thread with a suitable interval, or lower the
  max stats age threshold.

---

## Installation

This module is included as part of ThemisDB. Add the module headers to your
include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

---

## See Also

- [Implementation Documentation](../../src/metadata/README.md)
- [Architecture Guide](../../src/metadata/ARCHITECTURE.md)
- [Roadmap](../../src/metadata/ROADMAP.md)
- [Future Enhancements](../../src/metadata/FUTURE_ENHANCEMENTS.md)
- [Storage Module](../storage/README.md)
- [Index Module](../index/README.md)
- [Secondary Docs (German)](../../docs/de/metadata/README.md)

---

*Last Updated: May 2026*
*API Version: v1.6.0*
