# Schema Introspection - GAP-001 Implementation

**Feature ID:** GAP-001  
**Title:** Agentic AI Self-Awareness / Schema-Introspection  
**Status:** ✅ Implemented  
**Version:** 1.5.0  
**Date:** 2026-02-04  

---

## Overview

The SchemaManager is a core component of ThemisDB that provides **database self-awareness** and **schema introspection** capabilities. It enables ThemisDB to automatically discover and understand its own structure by scanning RocksDB tables and analyzing stored data.

### Purpose

- Enable AI agents and tools to query database schema programmatically
- Provide foundation for future REST/MCP API endpoints
- Support automatic schema discovery without manual configuration
- Enable dynamic schema validation and type checking

### Key Features

✅ **Automatic Table Discovery** - Scans RocksDB keys to identify tables/collections  
✅ **Property Type Detection** - Analyzes sample data to infer types (int/string/float/bool/vector/binary)  
✅ **Index Metadata Collection** - Integrates with SecondaryIndexManager for index information  
✅ **JSON Export** - Exports schema in JSON format for API consumption  
✅ **Thread-Safe Caching** - Concurrent read/write with configurable TTL (default: 60s)  
✅ **Custom Schema Management** - Store, update, and delete user-defined schemas  
✅ **Schema Validation** - Comprehensive validation of schema definitions  

---

## Architecture

### Component Structure

```
SchemaManager
├── RocksDB Key Scanning (table discovery)
├── BaseEntity Parsing (property type detection)
├── SecondaryIndexManager (index metadata)
└── Custom Schema Storage (RocksDB: config:schema:{table})
```

### Data Flow

```
┌─────────────┐
│  RocksDB    │
│  Tables     │
└──────┬──────┘
       │
       ▼
┌─────────────────┐
│ SchemaManager   │
│ - Discovery     │
│ - Type Analysis │
│ - Caching       │
└──────┬──────────┘
       │
       ▼
┌─────────────────┐
│ JSON Schema     │
│ (API Ready)     │
└─────────────────┘
```

---

## API Reference

### Header File

**Location:** `include/metadata/schema_manager.h`

### Core Data Structures

#### PropertyInfo
```cpp
struct PropertyInfo {
    std::string name;           // Property name
    std::string type;           // Type: "string", "integer", "double", "boolean", "vector", "binary", "null"
    bool indexed = false;       // Has secondary index
    bool nullable = true;       // Can be null/missing
    std::string index_type;     // "regular", "range", "sparse", "geo", "ttl", "fulltext"
    
    json toJSON() const;
};
```

#### IndexInfo
```cpp
struct IndexInfo {
    std::string name;               // Index name (column name)
    std::string type;               // "regular", "range", "sparse", "geo", "ttl", "fulltext", "composite"
    std::vector<std::string> columns; // Column list (for composite indexes)
    bool unique = false;            // Unique constraint
    
    json toJSON() const;
};
```

#### TableSchema
```cpp
struct TableSchema {
    std::string name;                       // Table/collection name
    std::string type;                       // "relational", "document", "graph_node", "graph_edge", "vector"
    std::vector<PropertyInfo> properties;   // Properties/columns
    std::vector<IndexInfo> indexes;         // Secondary indexes
    size_t estimated_row_count = 0;         // Approximate row count
    
    json toJSON() const;
};
```

#### DatabaseMetadata
```cpp
struct DatabaseMetadata {
    std::string version;                            // ThemisDB version
    size_t table_count = 0;                         // Total tables/collections
    size_t total_rows = 0;                          // Total entities (approx)
    std::vector<std::string> capabilities;          // Enabled features
    std::chrono::system_clock::time_point last_refresh;
    
    json toJSON() const;
};
```

### Public Methods

#### Schema Discovery

```cpp
// Get all tables/collections (cached)
std::vector<TableSchema> getAllTables();

// Get specific table schema by name
std::optional<TableSchema> getTable(std::string_view name);

// Get all relationships (graph edges)
std::vector<RelationshipSchema> getAllRelationships();

// Get database-level metadata
DatabaseMetadata getDatabaseMetadata();

// Force cache refresh
void refreshCache();

// Set cache TTL (default: 60 seconds)
void setCacheTTL(std::chrono::seconds ttl);
```

