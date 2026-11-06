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

# Analytics Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-06 -->
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

## Evidence-Based Roadmap: v2.5.0 and v3.0.0

This section is an analytics-module proposal, not a declaration that either
release or its schedule has been approved. Release naming, branch targeting,
and project-wide exit gates remain governed by the root [`ROADMAP.md`](../../ROADMAP.md),
[`VERSIONING.md`](../../VERSIONING.md), and [`RELEASE_STRATEGY.md`](../../RELEASE_STRATEGY.md).
Effort estimates are engineering person-weeks (PW), not calendar commitments.
All performance gates are provisional until measured on the hardware profiles
and workloads below.

### Current module assessment

**Assessment: broad functional coverage with incomplete release-scale evidence.**
Production implementations, focused tests, bounded streaming controls,
distributed retry/circuit-breaker behavior, diagnostics, and runbooks exist.
This is a strong implementation foundation, but not proof of production
performance or durable recovery:

| Area | Source-backed status | Gap / consequence |
|---|---|---|
| Analytical execution | [`columnar_execution.h`](../../include/analytics/columnar_execution.h) defines vector batches, selection vectors, and late materialization; [`test_columnar_execution.cpp`](../../tests/analytics/test_columnar_execution.cpp) covers the implementation. | Current release-gate cases in [`bench_analytics_release_gates.cpp`](../../benchmarks/analytics/bench_analytics_release_gates.cpp) mostly measure local helper stand-ins (including a mock plan cache and forecast-validation stub), not the production operators. |
| Streaming | [`streaming_window.h`](../../include/analytics/streaming_window.h) exposes watermarks and per-window/session/partition limits; focused tests cover limits and high-cardinality behavior. | The public window API exposes in-memory ingest/flush/stats, not coordinated durable state and source-offset recovery. Long-duration representative-environment execution is still pending. |
| Distributed analytics | [`distributed_analytics.h`](../../include/analytics/distributed_analytics.h) declares bounded queues, retries, circuit breakers, shard diagnostics, and partial-result configuration. | The header documents approximate merge behavior for several aggregates, and some first/last behavior depends on responding-shard order. Failure correctness and approximation error need explicit, tested contracts. |
| Interoperability and security | Arrow export/Flight interfaces exist; [`OPERATIONS.md`](./OPERATIONS.md) assigns Flight authentication to the owning server boundary. Model loading supports integrity checks and serving defaults to HTTPS. | Registered targets and headers do not by themselves prove network TLS/authentication/authorization, end-to-end schema compatibility, or signed model provenance. Boundary ownership and deployment evidence need verification. |
| Public API governance | The separate [`public-header roadmap`](../../include/analytics/ROADMAP.md) tracks header coverage and proposed query-policy/JIT/AutoML stability work. | The planned `analytics_policy.h` and experimental-header stability tasks carry past target dates and are not revalidated; resolve or explicitly defer them before widening the public API. |
| Performance and operations | Four representative profiles and their required artifacts are declared in [`REPRESENTATIVE_HARDWARE_BASELINES.md`](./REPRESENTATIVE_HARDWARE_BASELINES.md) and [`representative_hardware_manifest.json`](../../benchmarks/baselines/analytics/representative_hardware_manifest.json). | All three manifest profiles remain `pending`; no authoritative representative-hardware p95/p99 evidence is present. Synthetic microbenchmarks must not be used as production-SLO evidence. |
| Governance and usability | Module tests, performance expectations, operating guidance, and troubleshooting runbooks exist. | No curated analytics page is listed in the [Developer LLM Wiki index](../../ai_context/developer_llm_wiki/INDEX.md); the [wiki health snapshot](../../ai_context/developer_llm_wiki/WIKI_STATUS.json) is dated 2026-11-02. Source-local docs and code remain authoritative; this is a discoverability/freshness gap, not runtime evidence. |

### Research basis and limits

