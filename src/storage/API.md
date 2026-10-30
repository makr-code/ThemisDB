---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: api-contract
Canonical references: `include/storage/storage_engine.h`, `include/storage/mvcc_store.h`, `include/storage/wal_storage.h`, `src/storage/storage_engine.cpp`, `src/storage/wal_storage.cpp`
Related issues: #7093, #7167

# Storage API contract

## Public surfaces

- `StorageEngine` implements `IStorageEngine` operations including `open`, `put`, `get`, `del`, and callback-based scans. The operations use `Result` return values.
- `MVCCStore` writes timestamped versions, offers transaction-aware history writes, and exposes snapshot/version-read operations. Its constructor takes an already-open `RocksDBWrapper`; a null clock causes an internal HLC to be created.
- `WALStorage::open()` returns a result containing the WAL instance. Append, checkpoint, and flush operations also return `Result`; recovery is supplied through a callback.

## Contract details callers must preserve

- Inspect and propagate `Result` errors instead of treating failed reads, writes, or replay as success.
- A WAL delete is represented as a delete/tombstone entry. `WALStorage::Config::fsync_on_write` defaults to true but is configurable.
- Snapshot reads are timestamp-relative; do not substitute a latest-value read when the caller supplied a snapshot timestamp.
- The `StorageEngine` constructor’s dependency requirements and production-mode behavior are implemented in `src/storage/storage_engine.cpp`; use the header and implementation rather than assuming optional dependencies are interchangeable.

## Failure and validation

Test dependency validation with `tests/storage/test_storage_engine_di.cpp` and `tests/storage/test_storage_engine_prod.cpp`. For MVCC, WAL replay, and checkpoint changes, add or run the closest test registered in `tests/storage/CMakeLists.txt`.

See `IMPLEMENTATION.md` for change boundaries and `MODULE_GUIDE.md` for maintainer steps.
