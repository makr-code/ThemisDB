# Access Model Module - Roadmap

**Version:** 1.0.0  
**Status:** ACTIVE (Phase 1 delivery in progress)  
**Last Validated:** 2026-08-03

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->

---

## Current Status

**Phase 1 Complete:** Unified architecture documented, core interfaces defined.
**Phase 2 In Progress:** Core coordinator implementation.
**Phases 3-6 Planned:** Cache/storage integration, observability, tests.

---

## Implementation Phases

### Phase 1: Architecture & Interfaces ✅ (DONE)
- [x] Create unified access model documentation (UNIFIED_ACCESS_MODEL.md)
- [x] Create integration guide (CACHE_STORAGE_INTEGRATION.md)
- [x] Define AccessTier abstract interface
- [x] Define AccessCoordinator broker interface
- [x] Define promotion/demotion data structures
- [x] Define AgeBasedPolicy (unified aging)
- [x] Define AccessMetrics (observability)

### Phase 2: Core Coordinator Implementation 🟡 (IN PROGRESS)
- [ ] access_coordinator.cpp implementation (Target: Q3 2026)
  - [ ] Tier registry management
  - [ ] Eviction signal processing
  - [ ] Storage access pattern detection
  - [ ] Background promotion/demotion workers
  - [ ] Age-based policy enforcement
  - [ ] Correlation ID generation & tracking
- [ ] age_based_policy.cpp helper methods (Target: Q3 2026)
- [ ] access_metrics.cpp collectors (Target: Q3 2026)
- [ ] CMakeLists.txt module registration (Target: Q3 2026)
- [ ] Unit tests ACM-01..ACM-08 (Target: Q3 2026)

### Phase 3: Cache Module Integration 🟡 (IN PROGRESS — BLOCK 2)
- [~] Refactor cache_eviction_policy.cpp (Target: Q4 2026)
  - [x] Thresholds already named: `l1_promotion_threshold`, `l2_promotion_threshold`
  - [x] Added EvictionListener support to adaptive_query_cache.h/cpp
  - [x] Added `setEvictionListener()` method
- [~] Add storage feedback hooks to AdaptiveQueryCache (Target: Q4 2026)
  - [x] EvictionListener callback interface defined
  - [ ] Emit onCacheEvicted() during L1/L2 evictions (in progress)
- [~] Integration tests: cache→coordinator→storage (Target: Q4 2026)
  - [x] test_cache_storage_integration.cpp created (CAI-01..CAI-10)
  - [ ] Build and execute tests

### Phase 4: Storage Module Integration 🔵 (PLANNED — BLOCK 3, parallel with BLOCK 2)
- [ ] Extend TieredStorageManager with coordinator callbacks (Target: Q4 2026)
  - [ ] Add `setPromotionListener()` method
  - [ ] Emit onStorageAccess() when detecting hot tiers
- [ ] Add PromotionListener implementation (Target: Q4 2026)
- [ ] Implement predictive promotion path (cold→warm→L3) (Target: Q4 2026)
- [ ] Integration tests: storage→coordinator→cache (Target: Q4 2026)

### Phase 5: Observability & Diagnostics 🔵 (PLANNED)
- [ ] Structured logging for all transitions (Target: Q1 2027)
- [ ] Trace correlation ID propagation (Target: Q1 2027)
- [ ] Metrics dashboard panels (Target: Q1 2027)
- [ ] Operator runbooks (Target: Q1 2027)

### Phase 6: Tests & Hardening 🔵 (PLANNED)
- [ ] Integration tests: full stack e2e (test_access_model_e2e.cpp) (Target: Q1 2027)
- [ ] Concurrent operation tests (test_coordination_concurrency.cpp) (Target: Q1 2027)
- [ ] Benchmarks: promotion latency gates (Target: Q1 2027)
  - [ ] Gate: L1→L2 promotion ≤50µs
  - [ ] Gate: Cache eviction→storage feedback ≤100µs
  - [ ] Gate: Cold→warm promotion ≤100ms for ≤1MB objects
- [ ] Release-critical gate: GATE-ACM-01..06 (Target: Q1 2027)
- [ ] Zero regressions in cache/storage benchmarks (Target: Q1 2027)

---

## Production Readiness Checklist

- [x] Core architecture documented (UNIFIED_ACCESS_MODEL.md)
- [x] Integration guide documented (CACHE_STORAGE_INTEGRATION.md)
- [x] Interfaces frozen (access_tier_interface.h, access_coordinator.h, etc.)
- [ ] Core coordinator implemented (in progress)
- [ ] Unit tests ACM-01..ACM-08 (pending coordinator implementation)
- [ ] Cache integration (pending coordinator implementation)
- [ ] Storage integration (pending coordinator implementation)
- [ ] Observability (pending implementations)
- [ ] Integration tests (pending implementations)
- [ ] Release benchmarks (pending implementations)
- [ ] Operator runbooks (pending implementations)

---

## Known Issues & Limitations

- Phase 2 coordinator implementation not yet started (skeleton only)
- Cache/storage modules still isolated (integration pending Phase 3-4)
- Observability not yet wired (pending Phase 5)
- Performance gates not yet established (pending Phase 6)

---

## Success Criteria

- ✅ Zero breaking changes to existing cache/storage APIs
- ✅ All new code covered by focused tests (ACM-01..ACM-12)
- ✅ Release gates for promotion/demotion latency pass
- ✅ Cross-module integration tests green (CAI-01..CAI-08)
- ✅ Documentation audit pass (4/4 conformance)
- ✅ No regressions in cache/storage benchmarks

---

## Links

- Architecture: [`docs/architecture/UNIFIED_ACCESS_MODEL.md`](../../docs/architecture/UNIFIED_ACCESS_MODEL.md)
- Integration: [`docs/architecture/CACHE_STORAGE_INTEGRATION.md`](../../docs/architecture/CACHE_STORAGE_INTEGRATION.md)
- Cache: [`src/cache/ROADMAP.md`](../cache/ROADMAP.md)
- Storage: [`src/storage/ROADMAP.md`](../storage/ROADMAP.md)