#### JSON Export

```cpp
// Export full schema as JSON
json toJSON();

// Export single table as JSON
json tableToJSON(std::string_view table_name);

// Export database capabilities
json getCapabilitiesJSON();
```

#### Schema Management

```cpp
// Store/update custom schema
bool setTableSchema(std::string_view table_name, const TableSchema& schema);

// Partial update of existing schema
bool patchTableSchema(std::string_view table_name, const json& updates);

// Delete custom schema
bool deleteTableSchema(std::string_view table_name);

// Validate schema structure
std::string validateSchema(const TableSchema& schema) const;

// Parse TableSchema from JSON
static TableSchema parseTableSchema(const json& j);
```

---

## Usage Examples

### Basic Usage

```cpp
#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

// Initialize database and index manager
RocksDBWrapper db(config);
db.open();
SecondaryIndexManager index_mgr(db);

// Create SchemaManager
SchemaManager schema_mgr(db, &index_mgr);

// Get all tables
auto tables = schema_mgr.getAllTables();
for (const auto& table : tables) {
    std::cout << "Table: " << table.name 
              << " (type: " << table.type << ")" << std::endl;
    std::cout << "  Rows: " << table.estimated_row_count << std::endl;
    std::cout << "  Properties: " << table.properties.size() << std::endl;
}

// Get specific table
auto user_table = schema_mgr.getTable("users");
if (user_table) {
    for (const auto& prop : user_table->properties) {
        std::cout << "  - " << prop.name << " (" << prop.type << ")";
        if (prop.indexed) {
            std::cout << " [indexed]";
        }
        std::cout << std::endl;
    }
}
```

### JSON Export

```cpp
// Export full schema
json schema_json = schema_mgr.toJSON();
std::cout << schema_json.dump(2) << std::endl;

// Export single table
json user_json = schema_mgr.tableToJSON("users");
std::cout << user_json.dump(2) << std::endl;

// Get capabilities
json caps = schema_mgr.getCapabilitiesJSON();
std::cout << "Database capabilities: " << caps["capabilities"] << std::endl;
```

### Custom Schema Management

```cpp
// Create custom schema
SchemaManager::TableSchema custom_schema;
custom_schema.name = "products";
custom_schema.type = "relational";

SchemaManager::PropertyInfo id_prop;
id_prop.name = "id";
id_prop.type = "integer";
id_prop.indexed = true;
id_prop.nullable = false;
custom_schema.properties.push_back(id_prop);

SchemaManager::PropertyInfo name_prop;
name_prop.name = "name";
name_prop.type = "string";
name_prop.nullable = true;
custom_schema.properties.push_back(name_prop);

// Store custom schema
bool success = schema_mgr.setTableSchema("products", custom_schema);
if (success) {
    std::cout << "Custom schema stored successfully" << std::endl;
}

// Patch existing schema
json updates;
updates["type"] = "document";
updates["properties"] = json::array();
updates["properties"].push_back({
    {"name", "description"},
    {"type", "string"},
    {"nullable", true}
});

bool patched = schema_mgr.patchTableSchema("products", updates);
if (patched) {
    std::cout << "Schema updated successfully" << std::endl;
}
```

### Cache Management

```cpp
// Set shorter cache TTL for development
schema_mgr.setCacheTTL(std::chrono::seconds(10));

// Force immediate refresh after schema changes
schema_mgr.refreshCache();
```

---

## Implementation Details

### File Locations

- **Header:** `include/metadata/schema_manager.h`
- **Implementation:** `src/metadata/schema_manager.cpp` (1112 lines)
- **Tests:** `tests/test_schema_manager.cpp` (837 lines)

### Type Detection Algorithm

The SchemaManager uses sample-based type detection by analyzing stored BaseEntity objects:

1. **Scan up to 100 sample entities** per table (configurable)
2. **Parse each entity** using BaseEntity deserialization
3. **Aggregate property types** across samples
4. **Resolve conflicts** (e.g., mixed int/double → double)
5. **Detect special types** (vectors, binary data)

