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
  - "../../include/storage/mvcc_store.h"
  - "../../include/storage/wal_storage.h"
  - "../../include/storage/storage_engine.h"
  - "mvcc_store.cpp"
  - "wal_storage.cpp"
  - "storage_engine.cpp"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage data and component model

## MVCC records

`MVCCStore` encodes a versioned key as the base key, a NUL separator, and an
8-byte encoded HLC timestamp. The implementation uses empty values as
tombstones for MVCC deletion history. Timestamp ordering, timestamp-relative
reads, and latest reads are implemented by `mvcc_store.cpp`; callers should not
assume those semantics apply to unrelated `StorageEngine` operations.

## WAL records

`WALStorage` has its own append-only segment format and sequence numbers.
`appendDelete()` records a deletion entry; this is distinct from an MVCC
tombstone and from an empty value accepted by `appendPut()`. Append, replay,
checkpoint, and flush failures are returned through its `Result` API. Its
configured fsync policy determines whether append operations request a sync.

## Storage engine

`StorageEngine` delegates its key-value operations to a `RocksDBWrapper`.
`storage_engine.cpp` sets `enable_wal = true` when it opens RocksDB; this refers
to RocksDB's WAL and does not establish that the separate `WALStorage` class is
called for each engine write.

## Boundaries and verification

- The source does not establish a general claim that every storage write is
  atomically coordinated across the engine, MVCC, and standalone WAL APIs.
- Do not infer universal ACID, snapshot-isolation, no-data-loss, or exact
  recovery-time guarantees from the presence of these components.
- TODO: document any higher-level integration only after locating its source
  call path and matching integration tests.

See [`API.md`](API.md), [`IMPLEMENTATION.md`](IMPLEMENTATION.md), and
[`ARCHITECTURE.md`](ARCHITECTURE.md) for component contracts and source paths.
