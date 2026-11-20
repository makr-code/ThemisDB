---
Author: "ThemisDB Contributors"
Created: "2026-11-19"
Last Updated: "2026-11-20"
Status: approved
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# ADR DIS-05: Canonical 2PC Coordinator

## Status and scope

**Decision accepted 2026-11-19:** `TwoPhaseCommitCoordinator` is the canonical coordinator
for standalone 2PC. This ADR records the maintainer direction in the DIS-05 task; it is not
evidence that the adapter migration, recovery behavior, or release gates are complete.

This decision covers the named pair, `TwoPhaseCommitCoordinator` and
`CrossShardTransactionCoordinator`. `DistributedTransactionCoordinator` remains outside
DIS-05 and is not declared canonical by this ADR.

The consolidation is limited to that coordinator pair. It does not establish that the
coordinator decision WAL is replicated by per-shard Raft, that the separate
`DistributedTransactionCoordinator` RAID-1 example is integrated with this path, that
CrossShard 3PC decisions recover across leader changes, or that durable file-block
replication is completed before a client acknowledgement. Those end-to-end guarantees
remain unproven here and require their own integration and recovery evidence.

## Context

The named coordinators previously used
`include/sharding/detail/two_phase_commit_contract.h` for ordering but retained separate
participant dispatch, decision WAL, and recovery. The implementation now gives
`TwoPhaseCommitCoordinator` ownership of CrossShard 2PC participant dispatch and decision
replay. `CrossShardTransactionCoordinator` retains the public prepare/commit/abort lifecycle,
SSI validation, and all non-2PC protocols.

## Decision

- New standalone 2PC coordination uses `TwoPhaseCommitCoordinator`. Its decision record gates
  phase two; when WAL is configured, recovery re-drives unresolved decisions.
- Retain `CrossShardTransactionCoordinator` as the multi-protocol facade for 3PC, SAGA,
  Percolator, Calvin, and its associated transaction lifecycle and validation behavior.
- The CrossShard 2PC call path converges on the canonical coordinator through private staged
  prepare/commit/abort adapter operations. It does not write 2PC decisions to
  `TransactionWAL` or replay those decisions through CrossShard recovery.
- Canonical BEGIN records persist participant endpoints for restart-time phase-two dispatch.
  CrossShard startup first replays the canonical 2PC WAL. Unresolved legacy CrossShard 2PC
  records without a canonical decision record fail startup; there is no second-WAL fallback.
- Do not remove the CrossShard public API or its non-2PC protocols as part of this decision.
  Any API removal requires a separately reviewed migration and compatibility plan.
- This selection does not unify the coordinators' persisted WAL formats or claim
  cross-coordinator recovery. The adapter design must identify one authoritative durable
  decision/recovery path and must not silently fall back to a second WAL or recovery path.

## Implementation and verification gates

The adapter implementation is present, but DIS-05 remains in progress until focused build,
test, and platform evidence passes. Remaining gates:

1. Verify payload preservation, matching coordinator-issuer fencing tokens, SSI validation,
   and the public CrossShard lifecycle with tests.
2. Keep legacy CrossShard 2PC records fail-closed and document the operator migration path;
   this does not establish a general cross-coordinator recovery format or close DIS-06.
3. Complete production-path contract tests for both public entry points, including a
   decision-write failure injection, timeout/acknowledgement failure, duplicate delivery,
   and empty-input boundaries.
4. Verify that phase two never starts before a durable decision and a failed COMMIT
   acknowledgement is never rewritten as ABORT.
5. Record exact focused build/test commands and results. Do not claim unrun platforms pass.

## Consequences

- `TwoPhaseCommitCoordinator` is the recommended direct 2PC entry point.
- `CrossShardTransactionCoordinator` remains appropriate for its additional protocol and
  lifecycle features; its 2PC lifecycle is a facade over the canonical coordinator.
- DIS-05 remains in progress pending the listed tests and validation evidence. Do not describe
  the work as release-ready or claim an untested platform passes.

## Canonical references

- `audit/ROADMAP.md` — DIS-05 scope and milestone status.
- `include/sharding/detail/two_phase_commit_contract.h` — shared execution ordering.
- `include/sharding/two_phase_commit_coordinator.h` and
  `src/sharding/two_phase_commit_coordinator.cpp` — canonical standalone coordinator.
- `include/sharding/cross_shard_transaction.h` and
  `src/sharding/cross_shard_transaction.cpp` — retained multi-protocol facade and canonical
  2PC adapter.
- `tests/test_two_phase_commit.cpp` and `tests/sharding/CMakeLists.txt` — focused contract
  coverage and test registration.
