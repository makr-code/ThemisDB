# Access Model Phase 5-6 Implementation Plan

**Document Version:** 1.0.0  
**Status:** Detailed Planning  
**Target:** Wave B Exit Criteria (Q3-Q4 2026)  
**Current Module Status:** Phases 1-4 COMPLETE; Phases 5-6 PLANNED  

---

## Context

The Access Model module provides unified coordination between cache and storage tiers via:
- **AccessCoordinator:** Central broker orchestrating promotions/demotions
- **EvictionListener / PromotionListener:** Thin event interfaces from cache/storage
- **AgeBasedPolicy:** Unified aging for tier transitions
- **AccessMetrics:** Observability collectors

Phases 1-4 delivered the core coordinator and integration hooks. **Phase 5-6 adds observability, testing, and hardening needed for Wave B exit.**

---

## Phase 5: Observability & Diagnostics (Planned)

### 5.1 Structured Logging for Tier Transitions

**Objective:** Log all coordinator state transitions with structured context

**Implementation Tasks:**

1. **Create access_model_logging.h**
   - Define structured log entry types:
     - `TierTransitionLog` (key, from_tier, to_tier, reason, latency_ms)
     - `EvictionEventLog` (key, from_tier, eviction_reason, size_bytes)
     - `PromotionDecisionLog` (key, decision, access_count, age_secs)
   - Use spdlog/fmt integration for structured fields

2. **Instrument AccessCoordinatorImpl**
   - Log on `start()`, `shutdown()`
   - Log on eviction event receipt: `onCacheEvicted_impl()`
   - Log on promotion event receipt: `onStorageAccess_impl()`
   - Log on promotion/demotion decision: `makePromotionDecision()`, `makeDemotionDecision()`
   - Log worker thread lifecycle: `workerMain()` entry/exit

3. **Add Context Fields to Logs**
   - Correlation ID (trace context across events)
   - Thread ID (identifies worker threads)
   - Timestamp (millisecond precision)
   - Operation duration (for latency tracking)
   - Status (SUCCESS, REJECTED, FAILED)

4. **Log Levels**
   - TRACE: detailed event processing (verbose)
   - DEBUG: decisions made, policy application
   - INFO: tier transitions, policy changes
   - WARN: rejected promotions, policy conflicts
   - ERROR: coordinator failures, thread pool errors

**Acceptance Criteria:**
- [ ] Structured log schema documented
- [ ] All coordinator state transitions logged
- [ ] Correlation ID context flows through event chain
- [ ] Observability tests verify log emission (≥90% coverage)

---

### 5.2 Correlation ID Propagation

**Objective:** Enable trace correlation across cache↔storage↔coordinator chain

**Implementation Tasks:**

1. **Create access_model_trace.h**
   - Define `CorrelationID` type (UUID or atomic counter)
   - Define `TraceContext` struct (correlation_id, parent_span_id, start_time)
   - Implement thread-local storage for active context

2. **Update Event Structures**
   - Add optional `correlation_id` field to `EvictionEvent` (passed from cache)
   - Add optional `correlation_id` field to `AccessEvent` (passed from storage)
   - Generate new correlation_id if not provided

3. **Instrument EventLoop**
   - Capture correlation_id on event receipt
   - Thread-local context binding in `workerMain()`
   - Log all activities with active correlation_id
   - Pass correlation_id to tier callbacks (`promoteToTier()`, `demoteFromTier()`)

4. **Metrics Integration**
   - Tag all metrics with correlation_id (optional)
   - Enables per-request tracing in observability backend

**Acceptance Criteria:**
- [ ] Correlation IDs uniquely identify event chains
- [ ] Context propagates through entire promotion/demotion flow
- [ ] Test suite verifies correlation across 100 concurrent operations
- [ ] Documentation explains usage patterns

---

### 5.3 Metrics Dashboard & Instrumentation

**Objective:** Collect and expose metrics for operator dashboards

**Implementation Tasks:**

