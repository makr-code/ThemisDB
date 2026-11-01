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
  - ../../include/replication/raft_v2.h
  - ../../include/replication/async_wal_shipper.h
  - ../../include/replication/lag_alert_manager.h
  - ../../include/replication/logical_replication.h
  - ../../include/replication/geo_placement.h
  - ../../src/replication/
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication design notes

## Source-backed questions

### What control operations are declared?

`ReplicationManager` declares initialization/shutdown, replication and wait operations,
replica and witness management, promotion/failover methods, quorum/health queries,
read-preference operations, and placement-policy methods. `RaftV2ClusterConfig` and
`MembershipChangeManager` expose joint-consensus configuration transitions. Callers
must consult implementation and tests for exact conditions and side effects.

### What replication and lag controls exist?

`ReplicationMode` declares `SYNC`, `SEMI_SYNC`, and `ASYNC`; `ReplicationConfig`
contains timeouts, batching, WAL, quorum, failover, read, and security-related fields.
These are API declarations and defaults, not proof of deployed consistency guarantees
or an external configuration parser.

`AsyncWalShipper` uses a bounded segment queue and caller-provided transport handler.
`LagAlertManager` stores caller-reported lag and evaluates configured thresholds when
asked. Their headers do not establish end-to-end remote durability or automatic
integration of lag eligibility with manager failover.

### What logical-stream behavior is exposed?

`LogicalReplicationManager` declares filters, slot APIs, logical changes, DDL
recording, and change reads. `ReplicationSlot` exposes persistent slot state and LSN
progress. Exact replay, delivery, and retention guarantees should be established from
implementation and focused tests, not inferred from feature names.

## Assumptions requiring validation

- **Assumption:** the surrounding server or caller authenticates peers and authorizes
  membership, failover, and slot-control actions.
- **Assumption:** caller-provided transport handlers implement the intended retry,
  integrity, encryption, and delivery policy.
- **TODO:** Verify runtime wiring and support status for declared interfaces without a
  same-named implementation file under `src/replication/`.
- **TODO:** Verify the user-facing configuration schema and the consistency semantics
  observed by integrated callers.
- **TODO:** Attach current test and benchmark execution evidence before making
  readiness or performance claims.

For implementation facts, use
[architecture](ARCHITECTURE.md), [API notes](API.md), and the listed headers/sources as
the authority. This note does not make production-readiness claims.
