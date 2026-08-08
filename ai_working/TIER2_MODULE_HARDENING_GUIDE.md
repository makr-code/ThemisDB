# Tier 2 Module Hardening Implementation Guide

**Date Created:** 2026-08-08  
**Coordination:** Tier 1 GA closure + Tier 2 module hardening in parallel  
**Status:** BATCH 1 (Analytics) IN PROGRESS, BATCHES 2-4 QUEUED

---

## Overview

This guide provides the detailed implementation pathway for Q3 2026 module hardening work.
All work targets the `develop` branch and is independent of the v2.4.0 GA promotion.

**Deliverable Structure:**
- Production code (no stubs)
- Focused test suites with deterministic coverage
- Benchmark-backed performance gates
- Updated ROADMAP.md entries
- Public API Doxygen documentation

---

## BATCH 1: Analytics Module — Distributed Coordinator Safety Controls

**Target:** Complete `src/analytics/ROADMAP.md` item: "distributed analytics coordinator safety controls (Target: Q3 2026)"

**Scope:** Add production-ready safety mechanisms for high-load distributed analytics scenarios

### Current Status
- Runtime limits added: `max_open_windows`, `max_records_per_window`, `max_records_per_session`
- Eviction counters: `windows_evicted` added to `WindowStats`
- Remaining: Distributed coordinator safety controls for failover, timeout, degradation

### Implementation Blocks

#### 1.1 Circuit Breaker Pattern (Target: COMPLETED or IN PROGRESS)
**File:** `src/analytics/distributed_analytics.h` (modified 2026-08-08)

**Deliverables:**
```cpp
// Pseudo-code structure
class DistributedAnalyticsCircuitBreaker {
  enum State { CLOSED, OPEN, HALF_OPEN };
  
  // Config
  struct Config {
    size_t failure_threshold;     // Trips breaker after N failures
    std::chrono::duration reset_timeout;
    std::chrono::duration half_open_max_calls;
  };
  
  // Methods
  bool allow_request();           // Check if request allowed
  void record_success();          // Increment success counter
  void record_failure();          // Increment failure counter
  State current_state() const;    // CLOSED/OPEN/HALF_OPEN
};
```

**Acceptance Criteria:**
- [x] Structure and interfaces defined in `.h` file
- [ ] Implementation in `distributed_analytics.cpp`
- [ ] Unit tests: CB-01..CB-04 (success, failure, state transitions, timeout handling)

#### 1.2 Bounded Concurrency Guard (Target: Q3 2026)
**File:** `src/analytics/distributed_analytics.cpp`

**Deliverables:**
```cpp
// Pseudo-code
class DistributedAnalyticsCoordinator {
  std::atomic<size_t> active_merges_;
  size_t max_concurrent_merges_;
  
  bool try_start_merge() {
    if (active_merges_.load() >= max_concurrent_merges_) {
      return false; // Fail-closed: drop merge attempt
    }
    active_merges_.fetch_add(1);
    return true;
  }
  
  void complete_merge() {
    active_merges_.fetch_sub(1);
  }
};
```

**Acceptance Criteria:**
- [ ] Concurrent merge tracking with bounded queue
- [ ] Fail-closed on overflow (drop or queue-full signal)
- [ ] Tests: CM-01..CM-04 (success, overflow, recovery, parity)

#### 1.3 Timeout + Recovery Semantics (Target: Q3 2026)
**File:** `src/analytics/distributed_analytics.cpp`

**Deliverables:**
- Timeouts on inter-node merge operations (configurable, default 5 second)
- Exponential backoff retry (2^N ms, capped at 30s)
- Recovery state: degraded mode with local-only analytics
- Telemetry: metrics for timeout/retry/degradation events

**Acceptance Criteria:**
- [ ] Timeout enforcement on all remote operations
- [ ] Retry logic with exponential backoff
- [ ] Degradation mode fallback
- [ ] Tests: TO-01..TO-06 (timeout, retry, recovery, parity)

#### 1.4 Benchmarks & Release Gates (Target: Q3 2026)
**File:** `benchmarks/analytics/bench_analytics_distributed_coordinator.cpp` (new)

