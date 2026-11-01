---
Author: "ThemisDB Contributors"
Created: "2026-10-18"
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
  - "../../include/storage/tiered_storage.h"
  - "README.md"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage feature index

This is a navigation list, not a guarantee that every feature is enabled,
integrated, or production-configured.

| Surface | Source-backed scope |
|---|---|
| Storage engine | RocksDB-backed open/close, key-value operations, callback scans, dependency injection |
| MVCC | HLC-versioned records, timestamp reads, version scans, and version GC |
| WAL | Configurable append, replay callback, checkpoint, and flush operations |
| Backup/PITR | Separate manager APIs for backup and recovery; see their headers for exact options |
| Blob/tiering | Backend abstractions, encrypted blob decorator, and local tier-management APIs |
| Maintenance/diagnostics | Compaction, pressure, audit, integrity, and error-diagnostic components |

Use the public declarations and matching implementation as the authority for
specific behavior. Start at [`README.md`](README.md) and
[`ARCHITECTURE.md`](ARCHITECTURE.md); do not infer a combined `StorageEngine` →
MVCC → `WALStorage` transaction pipeline from this index.
