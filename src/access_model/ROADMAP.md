---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Module - Roadmap

**Version:** 1.0.0<br>
**Status:** ACTIVE (runtime transfer hardening implemented; verification and Wave B evidence pending)
**Last Validated:** 2026-11-06 (source review; runtime validation not performed)
**Phase 5-6 Plan:** `ai_working/ACCESS_MODEL_PHASE_56_IMPLEMENTATION_PLAN.md`

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->

---

## Current Status

**Phase 1 Complete:** Unified architecture documented, core interfaces defined.<br>
**Phase 2 Runtime Hardening:** Tier validation, real adapter transfers, bounded queue admission, completion ownership, metrics, and bounded transition history are implemented. Focused test/build verification remains pending.
**Phases 3-4 Integration:** Cache eviction and storage hot-access listeners are present; backend-level integration evidence remains deployment-specific.
**Phases 5-6 Observability & Hardening:** Structured logs, trace context, metrics, and focused regression coverage are present. Representative hardware, sanitizer, and release-gate evidence remains pending.
  - Release readiness is canonical in the repository [`ROADMAP.md`](../../ROADMAP.md). The current benchmark source does not substantiate the module's historical gate-pass claims; see [`MODULE_GAPS.md`](./MODULE_GAPS.md) for the documentation/source discrepancy.

---

## In Progress

- [~] keep module-level roadmap/checklist claims synchronized with source-verified wave gates and representative-hardware evidence updates (Target: Q4 2026)

## Implementation Phases

### Phase 1: Architecture & Interfaces ✅ (DONE)
- [x] Create unified access model documentation (UNIFIED_ACCESS_MODEL.md)
- [x] Create integration guide (CACHE_STORAGE_INTEGRATION.md)
- [x] Define AccessTier abstract interface
- [x] Define AccessCoordinator broker interface
- [x] Define promotion/demotion data structures
- [x] Define AgeBasedPolicy (unified aging)
- [x] Define AccessMetrics (observability)

### Phase 2: Core Coordinator Implementation 🟡 (runtime hardening implemented; verification pending)
- [x] access_coordinator.cpp implementation (530 LOC, Phase 2 core logic complete)
  - [x] Tier registry management
  - [x] Eviction signal processing
  - [x] Storage access pattern detection
  - [x] Background promotion/demotion workers
  - [x] Age-based policy enforcement
  - [x] Correlation ID generation & tracking
- [x] age_based_policy.cpp helper methods (303 LOC)
- [x] access_metrics.cpp collectors (262 LOC)
- [x] CMakeLists.txt module registration (2026-08-09: src/access_model/CMakeLists.txt created; sources added to cmake/CMakeLists.txt THEMIS_CORE_SOURCES)
- [x] Unit tests ACM-01..ACM-08 (tests/access_model/test_access_coordinator_focused.cpp)

### Phase 3: Cache Module Integration 🟢 (COMPLETE — BLOCK 2)
- [x] Refactor cache_eviction_policy.cpp (Target: Q4 2026)
  - [x] Thresholds already named: `l1_promotion_threshold`, `l2_promotion_threshold`
  - [x] Added EvictionListener support to adaptive_query_cache.h/cpp
  - [x] Added `setEvictionListener()` method
- [x] Add storage feedback hooks to AdaptiveQueryCache (Target: Q4 2026)
  - [x] EvictionListener callback interface defined
  - [x] Emit onCacheEvicted() during L1/L2 evictions (complete)
    - [x] emitEvictionEvent() helper method implemented
    - [x] L1 tier eviction callbacks (expired, lru_selection, lru_fallback)
    - [x] L2 tier eviction callbacks (lru_selection, lru_fallback)
    - [x] Full event payload: key, tier, size_bytes, access_count, age_secs, reason
- [x] Integration tests: cache→coordinator→storage (Target: Q4 2026)
  - [x] test_cache_storage_integration.cpp created (CAI-01..CAI-10)
  - [x] Mock implementations and test fixtures ready for Phase 3

