---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-22"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References: `../../include/sharding/sharding_api_contract.h`, `../../tests/sharding/CMakeLists.txt`
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Sharding Runbook

> **Scope limitation:** This is a recommended diagnostic sequence, not a set of verified commands
> or a deployment-specific recovery procedure. Error codes and recovery actions are component- and
> configuration-dependent. **TODO:** verify each action against the actual coordinator, transport,
> storage, and operator controls before production use.

## Purpose

This runbook is the operator-facing guide for the principal failure modes in the sharding module. It is grounded in the behavior encoded in `shard_router.cpp`, `cross_shard_transaction.cpp`, `wal_manager.cpp`, `health_monitor.cpp`, `auto_rebalancer.cpp`, and `sharding_api_contract.h`.

## Scenario 1: quorum loss

### Symptoms

- writes start failing with `QUORUM_LOST`,
- health metrics show missing replicas or degraded cluster state,
- routing decisions become limited or read-only by policy.

### Immediate actions

1. Confirm shard reachability and determine quorum count.
2. Check whether the primary and required replicas are reachable.
3. Inspect the coordinator and health monitor logs for failover decisions.
4. Stop write traffic to the affected shard family until quorum is restored.

### Recovery

- restore the required replica coverage,
- validate the topology and coordination state,
- resume writes only after quorum health is stable.

## Scenario 2: shard unavailable or read-only path

### Symptoms

- route-level failure indicates `SHARD_UNAVAILABLE`,
- health monitor marks the node as `DOWN` or `RECOVERING`,
- the client sees explicit routing or availability errors instead of silent fallback.

### Immediate actions

1. Check whether a node is disconnected or overloaded.
2. Verify the shard's health endpoint and failover action.
3. Confirm whether the request can be retried using the fallback route policy.
4. Ensure the fallback path remains inside the configured safety envelope.

### Recovery

- restore service at the failed shard,
- verify that the topology view is consistent,
- review the topology-change and rebalancer logs before returning to normal traffic.

## Scenario 3: 2PC in-doubt state

### Symptoms

- transactions remain in an unresolved `IN_DOUBT` state,
- participants block conflicting writes,
- coordinator recovery or WAL replay is required.

### Immediate actions

1. Confirm that the coordinator is the source of the ambiguity.
2. Inspect transaction WAL entries and participant state.
3. Determine whether the decision was interrupted before commit or abort.
4. Use the recovery path rather than force-committing a partially prepared transaction.

### Recovery

- Preserve the canonical `TwoPhaseCommitCoordinator` log and each participant's
  WAL. CrossShard uses `data_dir/two_phase_commit` for canonical decisions and
  `data_dir/wal` for lifecycle state. These are not replication WALs.
- Restore trusted `participant_endpoint_resolver` mappings (including TLS
  settings) for every persisted shard ID, then retry coordinator recovery.
  A durable COMMIT must be re-delivered as COMMIT; never replace it with ABORT.
  Only coordinator recovery may select durable ABORT when no decision exists.
- Participant timeouts are alerts, not leases. Do not delete WALs, force ABORT,
  release prepared locks or retry an uncertain business operation with a fresh
  transaction ID. An unreachable coordinator intentionally blocks 2PC.
- Inspect participant `ready`, `active_in_doubt`, `recovery_status` and
  `total_timeouts`; `recovery_complete` alone confirms replay/lock restoration,
  not resolution. Recovered/expired/failed-apply prepares reject new PREPARE
  but still accept terminal decisions and duplicates.
- Missing endpoints, incomplete acknowledgements or unresolved older
  CrossShard-only records remain recovery blockers; escalate rather than
  migrating or discarding them automatically. Before enabling the new path,
  quiesce the older coordinator and preserve all logs. Rollback before new
  2PC writes does not imply safe downgrade after activation: older binaries
  ignore the canonical log directory, even if its pending count is zero.
  Do not downgrade an activated coordinator without maintainer-reviewed state
  reconciliation/migration; preserve both logs and never delete the canonical
  directory as a rollback technique.
