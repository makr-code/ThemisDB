---
Author: "ThemisDB Team"
Created: "2026-11-06"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `src/failover/{auto_failover_manager.cpp,disaster_recovery_manager.cpp,quorum_log.cpp,topology_snapshot.cpp}`, `include/failover/`, `tests/failover/`, `benchmarks/failover/`
Related milestones: v2.5.0 (near-term), v3.0.0 (strategic)

# Failover Module Roadmap

> Root [`../../ROADMAP.md`](../../ROADMAP.md) remains authoritative for release
> readiness, release blockers, and sign-off. This module roadmap describes
> source-visible status and proposed work only. Wave completion requires the
> validation evidence defined below; source presence or historical wave labels
> are not evidence of a passing run.

## Current Status

**Assessment: implemented orchestration with significant production-evidence and
distributed-durability gaps; not yet demonstrated release-ready for multi-node
failover or disaster recovery.** This is an evidence-based assessment, not an
automated maturity score.

The inspected module surfaces include:

- `AutoFailoverManager`: health/failure tracking, bounded queue, quorum and
  fencing checks, failover state/events/statistics, promotion/spare hooks,
  optional local quorum log, topology snapshots, and adaptive health-check
  configuration.
- `DisasterRecoveryManager`: ordered plan validation and execution, quorum and
  epoch-fencing checks, adapter acknowledgement plus independent proof hooks,
  and a checksummed local completion log with same-path process locking.
- Focused failover tests are registered from `tests/failover/CMakeLists.txt`
  with `failover` and `release_critical` labels. Registration does not establish
  that they were run or passed.
- Three Google Benchmark executables are registered when benchmarks are
  enabled. Their workloads include in-memory mocks/simulations; benchmark
  source and declared thresholds are not measured results.

This assessment is grounded in the implementation inventory and limitations
below, and in [`AUDIT.md`](AUDIT.md). Neither source registration nor the
available source audit establishes a current test or benchmark pass.

### Research basis and code gap analysis

| Finding | Design implication | Evidence in this module / gap |
|---|---|---|
| Unreliable failure detectors can make mistakes in asynchronous systems; consensus depends on their completeness/accuracy properties and quorum assumptions, not a perfect timeout oracle. | Treat a missed heartbeat as suspicion, not proof; gate destructive action on quorum and fencing and exercise false suspicions/partitions. | The manager has configurable health checks, quorum and fencing paths. Representative network-partition tests and false-positive/false-negative rates are not established by source presence. |
| Raft separates leader election, log replication, and safety; randomized election timeouts reduce repeated election conflicts, while membership changes require overlap for safety. | Do not describe the current quorum checks as a complete consensus implementation. Select and verify a consensus-backed shared metadata authority before relying on multi-node DR decisions. | `AutoFailoverManager` and `QuorumLog` provide local orchestration/logging; the local log does not implement a consensus protocol or establish shared multi-node state. |
| SRE guidance describes overload and cascading-failure risks and recommends explicit load-management behavior. | Preserve bounded queues, make rejection/backpressure measurable, and validate recovery under queue/dependency saturation rather than only isolated hot paths. | Queue capacity and pressure events exist; published benchmarks are mostly synthetic and do not establish end-to-end behavior under dependency pressure. |
| NIST contingency-planning guidance calls for system-specific recovery priorities and tested plans. | Agree on service-specific RTO/RPO and recovery ownership before setting production thresholds; record achieved values from restore exercises. | DR steps verify checksums, term/LSN, and recovery-site evidence, but no approved RTO/RPO or representative restore measurement is documented here. |

Research references:

