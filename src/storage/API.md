---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-20"
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
Canonical references: `include/storage/storage_engine.h`, `include/storage/mvcc_store.h`, `include/storage/wal_storage.h`, `include/storage/streaming_ingest_manager.h`, `include/storage/rocksdb_wrapper.h`, `src/storage/storage_engine.cpp`, `src/storage/wal_storage.cpp`, `src/storage/streaming_ingest_manager.cpp`, `src/storage/mvcc_store.cpp`, `src/replication/replication_manager.cpp`, `tests/storage/test_mvcc_range_export.cpp`
Related issues: #7093, #7167
Related milestone: DOC-WEEKLY-2026-W45

# Storage API contract

## Public surfaces

- `StorageEngine` implements `IStorageEngine` operations including `open`, `put`, `get`, `del`, and callback-based scans. The operations use `Result` return values.
- `MVCCStore` writes timestamped versions, offers transaction-aware history writes, and exposes snapshot/version-read operations. Its constructor takes an already-open `RocksDBWrapper`; a null clock causes an internal HLC to be created.
- `WALStorage::open()` returns a result containing the WAL instance and accepts an optional `RecoveryResult*` output. Its `status` is `Complete`, `TruncatedTail`, `Corrupted`, `TimedOut`, or `Failed`. The default `RecoveryMode` is `Strict`; `Config::recovery_timeout` is in milliseconds, zero means unlimited, and a negative duration is rejected.
- `StreamingIngestManager::ingestBatch()` admits its entire input batch or none. It returns the accepted batch size, zero when DROP discards all events for insufficient capacity, or an error on rejection/timeout.
- `ObjectVersionBlockManifest` describes ordered object-version blocks up to 256 KiB each. `buildObjectVersionBlockSet()` computes SHA-256 and records caller-supplied shard assignments; `assembleObjectVersion()` validates metadata, ordinals, lengths, and hashes before returning bytes in manifest order. These helpers are local only and do not persist, place, authorize, replicate, or publish data.
- `RocksDBWrapper::SafeIterator::statusMessage()` returns an empty string for a healthy iterator or the underlying RocksDB iterator error; an uninitialized or moved-from iterator reports an explicit error.
- `RocksDBWrapper::createSnapshot()` returns a move-only `SnapshotHandle` owning one RocksDB snapshot. Its `sequence()` is the snapshot's sequence, and `newSafeIterator(snapshot, ...)` reads from that exact view. The handle must not outlive its wrapper; wrapper operation guards prevent database close while a snapshot is active.
- `MVCCStore::beginSnapshot()` returns a move-only `SnapshotReader` with the RocksDB snapshot sequence and captured HLC watermark. `exportSnapshot()` enumerates and selects versions against that same snapshot, emitting at most the configured record and byte bounds. Records are ordered by logical key and use explicit `Put` or `Delete` kinds; the current empty-value encoding is a tombstone, so this API cannot represent an empty live `Put`.

## Contract details callers must preserve

- Inspect and propagate `Result` errors instead of treating failed reads, writes, or replay as success.
- Prefix/range scans that require completeness must check `SafeIterator::statusMessage()` after iteration; an invalid iterator position alone can also mean normal end-of-range.
- A WAL delete is represented as a delete/tombstone entry. `WALStorage::Config::fsync_on_write` defaults to true but is configurable.
- In Strict recovery, mid-log corruption, timeout, callback abort, and I/O errors fail `open()`. Only an incomplete tail is truncated: a short final header must match the WAL magic prefix, and an incomplete record is not accepted as a tail if a later complete record is found. `SkipCorrupted` is an explicit override that quarantines the corrupt segment under `<wal_dir>/quarantine` and can return successful open with status `Corrupted`; it does not convert callback aborts or I/O errors into success.
- Replication recovery returns false from its WAL callback when `WALEntry::deserialize` fails, so invalid serialized replication entries fail startup rather than being silently skipped.
- Batch admission is all-or-nothing. During flush, each RocksDB WriteBatch chunk is atomic, not the entire ingested batch: earlier successful chunks remain committed. On write failure, the failed chunk and remaining suffix are requeued before newer events; the error identifies `failed_offset` and `total`. Retry preserves order and is PUT-idempotent.
- Snapshot reads are timestamp-relative; do not substitute a latest-value read when the caller supplied a snapshot timestamp.
- The MVCC range-export reader is a process-local snapshot, not a durable export/checkpoint. A restart loses the reader; callers need a new migration snapshot rather than assuming resume. It does not derive sharding tokens, authorize a range assignment, quiesce writes, persist target staging, or commit ownership.
- Snapshot export propagates cancellation, callback exceptions, invalid bounds, and RocksDB iterator errors; none means an empty successful export. The RocksDB sequence excludes transactions committed after snapshot capture even if their HLC was allocated before capture.
- The `StorageEngine` constructor’s dependency requirements and production-mode behavior are implemented in `src/storage/storage_engine.cpp`; use the header and implementation rather than assuming optional dependencies are interchangeable.

## Known recovery limitation

The current WAL frame CRC includes the header, but recovery evaluates it only
after declared key/value lengths fit within the remaining segment. In a real
fixture with 12 fsynced, acknowledged PUTs, flipping the low bit in any of the
64 individual bit flips across the eight length bytes of the final frame:
61 caused `TruncatedTail`, replayed only 11 entries, and truncated the newest
segment from 117 to 78 bytes; three smaller length values failed CRC validation
without mutation. Thus many damaged length fields in the newest segment are
currently indistinguishable from an incomplete append for this recovery path;
do not treat tail truncation as fail-closed corruption handling. The report
`wal-tail-length-all-bits.json` records per-bit outcomes. A versioned
frame/commit-boundary design and explicit migration/rollback policy are
required before this gap can be closed.

Separately, the current frame fixture has been tested with all eight bit
positions flipped individually at each non-length header, payload, and CRC
byte: 2,832 mutations across all 12 frames were rejected twice without WAL
mutation. This does not mitigate the length-field gap above; its report is
`wal-all-protected-bits.json`.

## Failure and validation

Test dependency validation with `tests/storage/test_storage_engine_di.cpp` and `tests/storage/test_storage_engine_prod.cpp`. For MVCC, WAL replay, and checkpoint changes, add or run the closest test registered in `tests/storage/CMakeLists.txt`. These test references are not execution evidence; the STO-01 crash/recovery and replay-performance gates and STO-10 focused test evidence remain open.

See `IMPLEMENTATION.md` for change boundaries and `MODULE_GUIDE.md` for maintainer steps.