- Release blocked participants only after the authoritative terminal record is
  durable. Apply callbacks must deduplicate by transaction ID across crashes.

### DIS-10 multi-process evidence

- `TransactionProcessCrashRecoveryTests` proves the canonical
  coordinator/participant objects, WAL replay, locks and RocksDB apply callbacks
  across POSIX process boundaries.
- On 2026-11-21, a direct Linux diagnostic executable linked from the
  `test_dis10_real_transport_process_recovery.cpp` source and required production
  objects passed 3/3 real gRPC/mTLS subprocess cases: commit success,
  coordinator SIGKILL recovery, and participant crash/restart healing.
- On 2026-11-22, the same direct executable passed 3 repeated iterations:
  9/9 individual test executions.
- On 2026-11-22, the initial 3-case version of the focused Linux Debug CMake
  target `DIS10RealTransportProcessRecoveryTests` passed once and in 3/3 repeated
  CTest executions with real gRPC/mTLS subprocess traffic and durable WAL
  recovery checks.
- The suite now includes `RecoversCommitAfterTcpPartitionHeals`: a transparent
  test proxy black-holes both TCP directions after the durable coordinator
  decision; the participant PREPARE remains durable, phase two is incomplete,
  and WAL recovery completes after healing and process restart. The 4-test
  registered suite passed once plus 3 repeated runs.
- `RecoversCommitAfterAsymmetricTcpPartitionHeals` forwards coordinator
  requests but drops participant response bytes. The participant COMMIT is
  durable while the coordinator lacks phase-two completion; after healing, a
  fresh coordinator replays its WAL and the participant's duplicate COMMIT
  leaves one commit record. The 5-test registered suite passed once plus 3
  repeated CTest runs in Linux Debug with strict core dependency checks.
- `RecoversCommitAfterReverseAsymmetricTcpPartitionHeals` waits for the durable
  coordinator COMMIT decision, then drops coordinator-to-participant TLS request
  bytes. The run exits with `FAILURE`, leaves no coordinator complete record,
  and keeps participant B PREPARED with no COMMIT record. After healing and fresh
  recovery, participant B receives one COMMIT and has exactly one COMMIT WAL
  record; the coordinator has exactly one complete record. This sixth bounded
  case is Linux real-gRPC/mTLS evidence and does not close DIS-10.
- `RecoversCommitAfterCoordinatorRequestLatency` delays post-decision
  coordinator-to-participant TLS bytes beyond the RPC deadline and verifies
  recovery from the durable decision. `RecoversThreeParticipantCommitAfterParticipantCrash`
  crashes the third participant during phase two, restarts it, and checks
  coordinator/participant WAL consistency across a transaction spanning three
  participants. The latter is a terminal-outcome consistency check, not a
  serializability checker.
- `RecoversFourParticipantCommitAfterParticipantCrash` crashes the fourth
  participant during phase two in a five-process transaction, restarts it, and
  checks one durable coordinator decision/completion, one COMMIT per participant,
  and no contradictory ABORT. Together with the preceding cases, the suite
  registered target passed once and in three repeated CTest runs on 2026-11-22
  using Ubuntu 24.04, Linux Debug, and strict dependency checks.
- `RecoversFiveParticipantCommitAfterParticipantCrash` covers a coordinator
  plus five participant processes and crash/recovery of the fifth participant
  during phase two.
- `DuplicateCommitAndReorderedAbortPreserveFirstTerminalDecision` sends
  application-level duplicate COMMIT and reordered ABORT RPCs. The test verifies
  that COMMIT remains authoritative and the participant WAL has one COMMIT and
  zero ABORT entries. Raw TCP/TLS bytes are not reordered or duplicated.
- The Wave B transaction workflow now builds this Linux integration target and
  records three CTest repeats as a CI artifact. This wiring is not evidence that
  a workflow run has passed; retain the run URL and artifact before treating CI
  validation as satisfied.
