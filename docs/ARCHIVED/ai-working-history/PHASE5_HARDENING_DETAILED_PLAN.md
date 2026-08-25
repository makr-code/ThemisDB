# Phase 5: Server & LLM Hardening — Implementation Plan

**Status:** 🔵 PLANNED (Kickoff: 2026-07-22)  
**Timeline:** 2026-07-22 to 2026-08-09 (3 weeks active)  
**Release Target:** v1.9.0-beta patch (Q4 2026)

---

## Scope & Objectives

### High-Level Scope

Address critical production-readiness gaps in **Server (wire-protocol retry logic)** and **LLM (exception safety, memory leaks)** modules by hardening fault recovery paths and resource management.

### Success Criteria

| Criterion | Measurement | Target | Status |
|-----------|-------------|--------|--------|
| **Server Recovery** | Fault injection: transient fault recovery rate | 99.9% success | 🔵 Planned |
| **Server Tests** | New tests passing (retry + timeout) | 28 new tests | 🔵 Planned |
| **LLM Exception Safety** | Exception safety score (1-5 scale) | >= 4/5 (vs. current 3.5) | 🔵 Planned |
| **LLM Memory** | Valgrind clean (zero leaks, data races) | 0 definitely lost bytes | 🔵 Planned |
| **LLM Tests** | New tests passing (exception + memory) | 52 new tests | 🔵 Planned |
| **Code Quality** | Scanner findings (CRITICAL + HIGH) | 0 new CRITICAL, < 3 new HIGH | 🔵 Planned |
| **Total Tests** | Phase 5 completeness | 80 new tests all PASS | 🔵 Planned |

---

## Server Hardening: Phase 5-S (1.5 weeks, Team C)

### P5-S01: Wire-Protocol Retry Logic (1 week, Team C1)

**Target Files:**
- `src/network/wire_protocol_handler.h/cc` — Retry mechanism
- `src/network/wire_protocol_client.h/cc` — Client-side retries
- `tests/network/test_wire_protocol_retry.cpp` — New tests

**Current State:**
- Basic wire protocol exists (Phase 4 complete)
- Retry logic: stub implementation (no exponential backoff)
- Known issue: Transient connection failures cause immediate abort

**Implementation Plan:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P5-S01-A | Design retry strategy (2-3 retries + exponential backoff: 100ms → 200ms → 400ms) | 1 day | C1 | Design doc |
| P5-S01-B | Implement wire-protocol retry coordinator | 2 days | C1 | 6 tests |
| P5-S01-C | Implement exponential backoff + jitter | 1 day | C1 | 4 tests |
| P5-S01-D | Implement idempotency tracking (request ID) | 1 day | C1 | 3 tests |
| P5-S01-E | Fault injection tests (connection drops, timeouts, corrupted packets) | 2 days | C1 | 6 tests |

**Acceptance Criteria:**
- ✅ 19 new tests (6+4+3+6 = 19) all passing
- ✅ Transient connection failure recovery: 99%+ success rate (fault injection verified)
- ✅ Retry latency: <= 1 second (max 3 retries × 400ms + overhead)
- ✅ Idempotency: duplicate requests handled correctly (no data loss)
- ✅ Doxygen complete for new public APIs
- ✅ No CRITICAL scanner findings

**Implementation Notes:**
- Retry trigger: Detect connection reset (ECONNRESET), timeout (ETIMEDOUT), transient errors
- Max retries: 3 (tunable)
- Backoff formula: `delay_ms = base_ms * (2 ^ retry_count) + random(0, jitter_ms)`
- Request tracking: Hash(query_id + request_sequence) -> timestamp + result
- Benchmark: Fault injection with 5% transient error rate

---

### P5-S02: HTTP Timeout Patterns & Graceful Shutdown (1 week, Team C2)

**Target Files:**
- `src/network/http_server.h/cc` — HTTP handler timeout logic
- `src/network/http_request_context.h/cc` — Request lifecycle
- `tests/network/test_http_timeout_patterns.cpp` — New tests

