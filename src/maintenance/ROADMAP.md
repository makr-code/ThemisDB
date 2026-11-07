# Maintenance Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-07 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · Issue #7434 -->
<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
-->

> Exact model backend not verifiable from final document text alone.

## Current Status

Production maintenance runtime exists for schedule orchestration, execution coordination, schedule persistence/reload, and registry-driven setup.

**Maturity assessment:** The module has an implemented operator-facing orchestration surface and production CRUD/persistence regression coverage, but its performance and resilience evidence is not release-grade: all nine benchmark manifest gates remain `UNVERIFIED`, there is no production-path p99 evidence, and distributed locking has no stale-owner fencing contract. Schedule persistence is optional, corrupt records are partially recovered, and completed task side effects are not rolled back; see [`README.md`](README.md) and [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

## In Progress

- [x] add schedule-data/serialization helper coverage (Target: Q3 2026) — evidence: `test_maintenance_hardening_phase23_focused.cpp` (MTN-09..MTN-16); these tests do not establish all runtime dispatch behavior.
- [x] add microbenchmark proxies for enum/switch/descriptor operations (Target: Q3 2026) — evidence: `bench_maintenance_release_gates.cpp` (GATE-MTN-01..GATE-MTN-04); these are not orchestrator hot-path benchmarks.
- [~] define diagnostics serialization/model coverage (Target: Q3 2026) — `test_maintenance_diagnostics_focused.cpp` (MTN-29..MTN-32) tests outcome serialization and a local buffer model, not end-to-end orchestrator diagnostics.

## Planned Features

### Short-term (3-6 months)
- [x] enforce schedule mutation churn limits on production CRUD paths (Target: Q4 2026) — create/update/patch/delete call `checkChurnLimit()` and production coverage is in `tests/database/test_database_maintenance_orchestrator.cpp`. The separate focused churn fixture remains a model test for its other cases.
- [x] add mock-store stress fixtures for serialization and handler-outcome shapes (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_stress_focused.cpp (MTN-21..MTN-28). **Note:** this fixture's all-or-nothing `PersistenceCorrupt` behavior is not the production store contract; production store tests are in `tests/database/test_database_maintenance_orchestrator.cpp`.
- [~] implement operator diagnostics and add serialization/model tests (Target: Q4 2026) — implementation: `database_maintenance_orchestrator.cpp` and `maintenance_health_report.h`; `test_maintenance_diagnostics_focused.cpp` does not exercise the production ring buffer.

### Mid-term (6-12 months)
- [~] define p99 thresholds for modeled maintenance scheduling/dispatch proxies (Target: Q1 2027) — evidence: benchmarks/maintenance/bench_maintenance_release_gates.cpp (GATE-MTN-05..GATE-MTN-07). **TODO:** run and retain release-profile results before claiming measured baselines.
- [~] add benchmark proxies for distributed scenarios and schedule listing (Target: Q1 2027) — evidence: benchmarks/maintenance/bench_maintenance_distributed_gates.cpp (GATE-MTN-DIST-01..GATE-MTN-DIST-02). These use mock/in-memory operations, not the production lock/orchestrator.
- [~] add an in-memory serialization/lifecycle endurance test (Target: Q1 2027) — evidence: tests/maintenance/test_maintenance_endurance_focused.cpp (MTN-ENDURANCE-01); this is not a sustained production-orchestrator soak test.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] distinguish helper contract types from the production API (Target: Q4 2026) — `maintenance_api_contract.h` now states that its helper taxonomy and types are not runtime guarantees; the orchestrator uses `MaintenanceScheduleEntry`, `MaintenanceHealthReport`, and repository `Result`/`Error`.
- [~] define a unified runtime error taxonomy for schedule, persistence, and handler execution (Target: Q1 2027) — current operations return repository `Error` values; the compatibility helper enum is not used by runtime code.

### Phase 2: Core Implementation
- [x] complete hardening for orchestrator and schedule-store internals (Target: Q4 2026) — evidence: src/maintenance/database_maintenance_orchestrator.cpp; src/maintenance/maintenance_schedule_store.cpp; tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-10, MTN-11, MTN-14, MTN-16)
- [x] align registry and execution behavior to bounded runtime contracts (Target: Q4 2026) — evidence: tests/maintenance/test_maintenance_hardening_phase23_focused.cpp (MTN-09, MTN-12)