- `DIS10ConcurrentSerializabilityTests` is a portable, in-process companion:
  four concurrent transactions read and write a shared key through two durable
  2PC participant objects. It records per-shard operation order, rejects a
  synthetic opposing-conflict cycle, and requires a topological serial replay
  to reproduce all observed reads. Wave B repeats it and validates a
  commit-linked JSON artifact; the release matrix runs the focused test on
  Linux AMD64/ARM64, Windows AMD64, and macOS ARM64, uploading one artifact per
  lane. These artifacts are not real-network or process-transport evidence.
- The portable target passed three repeated local CTest runs on 2026-11-22 in
  Ubuntu 24.04/Linux Debug. This confirms the local history checker and JSON
  evidence path only; successful CI artifacts and Windows/macOS lanes remain
  unverified.
- A recent Wave B run
  [45059649533](https://github.com/makr-code/ThemisDB/actions/runs/45059649533)
  ended `action_required` with zero jobs. No successful workflow artifact has
  yet been verified; rerun/approval and artifact inspection remain necessary.
- These are Linux-only test results, not evidence for Windows/macOS, release
  artifacts, Windows/macOS process coverage, 72-hour chaos, application-level
  RPC reordering, broader read/write history coverage, or general 3–5-node
  real-transport chaos. Release-matrix runs provide portable 2PC test coverage
  only and must be inspected before counting as platform evidence.
- The harness uses pipes only for process readiness/control; transaction
  traffic is real gRPC/mTLS, including through the TCP partition proxy.
- The proxy preserves TCP byte order; reordering or duplicating raw encrypted
  TLS bytes would corrupt the stream. Lost-ACK recovery already causes
  idempotent semantic COMMIT redelivery and checks that the participant WAL
  contains a single COMMIT record.
- Do not close a partition, transport or release-readiness incident based only
  on this target. Broader coordinator/participant/leader crash matrices, history
  validation and platform gates remain separate acceptance evidence.

## Scenario 4: migration conflict or rebalance fault

### Symptoms

- `MIGRATION_CONFLICT` or `MIGRATION_FAULT` is returned,
- shard movement or rebalance throughput falls below target,
- a partial or failed migration leaves the topology in a degraded state.

### Immediate actions

1. Confirm whether a second migration is targeting the same shard.
2. Inspect rebalance progress and rollback state.
3. Validate whether the source or target shard remains readable and writable.
4. Do not resume new migrations until the current fault is resolved.

### Recovery

- enforce serial execution for conflicting migration jobs,
- trigger rollback if the migration failed before verification,
- restore topology trust before allowing any new write traffic to rely on the new range assignment.

## Scenario 5: WAL corruption or replay failure

### Symptoms

- `WAL_CORRUPTION` is raised,
- replay stops operating on an ordered entry stream,
- the module requests operator intervention instead of continuing in uncertain state.

### Immediate actions

1. Stop automatic recovery procedures that assume the WAL is valid.
2. Retain the current WAL files before any repair attempt.
3. Validate the file integrity and checksum state.
4. Route the system into read-only or maintenance mode if required.

### Recovery

- repair or restore the WAL from a trusted checkpoint,
- replay only valid LSN ranges,
- re-enable traffic only after consistency checks pass.

## Operational notes

- health, quorum, and transaction checks should be treated as safety signals, not merely observability signals,
- error codes should be mapped to operator actions rather than hidden under generic error text,
- topology changes and repair jobs should be serialized when they touch the same shard group,
- degraded modes are acceptable only when the module remains explicit and fail-closed.

## Escalation

Escalate to the distributed systems or database reliability owner when any of the following is true:

- quorum cannot be restored within the configured service window,
- transaction in-doubt state persists beyond the allowed recovery timeout,
- migration rollback fails and leaves the topology uncertain,
- a WAL replay or corruption event affects multiple shards or a shared coordinator.
