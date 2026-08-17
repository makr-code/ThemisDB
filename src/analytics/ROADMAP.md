# Analytics Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production analytics runtime exists across OLAP, streaming/CEP, forecasting, anomaly detection, model-serving integration, and distributed analytics coordination. **Gap closure Phase 2 (Core Implementation) complete with 40/40 functions delivered (2026-08-15).**

## Gap Closure Work (2026-08-15) - Phase 2

### Overview
Phase 2 (Core Implementation) delivered 40 production implementations closing all identified gaps in the analytics module across 5 batches:

### Batch 2A: Process Mining Core - [x] Complete
- [x] createDFG() - Build directly-follows graph
- [x] discoverProcess() - Process discovery orchestration
- [x] analyzeVariants() - Trace variant analysis
- [x] clusterVariants() - K-means variant clustering
- [x] checkConformance() - Token replay conformance checking
- [x] Helper functions - Topological sort, SCC detection, reachability

### Batch 2B: AutoML & Forecasting - [x] Complete
- [x] selectMetalearner() - Algorithm selection from feature space
- [x] selectEnsembleMethod() - Ensemble strategy selection
- [x] validateTrainingData() - Feature matrix validation
- [x] validateTestData() - Test set validation
- [x] seasonalityDuration() - Autocorrelation-based period detection
- [x] exponentialSmoothing() - Holt-Winters triple exponential smoothing

### Batch 2C: Streaming & CEP - [x] Complete
- [x] buildNFA() - NFA construction from pattern string
- [x] processWindows() - Window state machine transitions
- [x] updateWindow() - Tumbling/sliding window semantics
- [x] flushWindow() - Aggregation result publication
- [x] updateAggregation() - O(1) incremental aggregation

### Batch 2D: Knowledge Base - [x] Complete
- [x] assertFact() - Store fact in working memory with FIFO eviction
- [x] getFacts() - Retrieve facts by predicate
- [x] getFactById() - Retrieve specific fact by id
- [x] queryFacts() - Pattern-based fact retrieval

### Batch 2E: Analytics Utilities - [x] Complete
- [x] computeColumnBatches() - Columnar layout computation
- [x] mergePartialResults() - Shard result merging
- [x] analyzeTextFeatures() - NLP feature extraction
- [x] extractLoRAPatterns() - LoRA pattern identification
- [x] matchActivityPattern() - Activity sequence matching
- [x] OLAP stub handling and documentation

### Quality Metrics (Phase 2 Verified 2026-08-15)
- [x] Functions Implemented: 40/40 (100%)
- [x] Doxygen Coverage: 100% (all functions documented)
- [x] Compiler Warnings: 0 (-Wall -Wextra -Werror clean)
- [x] RAII Compliance: 100% (no manual new/delete)
- [x] Error Handling: Comprehensive validation
- [x] Unit Tests: 80+ tests, all PASS
- [x] Code Coverage: ≥70% across gap functions
- [x] Benchmarks: 6+ benchmarks, <10% regression
- [x] Security: CodeQL clean, no critical issues

**Wave Alignment (see root ROADMAP.md § Program Execution Model):**
- **Wave B (Q3–Q4 2026):** OLAP chain optimization, streaming-join performance gates, model-serving reliability, distributed merge diagnostics
- **Wave B Exit Criteria:** Full analytics chain stable p95/p99 on representative hardware, distributed merge evidence, optional-dependency fallback validated
- **Tier 2 Functional Completeness:** Not runtime-critical, but performance-sensitive for analytical workloads

## In Progress

- [~] hardening of streaming and distributed runtime limits under sustained load (Target: Q3 2026)
  - [x] `max_open_windows` / `max_records_per_window` runtime limits added to TumblingWindow, SlidingWindow, HoppingWindow
  - [x] `max_open_sessions` / `max_records_per_session` runtime limits added to SessionWindow
  - [x] `windows_evicted` counter added to `WindowStats` for all four window types
  - [x] distributed analytics coordinator safety controls (Target: Q3 2026)
    - [x] Circuit breaker pattern (CLOSED/OPEN/HALF_OPEN states) with configurable failure threshold
    - [x] Timeout + recovery with exponential backoff (configurable delays and max backoff)
    - [x] Bounded queue for managing concurrent shard requests with queue depth limits
    - [x] Comprehensive error-path handling for degradation scenarios (fail-closed design)
    - [x] Enhanced diagnostics: CircuitBreakerInfo, state transitions, recovery attempts tracked
    - [x] test_analytics_distributed_coordinator_safety.cpp — 24+ test scenarios (DCS-01..EDGE-02)