- Chandra and Toueg, “Unreliable Failure Detectors for Reliable Distributed
  Systems,” *Journal of the ACM* 43(2), 1996. [Cornell publication
  page](https://www.cs.cornell.edu/info/people/sam/FDpapers.html).
- Ongaro and Ousterhout, “In Search of an Understandable Consensus Algorithm
  (Raft),” USENIX ATC 2014. [USENIX paper
  page](https://www.usenix.org/conference/atc14/technical-sessions/presentation/ongaro).
- Google SRE, [Handling Overload](https://sre.google/sre-book/handling-overload/).
- NIST SP 800-34 Rev. 1, [Contingency Planning Guide for Federal Information
  Systems](https://csrc.nist.gov/pubs/sp/800/34/r1/upd1/final).

These sources inform design and validation choices; they do not prove this
module meets their properties or prescribe universal numeric SLOs.

## In Progress

- [ ] Confirm service-owner RTO/RPO, failover authority, and supported deployment
  topology before freezing release SLOs (Target: v2.5.0).
- [ ] Replace source-declared performance gates with recorded, reproducible
  representative-hardware baselines and enforceable percentile checks (Target:
  v2.5.0).

## Planned Features

- [ ] Enforce bounded cancellation/shutdown and dependency-hook deadlines,
  including explicit outcome reporting when a deadline expires (Target:
  v2.5.0).
- [ ] Select and integrate a consensus-backed shared metadata authority for
  multi-node DR, with documented consistency and recovery semantics (Target:
  v2.5.0).
- [ ] Add networked multi-process failover/DR validation, fault injection, and
  operator evidence capture (Target: v2.5.0).
- [ ] Specify versioned durable-state and membership-change semantics, including
  replay and migration behavior (Target: v3.0.0).
- [ ] Demonstrate multi-site recovery against approved RTO/RPO, sustained
  partition/overload scenarios, and operational audit requirements (Target:
  v3.0.0).

## Implementation Phases

1. **Phase 1 — Design / API contract:** agree failover authority, supported
   topologies, cancellation/deadline semantics, RTO/RPO ownership, and shared
   metadata consistency requirements before selecting an adapter.
2. **Phase 2 — Core implementation:** deliver the bounded lifecycle contract
   and shared metadata integration in v2.5.0; defer any public contract change
   until compatibility and migration impact are reviewed.
3. **Phase 3 — Error handling / edge cases:** handle unavailable quorum,
   partition and false suspicion, expired deadlines, stale fencing epochs,
   malformed/corrupt metadata, replay, and partially completed DR plans.
4. **Phase 4 — Tests:** add deterministic unit and integration coverage,
   multi-process/networked scenarios, and chaos/fault-injection tests; retain
   source-only claims until CI execution artifacts exist.
5. **Phase 5 — Performance / hardening:** collect representative-hardware
   p50/p95/p99 latency, recovery duration, queue pressure, and saturation
   results; enforce regression budgets only after a stable baseline exists.
6. **Phase 6 — Documentation / acceptance:** update API/operations guidance,
   record RTO/RPO exercise results and security/governance evidence, and obtain
   release-critical CI plus human sign-off before promoting status in the root
   roadmap.

## Release Plan

Effort estimates below are engineering person-weeks, exclude external
infrastructure procurement and waiting for cross-team decisions, and have
roughly ±40% uncertainty until the design phase is complete. Release targets
are planning assumptions, not commitments from root release governance.

### v2.5.0 — Near-term reliability and evidence closure

#### Wave 1 — Contract, baseline, and release gates

- **Priority:** P0 — prerequisite for release claims.
- **Goal:** define measurable failover/DR acceptance and make current tests and
  performance claims reproducible.
- **Tasks:**
  - [ ] Record supported topology, fault model, service-owner RTO/RPO, and
    expected failure authority (Target: v2.5.0).
  - [ ] Run focused failover tests and all three benchmark targets on a named
    representative host; save toolchain, configuration, workload, and raw
    outputs (Target: v2.5.0).
  - [ ] Correct the FWB-02 threshold inconsistency and add actual p99
    measurement/threshold enforcement or mark the cases informational (Target:
    v2.5.0).
- **Dependencies:** service owner and release engineering provide SLO input and
  a reproducible benchmark runner.
- **Risk / trade-off:** hardware and environment variance may make existing
  thresholds unusable; avoid weakening correctness gates to preserve a
  synthetic latency target.
- **Effort:** 1–2 person-weeks.
- **Success metric / validation:** CI `release_critical` failover suite passes;
  benchmark output includes host/build details and p50/p95/p99; every claimed
  threshold has an executable comparison. Do not claim a latency target until
  results exist.

#### Wave 2 — Bounded lifecycle and fail-closed recovery

- **Priority:** P0 — prevent false completion and unbounded waits.
- **Goal:** ensure shutdown and adapter waits have truthful, enforceable
  deadline/cancellation semantics.
- **Tasks:**
  - [ ] Implement and test a bounded `AutoFailoverManager::stop()` contract;
    remove any implication that calculating a deadline bounds `join()` unless
    the worker actually observes it (Target: v2.5.0).
  - [ ] Define cancellation/deadline behavior for synchronous DR hooks and
    distinguish timeout from adapter failure in results/events (Target:
    v2.5.0).
  - [ ] Verify incomplete/expired operations cannot report success or permit
    stale/unfenced writes (Target: v2.5.0).
- **Dependencies:** Wave 1 contract decisions; adapter API compatibility review.
- **Risk / trade-off:** hard cancellation of arbitrary synchronous callbacks is
  not generally safe; may require cooperative adapter cancellation or process
  isolation, rather than detaching workers or reporting false completion.
- **Effort:** 2–4 person-weeks.
- **Success metric / validation:** under blocked-worker and blocked-hook tests,
  configured deadlines are respected, shutdown returns the documented result,
  no worker is silently abandoned, and stale-epoch writes are rejected in all
  tested cases.

#### Wave 3 — Shared metadata and multi-node safety

- **Priority:** P0 — multi-node DR release blocker.
- **Goal:** close the gap between local-file durability and shared,
  multi-node failover/DR coordination.
- **Tasks:**
  - [ ] Select and integrate a supported consensus-backed/shared metadata
    adapter with explicit consistency, availability, and failure semantics
    (Target: v2.5.0).
  - [ ] Test restart/replay, concurrent writers, partial completion, corrupt
    records, and membership/topology changes through the selected adapter
    (Target: v2.5.0).
  - [ ] Prove independent epoch fencing blocks stale-primary writes during
    partition and promotion (Target: v2.5.0).
- **Dependencies:** Wave 1 fault/topology contract; compatible shared storage
  or consensus service and its operational owner.
- **Risk / trade-off:** stronger consistency can reduce write availability
  during a partition; local lock/file semantics cannot substitute for a
  distributed consensus guarantee.
- **Effort:** 4–8 person-weeks, excluding external adapter/service delivery.
- **Success metric / validation:** multi-process integration and partition
  fault-injection runs show zero dual-writer acceptance; DR completion is
  accepted only after read-back proof; results and adapter configuration are
  attached to CI evidence.

#### Wave 4 — Resilience, observability, and operator acceptance

- **Priority:** P1 — required for operational acceptance after Waves 1–3.
- **Goal:** establish production-usable recovery evidence and operator
  diagnostics for the supported topology.
- **Tasks:**
  - [ ] Exercise node, dependency, network, quorum, disk, and queue-pressure
    failures with deterministic recovery and retained evidence (Target:
    v2.5.0).
  - [ ] Expose actionable failover/DR outcomes, deadline/queue metrics, and
    correlated event/trace context through the supported observability
    integration (Target: v2.5.0).
  - [ ] Document preconditions, manual takeover, rollback/compensation limits,
    and restore verification in the runbook (Target: v2.5.0).
- **Dependencies:** Waves 1–3; agreed service SLOs and telemetry integration.
- **Risk / trade-off:** injected faults and lab results may not model
  production network/storage behavior; explicitly report tested topology and
  uncovered failure modes.
- **Effort:** 3–5 person-weeks.
- **Success metric / validation:** all fault scenarios have reproducible
  pass/fail outcomes, no unverified DR step reports success, and a recovery
  exercise records achieved RTO/RPO against owner-approved targets. Missing
  targets or missing CI evidence block release-readiness claims.

### v3.0.0 — Strategic distributed recovery architecture

#### Wave 5 — Versioned durable-state and membership contract

- **Priority:** P0 for the v3 architecture baseline.
- **Goal:** evolve beyond local replay semantics with an explicit, migratable
  cluster-state and membership model.
- **Tasks:**
  - [ ] Specify durable failover state, log ownership, membership transitions,
    replay/idempotency, and schema-version migration before changing formats
    (Target: v3.0.0).
  - [ ] Implement upgrade/downgrade and crash-recovery tests for supported
    metadata versions (Target: v3.0.0).
- **Dependencies:** v2.5.0 shared metadata design and compatibility review.
- **Risk / trade-off:** format or authority changes may be breaking and need
  staged rollout or a major-version migration procedure.
- **Effort:** 4–8 person-weeks.
- **Success metric / validation:** deterministic replay yields identical
  accepted state from the same committed history; supported migration paths
  pass crash-injection and compatibility tests.

#### Wave 6 — Multi-site recovery and RTO/RPO proof

- **Priority:** P0 for multi-site production qualification.
- **Goal:** validate planned site recovery under realistic latency, partition,
  and partial-failure conditions.
- **Tasks:**
  - [ ] Implement runbook-driven restore, catch-up, traffic shift, independent
    verification, and explicit compensation/manual-recovery outcomes (Target:
    v3.0.0).
  - [ ] Execute repeated cross-site drills and sustained partition/overload
    scenarios against the approved deployment topology (Target: v3.0.0).
- **Dependencies:** Waves 3 and 5; production-like staging, service owners, and
  approved RTO/RPO targets.
- **Risk / trade-off:** cross-site latency and asynchronous replication may
  prevent simultaneous zero data loss and fast recovery; disclose measured
  trade-offs rather than claiming both universally.
- **Effort:** 6–10 person-weeks.
- **Success metric / validation:** all required drills meet owner-approved
  RTO/RPO targets, independently verify restored content and replication
  position, and archive results by topology and fault scenario.

#### Wave 7 — Scale, governance, and release acceptance

- **Priority:** P1 — final qualification after Waves 5–6.
- **Goal:** establish repeatable operating limits and long-term evidence
  governance for larger supported deployments.
- **Tasks:**
  - [ ] Benchmark realistic node counts, failure bursts, queue saturation, and
    recovery concurrency on representative hardware; set scale limits from
    measured results (Target: v3.0.0).
  - [ ] Add security review for metadata integrity/access, audit retention,
    least-privilege adapters, and operator actions (Target: v3.0.0).
  - [ ] Publish compatibility, upgrade, runbook, and evidence-retention policy
    and complete release-critical review (Target: v3.0.0).
- **Dependencies:** Waves 1–6, stable deployment profile, and security/
  operations sign-off.
- **Risk / trade-off:** more observability and audit retention add storage and
  operational cost; establish retention and cardinality bounds.
- **Effort:** 4–8 person-weeks.
- **Success metric / validation:** published capacity envelope is backed by
  reproducible measurements; security/governance checks and required
  `release_critical` suites pass on the target branch; human release sign-off
  is recorded.

## Production Readiness Checklist

- [ ] Owner-approved topology, failover authority, and RTO/RPO documented.
- [ ] Shutdown and adapter deadlines are enforced and tested, not merely
  calculated or described.
- [ ] Shared metadata/consensus and fencing behavior validated across processes
  and network partitions.
- [ ] Failover and recovery chaos tests pass with retained CI evidence.
- [ ] Representative-host benchmark baselines and enforceable gates are
  published; synthetic results are clearly distinguished.
- [ ] Operator runbook, observability, recovery drill, and security/governance
  review are complete.
- [ ] Root roadmap and release governance reflect the current evidence and
  required human approval.

## Known Issues & Limitations

- `AutoFailoverManager::stop()` calculates deadline timestamps but then calls
  blocking `join()` without enforcing those deadlines.
- A timed-out health-check future can continue running; later monitoring cycles
  are skipped until that future completes.
- `target_promote_id` is queued but ignored when selecting a promotion
  candidate.
- Quorum-log recovery is logged, not applied as restored manager state.
- Required spare activation depends on a runtime hook and independent `ACTIVE`
  read-back; failure blocks completion and records a degraded outcome.
- DR completion records use a local checksummed file and a same-path lock.
  This does not establish consensus-backed shared durability, network-filesystem
  guarantees, mixed-version writer safety, or certified power-loss behavior.
- DR hooks are synchronous and not bounded/cancelled by the manager; completed
  steps are not automatically compensated after a later step fails.
- Runtime does not currently export named Prometheus metrics or trace spans.
- Benchmarks include mocks/simulations; the benchmark documentation records
  inconsistent FWB-02 declared thresholds and no configured p99 assertion.
  No measured latency compliance is claimed.
- The inspected test/build evidence does not establish successful current CI,
  representative hardware behavior, or genuine networked multi-process
  failover/DR operation.

## Breaking Changes

No breaking change is scheduled or approved. A shared metadata format,
consensus authority, cancellation contract, or public adapter API introduced
for v3.0.0 may require a major-version migration plan; identify and approve
that impact before implementation. Do not add a compatibility path without
explicit maintainer approval and a removal target.

## References

- [`API.md`](API.md) · [`ARCHITECTURE.md`](ARCHITECTURE.md) ·
  [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- [`tests/failover/CMakeLists.txt`](../../tests/failover/CMakeLists.txt)
- [`benchmarks/failover/README.md`](../../benchmarks/failover/README.md)
- Public contracts: `include/failover/auto_failover_manager.h`,
  `include/failover/disaster_recovery_manager.h`,
  `include/failover/quorum_log.h`, `include/failover/failover_api_contract.h`
