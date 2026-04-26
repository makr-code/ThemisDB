# Composite Index Best Practices

## Overview

Composite indexes in ThemisDB allow efficient querying on multiple columns simultaneously. This guide provides best practices for designing, using, and maintaining composite secondary indexes.

## What are Composite Indexes?

A composite index is an index on multiple columns (minimum 2). Unlike single-column indexes, composite indexes enable efficient queries that filter or sort on multiple fields.

**Key Schema Format:**
```
idx:table:col1+col2:val1:val2:PK
```

## When to Use Composite Indexes

### Good Use Cases

1. **Multi-Column Filters (AND queries)**
   ```sql
   -- Query: Find all open tasks with high priority
   SELECT * FROM tasks WHERE status = 'open' AND priority = 'high'
   ```
   - Best with composite index on `(status, priority)`

2. **Hierarchical Data**
   ```sql
   -- Query: Sales by region and quarter
   SELECT * FROM sales WHERE region = 'EU' AND quarter = 'Q1'
   ```
   - Best with composite index on `(region, quarter)`

3. **Multi-Level Grouping**
   ```sql
   -- Query: Employees by department and role
   SELECT * FROM employees WHERE department = 'Engineering' AND role = 'Senior'
   ```
   - Best with composite index on `(department, role)`

### When NOT to Use

1. **OR Queries** - Composite indexes don't help with OR conditions
2. **Single Column Queries** - Use single-column index instead
3. **High Cardinality First Column** - Can lead to sparse indexes

## Design Principles

### 1. Column Order Matters

**Rule:** Place the most selective column first.

**Example:**
```cpp
// Good: user_id is unique, status has few values
std::vector<std::string> cols = {"user_id", "status"};
idx_mgr->createCompositeIndex("sessions", cols);

// Less optimal: status first means many duplicate entries
std::vector<std::string> cols = {"status", "user_id"};
```

**Why:** The first column acts as the primary sort key. High selectivity first = fewer index entries to scan.

### 2. Selectivity Considerations

**Selectivity = Number of Distinct Values / Total Rows**

**High Selectivity (Good for first column):**
- User IDs: Nearly 1:1
- Email addresses: Nearly 1:1
- Order IDs: Nearly 1:1

**Low Selectivity (Good for second column):**
- Status fields: 3-10 values
- Boolean flags: 2 values
- Categories: 10-50 values

### 3. Query Pattern Alignment

Design indexes to match your actual queries:

```cpp
// If you query: (country='USA' AND state='CA')
std::vector<std::string> cols = {"country", "state"};
idx_mgr->createCompositeIndex("addresses", cols);

// Query pattern matches index exactly
std::vector<std::string> values = {"USA", "CA"};
auto [status, keys] = idx_mgr->scanKeysEqualComposite("addresses", cols, values);
```

### 4. Number of Columns

**Recommended:** 2-3 columns for most use cases

**Maximum:** 4 columns (performance degrades beyond this)

**Rationale:**
- Each additional column increases index size
- More columns = longer keys = more I/O
- 2-3 columns balance query efficiency and storage

## Implementation Examples

### Example 1: E-commerce Product Search

```cpp
#include "index/secondary_index.h"

// Products can be filtered by category and price range
std::vector<std::string> cols = {"category", "price_tier"};
auto st = idx_mgr->createCompositeIndex("products", cols);

// Insert products
BaseEntity e1("prod1");
e1.setField("name", "Laptop");
e1.setField("category", "electronics");
e1.setField("price_tier", "high");
idx_mgr->put("products", e1);

// Query: Find high-end electronics
std::vector<std::string> values = {"electronics", "high"};
auto [status, keys] = idx_mgr->scanKeysEqualComposite("products", cols, values);
```

### Example 2: Time-Series Data

```cpp
// Logs with timestamp and severity
std::vector<std::string> cols = {"date", "severity"};
idx_mgr->createCompositeIndex("logs", cols);

// Query: All ERROR logs on 2024-01-15
std::vector<std::string> values = {"2024-01-15", "ERROR"};
auto [status, keys] = idx_mgr->scanKeysEqualComposite("logs", cols, values);
```

### Example 3: Multi-Tenant Applications

```cpp
// tenant_id should be first (high selectivity)
std::vector<std::string> cols = {"tenant_id", "status"};
idx_mgr->createCompositeIndex("resources", cols);

// Query: All active resources for tenant
std::vector<std::string> values = {"tenant_123", "active"};
auto [status, keys] = idx_mgr->scanKeysEqualComposite("resources", cols, values);
```

## Performance Optimization

### 1. Index Maintenance