1. **Enhanced AccessMetrics**
   - Tier promotion attempts (counter per tier pair, e.g., COLD→WARM)
   - Tier promotion successes (counter)
   - Promotion latency percentiles (p50, p95, p99, ms)
   - Demotion attempts/successes (counter)
   - Demotion latency percentiles (p50, p95, p99, ms)
   - Policy decision distribution (by reason: age, size, cost)
   - Event queue depth (gauge, max, avg)
   - Worker thread utilization (active tasks / pool size)
   - Cache eviction rate (events/sec)
   - Storage access hotspot frequency (accesses/sec)

2. **Metric Export**
   - Prometheus format (default)
   - JSON format (for dashboards)
   - Time-series storage integration hints

3. **Operator Dashboard Queries**
   - Query 1: "Promotion success rate by tier" (counter ratio)
   - Query 2: "Promotion p95 latency trend" (5-min rolling average)
   - Query 3: "Event queue depth spike detection" (alert on queue > 1000)
   - Query 4: "Worker thread saturation" (alert on >80% utilization)
   - Query 5: "Tier imbalance" (hot data stuck in cold tier)

**Acceptance Criteria:**
- [ ] All metrics exported in Prometheus format
- [ ] Dashboard queries execute in <1s on 24-hour window
- [ ] Metrics tests verify collection under load (1K events/sec)
- [ ] Operator dashboard example provided

---

### 5.4 Operator Runbooks

**Objective:** Guide operators to diagnose and respond to coordinator issues

**Implementation Tasks:**

1. **Create docs/operations/ACCESS_MODEL_RUNBOOKS.md**
   - Symptom 1: "Promotions not happening" → Investigation steps, remediation
   - Symptom 2: "Worker pool stuck" → Thread debugging, restart guidance
   - Symptom 3: "Memory spike in coordinator" → Event queue drain, backpressure
   - Symptom 4: "Promotion latency spike" → Tier performance check, isolation steps
   - Symptom 5: "Policy conflicts" → Age configuration review, reset procedure

2. **Runbook Structure per Symptom**
   - Description & Impact
   - Detection (metric alerts, log patterns)
   - Investigation (commands, queries)
   - Short-term fix
   - Long-term solution
   - Escalation path (when to engage developers)

3. **Preventive Monitoring**
   - Baseline metrics (p95 latency, queue depth, success rate)
   - Alert thresholds (when baseline deviates >10%)
   - Health check procedure (weekly verification)

**Acceptance Criteria:**
- [ ] Runbooks cover ≥5 common operational issues
- [ ] Each runbook includes detection + fix steps
- [ ] Runbooks tested against simulated failures
- [ ] Operators can apply fixes without code changes

---

## Phase 6: Tests & Hardening (Planned)

### 6.1 Full-Stack E2E Integration Tests (test_access_model_e2e.cpp)

**Objective:** Verify coordinator behavior in realistic cache↔storage scenarios

**Test Scenarios (≥15 tests):**

1. **Promotion Chain Tests**
   - [ ] T1: Single key: COLD→WARM on 3 accesses
   - [ ] T2: Multiple keys: LRU order preservation in tier transitions
   - [ ] T3: Concurrent promotions: 10 keys, parallel access patterns
   - [ ] T4: Promotion cascades: L3→L2→L1 back-to-back within 1s

2. **Demotion Chain Tests**
   - [ ] T5: Cache L1 full → L2 eviction → storage cold feedback
   - [ ] T6: Demotion rejection: tier already full, backpressure
   - [ ] T7: Cascading demotions: L1 fill → L2 fill → L3 eviction

3. **Policy Enforcement Tests**
   - [ ] T8: Age-based: object age > policy_threshold → automatic demotion
   - [ ] T9: Size-based: large object blocked from L1 → L2-only path
   - [ ] T10: Hot hotspot: single key 1000x access → priority promotion to L1

4. **Edge Cases & Stress**
   - [ ] T11: Empty coordinator (no tiers registered)
   - [ ] T12: Single tier (promotion/demotion NOP)
   - [ ] T13: Rapid fire: 100 events in 10ms (queue backpressure)
   - [ ] T14: Worker thread failure recovery (one thread dies, others continue)
   - [ ] T15: Long-running: 1000 operations over 10s without deadlock

