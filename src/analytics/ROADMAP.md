# Analytics Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-09-16 -->
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

- [x] hardening of streaming and distributed runtime limits under sustained load (Target: Q3 2026)
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
  - [x] `StreamingRuntimeLimits` struct added to `analytics_api_contract.h`: `max_events_per_window`, `max_window_memory_bytes`, `BackPressureMode` (DROP/BLOCK/SHED) (2026-08-24)
  - [x] `test_analytics_streaming_limits_focused.cpp` — SRL-01..10 tests added (2026-08-24)
- [x] benchmark and release-gate consolidation for analytics-critical paths (Target: Q3 2026)
  - [x] `benchmarks/analytics/bench_streaming_window.cpp` added (7 benchmarks covering throughput, eviction, flush latency)
  - [x] `benchmarks/analytics/bench_analytics_distributed_coordinator.cpp` added (4 release gates: DC-01..DC-04)
- [x] consistency hardening for optional dependency and fallback behavior (Target: Q3 2026)
  - [x] model import integrity guardrails: optional SHA-256 verification API + fail-closed enforcement toggle (`require_model_integrity`) (Completed 2026-08-19)
  - [x] TF Serving secure transport baseline: HTTPS default + explicit plaintext opt-in (`allow_insecure_transport`) (Completed 2026-08-19)
  - [x] LLM response schema hardening for fraud/5R/prediction tasks (type/range/bounds checks) (Completed 2026-08-19)
  - [x] prompt injection mitigation in LLM process analyzer: `sanitizeUserContent()` strips control characters and known injection prefixes from all user-supplied JSON fields before prompt embedding (Completed 2026-08-19)
  - [x] externalize `sanitizeUserContent()` injection prefix list to a configuration file: `LLMConfig::injection_prefix_config_path` added; `loadInjectionPrefixes()` loads from file with 13-pattern fallback; `config/analytics/injection_prefixes.txt` shipped as default template (Completed 2026-08-19 Batch 6)
  - [x] `test_analytics_optional_fallback_focused.cpp` — OPF-01..08 tests for `MLServingStatus::UNAVAILABLE`, `AnalyticsErrorCode::STREAM_BACKPRESSURE`, and struct composition added (2026-08-24)
  - [x] fail-closed behavior verified for Arrow IPC/Parquet/Feather export (`throwArrowUnavailable()`) and Arrow Flight in-process fallback (Completed 2026-08-19)
  - [x] multiplication overflow fix in circuit breaker exponential backoff: bit-shift capped to 30 to prevent UB when `recovery_attempts ≥ 32` (Completed 2026-08-19)

## Planned Features

### Short-term (3-6 months)
- [x] strengthen bounded-memory behavior in high-cardinality streaming windows (Completed 2026-08-19)
- [x] extend integration regression coverage for serving and export failure classes (Completed 2026-08-19)
- [x] improve distributed merge diagnostics and operator-facing telemetry (Completed 2026-08-19)

### Mid-term (6-12 months)
- [x] add/expand dedicated benchmarks for currently proxy-covered analytics paths (Completed 2026-09-16)
  - [x] `benchmarks/analytics/bench_analytics_dedicated_gates.cpp` added for ANA-BM-01..04 gate coverage
  - [x] `benchmarks/analytics/bench_analytics_operability_paths.cpp` added for direct export serialization, high-cardinality streaming, distributed retry, and serving fail-closed validation coverage
  - [x] `src/analytics/PERFORMANCE_EXPECTATIONS.md` migrated to direct analytics benchmark mappings with no proxy-only module targets remaining
- [~] re-baseline analytics latency and throughput envelopes per representative hardware profile (Target: Q1 2027)
  - [x] dedicated baseline gates now exist in `benchmarks/analytics/bench_analytics_dedicated_gates.cpp` (ANA-BM-01..04)
  - [x] representative-hardware matrix documented in `src/analytics/REPRESENTATIVE_HARDWARE_BASELINES.md`
  - [x] required artifact contract defined in `benchmarks/baselines/analytics/representative_hardware_manifest.json`
  - [ ] authoritative execution evidence still pending on representative hardware profiles
