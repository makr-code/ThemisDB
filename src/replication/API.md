---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
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
Canonical references: `include/replication/replication_manager.h`, `include/replication/async_wal_shipper.h`, `include/replication/replication_slot.h`, `tests/replication/test_replication_contract_hardening_focused.cpp`
Related issues: #7093, #7167

# Replication API contract

## Public surfaces

- `ReplicationManager` uses `ReplicationConfig` and exposes replica and cluster-management behavior. The header defines `ReplicationMode` values `SYNC`, `SEMI_SYNC`, and `ASYNC`; consult its config and method declarations for exact operation semantics.
- `WALEntry` contains sequence/term and operation fields; `serialize()` returns bytes and `deserialize()` returns `std::optional<WALEntry>`.
- `AsyncWalShipper` accepts `WalSegment` values, with queue depth and lag configuration in `WalShippingConfig`. The documented full-queue behavior is `enqueueSegment()` returning false.
- Slot, logical replication, and placement APIs are separate interfaces under `include/replication/`.

## Caller obligations and failure cases

- Check the boolean/optional/result type actually returned by the called interface; absence of a decoded WAL entry is not a valid entry.
- Treat queue saturation as backpressure and inspect the shipper statistics rather than assuming enqueue succeeded.
- Keep mode-specific acknowledgements, health, lag, and placement constraints explicit in caller logic.
- Avoid relying on undocumented ordering or retry behavior; use the relevant interface and implementation as authority.

## Validation references

Use `tests/replication/test_replication_contract_hardening_focused.cpp` for contract cases and `test_replication_async_wal_lag_alerts.cpp` for queue/lag behavior. Targets are registered in `tests/replication/CMakeLists.txt`.

See `IMPLEMENTATION.md` and `MODULE_GUIDE.md` for source navigation and maintainer workflow.
