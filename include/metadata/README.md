# Metadata Module - Public API
<!-- status: current | validated: 2026-04-06 | commit: 4c1a2dfc1 -->

Public interface definitions for ThemisDB metadata functionality.

## Headers

### schema_manager.h
**Purpose:** Database schema introspection and self-awareness

**Key Classes:**
- `SchemaManager`: Main metadata interface
- `TableSchema`: Table/collection schema representation
- `PropertyInfo`: Column/property metadata
- `IndexInfo`: Index metadata

**Usage:**
```cpp
#include "metadata/schema_manager.h"

using namespace themis;

// Create schema manager
SchemaManager schema_mgr(db_wrapper, index_manager);

// Get all tables
auto tables = schema_mgr.getAllTables();

// Get specific table schema
auto schema = schema_mgr.getTableSchema("users");

// Export to JSON
nlohmann::json json = schema_mgr.toJSON();
```

**Thread Safety:** Thread-safe with read-heavy optimization

---

### catalog_exporter.h
**Purpose:** Publish ThemisDB schema metadata to external data governance catalogs

**Key Classes:**
- `CatalogExporter`: Main export interface
- `CatalogExporter::Config`: Connection parameters (endpoint, auth, database name)
- `CatalogExporter::PublishResult`: Result with success flag, entity count, and error message
- `CatalogExporter::CatalogType`: `APACHE_ATLAS` or `DATAHUB`

**Usage:**
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
    spdlog::error("Publish failed: {}", result.error);
}

// DataHub
CatalogExporter::Config dh_cfg;
dh_cfg.type     = CatalogExporter::CatalogType::DATAHUB;
dh_cfg.endpoint = "http://datahub-gms:8080";
dh_cfg.token    = "my-token";

CatalogExporter dh_exporter(dh_cfg);
dh_exporter.publishTable(schema_mgr.getTable("users").value());
```

**Thread Safety:** Not shared-state; create per-thread or use external synchronization.

---

## Core Types

### TableSchema
Complete schema information for a table/collection.

**Fields:**
- `name`: Table name
- `type`: Table type ("collection", "edge", "view")
- `estimated_row_count`: Approximate row count
- `properties`: List of PropertyInfo
- `indexes`: List of IndexInfo

**Methods:**
- `toJSON()`: Serialize to JSON

### PropertyInfo
Column/property metadata.

**Fields:**
- `name`: Property name
- `type`: Data type ("string", "integer", "double", "boolean", "vector", "binary", "null")
- `indexed`: Has secondary index
- `nullable`: Can be null/missing
- `index_type`: Index type if indexed

### IndexInfo
Index metadata.

**Fields:**
- `name`: Index name
- `type`: Index type ("regular", "range", "sparse", "geo", "ttl", "fulltext", "composite")
- `columns`: Indexed columns (for composite)
- `unique`: Unique constraint

---

## API Conventions

### Namespace
```cpp
namespace themis {
    class SchemaManager { /* ... */ };
}
```

### Return Types
- `std::vector<TableSchema>`: For collection results
- `std::optional<TableSchema>`: For single results
- Direct types for non-fallible operations

### Thread Safety
- All methods are thread-safe
- Uses `std::shared_mutex` for optimal concurrency
- Safe for concurrent reads

---

## Integration Points

### With Storage
```cpp
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"

RocksDBWrapper db("path/to/db");
SecondaryIndexManager idx_mgr(&db);

SchemaManager schema_mgr(&db, &idx_mgr);
```

### With REST API
```cpp
// Export schema via HTTP
auto schema_json = schema_mgr.toJSON();
response->json(schema_json);
```

---

## Examples

### List All Tables
```cpp
SchemaManager schema_mgr(&db, &idx_mgr);

auto tables = schema_mgr.getAllTables();
for (const auto& table : tables) {
    std::cout << "Table: " << table.name
              << " (" << table.type << ")"
              << " Rows: ~" << table.estimated_row_count << std::endl;
}
```

### Get Table Properties
```cpp
auto schema = schema_mgr.getTableSchema("users");
if (schema.has_value()) {
    std::cout << "Properties:" << std::endl;
    for (const auto& prop : schema->properties) {
        std::cout << "  " << prop.name
                  << " : " << prop.type;
        if (prop.indexed) {
            std::cout << " [indexed: " << prop.index_type << "]";
        }
        std::cout << std::endl;
    }
}
```

### Export Schema to JSON
```cpp
// Export all metadata
nlohmann::json full_schema = schema_mgr.toJSON();

// Save to file
std::ofstream out("schema.json");
out << full_schema.dump(2);

// Export single table
auto table_schema = schema_mgr.getTableSchema("users");
if (table_schema.has_value()) {
    nlohmann::json table_json = table_schema->toJSON();
    std::cout << table_json.dump(2) << std::endl;
}
```

---

## Performance Characteristics

- **getAllTables()**: <100ms (typical), cached
- **getTableSchema()**: <1ms (cached)
- **toJSON()**: <10ms for 100 tables
- **Memory:** <50 MB for 100 tables (cached)

---

## Configuration

### Cache TTL
```cpp
// Default: 60 seconds
schema_mgr.setCacheTTL(std::chrono::seconds(60));

// Disable cache (always scan)
schema_mgr.setCacheTTL(std::chrono::seconds(0));

// Long cache for read-heavy workloads
schema_mgr.setCacheTTL(std::chrono::minutes(10));
```

### Invalidate Cache
```cpp
// Invalidate after schema change
db.createCollection("new_table");
schema_mgr.invalidateCache();

// Or let it expire naturally (TTL)
```

---

---

## column_lineage.h
**Purpose:** Column-level lineage and data provenance tracking

**Key Classes:**
- `ColumnLineageTracker`: Thread-safe, append-only DAG tracker for column derivations
- `ColumnRef`: Typed column identifier (`table.column`)
- `ColumnLineageEntry`: A single derivation step (target, sources, transformation, timestamp)
- `ColumnLineageRecord`: All entries recorded for one target column
- `TransformationType`: Derivation kind (`DIRECT_COPY`, `RENAME`, `CAST`, `COMPUTED`, `AGGREGATION`, `ANONYMIZATION`, `ENRICHMENT`, `CUSTOM`)

**Usage:**
```cpp
#include "metadata/column_lineage.h"

using namespace themis::metadata;

ColumnLineageTracker tracker;

// Record that full_name is computed from first_name + last_name
ColumnLineageEntry entry;
entry.target_column  = {"users", "full_name"};
entry.source_columns = {{"users", "first_name"}, {"users", "last_name"}};
entry.transformation = TransformationType::COMPUTED;
entry.transformation_expression = "first_name || ' ' || last_name";
entry.performed_by   = "etl-service";
tracker.recordDerivation(entry);

// Query all upstream sources of a column (transitive)
auto upstream = tracker.getUpstreamColumns({"users", "full_name"});

// Full structured provenance record as JSON
nlohmann::json prov = tracker.getColumnProvenance({"users", "full_name"});
```

**Thread Safety:** All public methods are thread-safe.
**Design:** Append-only — `recordDerivation()` never modifies or deletes existing entries.

---

## See Also

- [Implementation Documentation](../../src/metadata/README.md)
- [Storage Module](../storage/README.md)
- [Index Module](../index/README.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)

---

*Last Updated: April 2026*
*API Version: v1.5.0*

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