- [x] harden cross-cluster security and reliability controls in federated analytics scenarios (Target: Q1 2027)
  - [x] **AN1**: Wire federated query coordinator — per-shard retry with exponential backoff and ±20% jitter; permanent-failure fast-path skips retry; `Config::RetryConfig` (max_retries=2, base_delay_ms=50, max_delay_ms=500) (Completed 2026-08-26)
  - [x] **AN2**: Forecasting model integrity check — CRC-32 computed at `serialize()` time, verified at `deserialize()` time; legacy models without checksum pass with `THEMIS_WARN`; corrupted checksum returns error (Completed 2026-08-26)
  - [x] `tests/analytics/test_wave_next_analytics_hardening.cpp` — AN1-01..AN1-04, AN2-01..AN2-04 regression tests (Completed 2026-08-26)

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
- [x] align serving/export integration behavior to shared bounded execution policy (Completed 2026-09-16)
  - **Concrete plan**: Define a `BoundedExecutionPolicy` struct (max_latency_ms, max_concurrent_requests, queue_depth) in `analytics_api_contract.h`; apply to `MLServingClient::infer()` (ONNX + TF Serving paths) and `AnalyticsExporter::exportToFile()` using the existing circuit-breaker infrastructure as the enforcement layer. Tracked as Wave B exit criterion.
  - [x] `BoundedExecutionPolicy` struct added to `analytics_api_contract.h` with full field semantics and enforcement contract documentation (Completed 2026-08-19)
  - [x] `MLServingClient::infer(req, policy)` overload implemented: concurrency (`max_concurrent_requests`) and timeout (`max_latency_ms`) enforcement; new `TIMEOUT` and `POLICY_REJECTED` status codes added to `MLServingStatus` (Completed 2026-08-19)
  - [x] `IAnalyticsExporter::exportToFile(batch, path, options, policy)` non-virtual wrapper implemented: concurrency and timeout enforcement using atomic in-flight counter; `POLICY_REJECTED` added to `ExportStatus` (Completed 2026-08-19)
  - [x] `BoundedExecutionPolicy default_policy` integrated into `MLServingConfig`: `infer(req)` routes through `infer(req, default_policy)` when constrained (Completed 2026-08-19 Batch 6)
  - [x] `BoundedExecutionPolicy policy` integrated into `ExportOptions`: used as fallback in `exportToFile(…, policy)` when the explicit policy is unconstrained (Completed 2026-08-19 Batch 6)
  - [x] 15 targeted regression tests added for policy enforcement paths (BEP-01..BEP-15 in `test_analytics_bounded_execution_policy.cpp`) (Completed 2026-08-19 Batch 6)
  - [x] `operation_id`, `correlation_id`, `failure_class`, and `operator_hints` surfaced across export/serving/distributed result types (Completed 2026-09-16)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior across optional-backend and degraded states (Completed 2026-08-19)
  - [x] fail-closed policy for insecure TF Serving transport (Completed 2026-08-19)
  - [x] fail-closed policy for model import integrity mismatch (Completed 2026-08-19)
  - [x] prompt injection mitigation in LLM analyzer: `sanitizeUserContent()` helper (Completed 2026-08-19)
  - [x] fail-closed verified for Arrow IPC/Parquet/Feather export (`throwArrowUnavailable()`) and Arrow Flight in-process fallback (Completed 2026-08-19)
  - [x] multiplication overflow in circuit breaker backoff fixed: shift capped to 30 bits (Completed 2026-08-19)
- [x] enforce consistent diagnostics for parse/input/state validation failures (Completed 2026-08-19)
  - [x] stricter LLM JSON schema validation for high-risk tasks (Completed 2026-08-19)
  - [x] consistent `spdlog::debug` late-record diagnostics added to SlidingWindow, SessionWindow, HoppingWindow (TumblingWindow was already done) (Completed 2026-08-19)
  - [x] OLAP input validation diagnostics (empty collection, GroupingSets mode) already present (Verified 2026-08-19)

### Phase 4: Tests
- [x] expand focused regressions for high-load streaming, distributed merge, and integration failure paths (Completed 2026-07-29 — test_analytics_contract_hardening_focused.cpp, ANC-01..ANC-16)
- [x] extend deterministic fixture coverage for optional dependency off/on matrixes (Completed 2026-07-29)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for analytics-critical paths (Completed 2026-07-29 — bench_analytics_release_gates.cpp, ARG-01..ARG-06)
- [~] validate p95/p99 behavior under representative production load profiles (Target: Q4 2026) — ARG-01..ARG-06 + BM_AnalyticsChain_SustainedLoad require representative hardware execution; instrumentation complete, baseline run sandbox-blocked; pending hardware sign-off

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
- [x] BoundedExecutionPolicy integrated into MLServingConfig and ExportOptions (Batch 6, 2026-08-19)
   - [x] `MLServingConfig::default_policy` — per-client default applied to every `infer()` call
   - [x] `ExportOptions::policy` — per-export default used as fallback in `exportToFile(…, policy)`
   - [x] `test_analytics_bounded_execution_policy.cpp` — BEP-01..BEP-15 regression tests (2026-08-19)
- [x] injection prefix list externalized from static code to operator-configurable file (Batch 6, 2026-08-19)
   - [x] `LLMConfig::injection_prefix_config_path` field added
   - [x] `loadInjectionPrefixes()` helper with built-in 13-pattern fallback
   - [x] `config/analytics/injection_prefixes.txt` default template shipped
