# ThemisDB Replication Module — Example

This example (`example_replication.cpp`) demonstrates the primary building
blocks of the ThemisDB Replication module, covering both the core APIs and
the advanced features added in v1.6–v1.8.

## What It Demonstrates

| Demo | Class | Feature |
|------|-------|---------|
| 1. WALManager | `WALManager` | Append, read, serialize WAL entries |
| 2. ReplicationManager | `ReplicationManager` | Leader init, follower/witness management, WAL replication, Prometheus metrics |
| 3. VectorClock + HLC | `VectorClock`, `HybridLogicalClock` | Causal ordering, concurrent write detection, monotonic timestamps |
| 4. Active-Active | `BidirectionalReplicationManager` | Bidirectional replication with per-collection conflict strategies |
| 5. Multi-Tier | `MultiTierReplicationManager` | Hierarchical tiering with automatic promotion/demotion |
| 6. Analytics | `ReplicationAnalytics` | Lag history, bottleneck detection, Prometheus export |

## Building

The example is included in the main ThemisDB CMake build.  Build the project
normally and the `example_replication` target will be available:

```bash
cmake --preset default
cmake --build build --target example_replication
```

Or build it standalone (adjust include paths as needed):

```bash
g++ -std=c++17 -O2 \
    -I../../include \
    example_replication.cpp \
    ../../build/libreplication.a \
    -lzstd -llz4 -lsnappy \
    -o example_replication
```

## Running

```bash
./example_replication
```

Expected output (abbreviated):

```
========================================
  ThemisDB Replication Module Example
========================================

=== 1. WALManager ===
  Appending 4 WAL entries...
  Last assigned sequence: 4
  ...

=== 2. ReplicationManager ===
  ReplicationManager initialized
  Role: LEADER
  Registered replicas: 2
  ...

=== 3. VectorClock + HybridLogicalClock ===
  VectorClock comparison (a vs b): CONCURRENT (expected)
  After merge+increment: a happens-before b (expected)
  ...

=== 4. BidirectionalReplicationManager (active-active) ===
  BidirectionalReplicationManager started
  ...

=== 5. MultiTierReplicationManager ===
  Collection 'financial_transactions': tier=0 replicas=3 max_latency=10ms
  ...

=== 6. ReplicationAnalytics ===
  follower-1 lag history: avg=... p95=... p99=... max=...
  ...

========================================
  All demos completed successfully.
========================================
```

## Key Concepts

### Leader-Follower Replication

```cpp
ReplicationConfig cfg;
cfg.mode = ReplicationMode::SEMI_SYNC;
cfg.min_sync_replicas = 1;   // wait for at least 1 follower ACK

ReplicationManager mgr(cfg);
mgr.initialize();

ReplicaInfo follower;
follower.node_id  = "follower-1";
follower.endpoint = "10.0.0.2:7000";
mgr.addReplica(follower);

WALEntry entry;
entry.operation   = "INSERT";
entry.collection  = "users";
entry.document_id = "user_42";
entry.data        = R"({"name":"Alice"})";
mgr.replicate(entry);
```

### Active-Active (Bidirectional) Replication

```cpp
BidirectionalReplicationManager::BidiConfig bidi_cfg;
bidi_cfg.local_node_id  = "us-west-1";
bidi_cfg.remote_node_id = "us-east-1";
bidi_cfg.default_strategy = ConflictResolution::LAST_WRITE_WINS;
// Per-collection override:
bidi_cfg.collection_strategies["orders"] = ConflictResolution::VECTOR_CLOCK;

BidirectionalReplicationManager bidi(bidi_cfg);
bidi.start();
bidi.submitWrite(entry);
auto status = bidi.getSyncStatus();
```

### Multi-Tier Replication

```cpp
MultiTierConfig tier_cfg;
tier_cfg.auto_tiering_enabled = true;
tier_cfg.hot_access_threshold = 100;   // ≥ 100 accesses/window → Tier 1
tier_cfg.cold_access_threshold = 5;    // < 5 accesses/window  → Tier 3

MultiTierReplicationManager tier(tier_cfg);
tier.assignTier("payments",  ReplicationTier::TIER_1_CRITICAL);
tier.assignTier("analytics", ReplicationTier::TIER_3_ARCHIVAL);
```

## Related Documentation

- `src/replication/ROADMAP.md` — implementation phases and status
- `src/replication/FUTURE_ENHANCEMENTS.md` — design constraints and planned features
- `include/replication/replication_manager.h` — full API reference
- `docs/replication-ha-guide.md` — high-availability guide
