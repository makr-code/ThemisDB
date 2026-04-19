# RocksDB Merge Operators Guide

## Overview

ThemisDB implements custom RocksDB merge operators to enable atomic operations without read-modify-write cycles. This significantly improves performance for common operations like counters, logs, sets, and tracking maximum values.

## Available Merge Operators

### 1. CounterMergeOperator

**Purpose:** Atomic numeric increments and decrements for counters and statistics.

**Use Cases:**
- Query execution counters
- Page view statistics
- API request counts
- User activity metrics

**Example:**
```cpp
#include "storage/merge_operators.h"

// Create database with CounterMergeOperator
rocksdb::Options options;
options.merge_operator = std::make_shared<CounterMergeOperator>();

rocksdb::DB* db;
rocksdb::DB::Open(options, "./db", &db);

// Increment counter atomically
db->Merge(rocksdb::WriteOptions(), "stats:query:count", "5");
db->Merge(rocksdb::WriteOptions(), "stats:query:count", "3");

// Read result: "8"
std::string value;
db->Get(rocksdb::ReadOptions(), "stats:query:count", &value);
// value == "8"
```

**Features:**
- Supports positive and negative increments
- Thread-safe atomic operations
- No read-before-write overhead
- Survives database restarts

### 2. AppendMergeOperator

**Purpose:** Concatenate values with a configurable delimiter for append-only logs and event streams.

**Use Cases:**
- Application event logs
- Audit trails
- Activity streams
- Time-series data collection

**Example:**
```cpp
#include "storage/merge_operators.h"

// Create with custom delimiter (default is "|")
rocksdb::Options options;
options.merge_operator = std::make_shared<AppendMergeOperator>(",");

rocksdb::DB* db;
rocksdb::DB::Open(options, "./db", &db);

// Append events
db->Merge(rocksdb::WriteOptions(), "user:123:events", "login");
db->Merge(rocksdb::WriteOptions(), "user:123:events", "view_page");
db->Merge(rocksdb::WriteOptions(), "user:123:events", "logout");

// Read result
std::string value;
db->Get(rocksdb::ReadOptions(), "user:123:events", &value);
// value == "login,view_page,logout"
```

**Features:**
- Configurable delimiter
- Preserves event order
- Efficient string concatenation
- Minimal memory allocation

### 3. SetMergeOperator

**Purpose:** Maintain unique value sets with automatic deduplication.

**Use Cases:**
- Tag aggregation
- Unique user lists
- Feature flags
- Category collections

**Example:**
```cpp
#include "storage/merge_operators.h"

rocksdb::Options options;
options.merge_operator = std::make_shared<SetMergeOperator>();

rocksdb::DB* db;
rocksdb::DB::Open(options, "./db", &db);

// Add values (duplicates are automatically removed)
db->Merge(rocksdb::WriteOptions(), "post:456:tags", "cpp");
db->Merge(rocksdb::WriteOptions(), "post:456:tags", "database");
db->Merge(rocksdb::WriteOptions(), "post:456:tags", "cpp");      // duplicate
db->Merge(rocksdb::WriteOptions(), "post:456:tags", "rocksdb");

// Read result - sorted and unique
std::string value;
db->Get(rocksdb::ReadOptions(), "post:456:tags", &value);
// value == "cpp,database,rocksdb"
```

**Features:**
- Automatic deduplication
- Lexicographic sorting
- Batch value support (comma-separated)
- Memory-efficient set operations

### 4. MaxMergeOperator

**Purpose:** Track maximum values efficiently without read-before-write.

**Use Cases:**
- Temperature sensors (max readings)
- High water marks
- Peak load tracking
- Maximum latency monitoring