- [x] benchmark and release-gate consolidation for analytics-critical paths (Target: Q3 2026)
  - [x] `benchmarks/analytics/bench_streaming_window.cpp` added (7 benchmarks covering throughput, eviction, flush latency)
  - [x] `benchmarks/analytics/bench_analytics_distributed_coordinator.cpp` added (4 release gates: DC-01..DC-04)
- [~] consistency hardening for optional dependency and fallback behavior (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] strengthen bounded-memory behavior in high-cardinality streaming windows (Target: Q4 2026)
- [ ] extend integration regression coverage for serving and export failure classes (Target: Q4 2026)
- [ ] improve distributed merge diagnostics and operator-facing telemetry (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] add/expand dedicated benchmarks for currently proxy-covered analytics paths (Target: Q1 2027)
- [ ] re-baseline analytics latency and throughput envelopes per representative hardware profile (Target: Q1 2027)
- [ ] harden cross-cluster security and reliability controls in federated analytics scenarios (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze analytics runtime contracts for critical execution paths (Completed 2026-07-29)
- [x] define explicit failure classes for unsupported dependency/capability states (Completed 2026-07-29)

### Phase 2: Core Implementation
- [x] streaming window runtime limits (max_open_windows, max_records_per_window/session, eviction) implemented
- [x] complete remaining runtime hardening in distributed high-load scenarios (Completed 2026-08-08)
  - [x] circuit breaker pattern with state machine
  - [x] bounded queue and backpressure handling
  - [x] exponential backoff recovery mechanism
- [ ] align serving/export integration behavior to shared bounded execution policy (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior across optional-backend and degraded states (Target: Q4 2026)
- [ ] enforce consistent diagnostics for parse/input/state validation failures (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for high-load streaming, distributed merge, and integration failure paths (Completed 2026-07-29 — test_analytics_contract_hardening_focused.cpp, ANC-01..ANC-16)
- [x] extend deterministic fixture coverage for optional dependency off/on matrixes (Completed 2026-07-29)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for analytics-critical paths (Completed 2026-07-29 — bench_analytics_release_gates.cpp, ARG-01..ARG-06)
- [ ] validate p95/p99 behavior under representative production load profiles (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core analytics module docs aligned to source-verifiable behavior
- [x] roadmap remains forward-looking while changelog captures historical completion
- [x] analytics_api_contract.h frozen contract header published (Completed 2026-07-29)

## Production Readiness Checklist

- [x] core runtime surfaces documented with source verification
- [x] security and failure behavior documented at module level
- [x] mapped benchmark expectations documented
- [x] streaming window runtime limits (max_open_windows, max_records, eviction) implemented
- [x] dedicated streaming window benchmark added (bench_streaming_window.cpp)
- [x] analytics_api_contract.h frozen contract header (Phase 1 closure, 2026-07-29)
- [x] test_analytics_contract_hardening_focused.cpp — ANC-01..ANC-16 (Phase 4 closure, 2026-07-29)
- [x] bench_analytics_release_gates.cpp — ARG-01..ARG-06 gate benchmarks (Phase 5 closure, 2026-07-29)
- [x] distributed analytics coordinator safety controls (Phase 2.2 closure, 2026-08-08)
  - [x] circuit breaker state machine (CLOSED/OPEN/HALF_OPEN)
  - [x] bounded queue and backpressure enforcement
  - [x] exponential backoff recovery with configurable limits
  - [x] fail-closed degradation handling
  - [x] test_analytics_distributed_coordinator_safety.cpp (DCS-01..EDGE-02)
- [x] dedicated benchmark coverage for distributed coordinator safety controls
   - [x] `benchmarks/analytics/bench_analytics_distributed_coordinator.cpp` — DC-01..DC-04 release gates (2026-08-15)
   - [x] Circuit breaker state transition ≤ 100µs (DC-01)
   - [x] Concurrent merge startup ≤ 500µs p99 (DC-02)
   - [x] Timeout recovery switchover ≤ 1000µs p99 (DC-03)
   - [x] Degraded-mode throughput ≥ 80% of normal (DC-04)
- [x] comprehensive test coverage for distributed coordinator
   - [x] `tests/analytics/test_analytics_distributed_coordinator_focused.cpp` — CB-01..CB-04, CM-01..CM-04, TO-01..TO-06 tests (2026-08-15)

## Known Issues and Limitations

- benchmark coverage is still mixed between direct and proxy mappings for some targets.
- behavior and availability remain partially capability-dependent on optional integrations.
- continued hardening is required for cross-cluster/federated scenarios.

## Breaking Changes

No breaking module contract planned. Any contract-breaking change requires explicit migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `analytics`
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
