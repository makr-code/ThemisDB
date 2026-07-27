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
- [ ] freeze CDC capture/delivery/replay contracts for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for transport, replay, and admin failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for capture buffer, transport, and consumer-group internals (Target: Q4 2026)
- [ ] align DLQ/outbox behavior to bounded runtime contracts across feature flags (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for malformed events and degraded transport states (Target: Q4 2026)
- [ ] unify diagnostics across replay, acknowledgement, and redelivery failure classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for stream integrity, lag, and redelivery edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for transport/backend permutation matrixes (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for CDC capture/delivery hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core CDC module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core CDC surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for transport/replay edge cases
- [ ] release benchmark stabilization complete

## Module Evidence & Validation (2026-07-27)

- Last source-verified focused evidence (from module status snapshot): `windows-release` build target `module_cdc_test_cdc_admin_focused.exe`; execution `module_cdc_test_cdc_admin_focused.exe --gtest_brief=1`; result PASS (20 tests), validated 2026-07-18.
- Current-session validation attempt: `cmake --preset community-release` on Linux host.
- Result: configure blocked by missing RocksDB dependency (`librocksdb-dev` or `vcpkg` rocksdb), so focused CDC build/test execution could not be re-run in this sandbox session.
- Justified evidence gap: module test result was not reproducible locally due environment dependency; roadmap status remains synchronized and explicitly tracks this validation blocker.

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