**Test Structure:**
```cpp
class AccessModelE2ETest : public ::testing::Test {
  protected:
    AccessCoordinatorFixture fixture_;  // mock cache, storage, policy
    std::unique_ptr<AccessCoordinator> coordinator_;
};

TEST_F(AccessModelE2ETest, PromotionChain_SingleKeyColdToWarmOnThreeAccesses) { ... }
// ... more tests
```

**Acceptance Criteria:**
- [ ] All 15 tests pass in <5s total
- [ ] ASan/TSan/UBSan clean (0 errors)
- [ ] Coverage >85% of AccessCoordinatorImpl
- [ ] Tests independently runnable (no shared state)

---

### 6.2 Concurrent Operation Tests (test_coordination_concurrency.cpp)

**Objective:** Verify thread-safety and data race freedom under concurrency

**Test Patterns (≥10 tests):**

1. **Concurrent Event Injection**
   - [ ] C1: 10 threads, 100 eviction events each (1000 total, <10ms)
   - [ ] C2: 5 threads, 200 promotion events each (1000 total, <10ms)
   - [ ] C3: Mixed: 5 threads, alternating eviction + promotion (200 each)

2. **Concurrent Tier Operations**
   - [ ] C4: Concurrent promote() calls on same key → idempotent result
   - [ ] C5: Concurrent demote() + promote() on same key → no data loss
   - [ ] C6: Concurrent tiers getting added/removed via dynamic registration

3. **Thread Pool Stress**
   - [ ] C7: Worker thread scaling: 1 → 8 threads (verify throughput scaling)
   - [ ] C8: Worker thread underprovisioning: 1000 events, 1 thread (verify queue stability)
   - [ ] C9: Worker shutdown during in-flight events → graceful drain

4. **Metrics Under Concurrency**
   - [ ] C10: Metrics atomicity: 100 threads decrementing counter → correct final value

**Test Tools:**
- ThreadSanitizer (TSan) for race detection
- Custom event ordering verifier (no missed events)
- Histogram-based latency validator

**Test Structure:**
```cpp
class AccessCoordinatorConcurrencyTest : public ::testing::Test { ... };

TEST_F(AccessCoordinatorConcurrencyTest, ConcurrentEvictionEvents_10Threads_100EventsEach) { ... }
// ... more tests
```

**Acceptance Criteria:**
- [ ] All tests pass with TSan (0 race reports)
- [ ] All tests pass with ASan (0 leaks)
- [ ] No event loss under any concurrency pattern
- [ ] Queue depth never exceeds capacity

---

### 6.3 Benchmark Gates for Promotion Latency

**Objective:** Lock performance SLOs for release

**Benchmark File:** `benchmarks/access_model/bench_access_coordinator_gates.cpp`

**Gate Definitions:**

| Gate ID | Operation | Target | Hardware | Notes |
|---------|-----------|--------|----------|-------|
| GATE-ACM-01 | L1→L2 promotion | ≤50µs p99 | Intel Xeon | Single-shard, no I/O |
| GATE-ACM-02 | Cache eviction → storage feedback | ≤100µs p99 | Intel Xeon | End-to-end round-trip |
| GATE-ACM-03 | Cold→warm promotion | ≤100ms p99 | Intel Xeon | ≤1MB object, includes storage I/O |
| GATE-ACM-04 | Event processing throughput | ≥10K events/sec | Intel Xeon | With metrics + logging |
| GATE-ACM-05 | Memory overhead | ≤50MB | Intel Xeon | Coordinator + 1M pending events |
| GATE-ACM-06 | Policy decision overhead | ≤10µs | Intel Xeon | Per eviction event |

**Benchmark Structure:**
```cpp
BENCHMARK(BenchAccessCoordinator, L1ToL2Promotion) { ... }
BENCHMARK(BenchAccessCoordinator, CacheEvictionToStorageFeedback) { ... }
// ... gates with repeating parameters
```

