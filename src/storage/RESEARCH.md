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
  - "../../include/storage/storage_api_contract.h"
  - "storage_engine.cpp"
  - "mvcc_store.cpp"
  - "wal_storage.cpp"
  - "../../tests/storage/CMakeLists.txt"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage research notes

## Evidence boundary

The source provides distinct interfaces for the RocksDB-backed `StorageEngine`,
the timestamped `MVCCStore`, and the segment-based `WALStorage`. It does not by
itself establish that every write passes through all three components, or that
all module components share one transaction, durability, isolation, or recovery
contract.

## Source observations

- `StorageEngine::open()` constructs a `RocksDBWrapper` with its WAL option
  enabled. Its key-value methods call the wrapper.
- `MVCCStore` encodes versions with an HLC timestamp and offers
  timestamp-relative reads and history operations. Database write results in
  this implementation are not surfaced as a `Result` by every MVCC method.
- `WALStorage` separately exposes append, callback replay, checkpoint, and
  flush operations. Its `fsync_on_write` option defaults to true but is
  configurable.
- Backup, PITR, tiering, and encrypted blob behavior must be evaluated through
  their own declarations and implementations. A component's presence is not
  evidence of deployment configuration or an executed recovery test.

## Open verification questions

- TODO: trace application bootstrap and call sites to determine whether there
  is an integration path coordinating the three storage APIs.
- TODO: establish recovery and backup guarantees from the matching integration
  tests and recorded execution results; this document does not claim those
  guarantees.
- TODO: distinguish the normative requirements in
  `storage_api_contract.h` from behavior actually implemented by each
  component before publishing them as end-to-end guarantees.

Use the source-linked contracts in [`API.md`](API.md) and
[`ARCHITECTURE.md`](ARCHITECTURE.md) for current component details.
