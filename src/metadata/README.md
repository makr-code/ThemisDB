> **Build:** `cmake --preset linux-ninja-release && cmake --build build-linux-ninja-release --target <target>`

# Metadata Module

Database metadata and schema introspection for ThemisDB.

## Module Purpose

Manages the ThemisDB metadata catalog, providing schema management, collection metadata, type system definitions, and index metadata tracking.

## Subsystem Scope

**In scope:** Collection and schema metadata management, type system and field definitions, index metadata registry, metadata versioning.

**Out of scope:** Data storage (handled by storage module), index construction (handled by index module), configuration management (handled by config module).

## Relevant Interfaces

| File | Description |
|------|-------------|
| `schema_manager.cpp` | Central schema discovery, table/property/index/relationship metadata |
| `statistics_collector.cpp` | Cardinality, selectivity, equi-height histograms |
| `information_schema.cpp` | SQL:2003-standard INFORMATION_SCHEMA views |
| `schema_version_manager.cpp` | Schema version tracking, diff, migration scripts |
| `schema_audit_log.cpp` | Durable per-table audit trail (RocksDB-persisted) |
| `schema_consistency_checker.cpp` | Background health scan for metadata consistency |
| `schema_constraints.cpp` | User-defined schema constraint validation |
| `column_lineage.cpp` | Column-level derivation DAG and provenance tracking |
| `er_diagram_exporter.cpp` | Cross-collection ER diagram export (Mermaid, DOT, JSON) |
| `catalog_exporter.cpp` | Apache Atlas and DataHub integration |
| `distributed_catalog.cpp` | Distributed metadata catalog across shards |
| `index_recommender.cpp` | Query-pattern-driven index usage and recommendation engine |
| `imetadata_security_provider.h` | `IMetadataSecurityProvider`, `NoOpMetadataSecurityProvider`, `InMemoryRbacMetadataSecurityProvider` — pluggable RBAC interface |
| `imetadata_change_listener.h` | `IMetadataChangeListener`, `RecordingMetadataChangeListener`, `MetadataChangeEvent` — observer interface for schema change events |
| `imetadata_export_policy.h` | `IMetadataExportPolicy`, `AlwaysExportPolicy`, `NeverExportPolicy`, `FilteredExportPolicy` — external catalog export policy |
| `imetadata_encryption_provider.h` | `IMetadataEncryptionProvider`, `NoOpMetadataEncryptionProvider`, `FieldSetMetadataEncryptionProvider` — field-level encryption policy |
| `metadata_snapshot.h` | `MetadataSnapshot`, `IMetadataSnapshotStore`, `InMemoryMetadataSnapshotStore` — point-in-time schema snapshots |
| `schema_diff.h` | `SchemaDiff`, `SchemaDiffEngine`, `ColumnDiff`, `IndexDiff` — structural diff engine for table schemas |
| `aql_schema_bridge.h` | Free function bridge from `SchemaManager::TableSchema` to AQL `CollectionMetadata` |

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — All Phase 1–3 features shipped; schema introspection, statistics,
changefeeds, adaptive TTL, audit log, consistency checker, ER diagram export, external catalog
integration (Apache Atlas, DataHub), column lineage, distributed catalog, and the Schema API REST
endpoint are production-ready as of v1.5.x.
<!-- status: current | validated: 2026-04-06 | commit: 4c1a2dfc1 -->

## Components

- **Schema Manager**: Database self-awareness and schema discovery
- **System Catalog**: Metadata storage and retrieval
- **Statistics Collector**: Table and index statistics
- **Information Schema**: SQL-standard metadata views
- **Metadata Cache**: Performance-optimized metadata caching
- **Column Lineage Tracker**: Column-level derivation and data provenance
- **Catalog Exporter**: Publish schema metadata to Apache Atlas and DataHub

## Features

### Schema Discovery
- **Automatic table discovery**: Scan RocksDB keys to find all tables
- **Property type detection**: Analyze stored entities for schema
- **Index metadata**: Collect index information from IndexManager
- **Relationship discovery**: Detect graph edges and foreign keys
- **Thread-safe caching**: Configurable TTL with read-heavy optimization

### System Catalog
- **Table metadata**: Names, types, row counts, storage info
- **Column metadata**: Names, types, nullability, indexes
- **Index metadata**: Names, types, columns, uniqueness
- **Statistics**: Cardinality, selectivity, data distribution
- **Version tracking**: Schema version and change history

### Information Schema
- **INFORMATION_SCHEMA views**: SQL-standard metadata access
- **System tables**: `tables`, `columns`, `indexes`, `statistics`
- **Metadata queries**: Query metadata like regular data
- **Integration with AQL**: Use AQL to query schema

