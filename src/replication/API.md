---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: level1
SOT_Domain: api-contract
Canonical_References:
  - include/replication/replication_manager.h
  - include/replication/async_wal_shipper.h
  - include/replication/replication_slot.h
  - include/replication/logical_replication.h
  - include/replication/geo_placement.h
  - include/replication/replication_api_contract.h
  - tests/replication/test_replication_contract_hardening_focused.cpp
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: api-contract
Canonical references: `include/replication/replication_manager.h`, `include/replication/async_wal_shipper.h`, `include/replication/replication_slot.h`, `tests/replication/test_replication_contract_hardening_focused.cpp`
Related issues: #7373 (documentation synchronization); #7093 and #7167 are prior history.

# Replication API contract

## Public surfaces

- `ReplicationManager` uses `ReplicationConfig` and exposes replica and cluster-management behavior. The header defines `ReplicationMode` values `SYNC`, `SEMI_SYNC`, and `ASYNC`; consult its config and method declarations for exact operation semantics.
- Current lifecycle and operation entry points include `initialize()`, `shutdown()`, `replicate(const WALEntry&)`, `waitForReplication(sequence, timeout_ms)`, `addReplica()`, `removeReplica()`, `addWitnessNode()`, `triggerFailover()`, `promoteToLeader()`, `demoteToFollower()`, `hasQuorum()`, and placement-policy methods.
- `WALEntry` contains sequence/term and operation fields; `serialize()` returns bytes and `deserialize()` returns `std::optional<WALEntry>`.
- `AsyncWalShipper` accepts `WalSegment` values. `WalShippingConfig` has in-struct defaults (1,000 ms lag, queue depth 4,096, 16 histogram buckets); `enqueueSegment()` returns false when full. Its default handler does not send bytes; a transport handler must be supplied for actual delivery.
- `LogicalReplicationManager` exposes slot creation/listing, filtered `readChanges()`, `advanceSlot()`, and `recordDDLChange()`. `ReplicationSlot` exposes pause/resume/drop and confirmed-LSN advancement.
- `GeoReplicaPlacementManager` exposes `selectLeaderCandidate()`, `selectFailoverCandidate()`, and `validatePlacement()` over a caller-provided topology snapshot.
- `LagAlertManager` exposes `updateReplicaLag(s)`, `checkAndAlertLagViolations()`, `replicasEligibleForFailover()`, and metrics. Eligibility reporting is not, by itself, evidence of automatic manager failover integration.

## Caller obligations and failure cases

- Check the boolean/optional/result type actually returned by the called interface; absence of a decoded WAL entry is not a valid entry.
- Treat queue saturation as backpressure and inspect the shipper statistics rather than assuming enqueue succeeded.
- Keep mode-specific acknowledgements, health, lag, and placement constraints explicit in caller logic.
- Avoid relying on undocumented ordering or retry behavior; use the relevant interface and implementation as authority.
- Do not infer a supported YAML configuration mapping from the C++ configuration structs. Parser wiring for the module's `WalShippingConfig` and `SLOThresholds` is not established by the reviewed module source.

## Validation references

Use `tests/replication/test_replication_contract_hardening_focused.cpp` for contract cases and `test_replication_async_wal_lag_alerts.cpp` for queue/lag behavior. Targets are registered in `tests/replication/CMakeLists.txt`.

See `IMPLEMENTATION.md` and `MODULE_GUIDE.md` for source navigation and maintainer workflow.