**Deliverables:**
```cpp
// Pseudo-code structure
BENCHMARK(DistributedAnalyticsCircuitBreakerOpen) {
  // Measure state transition latency when breaker trips
  // Expected: < 100µs (DC-01)
}

BENCHMARK(DistributedAnalyticsCoordinatorMerge) {
  // Measure concurrent merge startup/completion latency
  // Expected: < 500µs (DC-02)
}

BENCHMARK(DistributedAnalyticsTimeoutRecovery) {
  // Measure timeout + degradation switchover latency
  // Expected: < 1000µs (DC-03)
}
```

**Release Gates (Target: PASS):**
- DC-01: Circuit breaker state transition ≤ 100µs
- DC-02: Concurrent merge startup/completion ≤ 500µs (p99)
- DC-03: Timeout recovery switchover ≤ 1000µs (p99)
- DC-04: Degraded-mode fallback throughput ≥ 80% of normal

**Acceptance Criteria:**
- [ ] All 4 benchmarks defined and executing
- [ ] All 4 gates PASS on current hardware baseline
- [ ] Repeatability verified (10 runs, < 5% variance)

### Testing Strategy

**Test File:** `tests/analytics/test_analytics_distributed_coordinator_focused.cpp`

**Test Cases:**

| Test ID | Area | Coverage | Status |
|---------|------|----------|--------|
| DC-TEST-01 | Circuit Breaker | State transitions (CLOSED→OPEN→HALF_OPEN→CLOSED) | PENDING |
| DC-TEST-02 | Circuit Breaker | Threshold-based trip and recovery | PENDING |
| DC-TEST-03 | Concurrency | Bounded merge queue overflow | PENDING |
| DC-TEST-04 | Concurrency | Concurrent merge parity | PENDING |
| DC-TEST-05 | Timeout | Operation timeout enforcement | PENDING |
| DC-TEST-06 | Timeout | Exponential backoff retry logic | PENDING |
| DC-TEST-07 | Recovery | Degraded-mode fallback | PENDING |
| DC-TEST-08 | Recovery | Parity recovery after timeout | PENDING |

**Acceptance:** All 8 tests PASS, 100% code coverage on new paths

### Verification Steps

```bash
# 1. Build
cmake --preset windows-release   # or your platform
cmake --build --preset windows-release --target module_analytics_test_*_focused

# 2. Run focused tests
ctest --preset windows-release -R "analytics.*distributed" --output-on-failure

# 3. Run benchmarks
./build/benchmarks/analytics/bench_analytics_distributed_coordinator --benchmark_counters_tabular=true

# 4. Verify gates
# All DC-01..04 gates should show PASS

# 5. Check ROADMAP update
# src/analytics/ROADMAP.md §In Progress should mark completion
```

---

## BATCH 2: AQL Module — Phase 4 Error Handling Consolidation

**Target:** Complete `src/aql/ROADMAP.md` remaining items:
- "performance gate consolidation for AQL assistance benchmark paths"
- "consistency hardening across helper and bridge integration surfaces"

**Current Status:** Phase 4 blocks 4.1-4.4 marked COMPLETED; performance gates + consistency remain

### Implementation Scope

#### 2.1 Performance Gate Consolidation
**File:** `benchmarks/aql/bench_aql_assistance_gates.cpp`

**Gates to consolidate:**
- AQL-ASS-01: validateAQLWithParser() ≤ 100µs (p99)
- AQL-ASS-02: translateNLToAQL() ≤ 500µs (p99)
- AQL-ASS-03: Bridge execution (embedding, schema) ≤ 1000µs (p99)
- AQL-ASS-04: Full pipeline (validation→translation→bridge) ≤ 1500µs (p99)

**Acceptance:** All 4 gates PASS, locked in benchmarks

#### 2.2 Consistency Hardening Across Surfaces
**Files:** 
- `src/aql/llm_aql_handler.cpp` (translation)
- `src/aql/llm_aql_embedding_bridge.cpp` (embedding bridge)
- `src/aql/llm_aql_schema_bridge.cpp` (schema bridge)

**Consistency Requirements:**
- All error handling paths use standardized [VALIDATION:*], [TRANSLATION:*], [BRIDGE:*] log tags
- All timeout paths use bounded exponential backoff
- All resource-limit paths use fail-closed semantics
- All diagnostics include operation ID, duration, error code for correlation