### Phase 3: Error Handling and Edge Cases
- [x] implement explicit missing-handler skip and per-record corrupt-JSON recovery (Target: Q4 2026) — evidence: `database_maintenance_orchestrator.cpp` and `maintenance_schedule_store.cpp`; production coverage in `tests/database/test_database_maintenance_orchestrator.cpp`. The standalone stress fixture only models handler outcomes and a different corruption policy.
- [~] define and test dispatch diagnostics (Target: Q4 2026) — implementation records selected outcomes in `database_maintenance_orchestrator.cpp`; `test_maintenance_diagnostics_focused.cpp` checks serialization and a local ring-buffer model, not the orchestrator's ring buffer.

### Phase 4: Tests
- [x] add focused contract/serialization fixtures and production orchestrator regression coverage (Target: Q4 2026) — evidence: `tests/maintenance/test_maintenance_contract_hardening_focused.cpp`, `test_maintenance_hardening_phase23_focused.cpp`, and `tests/database/test_database_maintenance_orchestrator.cpp`. The first two primarily test helper types/serialization.
- [~] extend model-based stress fixtures for maintenance edge cases (Target: Q4 2026) — evidence: `tests/maintenance/test_maintenance_stress_focused.cpp`; these tests do not establish production-store corruption behavior or full orchestrator execution behavior.

### Phase 5: Performance and Hardening
- [x] add benchmark source/target definitions for maintenance proxies (Target: Q4 2026) — evidence: benchmarks/maintenance/bench_maintenance_release_gates.cpp and benchmarks/maintenance/CMakeLists.txt
- [?] validate p95/p99 and throughput behavior against release baselines — the C++ cases emit repeated aggregates, not per-operation percentiles; all manifest gates are UNVERIFIED until a suitable measurement and evidence path exists.

### Phase 6: Documentation and Acceptance
- [x] core maintenance module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core maintenance surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] schedule/persistence/execution edge paths have production regression coverage — evidence: `tests/database/test_database_maintenance_orchestrator.cpp`; helper serialization coverage is in `test_maintenance_hardening_phase23_focused.cpp`.
- [~] benchmark source targets are registered; **TODO:** actual release-profile measurements and baseline comparisons are not established by source/manifest alone.

## Known Issues and Limitations

- `max_schedule_changes_per_interval` is an in-memory fixed-window limit; its
  counts reset after orchestrator restart. Existing-schedule attempts consume a
  slot even if persistence later fails; successful deletes clear their counters.
- Persistence errors are returned by schedule mutations or logged at startup,
  but are not emitted as `FAILED_PERSISTENCE` dispatch outcomes. The helper error
  taxonomy in `maintenance_api_contract.h` is intentionally not the runtime
  taxonomy; runtime APIs continue to use repository `Error` values.
- Maintenance benchmark targets are proxies; their C++ cases do not calculate
  p99, and no external processor connected to them was verified. The manifest
  keeps the p99 targets but marks all gates UNVERIFIED; production-path
  benchmarks and release-profile evidence remain future work.

## Breaking Changes

No breaking maintenance contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## v2.5.0 / v3.0.0 Research and Wave Plan

This module plan complements, but does not replace, the project-level Wave A–D
plan below. Version targets are planning horizons, not claims about release dates
or current release readiness. The module is an orchestration layer: cleanup,
compaction, and task side effects belong to registered handlers or other modules
([`README.md`](README.md)).

### Research and evidence-based gap analysis