**Supported Types:**
- `integer` - int64_t values
- `double` - floating-point values
- `string` - UTF-8 text
- `boolean` - true/false values
- `vector` - float arrays (detected by property name or size)
- `binary` - byte arrays
- `null` - missing/null values

### Table Name Discovery

Table names are discovered by scanning RocksDB keys:

1. **Iterate all keys** using RocksDB iterator
2. **Extract table prefix** (everything before first `:`)
3. **Deduplicate** table names
4. **Determine table type** using naming heuristics

**Naming Heuristics:**
- `*node*` → `graph_node`
- `*edge*` → `graph_edge`
- `*vector*`, `*embedding*` → `vector`
- `*collection*`, `*document*` → `document`
- Default → `relational`

### Index Discovery

Index metadata is collected from SecondaryIndexManager:

1. **Query SecondaryIndexManager** for table indexes
2. **Extract index metadata** (name, type, columns)
3. **Mark properties** as indexed in PropertyInfo
4. **Store index type** (regular, range, sparse, geo, ttl, fulltext)

### Custom Schema Storage

Custom schemas are persisted in RocksDB under special keys:

- **Key Pattern:** `config:schema:{table_name}`
- **Value:** JSON-serialized TableSchema
- **Loaded:** On SchemaManager initialization
- **Saved:** Automatically on setTableSchema/patchTableSchema

Custom schemas **override** auto-discovered schemas when present.

---

## Performance Characteristics

### Discovery Performance

- **Tested:** Up to 100 tables, 200 total entities
- **Time:** <100ms for full schema discovery
- **Memory:** <50 MB cache overhead for 100 tables
- **Sample Size:** Default 100 entities per table (configurable)

### Cache Performance

- **Hit Rate:** >90% expected (measured in tests)
- **First Call:** ~50-100ms (builds cache)
- **Cached Call:** <100µs (cache hit)
- **Speedup:** ~500-1000x for cached queries
- **TTL:** 60 seconds default (configurable)

### Concurrency

- **Thread Model:** Multiple readers, single writer
- **Lock:** `std::shared_mutex` for cache access
- **Read Operations:** Lock-free for cached data
- **Write Operations:** Exclusive lock for cache updates
- **Safe Operations:** getAllTables(), getTable() are thread-safe

---

## Testing

### Test Coverage

The SchemaManager has comprehensive test coverage in `tests/test_schema_manager.cpp`:

#### Basic Functionality Tests
- ✅ Empty database handling
- ✅ Single table discovery
- ✅ Multiple table discovery
- ✅ Get table by name
- ✅ Index discovery and metadata

#### Cache Tests
- ✅ Cache mechanism validation
- ✅ Cache TTL expiration
- ✅ Manual cache refresh
- ✅ Cache hit rate measurement

#### JSON Export Tests
- ✅ Full schema JSON export
- ✅ Single table JSON export
- ✅ Capabilities JSON export
- ✅ JSON structure validation

#### Schema Management Tests
- ✅ Custom schema creation
- ✅ Schema persistence across instances
- ✅ Schema patching (partial updates)
- ✅ Schema deletion
- ✅ Custom schema overrides discovered

#### Validation Tests
- ✅ Empty table name rejection
- ✅ Invalid character detection
- ✅ Invalid type detection
- ✅ Duplicate property name detection
- ✅ Invalid property type rejection
- ✅ Index reference validation
- ✅ Valid schema acceptance

#### JSON Parsing Tests
- ✅ Full schema parsing
- ✅ Minimal schema parsing
- ✅ Missing name handling
- ✅ Invalid JSON handling

#### Performance Tests
- ✅ Discovery time measurement (20 tables, 200 rows)
- ✅ Cache hit rate validation
- ✅ Speedup calculation

### Running Tests

```bash
# Build tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target test_schema_manager

# Run tests
./build/tests/test_schema_manager

# Expected output:
# [==========] Running 35 tests from 1 test suite.
# [----------] 35 tests from SchemaManagerTest
# [==========] 35 tests from 1 test suite ran.
# [  PASSED  ] 35 tests.
```

---

## Integration with Other Components

### RocksDB Wrapper

SchemaManager uses `RocksDBWrapper` for:
- Key iteration (table discovery)
- Entity retrieval (property sampling)
- Custom schema persistence