**Acceptance:** All three surfaces have unified error handling

### Testing

**Test File:** `tests/aql/test_aql_assistance_consistency_focused.cpp`

**Test Cases:** 8-10 focused tests covering:
- Validation error consistency
- Translation error consistency
- Bridge failure consistency
- End-to-end pipeline consistency

**Acceptance:** All tests PASS, 100% consistency verification

---

## BATCH 3: Prompt Engineering — Adversarial/Edge-Case Validation

**Target:** Harden template validation against adversarial inputs

**Current Status:** Phase 1-2 complete; Phase 3-6 remaining

### Implementation Scope

#### 3.1 Injection Detection Rules
**File:** `src/prompt_engineering/template_validator.h` (enhance)

**New Rules:**
1. SQL injection patterns: `'; DROP`, `1' OR '1'='1`, parameterized queries
2. Command injection: backticks, `$()`, `||`, `&&` patterns
3. Path traversal: `../`, absolute paths, symlink detection
4. XSS patterns: `<script>`, `javascript:`, event handlers
5. Template escape sequences: recursive template markers

**Acceptance Criteria:**
- [ ] All 5 rule sets implemented
- [ ] Detection tests PASS with > 95% true positive rate
- [ ] False positive rate < 1% on benign prompts

#### 3.2 Adversarial Test Payloads
**File:** `tests/prompt_engineering/test_prompt_engineering_adversarial_focused.cpp`

**Test Scenarios:**
- PE-ADV-01: SQL injection variants (10+ payloads)
- PE-ADV-02: Command injection variants (8+ payloads)
- PE-ADV-03: Path traversal + LFI (10+ payloads)
- PE-ADV-04: XSS vectors (8+ payloads)
- PE-ADV-05: Template recursion bombs (4+ payloads)
- PE-ADV-06: Unicode/encoding evasion (6+ payloads)
- PE-ADV-07: Null byte injection (3+ payloads)
- PE-ADV-08: Mixed/complex payloads (5+ payloads)

**Acceptance:** All 8 test classes PASS, 100% detection rate on known malicious payloads

#### 3.3 Documentation Alignment
**File:** `docs/architecture/prompt_engineering_security.md` (new/updated)

**Content:**
- Validation rules overview
- Threat model for prompt injection
- Fail-closed semantics
- Recovery and fallback behavior

**Acceptance Criteria:**
- [ ] Security model documented
- [ ] Failure modes documented
- [ ] Recovery paths documented

### Verification Steps

```bash
# 1. Build
cmake --build --preset windows-release --target module_prompt_engineering_test_*_adversarial_focused

# 2. Run adversarial tests
ctest --preset windows-release -R "prompt_engineering.*adversarial" --output-on-failure

# 3. Check coverage
# Code coverage report should show > 95% on validator paths

# 4. Update ROADMAP
# src/prompt_engineering/ROADMAP.md Phase 3 should mark completion
```

---

## BATCH 4: Retrieval Module — Hybrid Retrieval Phase A/B Validation

**Target:** Validate hybrid retrieval exact-first + ANN parity

**Current Status:** Phase A exact-first implemented; Phase B ANN parity tests needed

### Implementation Scope

#### 4.1 Exact-First Entry Criteria Validation
**File:** `src/retrieval/hybrid_retrieval_engine.cpp`

**Validation Logic:**
1. Check if query matches exact-match criteria
2. If exact match found, return immediately (bypass ANN)
3. If no exact match, fall through to ANN
4. Log decision point for diagnostics

**Acceptance Criteria:**
- [ ] Entry criteria check always executes
- [ ] Exact-first behavior verified in tests
- [ ] Diagnostics logged for monitoring

#### 4.2 ANN + CPU Parity Tests
**File:** `tests/retrieval/test_retrieval_hybrid_parity_focused.cpp`

