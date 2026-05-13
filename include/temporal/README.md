> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Temporal Module - Public API

Public interface definitions for ThemisDB temporal functionality.

## Module Navigation

- Internal implementation overview: [`../../src/temporal/README.md`](../../src/temporal/README.md)
- Module roadmap: [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md)
- Future enhancements: [`../../src/temporal/FUTURE_ENHANCEMENTS.md`](../../src/temporal/FUTURE_ENHANCEMENTS.md)
- German overview: [`../../docs/de/temporal/README.md`](../../docs/de/temporal/README.md)

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

### bi_temporal.h
**Purpose:** Bi-temporal table support (system-time + application-time)

---

### bitemporal_join.h
**Purpose:** Temporal join operations across bi-temporal tables

---

### interval_tree_index.h
**Purpose:** Interval tree index for efficient temporal range queries

---

### retention_manager.h
**Purpose:** Retention policy enforcement and background cleanup scheduling

---

### snapshot_manager.h
**Purpose:** Multi-table snapshot isolation and consistent read views

---

### system_versioned_table.h
**Purpose:** System-versioned tables with automatic history tracking

---

### temporal_aggregator.h
**Purpose:** Windowed temporal aggregations (tumbling, sliding, session)

---

### temporal_cdc.h
**Purpose:** Change data capture for temporal tables with replay support

---

### temporal_cold_store.h
**Purpose:** Cold-tier offloading of historical temporal data

---

### temporal_compressor.h
**Purpose:** Compression of historical temporal versions

---

### temporal_index.h
**Purpose:** General-purpose temporal index over (entity, time-interval) pairs

---

### temporal_migrator.h
**Purpose:** Schema and data migration for temporal tables

---

### temporal_query_engine.h
**Purpose:** AS-OF, FROM-TO, and temporal join query execution

---

### temporal_tier_manager.h
**Purpose:** Hot/warm/cold tiering lifecycle for temporal data

---

### temporal_types.h
**Purpose:** Core temporal type definitions (HLC, time intervals, version records)

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

## Runtime Behavior and Operational Limits

- **History retention is explicit:** historical versions remain available until retention policies remove/archive them.
- **DDL parser gap:** SQL syntax (`PERIOD FOR`, `FOR SYSTEM_TIME`) is not yet generally available in AQL parser; use C++ APIs from this directory.
- **Consistency expectation:** APIs assume monotonically progressing temporal metadata (HLC/system timestamps) and may reject malformed time ranges.
- **Capacity planning:** without retention/compression, versioned history can grow unbounded; configure retention + compression for long-running deployments.

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
- **CRDT_MERGE**: LWW-Register-per-field merge – fields from both snapshots are unioned, with the newer snapshot's value winning on conflicts.

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

## Implemented APIs (v1.1.0+)

All classes listed here are fully implemented and tested.

### System-Versioned Tables
```cpp
#include "temporal/system_versioned_table.h"

SystemVersionedTable employees("employees");

employees.insert("emp1", {{"name", "Alice"}, {"dept", "Eng"}});
employees.update("emp1", {{"dept", "Arch"}});         // closes old version, opens new
auto current  = employees.getCurrent("emp1");         // current version
auto asOf     = employees.getAsOf("emp1", t_past);    // version at a past time
auto history  = employees.getHistory("emp1");         // all versions
auto snapshot = employees.scan(t_past);               // all rows as of t_past

// Physically remove historical data
employees.purgeHistoricalVersions("emp1", [&](const auto& v) {
    return v.sys_time.end < cutoff;
});
```

### Time-Travel Queries
```cpp
#include "temporal/temporal_query_engine.h"

// AS-OF query across all keys
auto rows = TemporalQueryEngine::queryAsOf(table, timestamp);

// Range query [from, to)
auto range_rows = TemporalQueryEngine::queryFromTo(table, from, to);

// Temporal join: employees as of 2024 ⋈ departments as of 2024
auto joined = TemporalQueryEngine::joinAsOf(
    employees, departments, timestamp,
    [](const auto& emp, const auto& dept) {
        return emp.data["dept_id"] == dept.data["id"];
    });
```

