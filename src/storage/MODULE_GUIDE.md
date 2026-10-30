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
Canonical references: `include/storage/storage_engine.h`, `include/storage/mvcc_store.h`, `include/storage/wal_storage.h`, `src/storage/storage_engine.cpp`, `tests/storage/CMakeLists.txt`
Related issues: #7093, #7167

# Storage module guide

## Audience and ownership

For maintainers changing storage engine operations, MVCC, WAL, recovery, or storage-specific maintenance. This module owns persistence behavior, not query planning or client transport.

## Read by change type

- Engine/dependency setup: `include/storage/storage_engine.h`, `src/storage/storage_engine.cpp`.
- Versioning/snapshot behavior: `include/storage/mvcc_store.h`, `src/storage/mvcc_store.cpp`.
- WAL durability/replay: `include/storage/wal_storage.h`, `src/storage/wal_storage.cpp`.
- Module context and operational recovery: `README.md`, `ARCHITECTURE.md`, `RUNBOOK.md`, `SECURITY.md`.

## Maintainer workflow

1. Identify which public contract changes and trace it to the implementation.
2. Keep errors returned by storage and recovery operations observable.
3. Check how configuration affects durability (in particular `fsync_on_write`) and distinguish WAL tombstones from ordinary empty values.
4. Add a focused test in `tests/storage/`; use `test_storage_engine_di.cpp` or `test_storage_engine_prod.cpp` for engine setup behavior.
5. Run the corresponding test target registered by `tests/storage/CMakeLists.txt`; update `API.md` if caller obligations changed.

## Review checklist

- Snapshot versus latest read semantics remain distinct.
- WAL replay/append errors are not reported as complete success.
- Production dependency checks still match the active source implementation.

See `IMPLEMENTATION.md` for source boundaries and `API.md` for caller contracts.
