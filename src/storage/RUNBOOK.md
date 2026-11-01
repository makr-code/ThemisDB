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
  - "../../tests/storage/CMakeLists.txt"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage troubleshooting notes

These are triage recommendations, not a recovery procedure certified for every
deployment. Preserve evidence and follow the deployment's tested backup/restore
procedure. Do not manually replay logs, delete files, or rebuild indexes/caches
based only on this guide.

## Initial triage

1. Record the exact component, operation, error/result, configuration, and
   affected database path.
2. Preserve relevant logs, WAL segments, manifests, and backup artifacts before
   cleanup or restore attempts.
3. Determine whether the report concerns RocksDB through `StorageEngine`,
   `MVCCStore`, the standalone `WALStorage`, backup/PITR, or tiered/blob
   storage. These are distinct APIs.
4. If data integrity or write durability is uncertain, stop writes according
   to the deployment incident process and escalate to the storage owner.

## Component-specific checks

### Engine open/read/write failure

Check `StorageEngine::open()` results, the database path, and the dependency
configuration. In production mode, the implementation rejects a null index
manager and default no-op encryption/key-provider dependencies are not
production-safe. See `storage_engine.cpp` and
`tests/storage/test_storage_engine_prod.cpp`.

### MVCC read discrepancy

Capture the logical key and read timestamp; distinguish `getLatest()` from
`getAtTimestamp()`. Check the encoded key/history and the corresponding focused
tests before attempting any data repair. The current API does not expose a
documented operator command to rebuild its in-memory latest-version cache.

### Standalone WAL failure

Check the `WALStorage` result, configured directory, segment files, and
`fsync_on_write` setting. Do not infer this component's use from
`StorageEngine::open()`; it enables RocksDB's WAL separately. Replay is driven
by the `WALStorage::open()` recovery callback.

### Backup/PITR or blob failure

Retain the manifest and payloads, and inspect the specific backup/restore
result, required codecs/crypto options, and provider configuration. Use the
deployment's verified restore process; this guide does not establish a
restore-time bound, retention policy, or safe in-place recovery sequence.

### Tiering/maintenance issue

Record tier configuration, key/tier metadata, worker state, and returned
operation outcomes. Do not promise that migration is transparent or has a
particular latency without deployment-specific test evidence.

## Validation and escalation

Use the matching tests registered in `tests/storage/CMakeLists.txt` and the
current operator procedure for the deployment. Escalate unresolved data
integrity, replay, restore, or production dependency failures; do not resume
normal writes until the responsible owner confirms a safe recovery state.

See [`OPERATIONS.md`](OPERATIONS.md), [`SECURITY.md`](SECURITY.md), and
[`TRUST.md`](TRUST.md).