- [x] direct operability benchmark coverage added for export, distributed retry, high-cardinality streaming, and serving fail-closed validation (Completed 2026-09-16)
- [x] analytics Wave-D focused operability regression suite added (`tests/analytics/test_analytics_wave_d_operability.cpp`, WDO-01..WDO-07) (Completed 2026-09-16)
- [x] analytics operability runbook added (`docs/troubleshooting/analytics_operability_runbook.md`) (Completed 2026-09-16)
- [~] representative-hardware baseline matrix and manifest added; execution evidence pending (`src/analytics/REPRESENTATIVE_HARDWARE_BASELINES.md`, `benchmarks/baselines/analytics/representative_hardware_manifest.json`)

## Known Issues and Limitations

- representative-hardware execution evidence for analytics p95/p99 baselines is still pending.
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
- [~] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
  - [x] result surfaces now expose `operation_id`, `correlation_id`, `failure_class`, and operator remediation hints for export, serving, and distributed analytics
  - [x] high-cardinality stress coverage added in `tests/analytics/test_analytics_highcardinality_stress.cpp`
  - [x] fail-closed and degraded-path operability coverage added in `tests/analytics/test_analytics_wave_d_operability.cpp`
  - [x] direct benchmark coverage added in `benchmarks/analytics/bench_analytics_dedicated_gates.cpp` and `benchmarks/analytics/bench_analytics_operability_paths.cpp`
  - [ ] representative-hardware exporter reliability evidence still pending
- [~] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
  - [x] long-duration soak coverage added in `tests/integration/test_analytics_pipeline_soak.cpp`
  - [x] soak-style repeated distributed degradation coverage added in `tests/analytics/test_analytics_wave_d_operability.cpp` (WDO-07)
  - [ ] long-duration representative-environment soak execution evidence still pending
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Completed 2026-09-16)
  - [x] `docs/operability/RUNBOOK_ANALYTICS_PIPELINE.md` covers module-wide steady-state/SLO operations
  - [x] `docs/troubleshooting/analytics_operability_runbook.md` covers backpressure, open circuit breakers, export failures, TLS/integrity issues, and baseline regressions

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [~] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
  - [x] distributed, export, and serving fail-closed diagnostics are regression-tested (`tests/analytics/test_analytics_wave_d_operability.cpp`, WDO-01..WDO-07)
  - [ ] representative-hardware validation for optional accelerated environments still pending
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
  - [x] ANA-BM-01..04 dedicated benchmark gates exist
  - [x] hardware matrix and artifact manifest defined
  - [ ] authoritative benchmark results still pending
- [~] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
  - [x] diagnostic metadata surfaced on analytics result types
  - [x] module runbook coverage added (`docs/operability/RUNBOOK_ANALYTICS_PIPELINE.md`, `docs/troubleshooting/analytics_operability_runbook.md`)
  - [ ] alert wiring still depends on deployment-specific observability integration

---

## Wave D Delivery Snapshot (2026-09-16)

Source-complete Wave D analytics artifacts are in place, while representative-hardware evidence remains pending:

| Deliverable | File | Status |
|---|---|---|
| Analytics soak test | `tests/integration/test_analytics_pipeline_soak.cpp` | ✅ Delivered |
| Analytics high-cardinality stress test | `tests/analytics/test_analytics_highcardinality_stress.cpp` | ✅ Delivered |
| Analytics operability regression suite | `tests/analytics/test_analytics_wave_d_operability.cpp` | ✅ Delivered |
| Analytics operator runbooks | `docs/operability/RUNBOOK_ANALYTICS_PIPELINE.md`; `docs/troubleshooting/analytics_operability_runbook.md` | ✅ Delivered |
| Dedicated benchmark gates | `benchmarks/analytics/bench_analytics_dedicated_gates.cpp` | ✅ Delivered |
| Direct operability benchmarks | `benchmarks/analytics/bench_analytics_operability_paths.cpp` | ✅ Delivered |
| Representative-hardware baseline evidence | `src/analytics/REPRESENTATIVE_HARDWARE_BASELINES.md`; `benchmarks/baselines/analytics/representative_hardware_manifest.json` | ⏳ Pending execution/sign-off |

**Soak test acceptance criteria met:**
- `AnalyticsSoak_TimeSeriesAggregation`: throughput ≥ 10 000 ops/sec; no exceptions
- `AnalyticsSoak_ColumnarScanThroughput`: deterministic results; no exceptions
- `AnalyticsSoak_QueryExecutionStability`: query p99 ≤ 1 ms; cache hit rate ≥ 99%

**Stress test acceptance criteria met:**
- `HighCardinalityMetricIngestion`: 5000 distinct metrics ingested within 5 s
- `ConcurrentWindowAggregationStress`: 8 threads × 1000 adds; no exceptions; expected flush count
- `ColumnarProjectionUnderHighCardinality`: 5000 cardinality × 256 rows within 10 s

**Benchmark gates (ANA-BM-01..04):**
- ANA-BM-01: Time-series ingest p95 ≤ 2 µs/event
- ANA-BM-02: Columnar scan p95 ≤ 10 µs/batch
- ANA-BM-03: Aggregation throughput ≥ 10 000 ops/sec
- ANA-BM-04: Window rollup p99 ≤ 5 µs/flush
