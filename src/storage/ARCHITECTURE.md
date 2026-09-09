# Architecture - Storage Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The storage module composes durable key-value persistence, MVCC/WAL lifecycle behavior, blob/tiering and redundancy paths, backup-recovery behavior, and storage integrity/audit surfaces into a bounded subsystem.

## Main Execution Planes

1. Persistence and lifecycle plane
- RocksDB wrapper, storage engine, key schema, and WAL/MVCC behavior

2. Recovery and maintenance plane
- backup/PITR/compaction/pruning/index-maintenance behavior

3. Blob, tiering, and integrity plane
- blob/tiered/redundancy/encryption behavior and storage audit surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| persistence contract | durable read/write behavior with explicit lifecycle control |
| versioning contract | deterministic MVCC and replay semantics |
| recovery contract | explicit backup/PITR success/failure behavior |
| integrity contract | storage-side signature/audit and redundancy observability |

## Failure Semantics

- storage lifecycle and open/write/replay failures are explicit.
- backup/PITR faults remain diagnosable and non-silent.
- blob/tier/redundancy path failures surface deterministic outcomes.
- maintenance job failures remain observable through module diagnostics.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)

| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| `cdc` | `include/cdc/` | Change-data-capture events emitted on every committed write |
| `index` | `include/index/` | Storage drives index-maintenance calls post-write |
| `performance` | `include/performance/` | Perf counters and hot-path instrumentation |
| `sharding` | `include/sharding/redundancy_strategy.h` | Redundancy placement strategy for blob/tiered storage |
| `temporal` | `include/temporal/` | Temporal versioning hooks for time-travel queries |
| `transaction` | `include/transaction/snapshot_manager.h` | MVCC snapshot registration and release |
| `utils` | `include/utils/` | Utility helpers (encoding, error codes) |

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/storage/storage_engine.h`, `include/storage/rocksdb_wrapper.h` | Admin and data API direct access |
| `query` | `include/storage/rocksdb_wrapper.h`, `include/storage/storage_engine.h` | Collection scans and document reads |
| `transaction` | `include/storage/rocksdb_wrapper.h`, `include/storage/history_manager.h` | Durable KV writes, WAL, MVCC history |
| `sharding` | `include/storage/rocksdb_wrapper.h` | Shard-local data access after routing |
| `replication` | `include/storage/wal_storage.h` (indirectly via CDC/WAL) | WAL shipped to replicas |
| `index` | `include/storage/rocksdb_wrapper.h` | Secondary index data persistence |
| `cache` | `include/storage/` | Cache miss back-fill reads |
| `graph` | `include/storage/` | Graph edge/adjacency data persistence |
| `geo` | `include/storage/` | Spatial index and geometry data persistence |
| `analytics` | `include/storage/rocksdb_wrapper.h` | Columnar data reads for aggregations and scans |
| `process` | `include/storage/` | Process mining event log persistence |
| `training` | `include/storage/` | Model checkpoint and parameter persistence |
| `document` | `include/storage/` | Document blob and index data persistence |
| `temporal` | `include/storage/` | Temporal versioned record storage |
| `tensor` | `include/storage/` | Dense tensor data persistence |
| `ingestion` | `include/storage/` | Writes ingested data records to storage |
| `content` | `include/storage/` | Content blob and metadata persistence |

---

## Integration Points

### Critical Integration: Storage → CDC (Write Propagation)
**Files:** `src/storage/storage_engine.cpp` ↔ `include/cdc/`
**Contract:** After every committed write, storage emits a CDC event (key, value, operation type, sequence number). CDC consumers (replication, change-feed API) subscribe to the event stream.
**Thread Safety:** CDC event emission is lock-free append to a ring buffer; consumers use separate read cursors.
**Failure Mode:** CDC ring-buffer overflow → oldest events dropped with sequence gap marker; consumers must handle gap recovery.

### Critical Integration: Storage → Transaction (MVCC Snapshots)
**Files:** `src/storage/mvcc_store.cpp` ↔ `include/transaction/snapshot_manager.h`
**Contract:** Storage registers each MVCC snapshot with the transaction module's `SnapshotManager` on creation and deregisters on release. This allows the transaction module to track the oldest active snapshot for WAL/compaction GC.
**Thread Safety:** `SnapshotManager` uses a concurrent map; register/release are lock-free on the hot path.
**Failure Mode:** Leaked snapshot (crash without deregister) → compaction stalls until recovery restart.

### Critical Integration: Storage → Index (Post-Write Index Maintenance)
**Files:** `src/storage/storage_engine.cpp` ↔ `include/index/`
**Contract:** After a committed document write, storage triggers index maintenance callbacks registered by the index module. Index writes are not part of the storage atomic commit; they are best-effort with retry.
**Thread Safety:** Index maintenance callbacks are invoked on the commit thread; callback must not block.
**Failure Mode:** Index callback failure → logged; reconciliation sweep corrects divergence asynchronously.

### Critical Integration: Storage → Sharding (Redundancy Strategy)
**Files:** `src/storage/blob_redundancy_manager.cpp` ↔ `include/sharding/redundancy_strategy.h`
**Contract:** Blob and tiered storage consults sharding's `RedundancyStrategy` to determine replica placement and erasure-coding parameters before writing large objects.
**Thread Safety:** Strategy reads are stateless and concurrent-safe.
**Failure Mode:** Strategy unavailable → default redundancy factor applied; logged as degraded-placement warning.

---

## Sourcecode Verification (Module: storage/architecture)

- Verified files:
  - src/storage/rocksdb_wrapper.cpp
  - src/storage/storage_engine.cpp
  - src/storage/mvcc_store.cpp
  - src/storage/wal_storage.cpp
  - src/storage/backup_manager.cpp
  - src/storage/pitr_manager.cpp
  - src/storage/compaction_manager.cpp
  - src/storage/tiered_storage.cpp
  - src/storage/blob_redundancy_manager.cpp
  - src/storage/security_signature_manager.cpp
  - src/storage/storage_audit_logger.cpp
- Verified architecture claims:
  - persistence/lifecycle + recovery/maintenance + integrity/tiering plane split
  - explicit failure boundaries for storage, recovery, and maintenance faults
  - module-local ownership of storage-domain behavior