---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
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
  - "storage_engine.h"
  - "mvcc_store.h"
  - "wal_storage.h"
  - "rocksdb_wrapper.h"
  - "../../src/storage/ARCHITECTURE.md"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Public storage header architecture

`include/storage/` declares public and module-facing C++ interfaces. The
corresponding implementations are in `src/storage/`; the implementation
architecture is summarized in [`../../src/storage/ARCHITECTURE.md`](../../src/storage/ARCHITECTURE.md).

## Principal component boundaries

| Declaration | Role established by its interface |
|---|---|
| [`storage_engine.h`](storage_engine.h) | `StorageEngine` implements `IStorageEngine` operations and callback scans over a `RocksDBWrapper`. |
| [`rocksdb_wrapper.h`](rocksdb_wrapper.h) | RocksDB wrapper and its operations/configuration. |
| [`mvcc_store.h`](mvcc_store.h) | Separate HLC-versioned key/history API. |
| [`wal_storage.h`](wal_storage.h) | Separate segment-based WAL append/replay/checkpoint API. |
| [`backup_manager.h`](backup_manager.h), [`pitr_manager.h`](pitr_manager.h) | Backup and point-in-time recovery interfaces. |
| [`blob_storage_backend.h`](blob_storage_backend.h), [`blob_storage_manager.h`](blob_storage_manager.h), [`tiered_storage.h`](tiered_storage.h) | Blob abstractions, manager, and a tiered-storage manager. |
| [`storage_api_contract.h`](storage_api_contract.h) | Contract statements and error-code declarations; validate implementation conformance independently. |

These are component interfaces, not an asserted call graph. In particular,
`StorageEngine`'s use of RocksDB WAL does not prove that it calls the
standalone `WALStorage` API or that it uses `MVCCStore` for every operation.

## Integration and limits

- Inspect the matching `src/storage/*.cpp` implementation and focused test
  before documenting runtime guarantees.
- Compile-time options, backend availability, and caller wiring can affect
  behavior.
- Do not infer universal transaction isolation, synchronous durability,
  transparent tier migration, or deployment-level encryption from header
  names or Doxygen maturity labels.

See [`README.md`](README.md) for a concise header index and
[`ROADMAP.md`](ROADMAP.md) for the canonical planning link.
