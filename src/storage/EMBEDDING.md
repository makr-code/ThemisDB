---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
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
  - "../../include/storage/backup_manager.h"
  - "storage_engine.cpp"
  - "mvcc_store.cpp"
  - "wal_storage.cpp"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage component boundaries

The source tree contains storage components with separate APIs; the current
interfaces do not support describing them as one automatic write/recovery
pipeline.

| Component | Source-visible behavior | Boundary |
|---|---|---|
| `StorageEngine` | Wraps `RocksDBWrapper` for key-value operations and callback scans. `open()` enables RocksDB's WAL. | Does not construct or call the standalone `WALStorage` or `MVCCStore` in its implementation. |
| `MVCCStore` | Stores keys with an HLC timestamp suffix; provides latest/timestamp reads, transactional history helpers, scans, and GC. | Requires a supplied, already-open database wrapper. Its standalone `put()` does not return a `Result`. |
| `WALStorage` | Appends entries, replays segments through a callback, checkpoints, and flushes. | Uses its own configuration; `fsync_on_write` is configurable. Do not equate this class with RocksDB's internal WAL. |
| Backup/PITR | The backup manager exposes checkpoint, backup, restore, verification, and transport options; PITR is a separate manager. | A header/API surface does not prove a deployment schedule, retention policy, or end-to-end recovery result. |

## What callers can rely on

- Check each component's return type and propagate errors.
- Preserve the configured WAL sync policy; `fsync_on_write` defaults to true
  for `WALStorage`, but callers can configure it.
- Use `MVCCStore::getAtTimestamp()` when a timestamp-relative read is needed;
  `getLatest()` has distinct semantics.
- Treat backup encryption, encrypted-blob storage, RocksDB encryption, and
  audit-log protection as separate concerns.

## Unverified integration questions

- TODO: identify and test any application-level wiring that coordinates
  `StorageEngine`, `MVCCStore`, and `WALStorage`; it is not established by the
  cited component implementations alone.
- TODO: validate backup/PITR consistency, timing, and retention claims against
  a named deployment configuration and executed integration test before making
  operational guarantees.
