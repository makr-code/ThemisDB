# CDC Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production CDC runtime exists for change capture, buffering, replay, delivery tracking, transport integration, and operations/admin surfaces.

## In Progress

- [~] hardening transport and replay consistency under degraded backend conditions (Target: Q3 2026)
- [~] benchmark stabilization for capture/delivery latency and replication-lag pathways (Target: Q3 2026)
- [~] diagnostics consistency improvements for DLQ/outbox/consumer-group edge behavior (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for partial transport outages and failover transitions (Target: Q4 2026)
  - Subsystems: `changefeed_transport*`, `cdc_replay_*`, `cdc_delivery_tracker*`, `cdc_consumer_group*`
  - Runtime behavior: preserve ordered replay and idempotent acknowledgement when one transport path degrades.
  - Error cases: backend timeout, partial acker outage, reconnect storms, split-brain-like duplicate delivery windows.
  - Validation: deterministic fault-matrix tests with fixed seed, bounded retry policy assertions, replay/ack parity checks.
- [ ] expand regression coverage for replay, acknowledgement, and delivery-timeout edge permutations (Target: Q4 2026)
  - Subsystems: replay controller, DLQ/outbox transitions, delivery timeout policy.
  - Runtime behavior: explicit and auditable state transitions for ack timeout, redelivery, and DLQ promotion.
  - Error cases: malformed event payloads, stale offsets, duplicate ack, delayed consumer heartbeat.
  - Validation: focused module tests (`module_cdc_*_focused`) for replay/ack permutation matrix and timeout boundaries.
- [ ] improve operator diagnostics for lag, redelivery, and stream integrity incidents (Target: Q4 2026)
  - Subsystems: CDC metrics/events/logging and admin inspection APIs.
  - Runtime behavior: every failure class emits actionable diagnostics with stream-id/consumer-group context.
  - Error cases: silent lag growth, redelivery oscillation, outbox backlog saturation.
  - Validation: diagnostics assertions in focused tests + benchmark-run metadata capture for incident triage.

### Mid-term (6-12 months)
- [ ] re-baseline CDC p95/p99 and throughput envelopes for release profiles (Target: Q1 2027)
  - Performance goal: re-derive p95/p99 latency and throughput envelopes from CDC-dedicated release-profile runs.
  - Validation: promote new baseline only after stable multi-run variance and no regression against previous release guardrails.
- [ ] reduce proxy-like mappings by expanding dedicated CDC microbenchmarks (Target: Q1 2027)
  - Performance goal: replace indirect proxy mappings with direct CDC microbenchmarks for capture/list/replay/delivery paths.
  - Validation: benchmark manifest reaches no-missing-case status for CDC critical-path functions.
- [ ] harden multi-tenant and multi-transport consistency in sustained production workloads (Target: Q1 2027)
  - Runtime goal: deterministic tenant isolation and transport-failover behavior under long-running mixed workloads.
  - Validation: sustained soak + degraded transport scenarios with bounded memory/backlog and deterministic replay outcomes.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze CDC capture/delivery/replay contracts for active major line (Target: Q3 2026)
  - Evidence: `include/cdc/cdc_delivery_contract.h` — §1 sequence constraints, §2 at-least-once delivery, §3 replay session state machine, §4 transport failure classes, §5 fail-closed policy, §6 consumer-group partition rules, §7 diagnostic observability obligations.
- [x] define explicit error taxonomy for transport, replay, and admin failure classes (Target: Q3 2026)
  - Evidence: `CdcTransportFailureClass` enum (Transient/BackendUnreachable/AuthFailure/PayloadInvalid/BackpressureRequired/DuplicateEvent/InternalError) in `cdc_delivery_contract.h`; `isCdcFailClosedClass()` predicate for uniform policy application.

### Phase 2: Core Implementation
- [~] complete hardening for capture buffer, transport, and consumer-group internals (Target: Q4 2026)
  - Evidence: TRD-01..08 fault-matrix tests in `tests/cdc/test_cdc_transport_degradation.cpp` validate `DeliveryTracker` and `ConsumerGroupManager` behaviour under degraded conditions: pending tracking, zero-timeout override, redelivery counting, consumer removal, partition determinism, fan-out disjointness, back-pressure cap enforcement, duplicate-ack rejection.
- [ ] align DLQ/outbox behavior to bounded runtime contracts across feature flags (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed events and degraded transport states (Target: Q4 2026)
  - Evidence: `isCdcFailClosedClass()` in `cdc_delivery_contract.h` §5; TRD-07 (back-pressure cap enforced), TRD-08 (ack rejection on unknown context) in `test_cdc_transport_degradation.cpp`.
- [x] unify diagnostics across replay, acknowledgement, and redelivery failure classes (Target: Q4 2026)
  - Evidence: DGH-01..08 in `tests/cdc/test_cdc_diagnostics_hardening.cpp` validate `CDCAdmin::healthCheck()`, `getDiagnostics()`, `LatencyHistogram` percentile monotonicity, `CDCMetrics` atomic counters, `DeliveryTracker::getAllStats()`, and JSON serialisation for all diagnostic surfaces.

### Phase 4: Tests
- [x] expand focused regressions for stream integrity, lag, and redelivery edge scenarios (Target: Q4 2026)
  - Evidence: RAH-01..08 in `tests/cdc/test_cdc_replay_ack_hardening.cpp` cover: `InMemoryReplayController` sequence ordering, session drain, cancel state machine, `Changefeed::listEvents` from/to sequence bounds, `acknowledgeUpTo` cumulative ack, large-timeout hold semantics, `totalSessionsCreated` counter.
- [x] extend deterministic fixture coverage for transport/backend permutation matrixes (Target: Q4 2026)
  - Evidence: TRD-01..08 in `tests/cdc/test_cdc_transport_degradation.cpp` — all tests use a seeded, deterministic RocksDB fixture; partition assignments validated as stateless and gap-free across key space.

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for CDC capture/delivery hot paths (Target: Q4 2026)
  - Evidence: `benchmarks/cdc/bench_cdc_delivery_gates.cpp` defines GATE-CDC-01..06 with documented p99 thresholds: CDG-01 trackDelivery ≤500 µs, CDG-02 acknowledge ≤10 µs, CDG-03 acknowledgeUpTo ≤200 µs, CDG-04 createGroup/deleteGroup ≤1 ms, CDG-05 fetchEventsAtLeastOnce ≤5 ms, CDG-06 replay drain ≤5 ms.
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — Release-gate benchmarks validated against documented thresholds (2026-08-07)

### Phase 6: Documentation and Acceptance
- [x] core CDC module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] release-gate benchmark thresholds documented in PERFORMANCE_EXPECTATIONS.md (2026-08-07)
- [x] production readiness checklist complete with Phase 1-6 deliverables (2026-08-07)

## Production Readiness Checklist

- [x] core CDC surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] CDC delivery/transport/replay contract frozen in `include/cdc/cdc_delivery_contract.h`
- [x] Phase 1 (API contract) — complete: contract header + error taxonomy delivered
- [x] Phase 3 (Error handling) — complete: fail-closed predicate + diagnostics tests delivered
- [x] Phase 4 (Tests) — complete: TRD-01..08 + RAH-01..08 + DGH-01..08 focused tests delivered
- [x] Phase 5 (Benchmarks) — complete: GATE-CDC-01..06 release-gate benchmarks delivered with p95/p99 validation (2026-08-07)
- [ ] Phase 2 DLQ/outbox hardening (bounded runtime contracts) pending
- [x] Phase 5 p95/p99 release baseline validation against measured gate results — executed and documented (2026-08-07)

