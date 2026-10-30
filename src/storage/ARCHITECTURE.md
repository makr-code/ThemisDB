---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/storage/storage_engine.h"
  - "../../include/storage/rocksdb_wrapper.h"
  - "../../include/storage/mvcc_store.h"
  - "../../include/storage/wal_storage.h"
  - "storage_engine.cpp"
  - "mvcc_store.cpp"
  - "wal_storage.cpp"
  - "pitr_manager.cpp"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage Module Architecture

## Scope, boundaries, and ownership

`src/storage/` owns durable data access and storage lifecycle, the RocksDB wrapper, versioned record storage, storage WAL facilities, blob backends, and storage maintenance/recovery components. Callers supply domain operations and may inject dependencies; query planning, client transport, and key lifecycle are not storage responsibilities.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Engine and backend | [`StorageEngine`](../../include/storage/storage_engine.h), [`RocksDBWrapper`](../../include/storage/rocksdb_wrapper.h) | Storage orchestration and the RocksDB-facing operations used by consumers. |
| Versions and history | [`MVCCStore`](../../include/storage/mvcc_store.h), [`HistoryManager`](../../include/storage/history_manager.h) | Versioned keys, timestamp-based reads, version history, and cleanup surfaces. |
| Log and recovery | [`WALStorage`](../../include/storage/wal_storage.h), [`BackupManager`](../../include/storage/backup_manager.h), [`PITRManager`](../../include/storage/pitr_manager.h) | Append/replay, backup, and point-in-time recovery operations. |
| Blob and maintenance | [`BlobStorageManager`](../../include/storage/blob_storage_manager.h), [`TieredStorage`](../../include/storage/tiered_storage.h), [`CompactionManager`](../../include/storage/compaction_manager.h) | Optional blob backends, tier placement, and compaction/maintenance controls. |
| Contract and diagnostics | [`storage_api_contract.h`](../../include/storage/storage_api_contract.h), [`StorageErrorDiagnostics`](../../include/storage/storage_error_diagnostics.h) | Storage error taxonomy and diagnostic surfaces. |

`WALStorage` is a storage log interface; it should not be conflated with RocksDB's internal write-ahead logging.

## Data flow

1. A caller uses `StorageEngine` or `RocksDBWrapper` to address storage operations.
2. Versioned writes/read paths can use `MVCCStore`, whose version key includes the HLC timestamp; the implementation rejects a null database dependency.
3. `WALStorage` provides its append-only log and replay surface. Acknowledgement and recovery behavior depend on its configured backend and I/O results.
4. Backup/PITR, blob placement, tiering, and compaction are separate storage operations. `PITRManager` is constructed with RocksDB, changefeed, and transaction snapshot-manager dependencies.

See [`storage_engine.cpp`](storage_engine.cpp), [`mvcc_store.cpp`](mvcc_store.cpp), [`wal_storage.cpp`](wal_storage.cpp), and [`pitr_manager.cpp`](pitr_manager.cpp).

## Failure domains and edge cases

- Missing required dependencies, invalid storage configuration, or unavailable RocksDB/backend operations can prevent initialization or a read/write from completing.
- WAL I/O, malformed/corrupt log data, or replay errors affect recovery; restore is not a substitute for a successful integrity check.
- PITR requires its configured database, changefeed, and snapshot-manager dependencies; restore behavior depends on the available log/snapshot state.
- Optional blob provider, tiering, and maintenance failures are distinct from the core key/value path and must be diagnosed at their own interfaces.

## Explicit non-goals

- Query parsing, logical planning, and query result shaping.
- Client/network replication transport or shard ownership decisions.
- Authentication, authorization policy, or key-provider lifecycle.
- Claims of zero data loss, exact PITR accuracy, or universal ACID guarantees beyond the specific configured contract.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Query consumer | `query_engine.cpp` includes storage engine/wrapper APIs; see [query architecture](../query/ARCHITECTURE.md). |
| Replication | `replication_manager.cpp` includes `storage/wal_storage.h`; see [replication architecture](../replication/ARCHITECTURE.md). |
| Sharding | `sharding/wal_manager.cpp` uses `storage/wal_storage.h`; see [sharding architecture](../sharding/ARCHITECTURE.md). |
| Redundancy placement | Storage includes `sharding/redundancy_strategy.h` for blob redundancy placement. |
| Recovery collaborators | `pitr_manager.cpp` includes CDC changefeed and transaction snapshot-manager APIs; these are explicit constructor dependencies. |

## References

- [Module README](README.md) · [API notes](API.md) · [module roadmap](ROADMAP.md)
- [`storage_engine.h`](../../include/storage/storage_engine.h) · [`mvcc_store.h`](../../include/storage/mvcc_store.h) · [`wal_storage.h`](../../include/storage/wal_storage.h)
