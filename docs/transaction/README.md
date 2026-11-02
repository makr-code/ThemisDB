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

The canonical runtime coordinator and a complete coordinator/participant/
replication startup-readiness integration remain open under
[DIS-05/DIS-06](../../audit/ROADMAP.md#ws2--distributed-consistency--ha).
DTM WAL records now persist stable participant node IDs with BEGIN, PREPARE,
decision, and completion records. Endpoints, callback pointers, and affected
keys are deliberately excluded; recovery must resolve each node ID through
`recovery_endpoint_resolver`. Missing or malformed descriptors, unresolved
endpoints, failed delivery, and failed completion flushes leave the transaction
in-doubt. Legacy records without participant IDs are not guessed or silently
discarded.

## Evidence boundary

The focused transaction tests include a subprocess coordinator killed with
`SIGKILL` before PREPARE completion, after PREPARE, after durable decision,
during Phase 2, and after the commit ACK boundary. Recovery independently reads
the persisted participant WAL and checks the final decision. The participant
WAL is driven by an injected test transport inside the coordinator child, so
this is not evidence of participant- or replica-process crashes or a multi-node
network-chaos run. The Wave B workflow uploads its output when executed; a
successful CI run and artifact remain required release evidence.
