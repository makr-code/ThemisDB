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
SOT domain: module-behavior
Canonical references: `include/storage/storage_engine.h`, `include/storage/mvcc_store.h`, `include/storage/wal_storage.h`, `src/storage/storage_engine.cpp`, `src/storage/mvcc_store.cpp`, `src/storage/wal_storage.cpp`, `tests/storage/test_storage_engine_prod.cpp`
Related issues: #7093, #7167

# Storage implementation

## Purpose and boundary

For developers changing the storage engine, MVCC, or its WAL. `StorageEngine` implements the storage interface; `MVCCStore` and `WALStorage` are distinct components with their own public contracts. Do not infer one component’s durability semantics from another’s API.

## Implementation map

- `storage_engine.cpp`: engine dependencies, open/read/write/delete, and scan implementation.
- `mvcc_store.cpp`: HLC-versioned records, snapshot/version reads, and history operations.
- `wal_storage.cpp`: append-only records, recovery replay, segment rotation, and checkpoint/flush operations.

## Invariants and failure cases

- `MVCCStore` stores versions using a logical key plus an HLC timestamp; preserve timestamp ordering and snapshot-read behavior.
- `WALStorage` exposes `Result` from open, append, checkpoint, and flush paths. Its `fsync_on_write` option controls per-write syncing; do not claim unconditional per-entry sync when configuration disables it.
- Recovery replay can be stopped by its callback or by an error; do not accept partial recovery as a successful complete replay.
- The engine has production-mode checks around default no-op dependencies in `storage_engine.cpp`; update those checks and their tests together.
- A WAL delete entry is a tombstone in the WAL API. Do not conflate that with empty values in the storage engine.

## Change and validation steps

1. Follow the relevant public declaration into its matching source file.
2. For changes to startup dependencies, test production-mode validation with `tests/storage/test_storage_engine_prod.cpp`.
3. For engine contract changes, run `tests/storage/test_storage_engine_di.cpp` and the focused storage contract tests.
4. For MVCC or WAL changes, add a focused test under `tests/storage/`; run the target registered in `tests/storage/CMakeLists.txt`.

See `API.md`, `MODULE_GUIDE.md`, `SECURITY.md`, and `RUNBOOK.md` for adjacent contracts and operations.