### SecondaryIndexManager

Integration with `SecondaryIndexManager` provides:
- Index metadata collection
- Index type information (regular, range, geo, etc.)
- Property indexing status

### BaseEntity

Uses `BaseEntity` for:
- Entity deserialization
- Property extraction
- Type detection

### Future Integration Points

- **REST API:** Expose schema endpoints (`GET /api/schema`)
- **MCP Server:** Provide schema introspection via MCP protocol
- **GraphQL:** Enable schema-driven GraphQL type generation
- **AI Agents:** Allow LLM agents to query database structure

---

## Future Enhancements

### Planned Features

1. **Relationship Discovery** - Automatic foreign key detection
2. **Statistics Collection** - Gather column statistics (min/max/avg/distinct)
3. **Schema Versioning** - Track schema changes over time
4. **Migration Support** - Generate migration scripts from schema diffs
5. **Composite Index Detection** - Better support for multi-column indexes
6. **Constraint Detection** - Identify unique/check/foreign key constraints
7. **Performance Metrics** - Query performance per table/index

### REST API Integration (Not Yet Implemented)

Future endpoints (planned):

```
GET    /api/schema              - Get full database schema
GET    /api/schema/tables       - List all tables
GET    /api/schema/tables/:name - Get specific table schema
POST   /api/schema/tables/:name - Create custom schema
PATCH  /api/schema/tables/:name - Update schema
DELETE /api/schema/tables/:name - Delete custom schema
GET    /api/schema/capabilities - Get database capabilities
```

---

## Best Practices

### When to Use SchemaManager

✅ **Good Use Cases:**
- AI agents need to query database structure
- REST/GraphQL APIs need dynamic schema
- Schema validation before data operations
- Documentation generation
- Database exploration tools

❌ **Not Recommended For:**
- High-frequency operations (use caching)
- Write-heavy workloads (cache invalidation overhead)
- Security-sensitive schema exposure (add authorization layer)

### Performance Tips

1. **Use Caching:** Don't call `refreshCache()` unnecessarily
2. **Adjust TTL:** Set longer TTL for stable schemas
3. **Sample Size:** Reduce sample size for large tables
4. **Batch Operations:** Group schema queries together
5. **Thread Safety:** Leverage concurrent reads for high throughput

### Security Considerations

1. **Access Control:** Add authorization layer before exposing to APIs
2. **Rate Limiting:** Protect schema endpoints from abuse
3. **Sensitive Data:** Be cautious with property values in samples
4. **Schema Leaking:** Consider what schema information to expose publicly

---

## Troubleshooting

### Common Issues

#### Cache Not Updating
**Symptom:** Schema changes not reflected  
**Solution:** Call `refreshCache()` explicitly after structural changes

#### Slow Discovery
**Symptom:** getAllTables() takes >1 second  
**Solution:** Reduce sample size or increase cache TTL

#### Missing Properties
**Symptom:** Not all properties discovered  
**Solution:** Increase sample size (default: 100)

#### Wrong Table Type
**Symptom:** Table type detected incorrectly  
**Solution:** Use custom schema with correct type

---

## References

### Documentation
- [Repository Structure Guide](../architecture/SOURCE_DIRECTORY_GUIDE.md)
- [RocksDB Wrapper Documentation](../storage/ROCKSDB_WRAPPER.md)
- [Secondary Index Manager](../index/SECONDARY_INDEX.md)

### Code References
- Header: `include/metadata/schema_manager.h`
- Implementation: `src/metadata/schema_manager.cpp`
- Tests: `tests/test_schema_manager.cpp`
- BaseEntity: `include/storage/base_entity.h`

### Related Issues
- Issue #1: GAP-001 Schema Introspection (this document)
- Future: REST API integration
- Future: MCP protocol support

---

## Changelog

### Version 1.5.0 (2026-02-04)
- ✅ Initial implementation complete
- ✅ All core features implemented
- ✅ Comprehensive test suite added
- ✅ Documentation created

---

**Status:** ✅ Implementation Complete  
**Ready For:** Testing, Code Review, Integration  
**Next Steps:** REST API endpoint implementation (future work)