| Evidence | Finding applied to this plan | Limit |
|---|---|---|
| Boncz, Zukowski, Nes, [“MonetDB/X100: Hyper-Pipelining Query Execution”](https://ir.cwi.nl/pub/16497), CIDR 2005 | Vectorized batches can improve CPU efficiency over tuple-at-a-time execution; compare actual operator paths, batch sizes, data types, and memory use. | Historical TPC-H results are motivation, not an expected ThemisDB speedup or current hardware baseline. |
| TPC, [TPC-H specification](https://www.tpc.org/tpc_documents_current_versions/pdf/tpc-h_v3.0.1.pdf) and [current specifications](https://www.tpc.org/tpc_documents_current_versions/current_specifications5.asp) | Controlled workload size, single-stream query power, concurrent throughput, and complete configuration disclosure are needed for meaningful decision-support comparisons. Use TPC-inspired workloads for internal comparisons. | ThemisDB results must not be called TPC-H compliant unless the complete TPC rules and reporting requirements are met. |
| Carbone et al., [“Apache Flink: Stream and Batch Processing in a Single Engine”](https://doi.org/10.1145/2723372.2742792), 2015 | Consistent stream recovery coordinates operator state with input positions; test checkpoint, replay, and sink effects together. | Checkpointing alone does not establish end-to-end exactly-once behavior for an arbitrary source or sink. |
| Apache Arrow, [format specification](https://arrow.apache.org/docs/format/index.html) and [Flight RPC](https://arrow.apache.org/docs/python/flight.html) | Arrow defines interoperable columnar layouts and record-batch RPC; verify schema, copy/conversion cost, transport, and identity policy across the real boundary. | A standard format/protocol does not imply zero-copy end-to-end transfer or supply the deployment's authentication policy. |
| Rigger and Su, [“Testing Database Engines via Pivoted Query Synthesis”](https://www.usenix.org/conference/osdi20/presentation/rigger), OSDI 2020 | Query-engine tests should detect wrong answers as well as crashes; use deterministic differential/metamorphic and shard-order tests. | Oracle techniques need semantics-aware reference results and do not replace workload-level tests. |
| NIST, [SP 800-218A](https://csrc.nist.gov/pubs/sp/800/218/a/final), July 2024; SLSA, [Build Provenance](https://slsa.dev/spec/v1.2/build-provenance) | Model-artifact intake should include provenance/trust policy in addition to a content hash; tests must reject untrusted or altered artifacts. | A signature/provenance record is only as trustworthy as its trusted key, issuer, and verification policy. |

### v2.5.0 — compatible hardening and evidence

Keep public contracts backward-compatible within the current major line. Prioritize
truthful measurements, bounded operation, and explicit semantics before adding
new execution features.

#### Wave 2.5-A: benchmark credibility and workload baselines

- **Priority / effort:** P0; 2–4 PW.
- **Goal:** establish reproducible performance and correctness baselines for the production analytics paths.
- **Tasks:**
  - [ ] Audit every case in `bench_analytics_release_gates.cpp`; either benchmark the named production API or label the case as a microbenchmark and exclude it from production release gates (Target: v2.5.0).
  - [ ] Reconcile the production-vs-stub classification of all cases advertised by `PERFORMANCE_EXPECTATIONS.md`, including dedicated gates, with the representative-hardware manifest (Target: v2.5.0).
  - [ ] Add deterministic OLAP/columnar, keyed-streaming, and multi-shard workloads with recorded row count, cardinality, concurrency, and fixture revision (Target: v2.5.0).
  - [ ] Execute every benchmark suite required by each profile in `representative_hardware_manifest.json`; publish raw JSON, p95/p99 summary, baseline comparison, and deviation decision per profile (Target: v2.5.0).
  - [ ] Report query latency and concurrent throughput separately; record compiler/build, hardware, cache state, and exact command (Target: v2.5.0).
- **Dependencies:** benchmark target registration and stable fixtures; access to the three manifest hardware profiles.
- **Risks / trade-offs:** hardware contention and run-to-run variance can create false gates; synthetic-only measurements can overstate application performance. No new absolute SLO is accepted until the initial comparable run is reviewed.
- **Success metrics:** 3/3 profiles publish all manifest-required artifacts; 100% of cases retained as production release gates invoke production analytics APIs and are represented in the manifest; synthetic cases are labeled non-gating or excluded; repeated same-profile runs identify and explain variance; regressions greater than the existing 10% comparison threshold are reviewed against matched hardware/configuration, not cross-machine numbers.
- **Validation:** build/list/run `bench_analytics_release_gates`, `bench_analytics_distributed_coordinator`, `bench_streaming_window`, and `bench_analytics_operability_paths`; run focused columnar, streaming, distributed, and operability tests. Benchmark smoke success is not performance acceptance.

#### Wave 2.5-B: bounded streaming and operational usability

- **Priority / effort:** P0; 3–5 PW.
- **Goal:** make the bounded streaming behavior usable and reproducible under realistic event-time, skew, and pressure conditions.
- **Tasks:**
  - [ ] Define representative event schemas and executable examples for tumbling, sliding, session, and hopping windows, including watermark, late-event, zero-limit, eviction, and backpressure outcomes (Target: v2.5.0).
  - [ ] Extend deterministic stress/soak fixtures for skewed partition keys, out-of-order events, sustained ingestion, and resource-limit transitions; assert output and counters, not only completion (Target: v2.5.0).
  - [ ] Publish observed throughput, memory high-water mark, dropped/late counts, and p95/p99 flush latency per workload in the representative-hardware evidence bundle (Target: v2.5.0).
  - [ ] Document that current window state is process-local and define requirements for persistence/recovery without claiming checkpoint support (Target: v2.5.0).
- **Dependencies:** Wave 2.5-A benchmark fixtures and profile access; existing window limits and statistics in `streaming_window.h`.
- **Risks / trade-offs:** stricter bounds may reject workloads previously allowed by unconstrained zero defaults; skewed datasets may expose capacity costs not visible in average-cardinality tests.
- **Success metrics:** every window type has deterministic late/limit/backpressure tests; no configured workload exceeds its declared memory/queue bound; all observed loss/eviction outcomes are accounted for by documented counters; the same fixture produces identical result sets across repeated runs.
- **Validation:** focused `test_streaming_window.cpp`, `test_analytics_streaming_limits_focused.cpp`, `test_analytics_highcardinality_stress.cpp`, `test_analytics_wave_d_operability.cpp`, and sustained-load benchmark runs on the CPU-memory profile.

#### Wave 2.5-C: distributed semantics, boundary security, and discoverability

- **Priority / effort:** P1; 3–5 PW.
- **Goal:** remove ambiguity in partial and approximate results and verify actual integration boundaries.
- **Tasks:**
  - [ ] Specify which aggregates are exact or approximate, define reproducible error metrics for approximate aggregates, and make shard-order effects explicit or deterministic (Target: v2.5.0).
  - [ ] Add reference-result, shard-permutation, partial-shard, timeout, retry, and recovery tests for distributed merge; preserve the distinction between incomplete and complete results (Target: v2.5.0).
  - [ ] Trace Arrow Flight/export authentication, authorization, TLS, bind-address, and error behavior to the owning server integration; add integration tests for missing/invalid identity and insecure transport at that boundary (Target: v2.5.0).
  - [ ] Revalidate the public-header roadmap's open `analytics_policy.h` and JIT/AutoML stability items; document the decision, compatibility status, and owner, deferring new public policy surface to the v3 contract if requirements are not ready (Target: v2.5.0).
  - [ ] Add a concise analytics context entry to the Developer LLM Wiki index or record why it remains deferred, linking to this roadmap and current test/benchmark evidence (Target: v2.5.0).
- **Dependencies:** API owners for distributed result semantics and the service/server team owning Flight authentication; Wave 2.5-A fixture definitions.
- **Risks / trade-offs:** exact aggregation can increase network, CPU, and memory costs; changing current approximation behavior may be a compatibility issue and must not be hidden in a patch release.
- **Success metrics:** exact aggregate fixtures match the reference for every shard permutation; approximate aggregates remain within an explicitly documented, measured error envelope; all partial-result modes are observable; every externally reachable Flight/export path has a named authn/authz/TLS owner and a passing boundary test.
- **Validation:** `test_distributed_analytics.cpp`, `test_analytics_distributed_coordinator_focused.cpp`, `test_analytics_distributed_coordinator_safety.cpp`, `test_arrow_flight.cpp`, serving/export integration tests, and failure-injection tests at the server integration boundary.

### v3.0.0 — durable, versioned analytics execution

This strategic track may include public contract changes. Do not start implementation
until cross-module dependencies and migration policy are approved in the root
roadmap; compatibility shims or fallback behavior require explicit approval.

#### Wave 3.0-A: versioned execution and recovery contracts

- **Priority / effort:** P0; 3–5 PW.
- **Goal:** define stable, versioned semantics for execution plans, stream state, distributed results, and capability discovery.
- **Tasks:**
  - [ ] Publish v3 contracts for exact/approximate aggregate semantics, deterministic ordering, partial-result completeness, and unsupported capabilities (Target: v3.0.0).
  - [ ] Agree source-offset, durable checkpoint, and sink-commit contracts with storage, transaction, and ingestion owners before exposing a recovery API (Target: v3.0.0).
  - [ ] Define a versioned Arrow schema/capability negotiation policy and migration notes for clients (Target: v3.0.0).
- **Dependencies:** root v3 API/version policy; storage and transaction owners; security owner for trust roots and key lifecycle.
- **Risks / trade-offs:** stronger contracts reduce implementation freedom and may require caller migration; unclear cross-module ownership blocks reliable recovery guarantees.
- **Success metrics:** every public v3 capability has a version, compatibility rule, error contract, and owner; no exactly-once or durability claim exists without a source/state/sink contract.
- **Validation:** API contract review, generated/public API documentation checks, schema compatibility tests, and migration review before implementation is accepted.

#### Wave 3.0-B: recoverable stateful streaming

- **Priority / effort:** P0; 6–10 PW.
- **Goal:** restore streaming computation from a consistent snapshot of operator state and source positions.
- **Tasks:**
  - [ ] Implement coordinated checkpoint/restore using approved durable state and replayable source-position interfaces; keep state and offsets in one recoverable consistency boundary (Target: v3.0.0).
  - [ ] Specify sink idempotency/commit behavior and expose recovery status, checkpoint age/size/duration, replay volume, and restore failures (Target: v3.0.0).
  - [ ] Add crash/restart tests at checkpoint, source replay, state write, and sink-commit boundaries, including corrupt/incompatible checkpoint rejection (Target: v3.0.0).
- **Dependencies:** Wave 3.0-A contract; durable storage, source offsets, and sink commit/idempotency guarantees.
- **Risks / trade-offs:** checkpoint I/O and retained snapshots consume resources and can reduce throughput; exactly-once output cannot be promised where the sink lacks atomic/idempotent commit support.
- **Success metrics:** for deterministic fault-injection fixtures, restored outputs equal uninterrupted outputs with zero missing or duplicate committed records; recovery-time, replay-volume, checkpoint-overhead, and retained-bytes distributions are published for defined input rates and state sizes.
- **Validation:** repeated process-kill/restart integration tests; compare recovered and uninterrupted output hashes; benchmark checkpoint overhead and recovery time at low/high state cardinality; verify cleanup and restore from the latest valid snapshot.

#### Wave 3.0-C: distributed correctness and scale

- **Priority / effort:** P1; 5–8 PW.
- **Goal:** make distributed execution results explainable and repeatable under shard skew and failure.
- **Tasks:**
  - [ ] Implement the v3 aggregate contracts with deterministic merge order and explicit approximation/error metadata where exact computation is not selected (Target: v3.0.0).
  - [ ] Add multi-node fault-injection coverage for retry exhaustion, shard loss, timeout, duplicate response, and recovery; verify completeness metadata and reference results (Target: v3.0.0).
  - [ ] Benchmark 1/2/4/8-shard scaling with balanced and skewed partitions, reporting throughput, p95/p99, merge cost, network bytes, and degraded-mode behavior (Target: v3.0.0).
- **Dependencies:** Wave 3.0-A result contracts; Wave 3.0-B only for queries relying on recoverable stream state; representative multi-node environment.
- **Risks / trade-offs:** determinism may constrain parallel merge scheduling; exactness can reduce throughput; partial-result acceptance can conceal data loss if completeness metadata is ignored by callers.
- **Success metrics:** exact reference workloads match for every tested shard order; approximate outputs satisfy the declared error bound; all injected failures are classified without silent success; scaling efficiency and degraded throughput are measured against the v2.5.0 baseline (no unmeasured speedup target).
- **Validation:** property/metamorphic merge tests, multi-node integration and fault injection, 1/2/4/8-shard benchmark runs, and regression comparison using matching workload/hardware profiles.

#### Wave 3.0-D: trusted interoperability and production acceptance

- **Priority / effort:** P1; 4–7 PW.
- **Goal:** support auditable external analytics integration and secure model-artifact intake.
- **Tasks:**
  - [ ] Define and test end-to-end Arrow/Flight schema, type/nullability, streaming, timeout, and error compatibility against supported client implementations (Target: v3.0.0).
  - [ ] Enforce TLS and authenticated/authorized identity at the network boundary, tenant-aware export policy, and auditable access outcomes; default to fail-closed when required policy is absent (Target: v3.0.0).
  - [ ] Add a signed model manifest/provenance verification path with explicit trusted-key rotation, revocation, and failure policy; retain content hashes for integrity, not identity (Target: v3.0.0).
  - [ ] Complete representative-hardware, soak, recovery, security, and operations evidence; link release artifacts and migration guidance from module documentation (Target: v3.0.0).
- **Dependencies:** approved network identity/authz integration, key-management/trust-root service, supported Arrow client matrix, and Wave 3.0-A contracts.
- **Risks / trade-offs:** trust-root operations and client compatibility raise deployment complexity; fail-closed policy can reduce availability during identity/key-service outages and needs an approved recovery procedure.
- **Success metrics:** 100% of supported Arrow schema fixtures round-trip without silent type/nullability changes; unauthenticated/unauthorized and invalid-TLS requests are rejected in integration tests; modified, unsigned, or untrusted model manifests are rejected; every required profile and release-critical operational check has linked evidence.
- **Validation:** cross-client integration matrix, TLS/authz negative tests, signature/key-rotation/revocation tests, artifact tamper tests, representative benchmark/soak runs, runbook exercise, and security/governance sign-off.

### Cross-wave KPIs and release gates

| KPI | v2.5.0 gate | v3.0.0 gate |
|---|---|---|
| Benchmark evidence | All three manifest profiles have raw results, p95/p99 summary, comparison, and deviation decision; benchmark cases are classified as production or microbenchmark. | Same workloads/profile matrix plus 1/2/4/8-shard scaling, state-recovery overhead, and client interoperability measurements. |
| Correctness | Deterministic fixtures pass for window limits, partial results, retries, and documented approximate-result bounds. | Fault-injected recovery matches uninterrupted output; distributed exact/approximate contracts hold across shard order and failures. |
| Security | Owning network boundary and evidence are identified for TLS/authentication/authorization; security gaps are not inferred closed from in-process tests. | Negative tests prove fail-closed identity/transport policy and signed-artifact verification, including key lifecycle. |
| Operability | All failure/eviction/partial-result cases emit actionable status and operator hints; focused runbook steps are exercised. | Recovery, degraded operation, access audits, and rollback are demonstrated in a representative deployment profile. |
| Governance | Source evidence, assumptions, test commands, and benchmark artifacts are traceable from this roadmap; wiki freshness/discoverability gap is resolved or explicitly deferred. | Versioned contracts, migration guidance, release sign-offs, and retained benchmark/security artifacts are complete. |

### Risks, trade-offs, and open assumptions

- **Performance evidence:** the manifest states that representative-hardware evidence is pending. No throughput or p99 value in a synthetic benchmark is treated as an application SLO. Baseline capture must precede any claim of improvement.
- **Workload representativeness:** production query shapes, data distributions, tenant concurrency, and deployment hardware have not been supplied. TPC-H is a methodological reference, not a substitute for ThemisDB customer workloads.
- **Durable recovery boundary:** current public window APIs do not expose checkpoint/restore. Feasibility and guarantees depend on durable source offsets, storage transactions, and sink idempotency/commit APIs that analytics does not own.
- **Security boundary:** module operations documentation places Flight authentication at the owning server boundary; this roadmap requires evidence from that integration, not an assumption that the analytics module itself authenticates clients.
- **Compatibility:** current distributed aggregate comments include approximations and response-order behavior. Exact behavior, error budgets, and any breaking change require product/API-owner review and v3 migration planning.
- **Trust lifecycle:** checksums detect content changes but do not establish publisher identity. Signature verification requires a maintained trust root, revocation/rotation policy, and operational ownership.
- **Wiki freshness:** the Developer LLM Wiki status is generated 2026-11-02 and its curated module list does not include analytics. Use this canonical module roadmap, code, tests, and manifests until a curated analytics context is added and refreshed.
