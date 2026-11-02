---
Author: "ThemisDB Contributors"
Created: "2026-10-18"
Last Updated: "2026-11-02"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "../../include/storage/storage_engine.h"
  - "../../include/storage/mvcc_store.h"
  - "../../include/storage/wal_storage.h"
  - "../../include/storage/streaming_ingest_manager.h"
  - "../../include/storage/backup_manager.h"
  - "../../include/storage/tiered_storage.h"
  - "storage_engine.cpp"
  - "mvcc_store.cpp"
  - "wal_storage.cpp"
  - "streaming_ingest_manager.cpp"
  - "../replication/replication_manager.cpp"
  - "tests/storage/CMakeLists.txt"
Related_Milestone: "DOC-WEEKLY-2026-W45"
---

Exact model backend not verifiable from final document text alone.

# Storage module

The storage source tree contains several related but distinct components. This
overview makes no module-wide durability, ACID, recovery-time, or availability
claim; those depend on the component, configuration, backend, and caller.

## Component map

- `StorageEngine` provides `open`, `close`, `put`, `get`, `del`, and callback
  scans over its `RocksDBWrapper`. Its default dependency implementations are
  development/test shims; production-mode checks are implemented in
  `storage_engine.cpp`.
- `MVCCStore` stores HLC-versioned keys in a supplied, already-open
  `RocksDBWrapper`. It exposes latest and timestamp-relative reads, history
  scans, and version garbage collection. It is not automatically the same write
  path as `StorageEngine`.
- `WALStorage` is a separate append/replay/checkpoint API. Its configured
  `fsync_on_write` policy affects sync behavior; this API must not be conflated
  with RocksDB's internal WAL or assumed to wrap every `StorageEngine` write.
- `WALStorage::open()` can report `Complete`, `TruncatedTail`, `Corrupted`,
  `TimedOut`, or `Failed` through its optional `RecoveryResult` output. Strict
  recovery is the default; the configurable timeout is unlimited at zero and
  rejects negative values. Explicit `SkipCorrupted` quarantines a corrupt
  segment under `<wal_dir>/quarantine`; callback aborts and I/O errors fail
  open. These source-level behaviors do not establish crash-harness or benchmark
  evidence.
- `StreamingIngestManager::ingestBatch()` admits all events or none: BLOCK
  waits for full-batch capacity and times out without enqueueing; DROP returns
  zero and drops the entire batch if capacity is insufficient. Flush uses
  atomic RocksDB WriteBatch chunks; failed chunks and the remaining suffix are
  retained ahead of newer events for ordered, PUT-idempotent retry.
- Backup, PITR, blob, tiering, maintenance, integrity, and diagnostics
  components have their own interfaces and configuration. Their presence does
  not establish an end-to-end guarantee for every deployment.

## Source-backed limits

- A WAL operation reports errors through `Result`; `fsync_on_write` defaults
  to true but is configurable. See [`API.md`](API.md) and
  [`IMPLEMENTATION.md`](IMPLEMENTATION.md).
- MVCC timestamp reads and the WAL's recovery callback are separate contracts;
  neither implies a transaction-isolation or recovery guarantee for all module
  components.
- The tiered-storage header declares a local three-tier manager and migration
  controls. Its declarations do not establish measured latency or universal
  non-blocking behavior.
- `EncryptedBlobBackend` encrypts data routed through that decorator only. It
  does not imply encryption of RocksDB, WAL, backup, or audit files.
- The source and test registration maps are in
  [`ARCHITECTURE.md`](ARCHITECTURE.md),
  [`tests/storage/CMakeLists.txt`](../../tests/storage/CMakeLists.txt), and
  [`benchmarks/storage/CMakeLists.txt`](../../benchmarks/storage/CMakeLists.txt).

## Maintainer references

- [`API.md`](API.md) — caller-facing contracts
- [`IMPLEMENTATION.md`](IMPLEMENTATION.md) — source boundaries and validation
- [`OPERATIONS.md`](OPERATIONS.md) — operational signals and limitations
- [`RUNBOOK.md`](RUNBOOK.md) — operational troubleshooting; verify recovery
  actions against the deployment
- [`ROADMAP.md`](ROADMAP.md) — module planning and evidence status
- [`SECURITY.md`](SECURITY.md) and [`TRUST.md`](TRUST.md) — security boundaries