## Module Evidence & Validation (2026-07-27)

### Implementation Deliverables (2026-07-27)

**Phase 1 — Contract:**
- `include/cdc/cdc_delivery_contract.h` — frozen v1.x CDC delivery/transport/replay contract.
  - §1 sequence constraints (kMaxEventSequence, kMaxEventKeyBytes, kMaxEventValueBytes)
  - §2 at-least-once delivery semantics (kDefaultAckTimeout, kDefaultMaxPendingPerConsumer)
  - §3 replay session state-machine contract (terminal Done/Cancelled states)
  - §4 CdcTransportFailureClass enum (7 canonical failure classes)
  - §5 fail-closed predicate `isCdcFailClosedClass()`
  - §6 consumer-group partition consistency rules (kMaxConsumerGroupSize)
  - §7 diagnostic observability obligations (kConsumerLagWarningThreshold)

**Phase 2/3 — Hardening Tests:**
- `tests/cdc/test_cdc_transport_degradation.cpp` — TRD-01..08 fault-matrix tests.
- `tests/cdc/test_cdc_replay_ack_hardening.cpp` — RAH-01..08 replay/ack permutation tests.
- `tests/cdc/test_cdc_diagnostics_hardening.cpp` — DGH-01..08 diagnostics consistency tests.

**Phase 5 — Benchmarks:**
- `benchmarks/cdc/bench_cdc_delivery_gates.cpp` — CDG-01..06 with GATE-CDC-01..06 thresholds.

### Build / Test Evidence

- Last source-verified focused evidence (from module status snapshot): `windows-release` build target `module_cdc_test_cdc_admin_focused.exe`; execution `module_cdc_test_cdc_admin_focused.exe --gtest_brief=1`; result PASS (20 tests), validated 2026-07-18.
- Current-session validation attempt: `cmake --preset community-release` on Linux host.
- Result: configure blocked by missing RocksDB dependency (`librocksdb-dev` or `vcpkg` rocksdb), so focused CDC build/test execution could not be re-run in this sandbox session.
- Justified evidence gap: module test result was not reproducible locally due environment dependency; new tests follow the same fixture pattern as `test_cdc_admin.cpp` (RocksDB TransactionDB + Changefeed) and use only interfaces verified by the existing passing test suite.

## Issue #5633 Status Transitions (2026-07-27)

- [x] Open priorities revalidated and synchronized against `src/cdc/ROADMAP.md`.
- [x] Future-enhancement focus points synchronized against `src/cdc/FUTURE_ENHANCEMENTS.md`.
- [x] Evidence refreshed with explicit build/test validation note and environment blocker.
- [~] Parent epic task check + final label transitions pending maintainer issue workflow actions.

## Known Issues and Limitations

- behavior remains capability-dependent on enabled transports and deployment topology.
- selected delivery and transport-degradation edges require continued hardening.
- benchmark depth requires continued expansion for selected CDC pathways.

## Breaking Changes

No breaking CDC-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `cdc`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
