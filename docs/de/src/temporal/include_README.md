# Temporal Module - Public API

Public interface definitions for ThemisDB temporal functionality.

## Headers

### temporal_conflict_resolver.h
**Purpose:** Resolve conflicts between temporal snapshots using HLC timestamps

**Key Classes:**
- `TemporalSnapshot`: Snapshot with HLC versioning and checksums
- `ConflictRecord`: Conflict logging and monitoring
- `TemporalConflictResolver`: Main conflict resolution engine

**Key Enums:**
- `ConflictPolicy`: Resolution strategies (LAST_WRITE_WINS, FIRST_WRITE_WINS, NODE_PRIORITY, MANUAL, CRDT_MERGE)

**Usage:**
```cpp
#include "temporal/temporal_conflict_resolver.h"

using namespace themisdb::temporal;

// Create resolver with default policy
TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);

// Resolve conflict between snapshots
TemporalSnapshot local = /* ... */;
TemporalSnapshot remote = /* ... */;
auto winner = resolver.resolve(local, remote);

// Or specify policy per conflict
auto merged = resolver.resolve(local, remote, ConflictPolicy::CRDT_MERGE);

// Check conflict statistics
auto stats = resolver.getStats();
std::cout << "Total conflicts: " << stats.total_conflicts << std::endl;
```

**Thread Safety:** Thread-safe for concurrent conflict resolution

---

## API Conventions

### Namespace Structure
```cpp
namespace themisdb {
namespace temporal {
    // All temporal classes and functions
}}
```

### Return Types
- `Result<T>`: For operations that can fail (not yet implemented in v1.0)
- `std::optional<T>`: For operations that may not have a value
- Direct return: For operations that always succeed

### Error Handling
```cpp
// Current approach (v1.0.0)
auto snapshot = TemporalSnapshot::fromJson(json_data);
if (!snapshot.has_value()) {
    // Handle parse error
}

// Future approach (v1.1.0+)
auto result = resolver.resolveWithResult(local, remote);
if (!result.is_ok()) {
    std::cerr << "Error: " << result.error_message() << std::endl;
}
```

---

## Core Types

### TemporalSnapshot
Represents a versioned snapshot of data with timestamp ordering.

**Fields:**
- `snapshot_id`: Unique identifier for the snapshot
- `hlc`: Hybrid Logical Clock timestamp for ordering
- `source_node_id`: Originating node identifier
- `data`: JSON document data
- `checksum`: SHA-256 integrity checksum

**Methods:**
- `toJson()`: Serialize to JSON
- `fromJson(json)`: Deserialize from JSON
- `operator<(other)`: Compare HLC timestamps for ordering

**Serialization:**
```cpp
TemporalSnapshot snapshot;
snapshot.snapshot_id = "snap_123";
snapshot.data = {{"key", "value"}};

// Serialize
nlohmann::json j = snapshot.toJson();

// Deserialize
auto restored = TemporalSnapshot::fromJson(j);
```

---

### ConflictRecord
Records conflict resolution for auditing and monitoring.

**Fields:**
- `conflict_id`: Unique conflict identifier
- `entity_id`: Entity that had conflicting versions
- `local_version`: Local snapshot
- `remote_version`: Remote snapshot
- `resolution_policy`: Policy used for resolution
- `winner`: Resolution outcome ("local", "remote", "merged")
- `detected_at`: When conflict was detected
- `resolved`: Whether conflict is resolved

**Usage:**
```cpp
// Get unresolved conflicts
auto conflicts = resolver.getUnresolvedConflicts();
for (const auto& conflict : conflicts) {
    std::cout << "Conflict on entity: " << conflict.entity_id 
              << " detected at: " << format_time(conflict.detected_at)
              << std::endl;
}
```

---

### ConflictPolicy
Enumeration of resolution strategies.

**Values:**
- `LAST_WRITE_WINS`: Choose snapshot with highest HLC (default)
- `FIRST_WRITE_WINS`: Choose snapshot with lowest HLC
- `NODE_PRIORITY`: Use configured node priority for tiebreaking
- `MANUAL`: Queue for manual resolution
- `CRDT_MERGE`: Automatic merge using CRDT semantics (future)

**Selection Guide:**
- **LAST_WRITE_WINS**: Best for most use cases, simple and consistent
- **FIRST_WRITE_WINS**: Useful for audit scenarios where first value matters
- **NODE_PRIORITY**: Useful in multi-datacenter setups with primary regions
- **MANUAL**: For critical data requiring human review
- **CRDT_MERGE**: Future - for automatic convergent merging

---

## Integration Points

### With Replication Module
```cpp
#include "temporal/temporal_conflict_resolver.h"
#include "replication/multi_master_replication.h"

// HLC is shared between modules
using HLC = replication::HybridLogicalClock;

// Create snapshot with HLC from replication
TemporalSnapshot snapshot;
snapshot.hlc = hlc_clock.now();
```

### With Storage Module
```cpp
// Store temporal snapshots in RocksDB
rocksdb_wrapper->put(
    "temporal_snapshot:" + snapshot.snapshot_id,
    snapshot.toJson().dump()
);

// Retrieve and resolve
auto local_json = rocksdb_wrapper->get("temporal_snapshot:local");
auto remote_json = rocksdb_wrapper->get("temporal_snapshot:remote");

auto local = TemporalSnapshot::fromJson(nlohmann::json::parse(local_json));
auto remote = TemporalSnapshot::fromJson(nlohmann::json::parse(remote_json));

auto winner = resolver.resolve(*local, *remote);
```

---

## Planned APIs (Future Versions)

