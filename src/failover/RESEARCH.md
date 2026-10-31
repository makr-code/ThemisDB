---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Failover Research Notes

## Purpose and evidence

This note records the engineering basis for failover decisions and how to validate
them. It is for maintainers assessing failure-detection and recovery changes, not
a claim of a probabilistic failure predictor. The behavior described here is
implemented in [`auto_failover_manager.cpp`](auto_failover_manager.cpp) and
[`disaster_recovery_manager.cpp`](disaster_recovery_manager.cpp), with public
contracts in `../../include/failover/`.

## Decision method and assumptions

`AutoFailoverManager` tracks health outcomes and consecutive failures, applies the
configured `consecutive_failures_before_action` threshold (default 3), then places
eligible requests on a bounded queue. Promotion is gated by the configured
replication manager's quorum result and, when enabled, an epoch-fencing check.
Candidate selection uses replica health and excludes witness replicas. These
signals assume that the injected health, replication, and fencing managers report
the current cluster state; this module does not independently establish that
their observations are fresh or mutually consistent.

Disaster recovery is an ordered, deterministic step pipeline. It validates plan
identifiers, then evaluates prechecks, snapshot-ID presence, fencing, restore,
catch-up, traffic shift, and verification. A failure stops later steps, but
completed side effects are not compensated. In the current implementation,
snapshot validation does not query a snapshot store, and built-in restore and
traffic-shift steps do not invoke external adapters.

## Evaluation path

Use focused tests to validate decisions and boundaries:

- `tests/failover/test_failover_wave_a_health_timeout.cpp` for health-check
  deadlines and timeout diagnostics.
- `tests/failover/test_failover_wave_a_quorum_persistence.cpp` for quorum-log
  append/recovery behavior.
- `tests/failover/test_failover_wave_a_fencing.cpp` and
  `tests/failover/test_failover_wave_c_fencing_security.cpp` for fencing and
  promotion rejection.
- `tests/failover/test_failover_dr_edge_scenarios.cpp` for plan validation,
  dry-run, step failure, and statistics.
- `tests/failover/test_failover_wave_a_topology_versioning.cpp` for topology
  snapshots and change detection.

Benchmarks under `benchmarks/failover/` measure selected in-process operations.
For example, `bench_failover_wave_b_gates.cpp` explicitly models quorum-log
serialization without file I/O. Such microbenchmarks do not establish end-to-end
promotion or recovery latency. A deployment evaluation must include its real
health, replication, fencing, storage, and traffic-management integrations.

## Trade-offs and limits

- Conservative quorum/fencing gates reduce split-brain risk but can leave the
  cluster unavailable when dependencies cannot establish a safe decision.
- A bounded queue makes overload visible through rejection and counters, but
  rejected requests require operator diagnosis; they are not silently retried.
- Quorum-log CRC32 detects some accidental record corruption, not malicious
  modification or writer identity.
- Existing unit and stress tests establish specific code-path behavior; they do
  not prove correctness under every network partition or deployment topology.

## Related documentation

- [MODELING.md](MODELING.md) — decision variables and state transitions
- [IMPLEMENTATION.md](IMPLEMENTATION.md) — runtime behavior and limitations
- [RUNBOOK.md](RUNBOOK.md) — operator actions for unsafe or failed outcomes
- [ARCHITECTURE.md](ARCHITECTURE.md) — module boundaries
