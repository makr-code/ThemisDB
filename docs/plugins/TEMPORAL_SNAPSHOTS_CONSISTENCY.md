# Temporal Snapshots Consistency

## Overview

ThemisDB utilizes **Hybrid Logical Clocks (HLC)** for temporal snapshot versioning in the RPC Snapshot Transfer Framework. This integration provides deterministic conflict resolution during concurrent data modifications across distributed nodes.

## Hybrid Logical Clock (HLC) Integration

### HLC Structure

```
HLC = (physical_time, logical_counter, node_id)
```

**Components:**
- `physical_time`: Milliseconds since epoch (NTP-synchronized)
- `logical_counter`: Logical counter for concurrent events at the same physical time
- `node_id`: Unique node identifier (used as tiebreaker)

### Timestamp Comparison

HLC timestamps are ordered as follows:
1. Primary: Compare `physical_time` (higher is newer)
2. Secondary: Compare `logical_counter` if physical times are equal
3. Tertiary: Compare `node_id` lexicographically if both physical and logical are equal

### Example

```cpp
HLC_A = {physical: 1000, logical: 5, node_id: "node_a"}
HLC_B = {physical: 1000, logical: 10, node_id: "node_b"}

// HLC_B > HLC_A (same physical time, but higher logical counter)
```

## Conflict Resolution Policies

The Temporal Conflict Resolver supports multiple resolution strategies:

| **Policy** | **Description** | **Use Case** | **Winner Selection** |
|------------|------------------|--------------|---------------------|
| **LAST_WRITE_WINS** | Newest HLC wins | Standard distributed systems | Highest HLC timestamp |
| **FIRST_WRITE_WINS** | Oldest HLC wins | Immutable event logs, audit trails | Lowest HLC timestamp |
| **NODE_PRIORITY** | Node ID as priority | Master-Slave setups, primary nodes | Lexicographically lower node_id |
| **CRDT_MERGE** | Automatic merge | Collaborative editing, counters | Merged result (WIP) |
| **MANUAL** | Queue for manual resolution | Critical business data, compliance | User decision |

### Policy Selection Guide

**Use LAST_WRITE_WINS when:**
- Standard eventual consistency is acceptable
- Most recent update should always prevail
- Examples: User profile updates, configuration changes

**Use FIRST_WRITE_WINS when:**
- Historical immutability is required
- First event in sequence is authoritative
- Examples: Transaction logs, audit trails, event sourcing

**Use NODE_PRIORITY when:**
- Certain nodes have authority over others
- Primary/replica architecture
- Examples: Master-slave replication, hierarchical systems

**Use MANUAL when:**
- Human judgment is required
- Business logic cannot be automated
- Examples: Financial transactions, legal documents

**Use CRDT_MERGE when:**
- Automatic merge is possible
- No data loss acceptable
- Examples: Collaborative documents, shopping carts

## Implementation Details

### Basic Usage

```cpp
#include "temporal/temporal_conflict_resolver.h"

using namespace themisdb::temporal;

// Create resolver with default policy
TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);

// Create snapshots
TemporalSnapshot local;
local.hlc = {1000, 5, "node_a"};
local.data = {{"field", "local_value"}};

TemporalSnapshot remote;
remote.hlc = {1001, 3, "node_b"};
remote.data = {{"field", "remote_value"}};

// Resolve conflict
auto winner = resolver.resolve(local, remote);
// winner will be remote (newer timestamp)
```

### Integration with RPC Framework

```cpp
class SnapshotTransferHandler {
private:
    std::shared_ptr<replication::HybridLogicalClock> hlc_;
    std::shared_ptr<temporal::TemporalConflictResolver> conflict_resolver_;
    
public:
    void handleSnapshotTransfer(const SnapshotTransferRequest& request) {
        // 1. Update local HLC based on received timestamp
        hlc_->receive(request.source_hlc());
        
        // 2. Check for local modifications
        auto local_snapshot = getLocalSnapshot(request.entity_id());
        
        if (!local_snapshot) {
            // No local version → Apply remote
            applySnapshot(request);
            return;
        }
        
        // 3. Detect conflict based on HLC comparison
        if (request.source_hlc() > local_snapshot->hlc) {
            applySnapshot(request);
        } else if (request.source_hlc() < local_snapshot->hlc) {
            logConflict("local_newer", local_snapshot->hlc, request.source_hlc());
        } else {
            // Equal timestamps → CONFLICT!
            temporal::TemporalSnapshot local_ts = convertToTemporalSnapshot(*local_snapshot);
            temporal::TemporalSnapshot remote_ts = convertFromRequest(request);
            
            auto winner = conflict_resolver_->resolve(local_ts, remote_ts);
            
            if (winner.hlc == remote_ts.hlc) {
                applySnapshot(request);
            }
        }
    }
};
```

### Manual Conflict Resolution

```cpp
// Create resolver with MANUAL policy
TemporalConflictResolver resolver(ConflictPolicy::MANUAL);

// Resolve creates an unresolved conflict entry
auto result = resolver.resolve(local, remote);

// Get unresolved conflicts
auto conflicts = resolver.getUnresolvedConflicts();

for (const auto& conflict : conflicts) {
    std::cout << "Conflict ID: " << conflict.conflict_id << "\n";
    std::cout << "Local: " << conflict.local_version.toJson() << "\n";
    std::cout << "Remote: " << conflict.remote_version.toJson() << "\n";
}

// Manually resolve
resolver.resolveManually(conflicts[0].conflict_id, "remote");
```

