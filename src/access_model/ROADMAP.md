---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
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
**Last Validated:** 2026-10-31
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