### Phase 4: Storage Module Integration 🟢 (COMPLETE — BLOCK 3, parallel with BLOCK 2)
- [x] Extend TieredStorageManager with coordinator callbacks (Target: Q4 2026)
  - [x] Added `setPromotionListener()` method
  - [x] Emit onStorageAccess() when detecting hot tiers
- [x] Add PromotionListener implementation (Target: Q4 2026)
  - [x] emitPromotionEvent() helper method implemented
  - [x] Hot pattern detection in get() for WARM/COLD tiers
  - [x] Hot pattern detection in runMigrationCycle() for WARM tier
  - [x] Access window calculation (write_time → now)
  - [x] Full event payload: key, tier, access_count, access_window
- [x] Implement predictive promotion path (cold→warm→L3) (Target: Q4 2026)
  - [x] Detection logic implemented in get() and runMigrationCycle()
  - [x] Coordinator can now make promotion decisions based on events
- [x] Integration tests: storage→coordinator→cache (Target: Q4 2026)
  - [x] CAI-07,08,09,10 test cases ready in test_cache_storage_integration.cpp

### Phase 5: Observability & Diagnostics 🟡 (implementation present; validation pending)
- [x] Structured logging for queued transition attempts and policy-enabled event decisions
  - [x] Create `access_model_logging.h` with TierTransitionLog, EvictionEventLog, PromotionDecisionLog structs
  - [x] Instrument AccessCoordinatorImpl (10+ logging points)
  - [x] Use spdlog fmt integration for structured fields
- [x] Thread-local trace context and correlation IDs for emitted diagnostics
  - [x] Create `access_model_trace.h` with TraceContext, CorrelationID
  - [x] Thread-local context management
  - [x] Worker-scoped trace context and correlation IDs for emitted diagnostics
- [x] Metrics query surface for operation counters and event/promotion/demotion latency histograms
  - [x] Queue depth and configured capacity in aggregate snapshots
  - [ ] Embedding-application export and worker-utilization telemetry
- [x] Operator runbooks (5+ scenarios) — COMPLETE ✅
  - [x] Symptom 1: Promotions not happening
  - [x] Symptom 2: Worker pool stuck
  - [x] Symptom 3: Memory spike
  - [x] Symptom 4: Promotion latency spike
  - [x] Symptom 5: Policy conflicts
- [x] Dashboard guide & examples — COMPLETE ✅

### Phase 6: Tests & Hardening 🟡 (regression cases added; validation pending)
- [~] E2E integration tests (test_access_model_e2e.cpp)
  - [x] 15+ test scenarios (promotion chains, demotion chains, policy, edge cases)
  - [ ] Record focused-test, sanitizer, and coverage results from a configured build
- [~] Concurrent operation tests (test_coordination_concurrency.cpp)
  - [x] Concurrent events, tier operations, thread-pool stress, bounded queue admission, and shutdown drain coverage
  - [ ] Re-run under TSan; demonstrate queue-depth bound under sustained load
- [~] Benchmark gate target constants and six benchmark cases are present; current cases do not validate the named full paths
  - [x] GATE-ACM-01..06 target constants and case names are defined
  - [ ] Configure a valid policy and exercise the intended decisions/transfers
  - [ ] Measure latency percentiles and memory use with valid methods
  - [ ] Compare against a recorded baseline and fail validation on violations
- [ ] Release-critical GATE-ACM-01..06 performance verification
  - [ ] Re-capture representative-hardware results after benchmark coverage is corrected

---

## Production Readiness Checklist

- [x] Core architecture documented (UNIFIED_ACCESS_MODEL.md)
- [x] Integration guide documented (CACHE_STORAGE_INTEGRATION.md)
- [x] Interfaces frozen (access_tier_interface.h, access_coordinator.h, etc.)
- [x] Coordinator transfer, queue, lifecycle, metrics, and history logic implemented
- [~] Focused tests cover transfer behavior and failure paths; execute them in a configured build
- [x] Cache integration (CAI-01..CAI-10 in `test_cache_storage_integration.cpp`) ✅ **Done 2026-08-09**
- [x] Storage integration (CAI-07..10 via TieredStorageManager + PromotionListener) ✅ **Done 2026-08-09**
- [~] Observability counters and histograms validated after build
- [~] Integration tests: full-stack e2e
- [ ] Release benchmarks: GATE-ACM-01..06 with valid end-to-end coverage on representative hardware
- [x] Operator runbooks describe runtime contracts and failure recovery

