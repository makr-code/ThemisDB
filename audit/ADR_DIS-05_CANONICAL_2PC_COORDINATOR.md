---
Author: "ThemisDB Contributors"
Created: "2026-11-19"
Last Updated: "2026-11-19"
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

## Context

Both named coordinators use
`include/sharding/detail/two_phase_commit_contract.h` for prepare, decision-persistence,
and phase-two ordering. They retain separate participant transports, transaction lifecycles,
WAL formats, and recovery behavior. `TwoPhaseCommitCoordinator` can re-drive a recorded
decision from its WAL; `CrossShardTransactionCoordinator::recoverFromWAL()` restores state
and reports resume candidates without automatically re-driving phase two.

## Decision

- New standalone 2PC coordination uses `TwoPhaseCommitCoordinator`. Its decision record gates
  phase two; when WAL is configured, recovery re-drives unresolved decisions.
- Retain `CrossShardTransactionCoordinator` as the multi-protocol facade for 3PC, SAGA,
  Percolator, Calvin, and its associated transaction lifecycle and validation behavior.
- The CrossShard 2PC call path must converge on the canonical 2PC coordinator through an
  explicit adapter; it must not evolve as a second independent 2PC implementation.
- Do not remove the CrossShard public API or its non-2PC protocols as part of this decision.
  Any API removal requires a separately reviewed migration and compatibility plan.
- This selection does not unify the coordinators' persisted WAL formats or claim
  cross-coordinator recovery. The adapter design must identify one authoritative durable
  decision/recovery path and must not silently fall back to a second WAL or recovery path.

## Implementation and verification gates

The codebase does not yet satisfy the adapter decision: the CrossShard 2PC path still runs
its own participant and persistence callbacks through the shared execution helper. Before
counting DIS-05 complete, the implementation must:

1. Define how the adapter preserves participant operation payloads, fencing tokens, and
   CrossShard validation/lifecycle requirements.
2. Resolve how existing CrossShard WAL/snapshot state maps to the canonical coordinator's
   durable decision and replay behavior. Do not treat this as resolved by the choice of
   coordinator; coordinate with the DIS-06 recovery contract where needed.
3. Add production-path contract tests for both public entry points covering successful
   commit, prepare rejection/abort, decision-write failure, participant timeout or
   acknowledgement failure, duplicate delivery, and relevant empty-input boundaries.
4. Verify that phase two never starts before the selected decision is durably recorded and
   that a failed COMMIT acknowledgement is never rewritten as ABORT.
5. Record exact focused build/test commands and results. Do not claim unrun platforms pass.

## Consequences

- `TwoPhaseCommitCoordinator` is the recommended direct 2PC entry point.
- `CrossShardTransactionCoordinator` remains appropriate for its additional protocol and
  lifecycle features, but its 2PC execution is not yet a completed adapter.
- Until the adapter and recovery gates pass, DIS-05 remains in progress and the existing
  CrossShard 2PC path must not be described as fully consolidated.

## Canonical references

- `audit/ROADMAP.md` — DIS-05 scope and milestone status.
- `include/sharding/detail/two_phase_commit_contract.h` — shared execution ordering.
- `include/sharding/two_phase_commit_coordinator.h` and
  `src/sharding/two_phase_commit_coordinator.cpp` — canonical standalone coordinator.
- `include/sharding/cross_shard_transaction.h` and
  `src/sharding/cross_shard_transaction.cpp` — retained multi-protocol facade and current
  adapter gap.
- `tests/test_two_phase_commit.cpp` and `tests/sharding/CMakeLists.txt` — focused contract
  coverage and test registration.