### Column Lineage
- **Column-level derivation tracking**: Record how each column was produced from source columns
- **Transitive upstream/downstream traversal**: BFS through the derivation DAG
- **Provenance export**: Structured JSON for compliance and audit
### External Catalog Integration
- **Apache Atlas**: Publish `rdbms_db`, `rdbms_table`, `rdbms_column` entities via the v2 bulk API
- **DataHub**: Emit `datasetProperties` and `schemaMetadata` MetadataChangeProposals per table
- **Configurable auth**: Basic auth (Atlas) or Bearer token (DataHub)
- **No-network tests**: Injectable HTTP function for unit testing without a live catalog

### Performance Optimization
- **Metadata caching**: Cache with configurable TTL (default 60s)
- **Incremental updates**: Only scan changed tables
- **Lazy loading**: Load metadata on demand
- **Index statistics**: Use for query optimization

## Architecture

```
MetadataModule
├─→ SchemaManager (Primary metadata interface)
│   ├─→ RocksDB Key Scanning (table discovery)
│   ├─→ BaseEntity Parsing (property types)
│   └─→ SecondaryIndexManager (index metadata)
├─→ StatisticsCollector (Table/index statistics)
├─→ SystemCatalog (Metadata persistence)
├─→ InformationSchema (SQL-standard views)
└─→ ColumnLineageTracker (Column-level derivation DAG)
└─→ CatalogExporter (Apache Atlas & DataHub integration)
```

## Use Cases

### Database Introspection
- List all tables and collections
- Discover schema without prior knowledge
- Generate documentation automatically
- IDE autocomplete and IntelliSense

### Query Optimization
- Use statistics for query planning
- Choose optimal indexes
- Estimate result set sizes
- Adaptive query execution

### Schema Management
- Track schema changes over time
- Validate schema compatibility
- Generate migration scripts
- Enforce schema constraints

### Monitoring and Administration
- Monitor table growth
- Track index usage
- Identify unused indexes
- Capacity planning

## Performance Characteristics

### Schema Discovery
- **Discovery time**: <100ms for typical schemas (up to 100 tables)
- **Cache hit rate**: >90% expected
- **Memory overhead**: <50 MB for 100 tables
- **Throughput**: 1K+ metadata queries/second (cached)

### Statistics Collection
- **Collection time**: 1-10 seconds per table (depends on size)
- **Update frequency**: Configurable (default: hourly)
- **Storage overhead**: ~1-5% of table size
- **Accuracy**: Sample-based (configurable sample size)

## Configuration

### Schema Manager Setup
```cpp
#include "metadata/schema_manager.h"

using namespace themis;

// Create schema manager
SchemaManager schema_mgr(db_wrapper, index_manager);

// Optionally configure cache TTL
schema_mgr.setCacheTTL(std::chrono::seconds(60));

// Get all tables
auto tables = schema_mgr.getAllTables();

for (const auto& table : tables) {
    std::cout << "Table: " << table.name
              << " Type: " << table.type
              << " Rows: " << table.estimated_row_count << std::endl;

    for (const auto& prop : table.properties) {
        std::cout << "  - " << prop.name
                  << " (" << prop.type << ")"
                  << (prop.indexed ? " [indexed]" : "") << std::endl;
    }
}
```

### Export to JSON
```cpp
// Export all metadata to JSON
nlohmann::json schema_json = schema_mgr.toJSON();

// Save to file
std::ofstream out("schema.json");
out << schema_json.dump(2);

// Or serve via REST API
http_response->json(schema_json);
```

### Publish to Apache Atlas or DataHub
```cpp
#include "metadata/catalog_exporter.h"

// Apache Atlas
CatalogExporter::Config atlas_cfg;
atlas_cfg.type     = CatalogExporter::CatalogType::APACHE_ATLAS;
atlas_cfg.endpoint = "http://atlas-host:21000";
atlas_cfg.username = "admin";
atlas_cfg.password = "admin";

CatalogExporter atlas_exporter(atlas_cfg);
auto result = atlas_exporter.publishSchema(schema_mgr.getAllTables());
if (!result.success) {
    spdlog::error("Atlas publish failed: {}", result.error);
}

// DataHub
CatalogExporter::Config dh_cfg;
dh_cfg.type     = CatalogExporter::CatalogType::DATAHUB;
dh_cfg.endpoint = "http://datahub-gms:8080";
dh_cfg.token    = "my-token";

CatalogExporter dh_exporter(dh_cfg);
dh_exporter.publishSchema(schema_mgr.getAllTables());
```