---

## Scientific / Technical Current-State Review (2026-11-06)

**Scope:** This module coordinates data placement and movement between cache/storage tiers. It is not identity, authentication, or authorization access control. Source review confirms a deterministic age/access-count policy (`src/access_model/age_based_policy.cpp`), bounded queued work and adapter-mediated transfers (`src/access_model/access_coordinator.cpp`), and void cache/storage listener callbacks (`include/access_model/access_coordinator.h`). Tier adapters are independent, so transfer is not atomic; source invalidation can fail after destination placement. Shutdown drains accepted work and joins workers, and can therefore block on an adapter call. No runtime tests or performance gates were executed for this review.

**Ist-Reifegrad (module assessment):** Implemented subsystem; validation-incomplete, pre-release candidate—not production-ready. This source/evidence assessment does not set or override release readiness in root [`ROADMAP.md`](../../ROADMAP.md).

| Area | Evidence-based assessment |
|---|---|
| Maturity | Core policy, coordinator, metrics, and transfer paths are implemented. Focused, cache/storage integration, end-to-end, concurrency, and observability test sources exist (`tests/access_model/test_access_coordinator_focused.cpp`, `test_cache_storage_integration.cpp`, `test_access_model_e2e.cpp`, `test_coordination_concurrency.cpp`, `test_access_model_observability.cpp`); existence is not a passing build or release result. |
| Workload usability | Decisions use configured age and access-count thresholds; there is no evidence here of workload-adaptive policy selection. Appropriate thresholds and event quality for production workloads remain deployment-specific unknowns. |
| Scaling / performance | Waiting task admission is bounded (default capacity 10,000); retained key metrics and transition history are capped. Event callbacks and worker performance still require representative-load evidence. The benchmark fixture sets no `AgeBasedPolicy`; its callback cases therefore bypass policy decisions and transfers, average timing is not p99, it does not measure RSS, and violations are printed only (`benchmarks/access_model/bench_access_coordinator_gates.cpp`). |
| Security / resilience | Tier registration validates tier-level consistency and adapter health; transfer code checks health, reports adapter errors, and invalidates source only after destination placement. There is no cross-adapter transaction; duplicate copies can remain. Void listeners cannot propagate admission rejection; queue-full counters/records are the available signal. An indefinitely blocked adapter can stall shutdown. This review does not certify authorization or storage-backend security. |
| Governance / maintainability | The coordinator exposes counters/histograms, bounded transition records, structured logs, and trace context. Embedding-application export and worker-utilization telemetry remain unchecked; generated Doxygen freshness is separately open in `src/access_model/MODULE_GAPS.md`. |
| Operations | Existing runbooks cover common symptoms, but operational readiness still depends on tested alerting/export, queue saturation handling, adapter recovery, and validated shutdown behavior in the embedding deployment. |

Scientific/technical references inform evaluation criteria; publication results are not implementation evidence, and none of the policies below is proposed for adoption without workload-specific evaluation:

