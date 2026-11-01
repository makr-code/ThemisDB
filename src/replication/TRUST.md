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
SOT_Domain: security
Canonical_References:
  - ../../include/replication/replication_manager.h
  - ../../include/replication/async_wal_shipper.h
  - ../../include/replication/replication_slot.h
  - ../../include/replication/logical_replication.h
  - ../../include/replication/geo_placement.h
  - ../../src/replication/ARCHITECTURE.md
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication trust boundaries and assumptions

This note maps trust-sensitive responsibilities to current APIs. It does not assert
that a component is an authoritative security boundary or that a deployment enforces
all documented safeguards.

## Trust-sensitive interfaces

- **Membership and role state:** `ReplicationManager` exposes replica and witness
  management, quorum/health queries, and role transitions. **Assumption:** callers
  provide authenticated, authorized membership changes. The module headers do not
  prove this across all call sites.
- **Placement:** `GeoReplicaPlacementManager` evaluates caller-provided replica
  snapshots and returns a candidate; it does not perform promotion. Candidate data
  freshness and promotion authorization remain caller responsibilities.
- **WAL shipper:** `AsyncWalShipper` bounds its queue and reports enqueue failure, but
  uses a supplied ship handler. The API does not establish authenticated transport,
  at-least-once delivery, or remote durability.
- **Logical slots:** slot APIs expose progress and persistence operations. Validate
  restart, replay, and retention effects in implementation/tests before relying on
  them for recovery.
- **Conflict handling:** resolver interfaces expose named strategies; determinism and
  merge semantics depend on the selected implementation and input.

## Explicit assumptions and TODOs

- **Assumption:** node identities, endpoints, and topology input are validated by the
  surrounding cluster/service layer.
- **Assumption:** the caller handles false results, errors, lag alerts, and queue
  saturation without silently treating them as success.
- **TODO:** Identify the authentication/authorization boundary for each caller of
  membership, promotion, slot, and transport APIs.
- **TODO:** Verify transport security, integrity/replay defenses, and persistence
  guarantees for each concrete deployment.
- **TODO:** Confirm implementation and runtime wiring for declared APIs that have no
  same-named implementation file under `src/replication/`.

For threat-reporting instructions, see the project-level [SECURITY.md](../../SECURITY.md).