### v1.1.0: System-Versioned Tables
```cpp
// Not yet available - planned for v1.1.0
namespace themisdb {
namespace temporal {

class SystemVersionedTable {
public:
    SystemVersionedTable(const std::string& table_name);
    
    Result<bool> insert(const Document& doc);
    Result<bool> update(const std::string& key, const Document& updates);
    Result<bool> deleteRow(const std::string& key);
    
    Result<std::vector<Document>> getHistory(
        const std::string& key,
        const TimeRange& range
    );
};

}}
```

### v1.2.0: Time-Travel Queries
```cpp
// Not yet available - planned for v1.2.0
namespace themisdb {
namespace temporal {

class TemporalQueryEngine {
public:
    Result<std::vector<Document>> queryAsOf(
        const std::string& table_name,
        const std::string& query,
        const Timestamp& as_of_time
    );
    
    Result<std::vector<Document>> queryFromTo(
        const std::string& table_name,
        const std::string& query,
        const TimeRange& range
    );
};

}}
```

---

## Build Integration

### CMake
```cmake
# Link temporal module
target_link_libraries(your_target PRIVATE themisdb_temporal)

# Headers are automatically included via public interface
```

### Include Paths
```cpp
#include "temporal/temporal_conflict_resolver.h"
// Future headers:
// #include "temporal/system_versioned_table.h"
// #include "temporal/temporal_query_engine.h"
// #include "temporal/retention_manager.h"
```

---

## Dependencies

### Required
- `nlohmann/json`: JSON serialization
- `replication/multi_master_replication.h`: HLC support

### Optional
- `spdlog`: Logging (recommended)
- `openssl`: Checksum generation

---

## Compatibility

### C++ Standard
- **Minimum:** C++17
- **Recommended:** C++20 for better concepts support

### Platform Support
- Linux (primary)
- macOS (supported)
- Windows (experimental)

### ABI Stability
- **v1.x**: Stable ABI within minor versions
- **v2.0**: ABI break expected for major refactoring

---

## Examples

### Basic Conflict Resolution
```cpp
#include "temporal/temporal_conflict_resolver.h"
#include <iostream>

int main() {
    using namespace themisdb::temporal;
    
    // Create resolver
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    
    // Create conflicting snapshots
    TemporalSnapshot local;
    local.snapshot_id = "local_123";
    local.hlc = {1000, 0, "node1"};
    local.data = {{"value", 100}};
    
    TemporalSnapshot remote;
    remote.snapshot_id = "remote_456";
    remote.hlc = {1001, 0, "node2"};  // Newer
    remote.data = {{"value", 200}};
    
    // Resolve
    auto winner = resolver.resolve(local, remote);
    
    std::cout << "Winner: " << winner.snapshot_id << std::endl;
    std::cout << "Value: " << winner.data["value"] << std::endl;
    // Output: Winner: remote_456, Value: 200
    
    return 0;
}
```

### Manual Conflict Resolution
```cpp
#include "temporal/temporal_conflict_resolver.h"

void handleConflicts() {
    using namespace themisdb::temporal;
    
    TemporalConflictResolver resolver(ConflictPolicy::MANUAL);
    
    // Resolve - will queue for manual resolution
    auto result = resolver.resolve(local, remote);
    
    // Later, get queued conflicts
    auto conflicts = resolver.getUnresolvedConflicts();
    
    for (auto& conflict : conflicts) {
        // Manual decision logic
        if (shouldPickLocal(conflict)) {
            resolver.markResolved(conflict.conflict_id, "local");
        } else {
            resolver.markResolved(conflict.conflict_id, "remote");
        }
    }
}
```

---

## Testing

### Unit Tests
```cpp
#include <gtest/gtest.h>
#include "temporal/temporal_conflict_resolver.h"

TEST(TemporalTest, LastWriteWins) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    
    TemporalSnapshot older{/* ... */};
    TemporalSnapshot newer{/* ... */};
    
    auto winner = resolver.resolve(older, newer);
    EXPECT_EQ(winner.snapshot_id, newer.snapshot_id);
}
```

### Integration Tests
See `tests/temporal/` for comprehensive test suite.

---

## Performance Considerations

### Conflict Resolution
- **Latency:** <1ms for simple conflicts
- **Throughput:** 100K+ resolutions/second on modern hardware
- **Memory:** O(1) per conflict resolution

### Snapshot Storage
- **JSON serialization:** ~1-5μs per snapshot
- **Checksum generation:** ~10-50μs per snapshot
- **Storage overhead:** ~100-500 bytes per snapshot

---

## Migration Guide

### From v1.0 to v1.1 (Future)
When v1.1 is released with system-versioned tables:

1. Update includes:
```cpp
// Old (v1.0)
#include "temporal/temporal_conflict_resolver.h"

// New (v1.1)
#include "temporal/temporal_conflict_resolver.h"
#include "temporal/system_versioned_table.h"  // New
```

2. Existing conflict resolution code remains compatible
3. New features are opt-in

---

## Troubleshooting

### Common Issues

**Issue:** Conflict resolution always picks same node  
**Solution:** Check HLC clock synchronization across nodes

**Issue:** High memory usage from conflict records  
**Solution:** Call `resolver.clearResolvedConflicts()` periodically

**Issue:** Checksum mismatches  
**Solution:** Ensure consistent JSON serialization order

---

## See Also

- [Implementation Documentation](../../src/temporal/README.md) - Internal implementation details
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) - Planned features
- [Replication Module](../replication/README.md) - HLC and distributed coordination
- [Architecture Guide](../../ARCHITECTURE.md) - System architecture

---

*Last Updated: February 2026*  
*API Version: v1.0.0*  
*ABI Version: 1.0*
