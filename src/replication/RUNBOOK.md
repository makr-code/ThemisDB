---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - ../../include/replication/replication_manager.h
  - ../../include/replication/async_wal_shipper.h
  - ../../include/replication/lag_alert_manager.h
  - ../../include/replication/replication_slot.h
  - ../../include/replication/logical_replication.h
  - ../../src/replication/replication_manager.cpp
  - ../../src/replication/async_wal_shipper.cpp
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication component diagnostic checklist

> **Scope:** This is a developer-facing diagnostic checklist based on module APIs, not
> a production recovery procedure. No commands, service endpoints, restore steps, or
> deployment-specific escalation paths are verified here.

## WAL shipping queue or lag

1. Read `AsyncWalShipper::stats()` and `currentLagMs()`.
2. Check the `enqueueSegment()` result; `false` means queue capacity was reached.
3. Check whether a real handler was installed with `setShipHandler()`; the default is
   a no-op accounting handler.
4. Inspect the handler's success/failure behavior and callback/metric output in the
   implementation before retrying or dropping work.

## Replica health or promotion

1. Inspect the `ReplicationManager` replica snapshot and health/quorum APIs.
2. Check the configured mode, health timeouts, and placement policy.
3. Treat `triggerFailover()`, `promoteToLeader()`, and
   `GeoReplicaPlacementManager` candidate selection as explicit APIs; candidate
   selection does not itself perform promotion.
4. Confirm failure and rollback behavior against the manager implementation and
   focused tests before using these calls operationally.

## Slot or logical stream progress

1. Inspect slot state, confirmed LSN, and lag through `ReplicationSlot`/
   `ReplicationSlotManager`.
2. Check the return result of `pause()`, `resume()`, `drop()`, and `advance()`.
3. For logical changes, follow `readChanges()` and `advanceSlot()` API comments and
   validate persistence/replay semantics in source and tests.

## Unverified production procedure

**TODO:** Maintainers must add deployment-specific diagnosis commands, recovery and
rejoin procedures, data-integrity checks, rollback rules, escalation contacts, and
validated operational thresholds. The APIs cited above do not substantiate those
procedures.
