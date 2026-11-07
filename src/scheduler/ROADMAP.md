---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-07
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot; OpenAI API assistant
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: include/scheduler/*.h; src/scheduler/*.cpp; tests/scheduler/*.cpp; benchmarks/scheduler/*.cpp
Related issue: #7449
---

Exact model backend not verifiable from final document text alone.

# Scheduler module roadmap

This is a research-informed engineering plan, not a release-readiness statement
or a delivery-date commitment. `v2.5.0` and `v3.0.0` are planning horizons;
product version and release status remain governed by the root release documents.
Implementation claims require production-path tests and benchmark evidence.

## Current Status

### Maturity assessment

The module has **moderate functional breadth, but incomplete operational
validation**. It provides task lifecycle and execution, local/distributed
coordination, changefeed triggers, external-scheduler conversion, retention,
audit, result storage, and anomaly components. A shared coordinator gate now
denies dispatch until local leadership and coordinator health are valid and
closes on loss of either condition. Authorization and event-predicate handling
also have fail-closed paths. These are source-visible controls, not evidence of
deployment certification or cluster-scale capacity.

The evidence base is uneven:

- The integration suite exercises production task lifecycle behavior, while the
  coordination-failure suite skips its scenarios and some burst/retention cases
  remain simulated or placeholders.
- `bench_task_scheduler.cpp` calls production `TaskScheduler` APIs. Its registered
  CTest is a smoke check; no agreed hardware baseline, statistic, owner, or
  scheduler performance threshold is established. Other “gate” benchmark labels
  do not measure production task lifecycle.
- Security and deployment owners have not yet approved the required audit,
  key-provisioning, retention, authorization propagation, sandbox, timeout, and
  availability policies.

See [architecture](ARCHITECTURE.md), [security](SECURITY.md),
[production requirements](PRODUCTION_REQUIREMENTS.md),
[audit evidence](AUDIT.md), and
[performance expectations](PERFORMANCE_EXPECTATIONS.md) for source mappings and
limits. The headers' “production-ready” annotations are not used as maturity
evidence in this assessment.

### Research and gap analysis

| Research / technical finding | Scheduler implication | Current gap / limit |
|---|---|---|
| Borg describes large-scale scheduling as a combination of admission control, task packing, workload isolation, failure recovery, monitoring, and operational tooling—not just a dispatch algorithm. | Define workload and operational guarantees before adding scale-oriented mechanisms; measure recovery and scheduling behavior end to end. | The scoped benchmarks do not establish production throughput/latency limits or representative cluster-scale workloads. Borg is a design reference, not a scale comparison for this in-process/module scheduler. |
| Dominant Resource Fairness (DRF) provides a fairness model for sharing multiple resource types among users with heterogeneous demands. | Consider per-tenant/resource fairness only if supported workloads demonstrate contention across multiple resource classes. | The scoped scheduler contract does not establish per-tenant resource estimates or quotas; DRF adoption is conditional, not a presumed requirement. |
| Google SRE's overload guidance emphasizes explicit overload behavior, prioritization, and controlled load shedding instead of allowing overload to cascade. | Specify bounded admission/queue and overload behavior before claiming predictable latency under saturation. | The roadmap does not assume a particular queue or backpressure mechanism exists; current queueing limits and deployment SLOs need code/owner verification. |

Research references:

1. Abhishek Verma et al., “Large-scale cluster management at Google with Borg,”
   EuroSys 2015,
   [paper](https://research.google/pubs/large-scale-cluster-management-at-google-with-borg/).
2. Ali Ghodsi et al., “Dominant Resource Fairness: Fair Allocation of Multiple
   Resource Types,” USENIX NSDI 2011,
   [paper and proceedings](https://www.usenix.org/conference/nsdi11/dominant-resource-fairness-fair-allocation-multiple-resource-types).
3. Google SRE, “Handling Overload,”
   [SRE Book](https://sre.google/sre-book/handling-overload/).

These sources inform candidate design and validation methods; none supplies a
scheduler-specific target or demonstrates ThemisDB performance.

## In Progress

- [x] Enforce a shared coordinator-owned execution gate for manual, scheduled,
  and DAG dispatch; deny while leadership or coordinator health is unavailable.
- [x] Reject malformed, unknown-field, and unsupported CDC predicates at
  validation/evaluation boundaries.
- [x] Require explicit request contexts for security-sensitive scheduler
  operations and use operation-limited scopes in trusted callers.
- [ ] Replace skipped coordination-failure and simulated burst/retention
  scenarios with production-behavior tests.
- [ ] Obtain security/deployment-owner review of production requirements and
  configuration defaults.
- [ ] Establish a production-path benchmark baseline and an owner-approved
  release threshold.

The completed items above describe existing source behavior; they do not imply
that the open release gates have passed.

## Planned Features

- **v2.5.0:** close evidence gaps, test coordination and retention behavior
  against production components, establish reproducible performance baselines,
  and obtain operational/security sign-off.
- **v3.0.0:** make task-delivery and recovery semantics explicit, and evaluate
  durable/distributed scheduling and multi-tenant fairness only where workload
  requirements justify the added architecture.

Detailed dependencies, estimates, risks, metrics, and validation are in the
Implementation Phases below. Estimates are engineering person-weeks and exclude
release scheduling, external review, and hardware procurement.

## Implementation Phases

### Phase 1 — Design and API contract (Wave 1, v2.5.0)

**Goal:** Turn the current source inventory into reviewable workload, contract,
and benchmark acceptance criteria.

- [ ] Record supported task/trigger workloads, scheduling-delay expectations,
  failure semantics, retention needs, and expected concurrency with the
  scheduler and API owners (Target: v2.5.0).
- [ ] Map each acceptance scenario to its public contract, production
  implementation, focused test, and benchmark; resolve discrepancies before
  proposing API changes (Target: v2.5.0).
- [ ] Select representative hardware and benchmark statistic, run protocol,
  artifact format, and owner; document the noise floor before setting a gate
  (Target: v2.5.0).

**Dependencies:** scheduler/API owner and representative workload input.
**Risk / trade-off:** Without real workload and hardware, thresholds could
optimize synthetic use cases or produce flaky CI.
**Success metric:** 100% of proposed v2.5 gates have a named owner, source
mapping, reproducible validation method, and an agreed acceptance value; no
unverified workload assumption is presented as fact.
**Validation:** Review public headers against implementation and tests; record
baseline setup and repeatability results. **Estimate:** 1–2 person-weeks.

### Phase 2 — Core reliability and failure behavior (Wave 2, v2.5.0)

**Goal:** Verify coordinator and retention behavior through production
components rather than skipped/simulated scenarios.

- [ ] Replace skipped coordination-failure scenarios with deterministic tests
  for leadership loss, heartbeat failure, shutdown, leadership recovery, and
  concurrent dispatch using the actual coordinator/scheduler path
  (Target: v2.5.0).
- [ ] Replace simulated burst/retention cases with tests of actual
  `TaskResultStore` and `HybridRetentionManager` behavior, including configured
  capacity boundaries and failure reporting (Target: v2.5.0).
- [ ] Add regression coverage for concurrent registration, execution, and
  lifecycle changes against the real scheduler where the public contract
  permits concurrency (Target: v2.5.0).

**Dependencies:** Phase 1's approved scenarios and test seams.
**Risk / trade-off:** Timing-based fault tests can be flaky; prefer controlled
coordinator state and deterministic synchronization without changing production
semantics.
**Success metric:** Every scenario currently skipped or simulated is either
replaced by a production-path assertion or explicitly justified as out of scope;
all tested dispatch attempts are denied while the shared gate is closed.
**Validation:** Focused scheduler tests, repeated concurrency runs, and
ThreadSanitizer where supported. **Estimate:** 2–4 person-weeks.

### Phase 3 — Performance, security, and operations (Wave 3, v2.5.0)

**Goal:** Establish evidence-based performance checks and close deployment
policy gaps.

- [ ] Extend the production-path benchmark to report repeatable registration,
  listing, execution, concurrency, and scheduling-delay measurements; clearly
  distinguish scheduler cost from query/storage setup (Target: v2.5.0).
- [ ] Publish hardware, build flags, workload shape, sample count, and
  p50/p95/p99 or other owner-approved statistic with each baseline; enable a CI
  regression gate only after measuring variance (Target: v2.5.0).
- [ ] Obtain security/deployment-owner decisions for request-context
  propagation, audit/HMAC key provisioning, retention/archive, sandbox policy,
  rate limits, retries, timeouts, and availability targets; record required
  defaults and runbook actions (Target: v2.5.0).

**Dependencies:** Phase 1 baseline protocol; Phase 2 stable production tests;
representative CI hardware and security/deployment owners.
**Risk / trade-off:** Dedicated hardware increases CI cost; overly strict
thresholds can gate on noise. Keep timing informational until variance and
owners' acceptance are documented.
**Success metric:** A versioned baseline exists for every selected production
case; after noise characterization, the owner approves a numeric regression
limit (initial proposal: no more than 5% regression outside measured variance).
All listed deployment decisions have an owner and disposition.
**Validation:** Repeated runs on the fixed hardware, smoke in ordinary CI,
threshold evaluation in the designated benchmark lane, security/config review,
and validation of deployment docs. **Estimate:** 1–2 person-weeks, excluding
hardware/CI provisioning and owner review.

### Phase 4 — Delivery and recovery architecture (Wave 4, v3.0.0)

**Goal:** Decide whether distributed/durable task execution is a supported
product requirement before committing to a new architecture.

- [ ] Specify task delivery semantics (loss, duplicate execution, retry,
  cancellation, ordering, and result visibility) for restart and coordinator
  failover; obtain API and operations approval (Target: v3.0.0).
- [ ] Evaluate durable schedule/attempt state, lease or fencing protection,
  recovery/reconciliation, and migration requirements against the approved
  workload; implement only the selected design (Target: v3.0.0).
- [ ] Define observable recovery objectives and operator actions for
  leader-loss, restart, and stale/incomplete task states (Target: v3.0.0).

**Dependencies:** Wave 2 failure evidence and documented demand for durable or
multi-node execution.
**Risk / trade-off:** Stronger delivery guarantees require storage, consensus,
and compatibility decisions; avoid implying exactly-once execution without an
end-to-end proof and defined side-effect boundary.
**Success metric:** The approved contract specifies observable loss/duplicate
behavior; restart/failover tests meet the owner-approved recovery objective,
and zero dispatch occurs without valid coordination authority.
**Validation:** Failure injection at each persistence/coordination boundary,
restart/recovery integration tests, and measured recovery-time distribution.
**Estimate:** 2–4 person-weeks for design/prototype; implementation effort is
unestimated until the contract and persistence impact are approved.

### Phase 5 — Fairness, admission, and scale (Wave 5, v3.0.0)

**Goal:** Add resource-aware fairness or overload handling only for a
demonstrated multi-tenant or saturation use case.

- [ ] Measure per-tenant admission, queue wait, task duration, resource use,
  and starvation under representative saturation; establish whether multiple
  resource classes and tenant quotas are required (Target: v3.0.0).
- [ ] If heterogeneous resource contention is confirmed, compare a simple
  priority/quota policy with DRF against the measured workload and specify
  admission, overload, and backpressure semantics (Target: v3.0.0).
- [ ] Implement the selected policy behind an explicit, reviewed contract and
  expose per-tenant fairness/overload metrics (Target: v3.0.0).

**Dependencies:** Wave 1 workload data and Wave 3 production baseline; Wave 4
delivery contract if admission affects distributed dispatch.
**Risk / trade-off:** Resource estimates add accounting overhead and inaccurate
estimates can reduce fairness; do not adopt DRF without heterogeneous
multi-resource demand.
**Success metric:** Under a reproducible saturated multi-tenant workload,
report queue-wait and resource-share distributions, demonstrate no starvation
beyond the owner-approved bound, and keep scheduler overhead within the
Wave 3 regression budget. If no qualifying workload is identified, document
the evidence and defer rather than add unused machinery.
**Validation:** Controlled saturation, per-tenant fairness tests, noisy-neighbor
and overload tests, and comparison against the Wave 3 baseline.
**Estimate:** 4–8 person-weeks if the feature is justified; otherwise 1–2
person-weeks to measure and document the deferral.

### Phase 6 — Performance hardening, documentation, and acceptance (Wave 6, v3.0.0)

**Goal:** Complete cross-component verification and make the scheduler's
supported behavior operable and compatible.

- [ ] Review public scheduler API/configuration changes for SemVer impact;
  document migration and compatibility behavior before any breaking change
  (Target: v3.0.0).
- [ ] Publish scheduler deployment, monitoring, recovery, capacity, retention,
  security, and troubleshooting runbooks from the owner-approved contracts
  (Target: v3.0.0).
- [ ] Validate the selected design with failure injection, sustained-load
  benchmarks, sanitizer runs, and recovery exercises; resolve all release
  blockers before acceptance (Target: v3.0.0).

**Dependencies:** Waves 4–5 decisions and implementations; API, security,
operations, and release-owner review.
**Risk / trade-off:** Cross-component gates are slower and may vary by
platform; separate deterministic correctness gates from hardware-specific
performance lanes.
**Success metric:** 100% of supported API/configuration changes have migration
notes and contract tests; runbooks cover every approved failure mode; all
owner-approved correctness, recovery, and performance gates pass with retained
evidence.
**Validation:** Focused and integration tests, sanitizer/recovery evidence,
benchmark CI, documentation/link checks, and owner sign-off.
**Estimate:** 2–3 person-weeks, excluding cross-platform CI and release review.

## Production Readiness Checklist

- [ ] Production-path coordinator failure and retention tests replace skipped
  or simulated claims.
- [ ] Representative workload and hardware are identified; baseline and
  approved performance thresholds are reproducible.
- [ ] Security and deployment owners approve authorization, audit/key,
  retention, sandbox, rate-limit, timeout, and availability settings.
- [ ] Supported delivery, retry, duplicate/loss, and recovery semantics are
  documented and tested.
- [ ] Monitoring, recovery, and troubleshooting runbooks match implemented
  behavior.
- [ ] No release claim depends only on a benchmark label or public-header
  maturity annotation.

## Known Issues & Limitations

- The production scheduler benchmark is currently a smoke check, not a numeric
  release gate; benchmark files using stubs or enum/descriptor microbenchmarks
  are not production lifecycle evidence.
- Some coordination-failure and burst/retention test scenarios remain skipped,
  simulated, or placeholders. Test registration alone is not proof of coverage.
- Deployment defaults and security/operations requirements need explicit owner
  review; source configuration options alone do not establish deployment policy.
- No workload, tenant/resource model, service-level objective, or cluster-scale
  capacity target has been verified for this module.
- A `WEBHOOK` trigger value is declared in the public enum, but it is not treated
  here as an implemented trigger path without implementation and test evidence.

## Breaking Changes

No scheduler breaking change is approved by this roadmap. Any v3.0.0 API,
configuration, persistence, or delivery-semantics change must follow the root
SemVer/deprecation policy, include migration guidance, and pass API-owner review.
