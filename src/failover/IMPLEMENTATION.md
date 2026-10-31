---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
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

Operational boundary: current spare handling does not activate a spare. If a
spare manager is supplied, the method emits `SPARE_ACTIVATION_FAILED` and returns
`false`; the caller does not use this return value to fail the overall
failover. Metadata handling currently logs the old/new leader pair and returns
success without writing cluster metadata. A successful `FailoverResult` is
therefore not evidence that either operation occurred.

## Disaster-recovery path

`executePlan()` validates the plan, rejects concurrent uncached execution, then
runs the seven steps in the order documented in [MODELING.md](MODELING.md).
The first failed step sets the result error and stops the sequence. A non-empty
plan ID caches both successful and failed results in memory; reuse returns the
cached result. There is no automatic compensation for completed steps.

The built-in step implementations have important integration limits:

- Snapshot validation checks only that `snapshot_id` is non-empty.
- Restore reports success for the ID; it does not call a storage restore API.
- Traffic shift reports success for `recovery_site`; it does not call a traffic
  manager.
- Catch-up waits for quorum, not a replication-position or data-consistency
  condition.
- Verification checks for any healthy node and, when configured, quorum. It
  does not verify the recovery site or restored contents.
- Built-in dry-run handlers skip their operation; prechecks return without
  querying quorum. Registered step hooks replace the corresponding handler.

Do not use these status results as proof of completed storage recovery or a real
network/traffic cutover. Production integrations must implement those effects
and verify their results at the relevant adapter boundary.

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