**Current State:**
- Basic HTTP server exists
- Timeout handling: no per-request timeouts; global timeout only
- Graceful shutdown: not implemented (abrupt termination)

**Implementation Plan:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P5-S02-A | Design timeout architecture (per-request + global + read/write timeout levels) | 1 day | C2 | Design doc |
| P5-S02-B | Implement per-request timeout with async timer | 2 days | C2 | 5 tests |
| P5-S02-C | Implement graceful shutdown (drain + timeout + force-close) | 2 days | C2 | 6 tests |
| P5-S02-D | Implement read/write timeout patterns (detect slow clients) | 1 day | C2 | 4 tests |
| P5-S02-E | Integration testing (concurrent requests + timeouts + shutdown) | 1 day | C2 | 5 tests |

**Acceptance Criteria:**
- ✅ 20 new tests (5+6+4+5 = 20) all passing
- ✅ Per-request timeout: configurable, default 30 seconds
- ✅ Graceful shutdown: 99% of pending requests complete within timeout window
- ✅ Slow client detection: read timeout < 10 seconds
- ✅ No connection leaks during shutdown (netstat/ss verify)
- ✅ Doxygen complete

**Implementation Notes:**
- Request timeout levels: Global (30s) > Per-request (custom) > Read (5s) > Write (10s)
- Timer: Use `std::chrono` + async callback on timeout
- Graceful shutdown: Phase 1 (stop accepting new requests) → Phase 2 (drain pending, timeout 30s) → Phase 3 (force-close)
- Slow client: Detect if read/write stalls for > timeout window

---

## LLM Hardening: Phase 5-L (2 weeks, Team D)

### P5-L01: Exception Safety & RAII Wrapper Refactoring (2 weeks, Team D1)

**Target Files:**
- `src/llm/model_loader.h/cc` — Model loading exception safety
- `src/llm/model_cache.h/cc` — Cache exception-safe operations
- `src/llm/llm_executor.h/cc` — Execution path exception handling
- `tests/llm/test_llm_exception_safety.cpp` — New exception safety tests

**Current State (from module status scan 2026-07-18):**
- LLM module: 84.972 LOC, 81 stub markers (0.09% density)
- Exception safety score: 3.5/5 (basic safety present, not comprehensive)
- Known issue: Model loading can fail mid-way without cleanup (resource leak risk)

**Implementation Plan:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P5-L01-A | Audit model loading path (identify resource leak points) | 1 day | D1 | Audit report |
| P5-L01-B | Implement RAII wrappers (ModelRAII, CacheRAII, ExecutorRAII) | 3 days | D1 | 8 tests |
| P5-L01-C | Add exception handlers to critical paths (load, cache insert, execute) | 2 days | D1 | 10 tests |
| P5-L01-D | Implement try-catch with cleanup + re-throw semantics | 2 days | D1 | 6 tests |
| P5-L01-E | Exception safety testing (throw at each alloction point) | 2 days | D1 | 12 tests |
| P5-L01-F | Document exception contracts (Doxygen @throw) | 1 day | D1 | Doxygen audit |

**Acceptance Criteria:**
- ✅ 36 new exception-safety tests (8+10+6+12 = 36) all passing
- ✅ Exception safety score: >= 4/5 (strong guarantee for critical paths)
- ✅ No resource leaks on exception paths (Valgrind clean)
- ✅ All public APIs documented with @throw clauses
- ✅ Recovery path tested: exception → cleanup → state consistent

**Implementation Notes:**
- RAII pattern: `ModelRAII { on_ctor: allocate; on_dtor: release; }`
- Exception chain: Catch → log → cleanup → throw or recover
- Test strategy: `throw_at_allocation(iteration_N)` to force exceptions at different points
- Valgrind: `valgrind --leak-check=full --track-origins=yes app`

---

### P5-L02: Memory Leak Fixes (1.5 weeks, Team D2)

**Target Files:**
- `src/llm/model_cache.h/cc` — Cache cleanup on LRU eviction
- `src/llm/embedding_manager.h/cc` — Embedding buffer lifecycle
- `src/llm/tokenizer.h/cc` — Token buffer management
- `tests/llm/test_llm_memory_safety.cpp` — New memory tests

