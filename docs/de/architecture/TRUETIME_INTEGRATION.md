# TrueTime Integration for ThemisDB Sharding

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Table of Contents

- [Overview](#overview)
- [Components](#components)
    - [TrueTime Clock](#1-truetime-clock-truetimeh--truetimecpp)
    - [Distributed Transaction Coordinator](#2-distributed-transaction-coordinator-distributed_transactionh--distributed_transactioncpp)
    - [ShardRouter Integration](#3-shardrouter-integration-shard_routerh--shard_routercpp)
    - [RaftLog with Timestamps](#4-raftlog-with-timestamps-raft_logh)

This document describes the TrueTime-inspired clock synchronization and distributed transaction coordination implemented for ThemisDB's sharding system.

## Overview

TrueTime is a distributed time API inspired by Google Spanner that provides time with uncertainty bounds. This enables strict serializability and snapshot isolation across distributed shards without complex locking protocols.

## Components

### 1. TrueTime Clock (`truetime.h` / `truetime.cpp`)

The TrueTime class provides:
- **Time Intervals with Uncertainty**: Returns `TTInterval` with `[earliest, latest]` bounds
- **Clock Synchronization**: NTP-based sync to keep clocks accurate
- **Drift Detection**: Monitors and compensates for clock drift
- **Wait-Until-Certain**: Key operation for external consistency

#### Key Operations

```cpp
// Get current time with uncertainty
TTInterval now = truetime->now();

// Wait until a timestamp is definitely in the past
truetime->waitUntil(commit_timestamp);

// Get current uncertainty bound
auto epsilon = truetime->getUncertainty();
```

#### Configuration

```cpp
TrueTime::Config config;
config.base_uncertainty_us = 1000;      // 1ms base uncertainty
config.max_drift_us = 100000;           // 100ms max drift
config.sync_interval_s = 30;            // Sync every 30 seconds
config.ntp_servers = {"time.google.com"};
```

### 2. Distributed Transaction Coordinator (`distributed_transaction.h` / `distributed_transaction.cpp`)

Implements two-phase commit with TrueTime for cross-shard transactions:

#### Transaction Protocol

**Write Transactions (Two-Phase Commit):**
1. **Begin**: Coordinator assigns transaction to participating shards
2. **Execute**: Operations are buffered on each shard
3. **Prepare Phase**: All shards prepare to commit
4. **Assign Commit Timestamp**: `commit_ts = TT.now().latest`
5. **Wait**: `TT.waitUntil(commit_ts)` - ensures external consistency
6. **Commit Phase**: All shards commit with the same timestamp

**Read-Only Transactions (Wait-Free):**
1. **Snapshot Timestamp**: `snapshot_ts = TT.now().latest`
2. **Read**: Query all shards at `snapshot_ts`
3. **No Locking Required**: Reads don't block writes

#### Example Usage

```cpp
auto coordinator = std::make_shared<DistributedTransactionCoordinator>(truetime);

// Begin transaction across multiple shards
std::string txn_id = coordinator->beginTransaction({"shard1", "shard2"});

// Add operations
coordinator->addOperation(txn_id, "shard1", operation1);
coordinator->addOperation(txn_id, "shard2", operation2);

// Commit (2PC with TrueTime)
bool success = coordinator->commit(txn_id);

// Read-only transaction (wait-free)
auto results = coordinator->executeReadOnly(
    {"shard1", "shard2"},
    read_operations
);
```

### 3. ShardRouter Integration (`shard_router.h` / `shard_router.cpp`)

The ShardRouter now supports:
- **Snapshot Reads**: Read at specific timestamp for consistency
- **Transaction Coordination**: Access to distributed transaction coordinator

#### Example Usage

```cpp
// Create ShardRouter with TrueTime
auto router = std::make_shared<ShardRouter>(
    resolver, executor, config, metrics, truetime
);

// Read with snapshot timestamp
auto snapshot_ts = truetime->now().latest;
auto data = router->get(urn, snapshot_ts);

// Access transaction coordinator
auto txn_coordinator = router->getTransactionCoordinator();
```

### 4. RaftLog with Timestamps (`raft_log.h`)

Log entries now include timestamps for ordering:

```cpp
struct LogEntry {
    uint64_t term;
    uint64_t index;
    std::string command;
    uint64_t timestamp_ns;  // TrueTime timestamp
};
```

This enables:
- **Timestamp-based Ordering**: Total order across all shards
- **Snapshot Isolation**: Read at any past timestamp
- **Conflict Detection**: Detect conflicts based on timestamps

## Guarantees

### External Consistency

If transaction T1 commits before T2 begins, then T1's commit timestamp is less than T2's commit timestamp.

**Implementation:**
1. T1 commits at timestamp `commit_ts1 = TT.now().latest`
2. T1 waits until `TT.waitUntil(commit_ts1)` returns
3. T2 can now start, getting `commit_ts2 = TT.now().latest`
4. By TrueTime properties: `commit_ts1 < commit_ts2`

### Snapshot Isolation

Read-only transactions read a consistent snapshot without blocking writes.

**Implementation:**
1. Get snapshot timestamp: `ts = TT.now().latest`
2. Read all data with `timestamp <= ts`
3. No locks needed - writes continue concurrently

### Strict Serializability

All transactions appear to execute in timestamp order.

**Implementation:**
- Each transaction gets a unique commit timestamp
- Reads see all writes with `timestamp < read_timestamp`
- Writes are applied in timestamp order

## Phase 3 Implementation Status

✅ **Completed:**
- TrueTime clock with uncertainty bounds
- NTP-based clock synchronization
- Drift detection and compensation
- Two-phase commit with TrueTime
- Distributed transaction coordinator
- ShardRouter integration
- RaftLog timestamp support
- Wait-free read-only transactions

## Phase 4: Future Work

### GPS Time Source
- Integrate GPS receivers for precise time
- Fallback to NTP when GPS unavailable
- Uncertainty < 100μs with GPS

### Advanced Drift Compensation
- Machine learning for drift prediction
- Adaptive sync intervals
- Temperature-based compensation

### Multi-Datacenter Optimization
- WAN-aware time sync
- Regional time masters
- Cross-region consistency

### Performance Tuning
- Benchmark commit latency
- Optimize wait times
- Parallel prepare/commit
- Metrics and monitoring

## Testing

TODO: Add comprehensive tests for:
- TrueTime accuracy and uncertainty bounds
- Transaction isolation levels
- Conflict detection and resolution
- Multi-shard consistency
- Recovery after failures

## References

- [Google Spanner Paper (2012)](https://research.google/pubs/pub39966/)
- [Spanner: Becoming a SQL System (2017)](https://research.google/pubs/pub46103/)
- [Living Without Atomic Clocks (CockroachDB)](https://www.cockroachlabs.com/blog/living-without-atomic-clocks/)