**Bulk Inserts:**
```cpp
// Create index after bulk insert for better performance
// 1. Insert all entities
for (const auto& entity : entities) {
    idx_mgr->put(table, entity);  // Without index
}

// 2. Create index
idx_mgr->createCompositeIndex(table, cols);
```

**Updates:**
```cpp
// Composite indexes are automatically maintained on updates
BaseEntity e("entity1");
e.setField("col1", "old_value");
e.setField("col2", "value2");
idx_mgr->put(table, e);  // Old index entry created

// Update changes index automatically
e.setField("col1", "new_value");
idx_mgr->put(table, e);  // Old entry removed, new entry created
```

### 2. Memory Considerations

**Index Size Estimation:**
```
Index Size ≈ (Key Length + PK Length) × Number of Rows
Key Length = Table Name + Column Names + Value Lengths + Delimiters
```

**Example:**
```
Table: "products" (8 bytes)
Columns: "category" + "price_tier" (8 + 10 = 18 bytes)
Values: "electronics" + "high" (11 + 4 = 15 bytes)
PK: "prod123" (7 bytes)
Total per entry: ~50 bytes

For 1M products: ~50 MB index size
```

### 3. Query Performance

**Fast Queries (O(log n)):**
- Exact match on all columns
- Equality filter on composite key

**Slower Queries (O(n)):**
- Filtering only on second column
- Range queries on composite keys
- Queries not matching index order

## Edge Cases & Limitations

### 1. NULL/Empty Values

```cpp
// Empty strings are valid in composite indexes
BaseEntity e("entity1");
e.setField("col1", "");      // Empty first column
e.setField("col2", "value2");
idx_mgr->put("test", e);

// Query with empty string
std::vector<std::string> values = {"", "value2"};
auto [status, keys] = idx_mgr->scanKeysEqualComposite("test", cols, values);
// Works correctly
```

### 2. Special Characters

```cpp
// Special characters are supported
BaseEntity e("entity1");
e.setField("email", "user@example.com");
e.setField("domain", "example.com");
idx_mgr->put("users", e);

// Query with special characters
std::vector<std::string> values = {"user@example.com", "example.com"};
auto [status, keys] = idx_mgr->scanKeysEqualComposite("users", cols, values);
```

### 3. Very Long Keys

```cpp
// Composite keys > 1KB work but impact performance
std::string long_value(500, 'A');  // 500 characters

BaseEntity e("entity1");
e.setField("long_field1", long_value);
e.setField("long_field2", long_value);
idx_mgr->put("test", e);

// Works but slower due to key size
```

**Recommendation:** Keep composite key values under 256 bytes total for optimal performance.

## Testing Composite Indexes

Comprehensive tests are available in `tests/test_composite_index.cpp` (27 tests):

- Basic operations (6 tests)
- Multi-column sorting (4 tests)
- Index filtering (4 tests)
- Edge cases (4 tests)
- Performance tests (3 tests)

## Common Patterns

### Pattern 1: Hierarchical Filtering
```cpp
// Geography: country → state → city
std::vector<std::string> cols = {"country", "state"};
idx_mgr->createCompositeIndex("locations", cols);
```

### Pattern 2: Time + Category
```cpp
// Events by date and type
std::vector<std::string> cols = {"date", "event_type"};
idx_mgr->createCompositeIndex("events", cols);
```

### Pattern 3: User + Status
```cpp
// User-specific active items
std::vector<std::string> cols = {"user_id", "status"};
idx_mgr->createCompositeIndex("items", cols);
```

### Pattern 4: Multi-Level Priority
```cpp
// Support tickets
std::vector<std::string> cols = {"priority", "status", "assigned_to"};
idx_mgr->createCompositeIndex("tickets", cols);
```

## Monitoring & Maintenance

### Check Index Existence
```cpp
bool exists = idx_mgr->hasCompositeIndex(table, cols);
if (!exists) {
    // Create if needed
    idx_mgr->createCompositeIndex(table, cols);
}
```

### Estimate Index Size
```cpp
std::vector<std::string> values = {"value1", "value2"};
bool capped = false;
size_t count = idx_mgr->estimateCountEqualComposite(
    table, cols, values, 10000, &capped
);
```

### Drop Unused Indexes
```cpp
// Remove indexes that are no longer needed
auto st = idx_mgr->dropCompositeIndex(table, cols);
```

## Further Reading

- [Secondary Index Implementation](../include/index/secondary_index.h)
- [Test Examples](../tests/test_composite_index.cpp)
- [RocksDB Key Design](https://github.com/facebook/rocksdb/wiki/RocksDB-Basics)
