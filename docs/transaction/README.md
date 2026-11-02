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
drive server readiness. `GlobalTwoPhaseCommitRecoveryManager` aggregates one
recovery call per supplied coordinator; a non-zero `in_doubt_after` means the
reported pass remains unresolved, not ready.

The canonical runtime coordinator and a complete coordinator/participant/
replication startup-readiness integration remain open under
[DIS-05/DIS-06](../../audit/ROADMAP.md#ws2--distributed-consistency--ha).
DTM WAL records do not currently persist enough participant/acknowledgement
state to establish restart-time re-delivery and completion for every decision;
startup reconciliation must therefore not infer readiness from an empty
in-memory transaction snapshot alone.

## Evidence boundary

The focused transaction tests use in-process callbacks and WAL files. They are
unit-level behavior checks, not process-crash or multi-node evidence. A genuine
multi-process crash/network-chaos suite (at least three nodes), including
readiness blocking and independently inspected durable outcomes, remains a
follow-up release gate; no such evidence is claimed here.