**Current State:**
- Memory profiling (massif): identified leak patterns in model cache + tokenizer
- Known leaks: ~2-5 MB per model load/unload cycle

**Implementation Plan:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P5-L02-A | Profile current memory usage (Valgrind massif + daily baseline) | 1 day | D2 | Baseline report |
| P5-L02-B | Fix model cache eviction (ensure destructors called on LRU evict) | 2 days | D2 | 6 tests |
| P5-L02-C | Fix embedding buffer lifecycle (reuse vs. allocate) | 1 day | D2 | 4 tests |
| P5-L02-D | Fix tokenizer token buffer cleanup | 1 day | D2 | 4 tests |
| P5-L02-E | Long-running memory test (1000 model load/unload cycles) | 2 days | D2 | 1 long test |
| P5-L02-F | Document memory guarantees (allocation strategy, cleanup policy) | 1 day | D2 | Memory doc |

**Acceptance Criteria:**
- ✅ 15 new memory tests (6+4+4+1 = 15) all passing
- ✅ Valgrind clean: 0 definitely lost, 0 indirectly lost, < 10KB still reachable
- ✅ Memory growth over 1000 model cycles: < 50MB (vs. current ~2-5MB per cycle → 2-5GB leak)
- ✅ No TSan (thread sanitizer) data races
- ✅ Memory guarantees documented in Doxygen

**Implementation Notes:**
- Valgrind command: `valgrind --leak-check=full --show-leak-kinds=definite,indirect,reachable --track-origins=yes`
- TSan command: `clang++ -fsanitize=thread -O1 -g test.cpp`
- Buffer reuse: Implement `BufferPool` for token buffers (slab allocator)
- Cleanup: Explicit destructors in cache eviction path

---

## Test Infrastructure

### New Test Files (Phase 5)

| File | Component | Tests | Responsibility |
|------|-----------|-------|-----------------|
| `tests/network/test_wire_protocol_retry.cpp` | P5-S01 | 19 | Team C1 |
| `tests/network/test_http_timeout_patterns.cpp` | P5-S02 | 20 | Team C2 |
| `tests/llm/test_llm_exception_safety.cpp` | P5-L01 | 36 | Team D1 |
| `tests/llm/test_llm_memory_safety.cpp` | P5-L02 | 15 | Team D2 |

**Total: 90 tests** (exceeds 80 target by 10 tests)

### Fault Injection Strategy

```cpp
// Simulate transient failures for retry testing
class FaultInjector {
  bool should_inject(FailureMode mode) {
    // 5% transient connection failure rate
    // 1% timeout rate
    // 0.5% corrupted packet rate
  }
};

// Test:
fault_injector.enable(TRANSIENT_CONNECTION_RESET);
// 99.9% of retries should recover
assert(success_rate >= 0.999);
```

### Memory Profiling Pipeline

```bash
# Daily memory baseline (CI)
valgrind --leak-check=full --tool=massif --massif-out-file=massif.out ./llm_test

# Generate profile report
ms_print massif.out > memory_profile.txt

# Compare with previous baseline
python tools/memory_baseline_diff.py memory_profile.txt baseline.txt
```

---

## Team Structure & Responsibilities

### Team C: Server Hardening (2 engineers)
- **C1 (Lead):** P5-S01 (wire-protocol retry) — all 5 tasks
- **C2 (Member):** P5-S02 (HTTP timeout + shutdown) — all 5 tasks

### Team D: LLM Hardening (2 engineers)
- **D1 (Lead):** P5-L01 (exception safety) — all 6 tasks
- **D2 (Member):** P5-L02 (memory leaks) — all 6 tasks

### Collaboration Points
- **Weekly sync:** Tuesdays 10am (Phase 5 standup)
- **Cross-team review:** All PRs reviewed by member of other team
- **Shared resources:** Fault injection framework + memory profiling tools

---

## Performance Targets