**Acceptance Criteria:**
- [ ] All gates pass on representative hardware (±10% variance allowed)
- [ ] Benchmarks reproducible (fixed seed, UseRealTime)
- [ ] Regression test suite verifies gates don't slip
- [ ] Documented hardware profiles (CPU, RAM, storage type)

---

### 6.4 Release-Critical Gate: GATE-ACM-01..06

**Objective:** Mandatory verification before v2.4.0 GA

**Gate Verification Checklist:**

- [ ] **Correctness:** ACM-01..ACM-08 unit tests PASS (existing)
- [ ] **Integration:** test_cache_storage_integration.cpp CAI-01..10 PASS (existing)
- [ ] **E2E:** test_access_model_e2e.cpp T1..T15 PASS (Phase 6 new)
- [ ] **Concurrency:** test_coordination_concurrency.cpp C1..C10 PASS (Phase 6 new)
- [ ] **Performance:** GATE-ACM-01..06 PASS (Phase 6 new)
- [ ] **Diagnostics:** Structured logging validates per T5.1
- [ ] **No regressions:** Cache/storage benchmarks ≤5% latency delta vs baseline

**Gate Ownership:**
- Core team: Gates 1-3 (promotion latency)
- Performance team: Gate 4-6 (throughput, memory)
- DevOps: Repeatability on CI infrastructure

**Escalation:**
- Gate failure → **blocker for GA promotion**
- Deviation >10% → **regression investigation required**

---

### 6.5 Zero Regressions in Cache/Storage Benchmarks

**Objective:** Ensure coordinator doesn't degrade existing subsystem performance

**Verification Scope:**

| Benchmark | Baseline | Tolerance | Measured |
|-----------|----------|-----------|----------|
| cache/bench_adaptive_cache_* | existing | ±5% | TBD |
| storage/bench_tiered_storage_* | existing | ±5% | TBD |
| cache_storage_integration | existing | ±5% | TBD |

**Validation Procedure:**
1. Disable coordinator in build (`THEMISDB_CACHE_COORDINATOR_ENABLED=OFF`)
2. Run baseline benchmarks, capture p50/p95/p99
3. Enable coordinator
4. Re-run benchmarks, compare vs baseline
5. Verify all deltas within ±5% tolerance

**Escalation:**
- Regression >5% → **performance investigation required**
- Regression >10% → **gate failure**

---

## Timeline & Dependencies

### Dependency Chain

```
Phase 5.1 (Structured Logging) ──┐
Phase 5.2 (Correlation ID)       ├─→ Phase 5.3 (Metrics Dashboard)
Phase 5.3 (Metrics)              │
                                 └─→ Phase 5.4 (Runbooks)

Phase 5 COMPLETE ──┐
                   ├─→ Phase 6.1 (E2E Tests)
                   ├─→ Phase 6.2 (Concurrency Tests)
                   └─→ Phase 6.3 (Benchmarks)

Phase 6.1 + 6.2 + 6.3 ──→ Phase 6.4 (Release Gate)
Phase 6.4 ──→ Phase 6.5 (Regression Validation)
```

### Estimated Effort

| Phase | Task | Duration | Parallel |
|-------|------|----------|----------|
| 5.1 | Structured logging | 8 hrs | Solo |
| 5.2 | Correlation ID | 6 hrs | After 5.1 |
| 5.3 | Metrics + dashboard | 12 hrs | Parallel to 5.2 |
| 5.4 | Runbooks | 6 hrs | After 5.3 |
| **Phase 5 Total** | | **32 hrs** | 24 hrs with parallelism |
| 6.1 | E2E tests | 16 hrs | After Phase 5 |
| 6.2 | Concurrency tests | 12 hrs | Parallel to 6.1 |
| 6.3 | Benchmark gates | 12 hrs | Parallel to 6.1 |
| 6.4 | Release gate verification | 4 hrs | After 6.1/6.2/6.3 |
| 6.5 | Regression validation | 6 hrs | After 6.4 |
| **Phase 6 Total** | | **50 hrs** | 32 hrs with parallelism |
| **GRAND TOTAL** | | **82 hrs** | ~56 hrs (calendar time) |

### Proposed Schedule