### Statistics and Monitoring

```cpp
auto stats = resolver.getStatistics();

std::cout << "Total conflicts: " << stats["total_conflicts"] << "\n";
std::cout << "LWW resolutions: " << stats["lww_resolutions"] << "\n";
std::cout << "FWW resolutions: " << stats["fww_resolutions"] << "\n";
std::cout << "Manual resolutions: " << stats["manual_resolutions"] << "\n";
std::cout << "CRDT merges: " << stats["crdt_merges"] << "\n";
std::cout << "Unresolved: " << stats["unresolved_conflicts"] << "\n";
```

## Risk Matrix - Conflict Probability

| **Conflict Type** | **Probability** | **Impact** | **Mitigation** | **Status** |
|-------------------|-----------------|------------|----------------|------------|
| **Multi-Master Concurrent Writes** | 10-40% | 🔴 Data loss | HLC + CRDT | ✅ Resolved |
| **RPC Snapshot Transfer** | 10-50% | 🔴 Inconsistency | HLC Integration | ✅ **IMPLEMENTED** |
| **Temporal Snapshot Merge** | 5-20% | 🟡 Stale reads | Conflict Resolver | ✅ **IMPLEMENTED** |
| **Cross-Shard 2PC** | 1-20% | 🟡 Abort rate | TrueTime | ✅ Resolved |

### Conflict Probability by Scenario

**Snapshot Transfer During Active Writes:** 1-5%
- Low probability in typical workloads
- Increases with high write frequency

**Binary Adjacent Keys (RocksDB LSM-Tree):** 10-20%
- LSM-tree compaction can cause adjacent key conflicts
- Mitigation: HLC versioning at key level

**Differential Updates (High Frequency):** 30-50%
- Common in high-throughput scenarios
- Critical to have proper conflict resolution

## Configuration

### YAML Configuration Example

```yaml
temporal:
  conflict_resolution:
    # Default policy for all snapshots
    default_policy: LAST_WRITE_WINS
    
    # Enable conflict logging for audit
    enable_conflict_logging: true
    
    # Maximum unresolved conflicts before alerting
    manual_queue_size: 1000
    
    # Per-collection policy overrides
    collection_policies:
      financial_transactions: MANUAL
      audit_logs: FIRST_WRITE_WINS
      user_profiles: LAST_WRITE_WINS
      collaborative_docs: CRDT_MERGE
```

### C++ Configuration Example

```cpp
MMReplicationConfig config;
config.default_resolution_strategy = "LAST_WRITE_WINS";
config.collection_strategies["critical_data"] = "MANUAL";

auto resolver = std::make_shared<TemporalConflictResolver>(
    ConflictPolicy::LAST_WRITE_WINS
);
```

## Performance Considerations

### Time Complexity

- **Conflict Detection**: O(1) - Simple HLC comparison
- **Last-Write-Wins Resolution**: O(1) - Direct comparison
- **First-Write-Wins Resolution**: O(1) - Direct comparison
- **Node Priority Resolution**: O(1) - Direct comparison
- **Manual Resolution**: O(1) - Queue insertion

### Memory Usage

- **Per-Conflict Record**: ~500 bytes (including JSON data)
- **Conflict History**: Configurable retention
- **Unresolved Queue**: Bounded by configuration

### Recommendations

1. **Use LAST_WRITE_WINS for most scenarios** - Lowest overhead
2. **Enable conflict logging selectively** - Only for critical collections
3. **Monitor unresolved queue size** - Alert when approaching limit
4. **Periodically archive conflict history** - Prevent unbounded growth

## References

- **Paper**: "Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases" (Kulkarni et al., 2014)
- **Implementation**: `include/replication/multi_master_replication.h` (HybridLogicalClock)
- **Conflict Resolution**: `include/temporal/temporal_conflict_resolver.h`
- **RPC Framework**: `docs/plugins/RPC_FRAMEWORK_REVIEW.md` (PR #104)
- **TrueTime Integration**: `docs/architecture/TRUETIME_INTEGRATION.md`

## Future Enhancements

1. **CRDT Merge Implementation**: Full CRDT support for automatic merging
2. **Pluggable Conflict Resolvers**: Custom application-specific logic
3. **Conflict Prediction**: ML-based conflict probability estimation
4. **Automated Conflict Prevention**: Optimistic locking with HLC
5. **Distributed Conflict Resolution**: Consensus-based multi-node resolution

## Testing

Unit tests are available in `tests/temporal/test_temporal_conflict_resolver.cpp`:

```bash
# Run temporal conflict resolver tests
ctest -R TemporalConflictResolver -V

# Or with GoogleTest directly
./themis_tests --gtest_filter=TemporalConflictResolverTest.*
```

## Troubleshooting

### Common Issues

**Issue**: Conflicts not being detected
- **Cause**: HLC not properly synchronized across nodes
- **Solution**: Ensure NTP is configured and HLC receive() is called on remote messages

**Issue**: Wrong conflict winner selected
- **Cause**: Incorrect policy for use case
- **Solution**: Review conflict policy selection guide above

**Issue**: Unresolved queue growing unbounded
- **Cause**: Manual conflicts not being resolved
- **Solution**: Implement automated resolution workflow or increase queue size

**Issue**: Performance degradation
- **Cause**: Excessive conflict logging
- **Solution**: Disable logging for non-critical collections

## License

Copyright (c) 2025 VCC-URN Project
SPDX-License-Identifier: Apache-2.0