### Server Targets
- Retry success rate: 99.9% (transient faults injected at 5% rate)
- Per-request timeout: 30 seconds (default, tunable)
- Graceful shutdown: 99% requests complete within timeout
- Connection pool: no leaks, stable memory over time

### LLM Targets
- Exception safety score: >= 4/5 (vs. current 3.5/5)
- Memory leaks: 0 bytes definitely lost (Valgrind clean)
- Long-running stability: < 50MB growth over 1000 load/unload cycles
- Data races: 0 (TSan clean)

---

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Retry logic causes duplicated requests | Medium | Medium | Idempotency tracking + request ID deduplication |
| Graceful shutdown timeout too short | Low | Medium | Configurable timeout; default 30s (tunable) |
| Exception handling masks real errors | Medium | Medium | Always log before swallowing exceptions; alert on retry counts |
| Memory leak hard to reproduce | Medium | High | Continuous profiling + automated detection in CI |
| TSan false positives (benign races) | Low | Low | Document benign races + suppress via tsan.txt |

---

## Acceptance & Sign-Off Gate

### Release Gate Checklist (all must be ✅)

**Server (P5-S):**
- [ ] All 39 wire-protocol + HTTP tests passing (19 + 20)
- [ ] Transient fault recovery: 99.9% success rate (fault injection verified)
- [ ] Graceful shutdown: 99% pending requests complete
- [ ] Connection pool: no leaks, stable memory
- [ ] Scanner audit: 0 new CRITICAL, < 2 new HIGH findings
- [ ] Doxygen audit: 0 warnings

**LLM (P5-L):**
- [ ] All 51 exception-safety + memory tests passing (36 + 15)
- [ ] Exception safety score: >= 4/5
- [ ] Valgrind clean: 0 definitely lost, 0 indirectly lost
- [ ] TSan clean: 0 data races on model load/unload
- [ ] Memory growth over 1000 cycles: < 50MB
- [ ] Doxygen: All public APIs documented with @throw

**Overall Phase 5:**
- [ ] All 90 new tests passing (0 flakes in 3 consecutive runs)
- [ ] No regressions in existing server + LLM tests
- [ ] Scanner audit: 0 new CRITICAL findings
- [ ] Code review: >= 1 peer review from different team
- [ ] Security review: No vulnerabilities introduced

### Sign-Off Authorities
- **Tech Lead:** Code quality + architecture
- **Release Manager:** Timeline + acceptance criteria
- **Memory Lead:** Valgrind + TSan results

---

## References & Evidence

### Current State Documentation
- Server module status: [ROADMAP.md](../../ROADMAP.md) (line 597, "PRODUCTION_CANDIDATE")
- LLM module status: [ROADMAP.md](../../ROADMAP.md) (line 577, "PRODUCTION_CANDIDATE")
- Wire protocol: GitHub Issue #5382 (referenced in memories)

### Related Issues
- GitHub Issue (to be created): #XXXX — Phase 5: Server & LLM Hardening Epic
- Sub-issues (to be created): #XXXX-S01 (Retry), #XXXX-S02 (Timeout), #XXXX-L01 (Exception), #XXXX-L02 (Memory)

---

## Next Steps

1. **Week 1 (2026-07-22):** Kickoff meeting, baseline measurements (memory profiling, fault injection setup)
2. **Week 2:** Parallel implementation (Teams C+D working independently)
3. **Week 3:** Integration + fault injection verification
4. **Sign-Off:** Friday 2026-08-09

**Kickoff Actions (Tuesday 2026-07-22):**
- [ ] Assign Team C & D members
- [ ] Create `feature/server-phase5-hardening` branch
- [ ] Create `feature/llm-phase5-hardening` branch
- [ ] Create GitHub issues #XXXX-S01 through #XXXX-L02
- [ ] Setup fault injection framework + memory profiling CI pipeline
- [ ] Schedule weekly sync meetings (Tuesdays 10am)

---

**Document Owner:** Tech Lead  
**Last Updated:** 2026-07-18  
**Approval:** Pending architecture review (scheduled 2026-07-22)
