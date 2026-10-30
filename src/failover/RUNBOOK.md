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

# Failover Runbook

## Purpose and scope

Use this runbook for incidents in `AutoFailoverManager` and `DisasterRecoveryManager`. It covers promotion and recovery safety; it does not prescribe external traffic-manager, replication, or fencing procedures beyond their interfaces.

## Before changing state

1. Record the affected node/site, current failover or DR state, last result, and recent failover events.
2. Check replica health and quorum independently. Do not promote a node while quorum or fencing state is ambiguous.
3. Preserve the current topology and quorum-log evidence. If `quorum_log_path` is configured, confirm the log can be read and appended.
4. Use the deployment's authenticated admin control surface. The manager APIs are not a shell command interface.

## Incident procedures

### Failure detection or heartbeat alarms

**Signals:** `NODE_FAILURE_DETECTED`, `HEARTBEAT_MISSED`, `NETWORK_PARTITION_DETECTED`, failover state, `getFailingNodes()`, and statistics from `getStatistics()`.

1. Check node reachability and health-monitor results from more than one healthy location.
2. Distinguish an isolated node failure from a network partition. Check quorum before treating a missed heartbeat as a promotion decision.
3. If checks are transient, restore the network or health dependency and allow the configured monitoring cycle to re-evaluate; do not enqueue repeated manual promotions.
4. Escalate if the state remains `FAILED`, the manager is not running, or quorum cannot be established.

### Queue pressure or rejected failover request

**Signals:** `QUEUE_PRESSURE`, `current_queue_depth`, `max_queue_depth_observed`, `tasks_dropped_queue_full`, and `queue_pressure_events`.

1. Stop submitting duplicate manual requests; queue-full requests are rejected and are not silently retried.
2. Check whether an existing failover is progressing and whether its target remains healthy.
3. Restore the dependency causing slow processing. Retry only after queue pressure clears and the failed-node/topology state is revalidated.

The default configuration allows two concurrent failovers and emits queue pressure at 75% of that configured limit. Do not increase limits during an incident without the failover owner: concurrency can increase the promotion blast radius.

### Quorum, fencing, or promotion failure

**Signals:** `QUORUM_CHECK_FAILED`, `SPLIT_BRAIN_RISK_DETECTED`, a failed `FailoverResult`, quorum timeout, quorum-log append failure, or fencing-manager errors.

1. Keep the candidate from serving writes. A failed quorum check, unavailable fencing manager, or failed quorum-log append is a stop condition, not a bypassable warning.
2. Restore quorum and fencing dependencies; verify the quorum log is writable if configured.
3. Re-check the candidate's replication position and health against the current topology before another promotion attempt.
4. Require the failover owner to approve any manual action that changes leader or epoch state. Never disable split-brain prevention to clear an alert.

### Disaster-recovery plan failure

**Signals:** `DisasterRecoveryState`, `DisasterRecoveryResult.error_message`, and the ordered `step_results` (prechecks, snapshot validation, epoch fencing, restore, catch-up, traffic shift, verification).

1. Stop at the reported failed step. Do not start a second execution while the first is running; concurrent executions are rejected.
2. For plan-validation errors, correct the plan fields and validate the recovery site and snapshot before execution.
3. For an execution failure, inventory every completed step and any state it changed. Completed steps are not automatically rolled back.
4. A dry run performs prechecks but deliberately skips snapshot validation, fencing, restore, catch-up, traffic shift, and verification mutations; a passing dry run is not proof of a completed recovery.
5. After a failed execution, the result for a non-empty `plan_id` is cached. Reusing that ID returns the cached result; do not treat it as a retry. Assign a new plan ID only after reconciling partial state and obtaining recovery-owner approval.

## Recovery verification

Return service only when the leader/topology is unambiguous, quorum and fencing checks pass, the recovery site's state is verified, and the final failover/DR result is successful. Confirm the result and events through the manager's status/statistics callbacks and the server's configured admin surface.

## Rollback and escalation

There is no general automatic rollback for partially completed DR steps. Roll back only a specific configuration or topology change when the previous state is known safe and its data is reconciled. Escalate immediately for split-brain risk, lost quorum, quorum-log integrity/write failure, uncertain epoch state, or an incomplete DR restore; involve the failover owner and storage/replication owners before writes resume.

## References

- [OPERATIONS.md](OPERATIONS.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- `include/failover/auto_failover_manager.h`
- `include/failover/disaster_recovery_manager.h`
- `src/failover/auto_failover_manager.cpp`
- `src/failover/disaster_recovery_manager.cpp`
