---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Failover Operational Requirements and Implementation Boundaries

> **Status:** Operator guidance, not an enforced production-readiness certification.
> This page is subordinate to root [`ROADMAP.md`](../../ROADMAP.md) for aggregate
> maturity and release status. Runtime behavior is defined by the public headers
> and `src/failover/*.cpp`.

## Enforced behavior in this module

- Automatic failover requires an available `ReplicationManager` to establish quorum
  and request promotion. The configured queue size, health-check timeout, failure
  threshold, quorum timeout, retry interval, and recovery-attempt count are read
  from `AutoFailoverConfig`.
- When split-brain prevention is enabled, `preventSplitBrain()` fails closed if
  the fencing manager is missing or returns epoch 0. Replica selection also
  requests an epoch bump when a fencing manager is present.
- A configured `quorum_log_path` enables local append/recovery records. Write or
  flush failure blocks the corresponding quorum/promotion path. Startup logs the
  last valid record; it does not restore manager epoch or quorum state.
- `DisasterRecoveryManager::validatePlan()` requires non-empty plan, primary-site,
  and recovery-site IDs; non-dry-run plans also require a non-empty snapshot ID.
  Named plan results (success or failure) are cached in memory for that manager
  instance; empty IDs are not cached. Concurrent uncached execution is rejected.
- DR steps run in fixed order and stop at first failure. There is no compensation
  for previously completed steps.

## Integration gaps operators must account for

- The built-in snapshot check tests ID presence only. Restore and traffic-shift
  handlers report success without calling storage or traffic adapters.
- Catch-up checks quorum, not replication position. Verification checks for at
  least one healthy node and, if configured, quorum; it does not validate restored
  contents or recovery-site identity.
- A configured spare manager causes `activateSpareIfNeeded()` to emit
  `SPARE_ACTIVATION_FAILED` and return false; the main failover path ignores that
  return value. Metadata update logs IDs and returns true without writing metadata.
- A dry run skips built-in prechecks and operational steps. The
  `allow_dry_run_without_managers` configuration field does not change this
  behavior in the current implementation. Step hooks replace the built-in handler
  and may have side effects.
- `stop()` calls `std::thread::join()` directly. Although comments describe
  deadlines, there is no timed join or enforced upper bound in the implementation.
- Failover statistics, callbacks, and `spdlog` messages are in-process/runtime
  surfaces. This module does not publish named Prometheus metrics or trace spans.

## Deployment safeguards (recommendations)

These are deployment recommendations, not validations performed by the module:

- Keep quorum checks and epoch fencing enabled for write-capable production
  failover. Do not treat missing fencing or uncertain quorum as an override
  opportunity.
- Choose a persistent, access-controlled location for `quorum_log_path` if local
  decision records are needed. CRC32 detects some accidental corruption; it is
  not authentication or tamper resistance.
- Place manager APIs behind the deployment's authenticated administrative
  boundary. The failover classes do not authenticate callers or authorize plans.
- Integrate actual restore, traffic movement, metadata updates, spare activation,
  and restored-state validation through trusted adapters, and verify those effects
  independently before serving writes.
- Set timeouts and retry/queue limits for the deployment. The constructor does
  not validate all configuration combinations or certify them as safe.
- Run focused tests and representative deployment-level failure tests. Presence
  of benchmark targets or tests is not evidence that they passed or that production
  SLOs are met.

## Source references

- `include/failover/auto_failover_manager.h`
- `include/failover/disaster_recovery_manager.h`
- `include/failover/quorum_log.h`
- `src/failover/auto_failover_manager.cpp`
- `src/failover/disaster_recovery_manager.cpp`
- [`ARCHITECTURE.md`](ARCHITECTURE.md), [`API.md`](API.md),
  [`RUNBOOK.md`](RUNBOOK.md), and [`TRUST.md`](TRUST.md)
