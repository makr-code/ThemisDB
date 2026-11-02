---
Author: "ThemisDB Contributors"
Created: "2026-11-02"
Last Updated: "2026-11-02"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

# Transaction module recovery contract

## Distributed decision durability

`DistributedTransactionManager` (DTM) must append its COMMIT decision and receive a
successful WAL flush before it sends Phase-2 COMMIT. If no WAL is configured, or
the append/flush fails, `commitDistributed()` returns an error, sends no Phase-2
COMMIT, and leaves the transaction PREPARED and visible as unresolved. ABORT
delivery is likewise held until the ABORT record is confirmed durable.
`WALManager::flush()` returns the flush result so callers can fail closed.
`GlobalTransactionManager` also requires a WAL before preparing participants,
and suppresses Phase-2 delivery when its decision append/flush is not confirmed.
Successful DTM completion is recorded separately from the durable decision so
restart recovery can distinguish a finished delivery from an unresolved one.
The shard-side `TwoPhaseCommitParticipant` synchronously persists each PREPARE
vote, including ABORT votes, before replying. COMMIT is acknowledged only after
the participant COMMIT record is flushed; a failed write leaves the participant
unavailable and the transaction unresolved. Participant recovery restores
prepared operations and reacquires their locks before serving requests. The
operation-application callback must be idempotent by transaction ID because a
crash after applying operations but before the participant COMMIT record can
cause the coordinator to redeliver its durable decision. The snapshot-isolation
Percolator path likewise requires a durable coordinator WAL and records its
decision before Phase 2.

The replication WAL loader also aborts recovery on a payload that cannot be
deserialized. At the shared `WALStorage` layer, only a physically incomplete
final record is truncated; malformed records, checksum failures, callback
aborts, and recovery timeouts make WAL open fail.

## Coordinator and recovery scope

`IRecoverableTwoPhaseCoordinator` is a coordinator-local reporting contract.
There are five current implementers in source (`TwoPhaseCommitCoordinator`,
`CrossShardTransactionCoordinator`, `DistributedTransactionCoordinator`,
`GlobalTransactionManager`, and `DistributedTransactionManager`). They retain
independent execution APIs and WAL formats. The contract does not make one
implementation canonical, make another implementation's WAL replayable, or
drive server readiness by itself. `GlobalTwoPhaseCommitRecoveryManager` returns
`ready_for_service=false` if any supplied coordinator is null or has unresolved
transactions after recovery. Runtime startup/readiness code must withhold
traffic unless that result is true; no current server startup path registers the
distributed coordinators with this manager.

The HTTP `/dtxn/*` API uses `DistributedTransactionCoordinator` with a durable
WAL below the configured storage WAL directory (or database directory when no
separate WAL directory is configured). Its constructor replays that WAL before
the server is constructed; WAL replay errors propagate and prevent startup.
`/health/ready` reports unavailable while that coordinator retains durable
recovery blockers after a failed replay or incomplete phase delivery, or while
its recovery snapshot cannot be read. Ordinary in-flight ACTIVE/PREPARING
transactions do not suppress readiness. Recovery does not treat an absent
participant ID or endpoint as a successful empty replay; the transaction
remains unresolved until it can be re-driven.
Readiness-triggered recovery retries only IDs already marked as unresolved, so
an active transaction's durable BEGIN record is never mistaken for a crashed
transaction and presumed-aborted during a live topology refresh.
At HTTP-server startup and when shard topology changes, the runtime refreshes
the coordinator's endpoint map from `ShardingManager` (`node_id` and
`node_address`, including numeric and `shard<node_id>`/`shard-<node_id>`
aliases) and periodically retries unresolved recovery from readiness checks.
Malformed participant
descriptors remain blocked rather than being dropped from the replay set.

This closes the startup/readiness gap for the HTTP distributed-transaction
coordinator only. The canonical coordinator across all internal APIs and a
complete independent coordinator/participant/replica crash matrix remain open
under [DIS-05/DIS-06](../../audit/ROADMAP.md#ws2--distributed-consistency--ha).
DTM WAL records now persist stable participant node IDs with BEGIN, PREPARE,
decision, and completion records. Endpoints, callback pointers, and affected
keys are deliberately excluded; recovery must resolve each node ID through
`recovery_endpoint_resolver`. Missing or malformed descriptors, unresolved
endpoints, failed delivery, and failed completion flushes leave the transaction
in-doubt. Legacy records without participant IDs are not guessed or silently
discarded.

## Evidence boundary

The focused transaction tests include a subprocess coordinator killed with
`SIGKILL` before Phase 1 starts, after participant PREPARE but before the
coordinator records the vote, after PREPARE, after durable decision, during
Phase 2, and after the commit ACK boundary. Recovery independently reads the
persisted participant WAL and checks the final decision. The participant WAL is
driven by an injected test transport for those coordinator scenarios. Additional
subprocess tests crash/restart the participant across prepare/apply/ack boundaries,
exercise replica-WAL recovery at prepare/decision/commit boundaries, and combine
coordinator, participant, and replica crashes in one scenario. These tests are
source coverage only until successfully built and executed; they do not prove
network replication or production storage-engine application. The Wave B workflow
uploads its output when executed; a successful CI run and artifact remain required
release evidence.