### Bi-Temporal Tables
```cpp
#include "temporal/bi_temporal.h"

BiTemporalTable contracts("contracts");
contracts.insertWithValidTime("c1", {{"amount", 1000}}, {valid_from, valid_to});
auto rows = contracts.queryBiTemporal("c1", sys_as_of, valid_at);
auto overlaps = contracts.findOverlaps("c1");
```

### Temporal Index
```cpp
#include "temporal/temporal_index.h"

TemporalIndex idx("employees_sys_time");
idx.insert({"emp1", {t_start, t_end}, payload});
auto at_t = idx.queryPoint(t);
auto range = idx.queryRange(t_from, t_to);
```

### Retention Policies
```cpp
#include "temporal/retention_manager.h"

RetentionManager mgr;
RetentionPolicy p;
p.type                  = RetentionType::TIME_BASED;
p.retention_period      = std::chrono::hours(24 * 365);
p.archive_before_delete = true;
mgr.setPolicy("employees", p);
auto stats = mgr.enforceRetention(employees);

// Background scheduler – runs every hour
mgr.scheduleTable(employees, std::chrono::hours(1));
mgr.startScheduler();   // non-blocking
// ...
mgr.stopScheduler();
```

### Temporal Aggregation
```cpp
#include "temporal/temporal_aggregator.h"

TemporalAggregator agg;
AggregationSpec spec;
spec.window_type    = WindowType::TUMBLING;
spec.window_size_ms = 3600'000;          // 1-hour buckets
spec.func           = AggregateFunc::SUM;
spec.measure_field  = "revenue";
auto results = agg.aggregate(sales, spec, from, to);
```

### Snapshot Isolation
```cpp
#include "temporal/snapshot_manager.h"

TemporalSnapshotManager smgr;
auto handle = smgr.createSnapshot({{"employees", &employees}, {"depts", &depts}});
auto rows   = smgr.querySnapshot(handle, "employees");
smgr.releaseSnapshot(handle);
```

### Conflict Resolution & Audit Trail
```cpp
#include "temporal/temporal_conflict_resolver.h"

TemporalConflictResolver resolver(ConflictPolicy::CRDT_MERGE);
auto winner = resolver.resolve(local, remote);
auto log    = resolver.exportAuditLog();   // JSON array with full history
auto hist   = resolver.getConflictHistory();
```

---

## Build Integration

### CMake
```cmake
# Link temporal module
target_link_libraries(your_target PRIVATE themisdb_temporal)

# Headers are automatically included via public interface
```

## Include Paths
```cpp
#include "temporal/temporal_conflict_resolver.h"
#include "temporal/system_versioned_table.h"
#include "temporal/temporal_query_engine.h"
#include "temporal/retention_manager.h"
#include "temporal/bi_temporal.h"
#include "temporal/bitemporal_join.h"
#include "temporal/interval_tree_index.h"
#include "temporal/snapshot_manager.h"
#include "temporal/temporal_aggregator.h"
#include "temporal/temporal_cdc.h"
#include "temporal/temporal_cold_store.h"
#include "temporal/temporal_compressor.h"
#include "temporal/temporal_index.h"
#include "temporal/temporal_migrator.h"
#include "temporal/temporal_tier_manager.h"
#include "temporal/temporal_types.h"
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

### From v1.0 to v1.1+

All temporal classes that were listed as "planned" in v1.0 are now available:

```cpp
// v1.1+: include the new headers
#include "temporal/temporal_conflict_resolver.h"  // unchanged
#include "temporal/system_versioned_table.h"       // NEW
#include "temporal/temporal_query_engine.h"        // NEW
#include "temporal/temporal_index.h"               // NEW
#include "temporal/retention_manager.h"            // NEW
#include "temporal/bi_temporal.h"                  // NEW
#include "temporal/snapshot_manager.h"             // NEW
#include "temporal/temporal_aggregator.h"          // NEW
```

Existing conflict-resolution code is fully compatible; new features are opt-in.

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
- [Future Enhancements](../../src/temporal/FUTURE_ENHANCEMENTS.md) - Planned features
- [ROADMAP](../../src/temporal/ROADMAP.md) - Delivery status and next milestones
- [Replication Module](../replication/README.md) - HLC and distributed coordination
- [Architecture Guide](../../ARCHITECTURE.md) - System architecture

---

*Last Updated: April 2026*
*API Version: v1.1.0*
*ABI Version: 1.1*

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