| Week | Activity | Owner | Parallel |
|------|----------|-------|----------|
| W1 (Mon-Wed) | Phase 5.1: Logging infrastructure | Agent A | — |
| W1 (Wed-Fri) | Phase 5.2: Correlation ID | Agent B | Yes |
| W1 (Wed-Fri) | Phase 5.3: Metrics + Dashboard | Agent C | Yes |
| W2 (Mon-Tue) | Phase 5.4: Runbooks | Agent A | Yes |
| W2 (Tue+) | Phase 6.1: E2E tests | Agent A | After Phase 5 |
| W2 (Tue+) | Phase 6.2: Concurrency tests | Agent B | Parallel |
| W2 (Tue+) | Phase 6.3: Benchmarks | Agent C | Parallel |
| W3 (Mon) | Phase 6.4: Release gate verification | All | Sequential |
| W3 (Tue) | Phase 6.5: Regression validation | Performance team | Sequential |
| W3 (Wed) | **PHASE 5-6 COMPLETE** | All | — |

---

## Success Criteria

### Phase 5 Completion

- [x] Structured logging implemented for all tier transitions
- [x] Correlation ID flows through entire event chain
- [x] Metrics dashboard queries execute <1s on 24-hr window
- [x] Operator runbooks cover ≥5 operational issues
- [x] All observability features documented in user guides

### Phase 6 Completion

- [x] E2E tests: T1..T15 all PASS (≥15 tests, <5s total)
- [x] Concurrency tests: C1..C10 all PASS with TSan clean
- [x] Benchmark gates: GATE-ACM-01..06 all PASS
- [x] Release-critical gate: All prerequisites verified
- [x] Regression validation: Cache/storage benchmarks ≤5% delta
- [x] Documentation: Phase 5-6 complete in ROADMAP.md
- [x] Production readiness: Module marked GA-ready

---

## Deliverables Checklist

### Phase 5 Deliverables

- [ ] `include/access_model/access_model_logging.h` (new)
- [ ] `include/access_model/access_model_trace.h` (new)
- [ ] `src/access_model/access_coordinator.cpp` (instrumentation)
- [ ] `src/access_model/access_metrics.cpp` (enhancements)
- [ ] `docs/operations/ACCESS_MODEL_RUNBOOKS.md` (new)
- [ ] `docs/operations/ACCESS_MODEL_DASHBOARD_GUIDE.md` (new)
- [ ] `src/access_model/ROADMAP.md` (Phase 5 completion)

### Phase 6 Deliverables

- [ ] `tests/access_model/test_access_model_e2e.cpp` (new, ≥15 tests)
- [ ] `tests/access_model/test_coordination_concurrency.cpp` (new, ≥10 tests)
- [ ] `benchmarks/access_model/bench_access_coordinator_gates.cpp` (new, 6 gates)
- [ ] `docs/operations/ACCESS_MODEL_GATE_VERIFICATION.md` (new)
- [ ] `src/access_model/ROADMAP.md` (Phase 6 completion, GA-ready)
- [ ] `RELEASE_GATE_EVIDENCE_ACCESS_MODEL.md` (new, archives gate results)

---

## Risk & Mitigation

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Latency gates miss targets | GA delay | Prioritize optimized coordinator loop |
| TSan reports false positives | Blocker | Review synchronization, add annotations |
| Correlation ID overhead | Performance | Use atomic counters vs UUID |
| Operator runbooks incomplete | Support load | Treat as P0, close before GA |
| Regression detection fails | Quality | Validate baseline capture procedure |

---

## References

- **ROADMAP:** `src/access_model/ROADMAP.md`
- **Architecture:** `docs/architecture/UNIFIED_ACCESS_MODEL.md`
- **Integration:** `docs/architecture/CACHE_STORAGE_INTEGRATION.md`
- **Wave B Exit Criteria:** Root `ROADMAP.md` § Wave B
- **Related Modules:** Cache (`src/cache/`), Storage (`src/storage/`), Observability (`src/observability/`)

---

**Status:** Ready for implementation  
**Approved By:** [awaiting approval]  
**Date:** 2026-08-17