**Example:**
```cpp
#include "storage/merge_operators.h"

rocksdb::Options options;
options.merge_operator = std::make_shared<MaxMergeOperator>();

rocksdb::DB* db;
rocksdb::DB::Open(options, "./db", &db);

// Track maximum temperature
db->Merge(rocksdb::WriteOptions(), "sensor:1:max_temp", "25.5");
db->Merge(rocksdb::WriteOptions(), "sensor:1:max_temp", "30.2");
db->Merge(rocksdb::WriteOptions(), "sensor:1:max_temp", "27.8");

// Read maximum value
std::string value;
db->Get(rocksdb::ReadOptions(), "sensor:1:max_temp", &value);
// value == "30.2"
```

**Features:**
- Supports floating-point numbers
- Works with negative values
- No read overhead
- Compaction-safe

## Performance Benefits

### Traditional Read-Modify-Write
```cpp
// Requires 1 read + 1 write = 2 operations
std::string value;
db->Get(key, &value);
int counter = std::stoi(value);
counter += 5;
db->Put(key, std::to_string(counter));
```

### With Merge Operators
```cpp
// Single atomic operation
db->Merge(key, "5");
```

**Benefits:**
- 50% reduction in I/O operations
- No race conditions
- Better compaction efficiency
- Lower latency for concurrent updates

## Integration with ThemisDB

### Using in RocksDBWrapper

```cpp
#include "storage/rocksdb_wrapper.h"
#include "storage/merge_operators.h"

RocksDBWrapper::Config config;
config.db_path = "./data";
// Note: Merge operators need to be registered per column family
// See RocksDBWrapper implementation for column family support

RocksDBWrapper db(config);
db.open();
```

### Best Practices

1. **Choose the Right Operator:**
   - Use `CounterMergeOperator` for numeric aggregation
   - Use `AppendMergeOperator` for ordered event streams
   - Use `SetMergeOperator` for unique collections
   - Use `MaxMergeOperator` for tracking peaks

2. **Key Design:**
   - Use descriptive key prefixes: `stats:`, `log:`, `set:`, `max:`
   - Include entity identifiers: `user:123:`, `sensor:1:`
   - Keep keys consistent and hierarchical

3. **Compaction:**
   - Merge operators work seamlessly with RocksDB compaction
   - Values are merged during compaction, reducing storage
   - No special configuration needed

4. **Persistence:**
   - All merge operations are durable (with WAL enabled)
   - Survives database restarts
   - Compatible with backups and snapshots

5. **Limitations:**
   - Merge operator must be consistent across database opens
   - Cannot mix merge operators for the same key
   - Values should be in expected format (numeric for Counter/Max, delimited for Append/Set)

## Testing

Comprehensive tests are available:
- `tests/test_merge_operator_counter.cpp` - 9 tests for CounterMergeOperator
- `tests/test_merge_operator_append.cpp` - 10 tests for AppendMergeOperator
- `tests/test_merge_operator_set.cpp` - 11 tests for SetMergeOperator
- `tests/test_merge_operator_max.cpp` - 12 tests for MaxMergeOperator
- `tests/test_merge_operators_integration.cpp` - 8 integration tests

## API Reference

### CounterMergeOperator
```cpp
class CounterMergeOperator : public rocksdb::AssociativeMergeOperator;
// Merges integer strings by addition
```

### AppendMergeOperator
```cpp
class AppendMergeOperator : public rocksdb::AssociativeMergeOperator;
// Constructor: AppendMergeOperator(std::string delimiter = "|")
```

### SetMergeOperator
```cpp
class SetMergeOperator : public rocksdb::AssociativeMergeOperator;
// Merges comma-separated values into unique sorted set
```

### MaxMergeOperator
```cpp
class MaxMergeOperator : public rocksdb::AssociativeMergeOperator;
// Keeps maximum numeric value
```

## Further Reading

- [RocksDB Merge Operator Documentation](https://github.com/facebook/rocksdb/wiki/Merge-Operator)
- [ThemisDB Storage Architecture](../docs/storage-architecture.md)
- [Performance Benchmarks](../benchmarks/merge-operators.md)