- Megiddo and Modha, *ARC: A Self-Tuning, Low Overhead Replacement Cache* (FAST 2003), [USENIX paper](https://www.usenix.org/conference/fast-03/arc-self-tuning-low-overhead-replacement-cache). ARC adapts recency/frequency online in a uniform-page cache-replacement setting. This motivates workload-sensitivity tests, but differs from this coordinator's deterministic age/count decisions and multi-adapter tier movement.
- Einziger et al., *TinyLFU: A Highly Efficient Cache Admission Policy* (ACM TOS 2017), [arXiv paper](https://arxiv.org/abs/1512.00727). Approximate frequency-based admission is a possible cache-admission comparison, not a tier-migration policy; adoption is not proposed.
- Guerra et al., *Cost-Effective Storage Using Extent-Based Dynamic Tiering* (FAST 2011), [IBM Research publication](https://research.ibm.com/publications/cost-effective-storage-using-extent-based-dynamic-tiering). Its cost/performance placement under changing workloads and migration overhead is directly relevant to evaluating storage-tier placement, but does not validate this coordinator's policy or transfers.
- Yang et al., *FIFO Queues Are All You Need for Cache Eviction* (SOSP 2023), [publisher DOI](https://doi.org/10.1145/3600006.3613147). The cache-eviction study evaluates 6,594 traces across 14 datasets and reports up to 6× throughput versus optimized LRU at 16 threads in its setting; it concerns cache eviction only and does not validate this asynchronous multi-adapter transfer path.
- NIST SP 800-209, *Security Guidelines for Storage Infrastructure* (2020), [NIST publication](https://csrc.nist.gov/pubs/sp/800/209/final). Its storage-infrastructure scope is relevant to deployment threat and resilience reviews, but is not evidence that this coordinator enforces identity or authorization controls.
- [SNIA Emerald Specification v3.0.3](https://www.snia.org/emerald/download/Spec_v3.0) standardizes storage-device power/efficiency test configuration, workload, and instrumentation; it does not define p95/p99 latency gates or validate this coordinator's adapter I/O. The [Google Benchmark user guide](https://google.github.io/benchmark/user_guide.html) documents repetitions and randomized interleaving, but those techniques alone do not establish representative workloads or adapter-I/O validity. These references inform only the applicable parts of the proposed validation; no compliance or benchmark result is claimed.

**Assumptions and unknowns:** Effort below is rough engineering effort in person-days, excluding external Wave A dependencies and release/hardware scheduling. Estimates assume existing APIs and test fixtures remain usable. Representative hardware, workload distributions, adapter latency/failure behavior, key cardinality, acceptable p95/p99 and RSS limits, and reconciliation ownership are not established by this source review. Numeric gate constants are targets only until the harness and evidence meet the validation strategy. These conditional module targets do not alter release versions, wave status, or readiness; root [`ROADMAP.md`](../../ROADMAP.md) remains the release and wave source of truth.

## Conditional Module Priorities — v2.5.0

These are module-plan priorities only, not a release commitment or readiness declaration.

### Wave Step B1 — Verification and benchmark evidence (estimate: 4–7 person-days)
- **Goal:** Establish reproducible correctness and performance evidence for implemented behavior.
- [ ] Run focused, integration, end-to-end, concurrency, and observability suites in a configured build; record build profile and results.
- [ ] Correct the benchmark fixture to configure a valid policy and exercise the named decision/transfer path; measure latency percentiles and external RSS, and make target violations fail validation.
- [ ] Capture representative-hardware results and queue saturation behavior; distinguish callback-only timing from adapter transfer timing.
- **Dependencies:** Configured build dependencies, usable adapters/fixtures, representative hardware, and Wave A closure as required by the root Wave B gate.
- **Risk / trade-off:** Adapter variability and contention may make measurements noisy; adding realistic I/O lowers synthetic throughput but improves relevance.
- **Measurable success metrics:** Each named suite has a recorded pass/fail result; all six benchmark cases report what path they exercised; correctly mapped metrics are compared with the target values in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) and its documented ≤10% baseline-regression limit; p95/p99 and RSS are measured where claimed; queue depth never exceeds configured capacity. Target thresholds are not evidence until the corrected case actually exercises its named path, and no gate is called passed without reproducible evidence.
- **Validation strategy:** Run the named `tests/access_model/` suites and corrected GATE-ACM-01..06 benchmark in a release profile; use Google Benchmark repetitions/randomized interleaving as appropriate, while independently validating representative workloads and adapter-I/O coverage. Apply [SNIA Emerald v3.0.3](https://www.snia.org/emerald/download/Spec_v3.0) only to any storage-device power/efficiency measurements, not as evidence for p95/p99 or transfer latency. Inspect retained transition/counter outcomes against expected transfer results.

### Wave Step B2 — Operational contract and workload qualification (estimate: 3–5 person-days)
- **Goal:** Make the coordinator supportable for explicitly characterized storage workloads without overstating guarantees.
- [ ] Document supported event and adapter assumptions, queue-full handling for void listeners, duplicate-copy reconciliation, and shutdown blocking behavior/ownership in the module runbook.
- [ ] Characterize at least two recorded workload profiles (access skew and key cardinality) against the deterministic policy; retain thresholds and outcomes.
- [ ] Record queue, worker, export, and alerting gaps with an owner and follow-up issue if unresolved.
- **Dependencies:** B1 results and an embedding application/adapters that can supply representative traces and operational ownership.
- **Risk / trade-off:** More instrumentation and workload testing cost throughput and implementation time; workload results may require revising thresholds rather than universal defaults.
- **Measurable success metrics:** Two workload profiles have reproducible inputs/results; queue-full events and unresolved transfers are observable; runbook procedures are exercised and outcomes recorded.
- **Validation strategy:** Replay deterministic traces in tests or benchmark-only fixtures (clearly labeled, no runtime simulation path), inject adapter failures in tests, and review runbook steps against recorded metrics and logs.

## Conditional Module Priorities — v3.0.0

These follow-on goals are conditional on v2.5.0 evidence and the root roadmap's wave gates; they are not release readiness claims.

### Wave Step C1 — Storage-transfer resilience and security review (estimate: 5–9 person-days)
- **Goal:** Reduce and make auditable partial-transfer and adapter-failure risk within the coordinator's storage-tier scope.
- [ ] Threat-model adapter trust boundaries, key/data handling, and failure recovery; explicitly leave identity/authorization enforcement to owning layers.
- [ ] Test destination failure, source invalidation failure, exceptions, queue saturation, and shutdown with bounded and unresponsive adapters; document duplicate reconciliation and lifecycle contracts.
- [ ] Decide from evidence whether cancellation/timeouts, idempotency, or reconciliation hooks merit an API change; do not introduce a runtime simulation or legacy pathway.
- **Dependencies:** B1/B2 evidence, adapter contract owners, and root Wave B baseline closure before Wave C security validation.
- **Risk / trade-off:** Stronger timeout/retry behavior can alter delivery semantics or leave uncertain adapter outcomes; API changes need compatibility and migration review.
- **Measurable success metrics:** Every defined fault case has a deterministic expected result; no false-success transition is recorded; duplicate or uncertain outcomes are surfaced for reconciliation; any new latency/retry target is explicitly approved before adoption.
- **Validation strategy:** Focused fault-injection tests using test-only adapters, concurrency and shutdown regressions, transition-log review, and security review of documented trust boundaries.

### Wave Step D1 — Operability and maintainability closure (estimate: 4–8 person-days)
- **Goal:** Provide deployment-level visibility and repeatable recovery without making the module an independent source of release status.
- [ ] Define and validate queue saturation, transfer failure, adapter health, and shutdown alerts/runbook actions with embedding-application owners.
- [ ] Evaluate worker-utilization and metrics-export needs; add only contract-backed telemetry with cardinality and overhead tests.
- [ ] Refresh generated Doxygen through its supported generator and close or track remaining source-marker gaps.
- **Dependencies:** C1 contract review, B1 representative baselines, exporter/alerting owners, and root Wave C/D ordering.
- **Risk / trade-off:** More telemetry improves diagnosis but can add overhead, high-cardinality cost, and operational coupling; retain only measured, owned signals.
- **Measurable success metrics:** Every accepted alert has a tested operator action; exported metrics preserve bounded cardinality; telemetry overhead is reported against the B1 baseline; generated documentation is dated and source-linked.
- **Validation strategy:** Exercise runbook scenarios in a test deployment, verify exporter/alert behavior and overhead, run module tests, and validate generated Doxygen using the repository generator.

---

## Known Issues & Limitations

- Focused runtime tests and full module builds must pass before this follow-up is considered verified; the current local diagnostic configure is blocked by missing system `fmt`.
- Tier adapters are not coordinated transactionally. A failed source invalidation after destination placement may leave duplicate copies; the owning application must reconcile them.
- An adapter call that blocks indefinitely can delay `shutdown()` because accepted tasks are drained rather than cancelled.
- Queue admission is bounded, but `void` event listeners cannot return a rejection; consumers must monitor queue-full failure counters and transition records.
- The current benchmark fixture does not set an `AgeBasedPolicy`; callback cases therefore bypass policy decisions and transfers. Its memory case does not measure RSS, and latency checks report average elapsed time rather than p99. See [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
- Wave B entry gate remains `[~]` pending authoritative hardware artifacts for Transaction/GPU (Wave A external dependency).

---

## Breaking Changes

No breaking access-model contract is planned; any API/behavioral break requires migration notes and changelog synchronization before merge.

---

## Success Criteria

- ✅ Zero breaking changes to existing cache/storage APIs
- [~] Focused and integration test sources exist; configured-build and sanitizer results still need to be recorded
- [ ] Release gates for promotion/demotion latency pass on representative hardware
- [ ] Cross-module integration tests pass in a configured build
- [ ] Benchmark harness measures the documented policy, transfer, percentile, and memory targets
- [ ] Cache/storage benchmark regression results are attached

---

## Links

- Architecture: [`docs/architecture/UNIFIED_ACCESS_MODEL.md`](../../docs/architecture/UNIFIED_ACCESS_MODEL.md)
- Integration: [`docs/architecture/CACHE_STORAGE_INTEGRATION.md`](../../docs/architecture/CACHE_STORAGE_INTEGRATION.md)
- Cache: [`src/cache/ROADMAP.md`](../cache/ROADMAP.md)
- Storage: [`src/storage/ROADMAP.md`](../storage/ROADMAP.md)


## Program Execution Model — Wave Context

This module is scoped to **Wave B — Performance Consolidation** in the program-level wave model.
Wave B begins only after Wave A exit criteria are met.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full Wave A → B → C → D gate model and exit criteria.

### Wave B Scope for `access_model`
- [x] Phase 5–6 observability and test sources delivered.
- [~] GATE-ACM-01..06 benchmark closure requires a corrected harness and reproducible evidence; the current cases do not exercise the named full paths.

### Wave B Entry Gate (prerequisite from Wave A)
- [~] Wave A gate closure in progress (Target: Q4 2026): Transaction and GPU `release_critical` CI lanes are green on `develop` (runs `34313051247`/`34313042741`); authoritative representative-hardware artifact output and chaos/recovery evidence remain pending.
  - `src/transaction/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` — focused suites registered; hardware CI pending Q4 2026
  - `src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` — RAII guards confirmed; hardware baselines pending Q4 2026
  - See root `ROADMAP.md` §Wave A Closure Batch for current execution order

### Wave B Exit Criteria (this module's contribution)
- [ ] Stable p95/p99 and bounded memory confirmed on representative hardware
- [ ] Benchmark and observability gates closed with reproducible evidence
- [ ] Release decisions supported by representative-hardware baselines
- [~] Correct benchmark coverage, then capture representative-hardware baselines; see [`WAVE_B_BENCHMARK_EVIDENCE_BUNDLE.md`](./WAVE_B_BENCHMARK_EVIDENCE_BUNDLE.md)

### Dependencies on Later Waves
- Wave C security validation depends on stable Wave B performance baselines.
- Wave D operability hardening depends on all prior waves being gate-complete.

### Wave D Cross-Links (access_model)
- Runbook with D1 Trace Span annotations: [`docs/operability/RUNBOOK_ACCESS_MODEL_PROMOTION.md`](../../docs/operability/RUNBOOK_ACCESS_MODEL_PROMOTION.md) — D1 Span table and `otel-query` examples added 2026-09-16
- Phase 2A trace instrumentation targets for `access_coordinator.cpp` / `include/access_model/access_model_trace.h`: See runbook §Wave D D1 Distributed Trace Span Cross-Links
- Wave D roadmap: [`docs/operability/WAVE_D_ROADMAP.md`](../../docs/operability/WAVE_D_ROADMAP.md)

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