| Area | Technical finding | Current evidence and gap | Roadmap implication |
|---|---|---|---|
| Performance and scale | Background database maintenance competes with foreground work for I/O; resource throttling and workload-specific observation are needed. | Existing nine benchmark gates are synthetic/helper proxies, do not calculate p99, and remain `UNVERIFIED` ([performance expectations](PERFORMANCE_EXPECTATIONS.md), [manifest](../../benchmarks/maintenance/release_gate_manifest_mtn.json)). | Measure production orchestrator/store paths and report tail latency under representative concurrency before setting absolute SLOs. |
| Usability and operations | Operational signals should measure user-relevant outcomes and support actionable alerting, rather than treating metric presence as an SLO. | Runtime exports job/task counters and durations, but alert thresholds are deployment-owned ([OPERATIONS.md](OPERATIONS.md)). | Define example SLI/SLO templates and operator actions without imposing one deployment's thresholds globally. |
| Security and resilience | A lease timeout alone cannot stop a paused former lock holder from resuming; correctness-sensitive work needs stale-owner protection at the state-mutation boundary. | The injected lock API is `tryAcquire(key, ttl)`; no fencing token or lease-loss signal is represented in the interface. Task side effects are not rolled back; cancellation is not transactional ([README.md](README.md), [`i_distributed_lock.h`](../../include/maintenance/i_distributed_lock.h)). | For v3, specify lock ownership epochs and task replay/idempotency semantics before changing the interface; fault-inject stale owners and partial execution. |
| Persistence and recovery | Routine maintenance must be observable and recovery behavior must distinguish valid state from damaged records. | The production store retains valid records while skipping malformed ones; scan errors are logged and startup can continue. A focused mock test models a different all-or-nothing corruption contract ([README.md](README.md), [test README](../../tests/maintenance/README.md)). | Close production-path test gaps and make partial-recovery operator signals explicit; do not infer production behavior from model fixtures. |
| Governance and maintainability | Repeatable evidence, explicit SLOs, and traceable operational decisions support reliable service ownership. | Production tests exist, but the standalone endurance source is not registered with CTest; benchmarks lack production-path distributions. `FAILED_PERSISTENCE` is declared but not recorded as a dispatch outcome ([test README](../../tests/maintenance/README.md), [OPERATIONS.md](OPERATIONS.md)). | Tie each gate to an executable production test/benchmark and document unresolved deployment-owned assumptions. |

