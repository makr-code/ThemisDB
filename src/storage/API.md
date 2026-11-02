---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-02"
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
Canonical references: `include/storage/storage_engine.h`, `include/storage/mvcc_store.h`, `include/storage/wal_storage.h`, `include/storage/streaming_ingest_manager.h`, `src/storage/storage_engine.cpp`, `src/storage/wal_storage.cpp`, `src/storage/streaming_ingest_manager.cpp`, `src/replication/replication_manager.cpp`
Related issues: #7093, #7167
Related milestone: DOC-WEEKLY-2026-W45

# Storage API contract

## Public surfaces

- `StorageEngine` implements `IStorageEngine` operations including `open`, `put`, `get`, `del`, and callback-based scans. The operations use `Result` return values.
- `MVCCStore` writes timestamped versions, offers transaction-aware history writes, and exposes snapshot/version-read operations. Its constructor takes an already-open `RocksDBWrapper`; a null clock causes an internal HLC to be created.
- `WALStorage::open()` returns a result containing the WAL instance and accepts an optional `RecoveryResult*` output. Its `status` is `Complete`, `TruncatedTail`, `Corrupted`, `TimedOut`, or `Failed`. The default `RecoveryMode` is `Strict`; `Config::recovery_timeout` is in milliseconds, zero means unlimited, and a negative duration is rejected.
- `StreamingIngestManager::ingestBatch()` admits its entire input batch or none. It returns the accepted batch size, zero when DROP discards all events for insufficient capacity, or an error on rejection/timeout.

## Contract details callers must preserve

- Inspect and propagate `Result` errors instead of treating failed reads, writes, or replay as success.
- A WAL delete is represented as a delete/tombstone entry. `WALStorage::Config::fsync_on_write` defaults to true but is configurable.
- In Strict recovery, mid-log corruption, timeout, callback abort, and I/O errors fail `open()`. Only an incomplete tail is truncated: a short final header must match the WAL magic prefix, and an incomplete record is not accepted as a tail if a later complete record is found. `SkipCorrupted` is an explicit override that quarantines the corrupt segment under `<wal_dir>/quarantine` and can return successful open with status `Corrupted`; it does not convert callback aborts or I/O errors into success.
- Replication recovery returns false from its WAL callback when `WALEntry::deserialize` fails, so invalid serialized replication entries fail startup rather than being silently skipped.
- Batch admission is all-or-nothing. During flush, each RocksDB WriteBatch chunk is atomic, not the entire ingested batch: earlier successful chunks remain committed. On write failure, the failed chunk and remaining suffix are requeued before newer events; the error identifies `failed_offset` and `total`. Retry preserves order and is PUT-idempotent.
- Snapshot reads are timestamp-relative; do not substitute a latest-value read when the caller supplied a snapshot timestamp.
- The `StorageEngine` constructor’s dependency requirements and production-mode behavior are implemented in `src/storage/storage_engine.cpp`; use the header and implementation rather than assuming optional dependencies are interchangeable.

## Failure and validation

Test dependency validation with `tests/storage/test_storage_engine_di.cpp` and `tests/storage/test_storage_engine_prod.cpp`. For MVCC, WAL replay, and checkpoint changes, add or run the closest test registered in `tests/storage/CMakeLists.txt`. These test references are not execution evidence; the STO-01 crash/recovery and replay-performance gates and STO-10 focused test evidence remain open.

See `IMPLEMENTATION.md` for change boundaries and `MODULE_GUIDE.md` for maintainer steps.