### Query Metadata
```cpp
// Get specific table schema
auto table_schema = schema_mgr.getTableSchema("users");

if (table_schema.has_value()) {
    std::cout << "Table: " << table_schema->name << std::endl;
    std::cout << "Properties: " << table_schema->properties.size() << std::endl;
    std::cout << "Indexes: " << table_schema->indexes.size() << std::endl;
}
```

## Integration Points

- **Storage Layer**: RocksDB key scanning for table discovery
- **Index Module**: Index metadata and statistics
- **Query Module**: Query optimization using statistics
- **API Module**: REST API for schema introspection
- **MCP Module**: Model Context Protocol integration

## Thread Safety

- Thread-safe with `std::shared_mutex` for read-heavy workloads
- Multiple concurrent readers supported
- Single writer for cache updates
- Safe for high-concurrency access

## Dependencies

- **RocksDB**: Key scanning and storage
- **Secondary Index Manager**: Index metadata
- **nlohmann/json**: JSON serialization
- **spdlog**: Logging

## Documentation

For detailed implementation documentation, see:
- [Public API Headers](../../include/metadata/README.md)
- [Architecture](ARCHITECTURE.md)
- [Roadmap](ROADMAP.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)
- [Secondary Docs (German)](../../docs/de/metadata/README.md)

## Version History

- **v1.0.x**: Initial schema manager with automatic discovery
- **v1.1.x**: Statistics collector and INFORMATION_SCHEMA views
- **v1.2.x**: Schema versioning, diff, and migration script generation
- **v1.3.x**: Real-time schema change notifications via changefeeds; adaptive TTL
- **v1.4.x**: Column lineage, Apache Atlas/DataHub integration, ER diagram export
- **v1.5.x**: Schema audit log, consistency checker, distributed catalog, full production readiness

## Examples

### List All Tables
```cpp
SchemaManager schema_mgr(db, idx_mgr);
auto tables = schema_mgr.getAllTables();

for (const auto& table : tables) {
    std::cout << table.name << std::endl;
}
```

### Check if Column Exists
```cpp
auto schema = schema_mgr.getTableSchema("users");
if (schema.has_value()) {
    auto it = std::find_if(
        schema->properties.begin(),
        schema->properties.end(),
        [](const auto& prop) { return prop.name == "email"; }
    );

    if (it != schema->properties.end()) {
        std::cout << "Column 'email' exists, type: " << it->type << std::endl;
    }
}
```

### Get Index Information
```cpp
auto schema = schema_mgr.getTableSchema("users");
if (schema.has_value()) {
    for (const auto& idx : schema->indexes) {
        std::cout << "Index: " << idx.name
                  << " Type: " << idx.type
                  << " Unique: " << (idx.unique ? "yes" : "no") << std::endl;
    }
}
```

## Best Practices

### Cache Management
1. **Set appropriate TTL**: Balance freshness vs performance
2. **Invalidate on schema changes**: Ensure cache consistency
3. **Warm cache on startup**: Preload frequently accessed metadata

### Performance
1. **Use cached access**: Don't bypass cache unless necessary
2. **Batch metadata queries**: Reduce overhead
3. **Monitor cache hit rate**: Tune TTL based on metrics

### Schema Evolution
1. **Track schema versions**: Document changes over time
2. **Use migration scripts**: Automate schema updates
3. **Test compatibility**: Verify backward compatibility

## See Also

- [Header Documentation](../../include/metadata/README.md) - Public API definitions
- [Storage Module](../storage/README.md) - Underlying storage layer
- [Index Module](../index/README.md) - Index metadata
- [Query Module](../query/README.md) - Query optimization
- [Architecture Guide](../../ARCHITECTURE.md) - Metadata architecture

## Scientific References

1. ISO/IEC. (2013). **Information Technology – Metadata Registries (MDR) – Part 1: Framework**. ISO/IEC 11179-1:2013. https://www.iso.org/standard/61932.html

2. W3C. (2013). **PROV-O: The PROV Ontology**. W3C Recommendation. https://www.w3.org/TR/prov-o/

3. Bernstein, P. A., & Melnik, S. (2007). **Model Management 2.0: Manipulating Richer Mappings**. *Proceedings of the 2007 ACM SIGMOD International Conference on Management of Data*, 1–12. https://doi.org/10.1145/1247480.1247482

4. Doan, A., Halevy, A., & Ives, Z. (2012). **Principles of Data Integration**. Morgan Kaufmann. ISBN: 978-0-124-16248-4

5. Gray, J., Bosworth, A., Layman, A., & Pirahesh, H. (1997). **Data Cube: A Relational Aggregation Operator Generalizing Group-By, Cross-Tab, and Sub-Totals**. *Data Mining and Knowledge Discovery*, 1, 29–53. https://doi.org/10.1023/A:1009726021843

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
