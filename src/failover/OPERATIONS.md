---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Failover Operations Guide

## Purpose

This guide describes the operational surfaces and safe operating boundaries for automatic failover and disaster recovery. Consult [RUNBOOK.md](RUNBOOK.md) for incident procedures.

## Signals and alert actions

The inspected failover runtime does not publish named Prometheus metrics or create trace spans. The architecture and Wave D evidence pages describe such instrumentation, but it is not present in the current `auto_failover_manager.cpp` or `disaster_recovery_manager.cpp`; do not configure alerts against those claimed series/spans until runtime instrumentation is verified. Use the manager's state/statistics, lifecycle events, and structured `spdlog` output, and ensure the server's configured admin surface makes these visible to operators.

| Signal | Source | Action |
|---|---|---|
| Current state, in-progress flag, last result, failing nodes | `AutoFailoverManager::getState()`, `isFailoverInProgress()`, `getLastFailoverResult()`, `getFailingNodes()` | Verify the detected failure and the current operation before submitting another request. |
| Failover success/failure, duration, retry/recovery totals | `AutoFailoverManager::getStatistics()` | Investigate sustained failures or retry growth; confirm success with a healthy replica and quorum. |
| Queue depth, dropped-full requests, queue-pressure events | `AutoFailoverManager::getStatistics()` and `QUEUE_PRESSURE` callbacks | Stop duplicate submissions; repair slow dependencies before retrying. |
| Quorum/partition/fencing and node lifecycle events | Registered `FailoverEventCallback` and structured `spdlog` messages | Treat quorum uncertainty, log append failure, or split-brain risk as promotion blockers. |
| DR state, result, failed step, and duration | `DisasterRecoveryManager::getState()`, `getStatistics()`, and `DisasterRecoveryResult::step_results` | Stop at the failed step and reconcile completed mutations before recovery resumes. |

These are in-process API surfaces, not metric names guaranteed to be scraped by an external backend. Configure alerts in the deployment's monitoring layer from exported status/events; do not assume an alert rule is built into this module.

## Routine checks

- Confirm the manager is running and the last result is understood.
- Track the `current_queue_depth`, `tasks_dropped_queue_full`, `failed_failovers`, retry totals, and `failed_runs` trends.
- Verify quorum, replica health, and fencing before planned failover/DR activity.
- Run a dry-run plan review before non-dry-run recovery, while remembering that dry run skips the mutation/restore phases.
- Retain quorum-log and DR step-result evidence through incident closure.

## Operating safeguards

- Defaults include three consecutive failures before action, a 5-second health-check call timeout, a 30-second quorum timeout, and two concurrent failovers. Tune only with owner review and after checking deployment-specific quorum and latency.
- Queue pressure is diagnostic, not permission to increase concurrency.
- Quorum/fencing/log append failures must remain fail-closed.
- Failed DR steps can leave prior steps applied. The manager does not provide a compensating rollback.
- Results with a non-empty `plan_id` are cached, including failures; do not replay a failed plan under the same ID and expect a retry.

## References

- [RUNBOOK.md](RUNBOOK.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- `include/failover/auto_failover_manager.h`
- `include/failover/disaster_recovery_manager.h`
