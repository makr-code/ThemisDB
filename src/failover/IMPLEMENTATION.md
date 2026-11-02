---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-02"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Failover Implementation Guide

## Scope and source map

This guide is for developers changing the failover control plane. Public
interfaces are in `include/failover/`; implementation ownership is in
`src/failover/`.

| File | Responsibility |
|---|---|
| `auto_failover_manager.cpp` | Health/failure tracking, bounded request queue, worker lifecycle, quorum/fencing gates, replica promotion, callbacks, and statistics |
| `disaster_recovery_manager.cpp` | Plan validation, single-run guard, step sequencing, result caching, and aggregate statistics |
| `quorum_log.cpp` | Append-only pipe-delimited decision records with CRC32 and recovery of the last valid record |
| `topology_snapshot.cpp` | Deterministic sorted node snapshots and added/removed-node comparison |

## Automatic failover path

1. Construct `AutoFailoverManager` with its configuration and manager
   dependencies. `quorum_log_path` enables the optional local quorum log; startup
   recovery logs the last valid record but does not restore a manager epoch from
   that record.
2. `start()` launches monitoring and queue-worker threads. Manual requests use
   `triggerManualFailover()`; the monitor can also identify nodes after the
   configured failure threshold.
3. The bounded queue rejects requests when stopped or full. Queue pressure and
   dropped-request counts are exposed through statistics/events.
4. Processing requires a successful quorum check. Split-brain prevention
   requires a valid epoch from the configured fencing manager when the guard is
   enabled. Promotion selects a healthy non-witness replica and asks the
   replication manager to trigger failover.
5. Completion is checked with the replication manager's quorum result.

`stop()` notifies the worker and joins both threads directly. The deadline
calculations in the source do not impose a timeout on `join()`, so there is no
enforced maximum stop duration.

Operational boundary: spare activation requires the configured runtime hook to
perform the shard transition and then independently confirm `HotSpareManager`
state `ACTIVE`. Missing or failed activation emits `FAILOVER_DEGRADED`, increments
the degraded counter, and blocks failover completion. The built-in failover
metadata step appends to the configured quorum log and verifies the last record;
it fails closed when the log or fencing manager is unavailable.

## Disaster-recovery path

`executePlan()` validates the plan, rejects concurrent uncached execution, then
runs the seven steps in the order documented in [MODELING.md](MODELING.md).
The first failed step sets the result error and stops the sequence. A non-empty
plan ID caches both successful and failed results in memory; reuse returns the
cached result. Successful non-dry-run plans append a checksummed completion
record containing the plan fingerprint, fencing epoch, duration, and step
results before returning success. The manager replays these records at
construction. There is no automatic compensation for completed steps.

The built-in step implementations fail closed for non-dry-run operations that
need external effects or evidence:

- Snapshot validation fails unless an operation hook acknowledges and a separate
  verification hook reports the expected checksum.
- Restore fails unless an operation hook performs the storage restore and a
  separate verification hook confirms the expected checksum.
- Catch-up fails unless an operation hook acknowledges and a separate
  verification hook reports at least the required LSN in the required term;
  quorum alone is not catch-up evidence.
- Requested traffic shift fails unless an operation hook acknowledges and a
  separate verification hook confirms the recovery site. A plan that disables
  traffic shift skips that operation.
- Recovered-state verification fails unless separate hooks report matching
  checksum evidence.
- Built-in dry-run handlers skip their operation; prechecks return without
  querying quorum. Registered step hooks replace the corresponding handler.

Non-dry-run prechecks require quorum and fencing to remain enabled, actual
quorum, an available fencing manager, and a writable metadata log. The manager
compares returned proof fields against the plan, but production hooks remain
responsible for truthful external read-back. Processes using the same metadata
path are serialized through a persistent sibling lock; lock acquisition is
bounded by `metadata_lock_timeout`, and the completion record is flushed before
success. This relies on filesystem locking/sync semantics and does not provide
consensus-backed shared metadata or exactly-once external effects across a crash
before the completion record. Shared metadata and multi-process
failover/recovery E2E validation remain open.

## Quorum log format and failure behavior

`QuorumLog::append()` writes
`epoch|node_id|decision|timestamp_ms|crc32` and rejects `|` in either text
field. Open, write, or flush failure returns `false`. Recovery skips malformed,
unparseable, and CRC-mismatched lines; an absent or unreadable file yields an
empty state. CRC32 is an accidental-corruption check, not an integrity or
authentication control. The caller treats append failure as a quorum/promotion
blocker.

## Change validation

For behavioral changes, extend the corresponding focused tests under
`tests/failover/`. Include the relevant edge cases: missing manager, stale or
invalid fencing epoch, quorum loss, queue saturation, malformed/corrupt log
entry, dry-run, step failure, repeated plan ID, and partial completion. See
[MODULE_GUIDE.md](MODULE_GUIDE.md) for test target discovery.

## Related documentation

- [API.md](API.md) — public contract details
- [ARCHITECTURE.md](ARCHITECTURE.md) — module boundaries and failure domains
- [RUNBOOK.md](RUNBOOK.md) — operator response
