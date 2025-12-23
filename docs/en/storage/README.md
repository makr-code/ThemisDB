# Replication Module

**Date:** December 22, 2025  
**Version:** v1.3.0  
**Category:** 💾 Storage & Replication

---

## 📑 Table of Contents

- [Overview](#overview)
- [Replication Strategies](#replication-strategies)
- [Source Code Reference](#source-code-reference)
- [Implemented Classes](#implemented-classes)
- [Features](#features)
- [Configuration](#configuration)
- [Related Documentation](#related-documentation)

---

## Overview

The Replication Module provides distributed data consistency for ThemisDB with two main strategies:
1. **Leader-Follower Replication** - WAL-based with automatic failover
2. **Multi-Master Replication** - Writes on any node with CRDT conflict resolution

## Source Code Reference

| Component | Header | Source | LOC |
|-----------|--------|--------|-----|
| ReplicationManager | `replication_manager.h` | `replication_manager.cpp` | ~500 |
| MultiMasterReplication | `multi_master_replication.h` | - | ~900 |

**Total:** 2 Headers, 1 Source File, ~1,600 LOC

## Implemented Classes

### Leader-Follower Replication

```cpp
// replication_manager.h
enum class ReplicationRole { LEADER, FOLLOWER, CANDIDATE };
enum class ReplicationMode { SYNC, ASYNC, SEMI_SYNC };

class ReplicationManager {
    void start();
    void stop();
    void promoteToLeader();
    void demoteToFollower();
    void appendEntry(const WALEntry& entry);
    ReplicationStatus getStatus();
};

class WALManager {
    uint64_t append(const WALEntry& entry);
    std::vector<WALEntry> readSince(uint64_t lsn);
    void checkpoint();
};

class LeaderElection {
    void startElection();
    void vote(const std::string& candidate_id);
    std::string getLeader();
};
```

### Multi-Master Replication

```cpp
// multi_master_replication.h
enum class MMNodeState { ACTIVE, SYNCING, PARTITIONED, RECOVERING, OFFLINE };
enum class ConflictType { CONCURRENT_UPDATE, DELETE_UPDATE, SCHEMA_CONFLICT, CONSTRAINT_VIOLATION };

class VectorClock {
    void increment(const std::string& node_id);
    void merge(const VectorClock& other);
    bool happensBefore(const VectorClock& other) const;
    bool isConcurrent(const VectorClock& other) const;
};

class HybridLogicalClock {
    Timestamp now();
    Timestamp receive(const Timestamp& received);
};

class ConflictResolver {
    virtual MMWriteEntry resolve(const MMWriteEntry& local, const MMWriteEntry& remote) = 0;
};

class LastWriteWinsResolver : public ConflictResolver { ... };
class CRDTMergeResolver : public ConflictResolver { ... };
class CustomResolver : public ConflictResolver { ... };

class MultiMasterReplicationManager {
    void start();
    void stop();
    void write(const MMWriteEntry& entry);
    void syncWithPeer(const std::string& peer_id);
    void resolveConflicts();
};
```

## Features

### Vector Clocks
- Causality tracking between nodes
- `happensBefore()` ordering
- Concurrent write detection

### Hybrid Logical Clocks (HLC)
- Combination of physical time and logical counters
- Paper: "Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases"
- Millisecond resolution with logical counter

### Conflict Resolution

| Strategy | Description | Use Case |
|----------|-------------|----------|
| **Last-Write-Wins** | Latest timestamp wins | Simple data |
| **CRDT Merge** | G-Counter, PN-Counter, LWW-Register, OR-Set | Complex data |
| **Custom** | Application-specific logic | Domain-specific |

### CRDT Types

```cpp
enum class CRDTType {
    G_COUNTER,      // Grow-only Counter
    PN_COUNTER,     // Positive-Negative Counter
    LWW_REGISTER,   // Last-Writer-Wins Register
    MV_REGISTER,    // Multi-Value Register
    G_SET,          // Grow-only Set
    OR_SET,         // Observed-Remove Set
    LWW_MAP         // Last-Writer-Wins Map
};
```

## Configuration

```yaml
replication:
  mode: multi_master  # leader_follower, multi_master
  
  leader_follower:
    sync_mode: semi_sync
    min_replicas: 2
    failover_timeout_ms: 5000
    
  multi_master:
    conflict_resolution: crdt_merge  # lww, crdt_merge, custom
    anti_entropy_interval_ms: 1000
    vector_clock_prune_interval_ms: 60000
```

## Related Documentation

- [Sharding: Redundancy Architecture](../../de/sharding/sharding_redundancy.md) - RAID-like redundancy
- [Sharding: Streaming Protocol](../../de/sharding/sharding_streaming.md) - Streaming architecture
- [Features: Transactions](../features/README.md) - Transaction semantics

---

> **Note:** For detailed storage and replication documentation, please refer to the [German storage documentation](../../de/storage/).

**Version:** 1.3.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
