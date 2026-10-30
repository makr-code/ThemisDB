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
SOT domain: module-behavior
Canonical references: `include/replication/replication_manager.h`, `include/replication/async_wal_shipper.h`, `src/replication/replication_manager.cpp`, `src/replication/async_wal_shipper.cpp`, `tests/replication/test_replication_async_wal_lag_alerts.cpp`
Related issues: #7093, #7167

# Replication implementation

## Purpose and boundary

For replication maintainers changing leader/replica management or asynchronous cross-region WAL shipping. The public declarations and runtime behavior are split across multiple components; this page does not claim that all replication modes have identical acknowledgement semantics.

## Implementation map

- `replication_manager.cpp` implements the manager declared in `replication_manager.h`, including replica/role/health and replication configuration surfaces.
- `async_wal_shipper.cpp` implements the bounded queue and worker declared in `async_wal_shipper.h`.
- Logical replication, slots, placement, and conflict resolution have separate headers and source files; trace the matching pair for those changes.

## Invariants and failure cases

- Preserve WAL sequence/term information and the declared serialization/deserialization result behavior.
- `AsyncWalShipper` applies backpressure: its header specifies `enqueueSegment()` returns false when `max_queue_depth` is reached; keep queue-full accounting and alert/statistics behavior consistent.
- Keep synchronous, semi-synchronous, and asynchronous modes distinct; do not describe one mode’s acknowledgement behavior as universal.
- Health, lag, and placement outcomes must be handled through the relevant component’s declared result/status rather than silently treated as equivalent.

## Change and validation steps

1. Identify whether the change is manager, shipper, slot, logical stream, or placement behavior.
2. Read the corresponding public header and implementation before changing state or callback behavior.
3. Use `tests/replication/test_replication_async_wal_lag_alerts.cpp` for shipper/lag changes and `test_replication_contract_hardening_focused.cpp` for contract behavior.
4. Run the affected target registered in `tests/replication/CMakeLists.txt`.

See `API.md`, `MODULE_GUIDE.md`, and `RUNBOOK.md` for callers and operations.