Research inputs (accessed 2026-11-07): [PostgreSQL 18, Routine Vacuuming](https://www.postgresql.org/docs/current/routine-vacuuming.html) documents background-work impact and resource tuning; [Google SRE, Monitoring Distributed Systems](https://sre.google/sre-book/monitoring-distributed-systems/) and [Service Level Objectives](https://sre.google/sre-book/service-level-objectives/) motivate user-relevant indicators and objectives; [Kleppmann, “How to do distributed locking” (2016)](https://martin.kleppmann.com/2016/02/08/how-to-do-distributed-locking.html) explains why lease expiry alone does not fence stale clients. These are design references, not evidence that the maintenance module already implements those mechanisms.

### v2.5.0 — production evidence and operational hardening

Priorities: **P0** for production correctness/evidence (Waves 1–2), **P1**
for resilience and operator readiness (Wave 3). Estimates are engineering
person-weeks, not calendar commitments; they assume one engineer familiar with
the module and exclude work owned by delegated task handlers.

#### Wave 1 — Close production contract and test gaps (P0; 2–3 person-weeks)

- **Goal:** Make the documented lifecycle, persistence, dispatch, and recovery behavior verifiable on production orchestrator/store paths.
- **Tasks:** Add integration cases for storage scan/write/remove failures, partial corrupt-record reload, lock contention/acquisition failure, handler failure and missing handler, cancellation/stop behavior, and already-completed task effects. Reconcile focused mock expectations with actual production semantics. Specify and test how persistence failures appear in operator-visible outcomes.
- **Dependencies:** Existing orchestrator/store APIs and the production regression fixture in `tests/database/test_database_maintenance_orchestrator.cpp`.
- **Risk / trade-off:** Tests may expose behavior that requires a compatibility decision; do not silently change partial recovery or task-side-effect semantics.
- **Success metrics:** All listed failure cases have production-path regression coverage; zero conflicting mock-versus-runtime assertions remain undocumented; persistence failure is either represented in the production outcome stream or explicitly documented as an API/log-only failure.
- **Validation:** Focused maintenance and database-orchestrator CTest targets; test injected failures and restart/reload state against the real `MaintenanceScheduleStore` implementation with a test storage engine.

#### Wave 2 — Establish production performance baselines (P0; 2–4 person-weeks)

- **Goal:** Replace proxy-only release evidence with reproducible measurements of schedule CRUD/list/load and dispatch under contention.
- **Tasks:** Add benchmarks around the production orchestrator and schedule store; cover 1, 100, and 1,000 schedules, single and concurrent callers, warm/cold reload, and local versus injected distributed-lock paths. Capture per-operation p50/p95/p99, throughput, workload, compiler, host profile, and raw output. Retain the existing helper microbenchmarks only as helper-level measurements.
- **Dependencies:** Wave 1 runtime semantics; a named, repeatable release benchmark host/profile.
- **Risk / trade-off:** Hardware and storage variance can make absolute thresholds misleading; normalize comparisons to the same pinned profile and do not convert proxy targets into production claims.
- **Success metrics:** Every selected production workload emits p50/p95/p99 and throughput in five repeat runs; selected gates have a recorded baseline and an agreed regression budget (initial proposal: no more than 10% p99 regression on the same profile); no selected gate is marked PASS without retained run evidence.
- **Validation:** Run in release profile on the named representative host; compare distributions and retain environment metadata and raw results with the release-gate manifest. Existing nine proxy gates remain `UNVERIFIED` until they have valid percentile evidence or are replaced by production-path gates.

#### Wave 3 — Failure-injection and operator readiness (P1; 2–4 person-weeks)

- **Goal:** Demonstrate bounded, diagnosable behavior through restart, persistence faults, dispatch failure, and prolonged operation.
- **Tasks:** Exercise restart during/after schedule mutation, corrupt and unavailable storage, lock contention/expiry, handler timeout/failure, and partial multi-task completion. Promote production-orchestrator endurance coverage into a registered test/soak path. Document operator actions and example alert conditions for stale health, failed tasks, unexpected skips, and duration-budget violations.
- **Dependencies:** Wave 1 failure contracts; Wave 2 workload profile for duration budgets; deployment owners for alert thresholds.
- **Risk / trade-off:** Module-level tests cannot prove the delegated handler's side effects are transactional; retries must not be added until idempotency is established.
- **Success metrics:** Every injected failure is surfaced in job state, outcome, error, or documented startup log; zero silent schedule loss in restart/reload tests; a 24-hour representative soak has no unbounded growth in retained jobs, diagnostics, or schedule state.
- **Validation:** Production-path fault-injection tests, registered soak/CTest evidence, review of runbook procedures, and alert checks against synthetic failures; preserve the distinction between module evidence and handler-owned guarantees.

### v3.0.0 — distributed correctness and policy-driven maintenance

These are strategic changes and may require a major-version API migration. No
compatibility shim or dual legacy path is assumed; any public contract change
must follow `VERSIONING.md` and the existing breaking-change process.

#### Wave 4 — Fenced distributed execution and explicit recovery (P0; 4–8 person-weeks)

- **Goal:** Prevent stale lock holders from committing correctness-sensitive work and define safe restart/retry behavior for partially completed jobs.
- **Tasks:** Specify a lock ownership epoch/fencing-token contract and enforce it at the protected state-mutation boundary; define lock-loss handling, task idempotency/replay declarations, checkpointing or explicit non-retryable outcomes, and durable job execution state. Do not promise exactly-once task effects unless the handler/resource contract can enforce them.
- **Dependencies:** Wave 1 error/lifecycle semantics; a distributed-lock backend capable of monotonic fencing tokens; handler-owner agreement on idempotency and recovery.
- **Risk / trade-off:** Fencing is ineffective if the resource that commits side effects does not validate the token; durable execution state adds write cost and migration complexity.
- **Success metrics:** In deterministic stale-owner fault tests, zero stale-epoch writes are accepted; every interrupted task is classified as resumable, safely replayable, or operator-intervention-required; no unqualified exactly-once claim.
- **Validation:** Multi-node integration tests that pause a former owner past lease expiry, acquire with a newer epoch, then attempt the former owner's write; crash/restart tests at each persisted job transition; migration and API compatibility review.

#### Wave 5 — Bounded workload-aware scheduling (P1; 3–6 person-weeks)

- **Goal:** Protect foreground service objectives while scaling background maintenance fairly across tenants and workload classes.
- **Tasks:** Define deterministic per-tenant and global concurrency/resource budgets, queue/backpressure behavior, starvation bounds, and operator overrides. Use measured load/maintenance signals to defer or throttle work; keep scheduling policy separate from the task handler's storage algorithm.
- **Dependencies:** Wave 2 production SLO baselines; Wave 4 durable ownership/execution semantics where work can move across nodes; deployment SLO and resource-budget inputs.
- **Risk / trade-off:** More conservative throttling can delay required maintenance and increase storage debt; adaptive policy complexity must remain inspectable and overrideable.
- **Success metrics:** 100% of dispatched work respects configured concurrency/resource budgets; no tenant exceeds its declared quota in stress tests; p99 foreground latency remains within the agreed same-profile regression budget while maintenance completes within its configured deadline.
- **Validation:** Mixed foreground/background load tests, fairness/starvation assertions, resource-budget boundary tests, and controlled comparison against the Wave 2 baseline across low/high load.

#### Wave 6 — Auditable, supportable maintenance lifecycle (P1; 2–4 person-weeks)

- **Goal:** Make administrative schedule and execution decisions traceable and supportable without imposing an unspecified regulatory regime.
- **Tasks:** Define stable audit fields for actor/tenant, schedule/job, requested action, decision, and outcome; define retention/redaction boundaries and access checks for sensitive history; publish SLO/alert examples and recovery procedures for partial restore, stale ownership, and replay-required jobs.
- **Dependencies:** Wave 4 durable job state; Wave 5 tenant/policy model; security and operations owner review of data retention and actor identity sources.
- **Risk / trade-off:** Durable audit/history increases storage and may retain sensitive metadata; actual retention periods and compliance obligations are deployment-specific and require owner input.
- **Success metrics:** 100% of privileged schedule CRUD, forced runs, and recovery decisions produce correlated actor/action/outcome audit records; retention and redaction tests pass; every documented high-severity scenario has an actionable runbook procedure.
- **Validation:** Authorization and audit integration tests; retention/redaction tests on representative records; security review and operator tabletop exercise. No compliance certification is implied by these checks.

### Open assumptions and knowledge gaps

- Release milestones and dates for v2.5.0/v3.0.0 are not inferred from this roadmap; confirm via release milestones and `RELEASE_STRATEGY.md`.
- A representative benchmark host, storage profile, foreground workload, and acceptable service-level objectives have not been supplied; Wave 2 must establish them before absolute latency budgets are approved.
- Distributed lock backend ownership is outside the module's current scope. Fencing requires changes at the backend and protected resource boundary, not only in the maintenance scheduler.
- Task-handler transactionality, idempotency, and replay guarantees are external to this module and need per-handler contracts.
- The appropriate audit/history retention period, redaction requirements, and applicable compliance controls are deployment-owner decisions.
- The Developer LLM Wiki snapshot (`WIKI_STATUS.json`, generated 2026-11-02) predates this review; module implementation, tests, and benchmark artifacts were checked directly and take precedence over that secondary summary.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `maintenance`
- [?] Distributed tracing/exporter reliability and operator remediation validation — **TODO:** no maintenance-specific evidence was verified for tracing/exporter behavior; high-cardinality tests use stubs.
- [~] In-memory lifecycle endurance fixture exists; **TODO:** production-orchestrator soak coverage is not established (Target: Q1 2027).
- [~] Maintain runbook coverage for operator-critical maintenance scenarios (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [?] Fail-closed behavior for distributed/acceleration paths — **TODO:** the production orchestrator has no fail-closed health gate; the compatibility helper predicate does not control runtime dispatch.
- [?] Benchmark-backed p95/p99 baselines on representative hardware — **TODO:** no measured run evidence is cited.
- [~] Operator diagnostics and runbooks exist; **TODO:** alert thresholds/integration are deployment-owned and not defined by the module.