**Test Matrix:**
| Test ID | Scenario | Validation |
|---------|----------|------------|
| HYB-01 | Exact match found, bypass ANN | Result matches exact |
| HYB-02 | No exact match, use ANN | Result ranks same as ANN-only baseline |
| HYB-03 | Mixed dataset: exact + ANN candidates | Parity on rank order |
| HYB-04 | High-cardinality exact candidates | Exact subset matches ANN top-K |
| HYB-05 | Empty exact results, ANN fallback | ANN results correct |
| HYB-06 | Concurrent exact + ANN queries | Thread-safe parity |
| HYB-07 | Latency comparison (exact vs ANN) | Exact-first speedup verified |
| HYB-08 | Edge case: NULL/empty/malformed inputs | Fail-closed behavior |

**Parity Definition:**
- Exact-first results ⊆ (ANN results ∪ exact results)
- Rank order correlation ≥ 0.95 (Spearman)
- Latency ratio: exact-first ≤ ANN baseline (median)

**Acceptance Criteria:**
- [ ] All 8 tests PASS
- [ ] Parity verified across Category A kernels (L2, cosine, inner-product)
- [ ] Deterministic fixtures for reproducible testing

#### 4.3 Planning-to-Contract Traceability
**File:** `docs/retrieval/HYBRID_RETRIEVAL_DESIGN_TRACE.md` (update/new)

**Content:**
- Exact-first logic design
- ANN parity requirements
- Contract boundaries
- Test-to-requirement mapping

**Acceptance Criteria:**
- [ ] Design document complete
- [ ] All requirements traced to tests
- [ ] Contract boundaries documented

### Verification Steps

```bash
# 1. Build
cmake --build --preset windows-release --target module_retrieval_test_*_hybrid_parity_focused

# 2. Run parity tests
ctest --preset windows-release -R "retrieval.*hybrid.*parity" --output-on-failure

# 3. Verify parity metrics
# All HYB-01..08 tests must show PASS with documented metrics

# 4. Update ROADMAP
# src/retrieval/ROADMAP.md Phase B should mark completion
```

---

## Cross-Batch Dependencies & Coordination

**No direct dependencies** between batches 1-4 (can run in parallel).

**Shared patterns:**
- All use focused test suites (test_*_focused.cpp)
- All use benchmark-backed gates with PASS/FAIL status
- All update their respective module ROADMAPs
- All target 100% code coverage on new paths

---

## Integration with Tier 1 (GA Sign-Off)

**Independence:** Tier 2 work is completely independent of v2.4.0 GA promotion
- Tier 2 targets v2.5.0-rc1 in future cycle
- Tier 1 is for v2.4.0 GA (stable, already frozen)
- Both can proceed in parallel without blocking each other

**Timeline:**
- Tier 1: Human sign-off (~1-2 days)
- Tier 2: All 4 batches (~3-5 days depending on parallelism)
- Combined delivery: Ready for v2.5.0-rc1 validation cycle

---

## Quality Checklist (Per Batch)

Before marking a batch COMPLETE:

- [ ] All implementation blocks 100% complete (no stubs)
- [ ] All focused tests PASS with 100% code coverage
- [ ] All benchmarks executing with gates PASS
- [ ] All ROADMAP entries updated
- [ ] All public APIs documented with Doxygen
- [ ] No new compiler warnings
- [ ] All security tests PASS (ASan, UBSan, TSan clean)

---

## Commit Message Standards

```
feat(analytics): Implement distributed coordinator safety controls

- Add circuit breaker pattern for failover handling
- Add bounded concurrency guard for remote merge operations
- Add timeout + recovery semantics with exponential backoff
- Add 4 benchmark-backed performance gates (DC-01..04)
- Add 8 focused tests covering all safety paths (DC-TEST-01..08)

Tests: DC-TEST-01..08 all PASS
Benchmarks: DC-01..04 all PASS
Coverage: 100% on new safety control paths

Fixes: src/analytics/ROADMAP.md "distributed analytics coordinator safety controls"
```

---

## Document History

| Date | Status | Action |
|------|--------|--------|
| 2026-08-08 | IN PROGRESS | BATCH 1 (Analytics) started; BATCHES 2-4 queued |
| TBD | PENDING | BATCH 1 completion |
| TBD | PENDING | BATCH 2 completion |
| TBD | PENDING | BATCH 3 completion |
| TBD | PENDING | BATCH 4 completion |

**Expected Completion:** All Tier 2 batches PASS by end of Q3 2026 hardening window
