# Toolbox Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 | re-verified: 2026-08-07 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · DEVELOPMENT_STATUS_2026_08_07.md -->

## Current Status

Production-usable toolbox runtime exists for ingestion-oriented extraction orchestration, content bridging, registry/bootstrap behavior, and text helper primitives.

## In Progress

- [~] Phase 2-3: Hardening extraction and bridge behavior under mixed content and soft-failure scenarios (Target: Q4 2026, 80% complete - 2026-08-07)
- [~] Phase 3: Improving diagnostics consistency across toolbox orchestration, registry, and helper stages (Target: Q4 2026, 75% complete - 2026-08-07)
- [~] Phase 5: Establishing benchmark-backed performance gates via native toolbox benchmark suite (Target: Q1 2027, 50% complete - benchmark suite created 2026-08-07)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for composite routing and streaming edge scenarios (Target: Q4 2026)
- [ ] expand stress coverage for toolbox bridge and helper workloads (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for extraction and content-bridge incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for extraction and text-processing-adjacent paths (Target: Q1 2027)
- [ ] add dedicated benchmark coverage for toolbox-native orchestration and helper workloads (Target: Q1 2027)
- [ ] harden long-run reliability under sustained toolbox extraction traffic (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze toolbox orchestration/bridge/helper contracts for current major line (Target: Q3 2026)
- [x] define explicit error taxonomy for extraction and registry incident classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] fix CRITICAL data_race in ingestion_toolbox.cpp:224 (atomic fetch_add) — COMPLETED 2026-08-07
- [x] fix HIGH null_dereference in content_toolbox_bridge.cpp (null checks + error messages) — COMPLETED 2026-08-07
- [x] add missing latency metrics to ContentFingerprint (latency_us field, steady_clock measurement) — COMPLETED 2026-08-07
- [x] optimize copy overhead in language_detector.cpp (vector.reserve(32)) — COMPLETED 2026-08-07
- [x] update doc linksets (FUTURE_ENHANCEMENTS.md, PRODUCTION_REQUIREMENTS.md) — COMPLETED 2026-08-07
- [~] complete remaining hardening for builder, bridge, and registry internals (Target: Q4 2026)
- [~] align helper and streaming behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] standardize bridge sink failure handling with descriptive error messages — COMPLETED 2026-08-07
- [x] implement explicit empty-extraction tracking via extract_empty_results counter — COMPLETED 2026-08-07
- [x] add empty-result diagnostics to Prometheus metrics (toolbox_extract_empty_results_total) — COMPLETED 2026-08-07
- [x] add IT-11, IT-12 tests for empty extraction tracking — COMPLETED 2026-08-07
- [~] unify incident taxonomy across extraction, bridge, registry, and helper classes (Target: Q4 2026)
- [~] expand stress coverage for edge cases (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for toolbox bridge, registry, and helper edge scenarios (Target: Q4 2026)
- [x] extend deterministic stress fixtures for toolbox-adjacent extraction workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] create dedicated toolbox-native benchmark suite (Target: Q3 2026, completed 2026-08-07)
- [x] establish initial performance gates GATE-TBX-P1..P6 (Target: Q3 2026, completed 2026-08-07)
- [~] validate proxy and direct benchmark behavior against release baselines (Target: Q4 2026)
- [ ] lock release gates at Q3 2026 baselines and document regression limits (Target: Q1 2027)

### Phase 6: Documentation and Acceptance
- [x] core toolbox module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core toolbox surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] proxy benchmark mapping documented in performance expectations
- [x] dedicated toolbox-native benchmark suite created (Phase 5 - 2026-08-07)
- [ ] remaining hardening tasks closed for orchestration/bridge/helper edge paths
- [ ] baseline measurements collected and release gates certified

## Known Issues and Limitations

- Phase 5 native benchmark suite created but baseline measurements pending (scheduled for Q4 2026 release run)
- Runtime behavior depends on ingestion/content subsystem integrations and workflow profiles
- Selected bridge and helper edge scenarios have hardening in progress (Phase 2-3)

## Breaking Changes

No breaking toolbox contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.