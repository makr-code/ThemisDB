# Network Module Gap Closure — Batch 4 Quick Reference

**Status:** Analysis Complete ✅  
**Date:** 2026-08-15  
**Total Gaps:** 2,083  
**GA-Blocking:** 520 (CRITICAL: 29, HIGH: 491)  

---

## One-Page Summary

### Gap Distribution

```
2,083 Total Gaps
├── 29 CRITICAL (1.4%)    [MUST fix for GA]
├── 491 HIGH (23.6%)      [MUST fix for GA]
├── 1,561 MEDIUM (75.0%)  [Wave B Phase 7+]
└── 2 LOW (0.1%)          [Monitor]
```

### Batch Strategy

| Batch | When | What | Count | Gate |
|-------|------|------|-------|------|
| **4.1** | Aug 16-22 | CRITICAL fixes | 19 | ASan=0, tests=✓ |
| **4.2** | Aug 23-Sep 6 | Lock ordering + safety | ~150 | TSan=0, deadlock-free |
| **4.3** | Sep 6-20 | Resilience patterns | ~200 | Retry tests=✓ |
| **4.4** | Sep 21-Oct 4 | C++ safety | ~140 | Static analysis=✓ |
| **Total** | 8 weeks | All HIGH + CRITICAL | ~519 | GA Ready ✅ |

---

## Critical Issues by Category

### Category 1: Brace Imbalance (5) — BLOCKER
- `kernel_bypass.cpp:1`
- `quic_server.cpp:1`
- `raft_load_balancer.cpp:1`
- `wire_protocol_performance.cpp:1`
- `wire_protocol_v2.cpp:1`

**Fix:** Balance braces at file start  
**Impact:** Compilation failure

### Category 2: Missing Destructor (3) — BLOCKER
- `socket_timeout_manager.cpp:71`
- `service_mesh.cpp:175`
- `service_mesh.cpp:194`

**Fix:** Add virtual destructor  
**Impact:** Resource leak

### Category 3: No Timeout (3) — BLOCKER
- `wire_protocol_zero_copy.cpp:112`
- `wire_protocol_zero_copy.cpp:160`
- `service_mesh.cpp:243`

**Fix:** Add timeout parameter to blocking ops  
**Impact:** Deadlock risk

### Category 4: Buffer Overflow (2) — BLOCKER
- `connection_compression.cpp:114` (unchecked_memcpy)
- `io_uring_batcher.cpp:251` (unchecked_memcpy)

**Fix:** Validate buffer sizes before memcpy  
**Impact:** Buffer overflow/underflow

### Category 5: Smart Pointer Misuse (1) — BLOCKER
- `socket_timeout_manager.cpp:202`

**Fix:** Use RAII smart pointers correctly  
**Impact:** Undefined behavior

### Category 6: Exception in Destructor (1) — BLOCKER
- `wire_protocol_zero_copy.cpp:220`

**Fix:** Mark destructor noexcept, wrap throws in try-catch  
**Impact:** Crash during exception unwinding

### Category 7: Blocking No Timeout (1) — BLOCKER
- `wire_protocol_performance.cpp:232`

**Fix:** Add timeout enforcement  
**Impact:** Deadlock risk

### Category 8: Connection Leak (3) — BLOCKER
- `raft_load_balancer.cpp:288`
- `raft_load_balancer.cpp:289`
- `raft_load_balancer.cpp:290`

**Fix:** Use RAII wrapper or explicit close() in all paths  
**Impact:** Connection pool exhaustion

---

## Batch 4.1 Implementation (Week 1: Aug 16-22)

### 19 Critical Fixes

| Fix ID | File | Issue | Line | Category | Fix |
|--------|------|-------|------|----------|-----|
| R01 | kernel_bypass.cpp | braces_imbalance | 1 | Braces | Balance braces |
| R02 | quic_server.cpp | braces_imbalance | 1 | Braces | Balance braces |
| R03 | raft_load_balancer.cpp | braces_imbalance | 1 | Braces | Balance braces |
| R04 | wire_protocol_performance.cpp | braces_imbalance | 1 | Braces | Balance braces |
| R05 | wire_protocol_v2.cpp | braces_imbalance | 1 | Braces | Balance braces |
| R06 | socket_timeout_manager.cpp | missing_dtor | 71 | Destructor | Add virtual dtor |
| R07 | service_mesh.cpp | missing_dtor | 175 | Destructor | Add virtual dtor |
| R08 | service_mesh.cpp | missing_dtor | 194 | Destructor | Add virtual dtor |
| R09 | wire_protocol_zero_copy.cpp | no_timeout | 112 | Timeout | Add timeout param |
| R10 | service_mesh.cpp | no_timeout | 243 | Timeout | Add timeout param |
| R11 | wire_protocol_zero_copy.cpp | no_timeout | 160 | Timeout | Add timeout param |
| R12 | connection_compression.cpp | unchecked_memcpy | 114 | Buffer | Validate size |
| R13 | io_uring_batcher.cpp | unchecked_memcpy | 251 | Buffer | Validate size |
| R14 | socket_timeout_manager.cpp | smart_ptr_misuse | 202 | SmartPtr | Use RAII correctly |
| R15 | wire_protocol_zero_copy.cpp | exception_in_destructor | 220 | Exception | Mark noexcept |
| R16 | wire_protocol_performance.cpp | blocking_no_timeout | 232 | Timeout | Add timeout param |
| R17 | raft_load_balancer.cpp | db_connection_leak | 288 | ConnLeak | RAII wrapper |
| R18 | raft_load_balancer.cpp | db_connection_leak | 289 | ConnLeak | RAII wrapper |
| R19 | raft_load_balancer.cpp | db_connection_leak | 290 | ConnLeak | RAII wrapper |

### Quality Gates

- ✅ Compiles without errors
- ✅ All tests PASS
- ✅ ASan = 0 alerts
- ✅ UBSan = 0 alerts
- ✅ TSan = 0 alerts
- ✅ No performance regression
- ✅ Documentation complete

---

## Batch 4.2 — Threading & Safety (Weeks 2-3: Aug 23-Sep 6)

### Focus Areas
1. **Lock Ordering** (100 items)
   - Enforce strict lock ordering with ordered_lock wrappers
   - Document lock ordering contracts
   - ThreadSanitizer validation

2. **Exception Safety** (42 + 8 items)
   - Replace manual cleanup with RAII
   - Use scope guards for exception-safe cleanup
   - Exception-safety tests

3. **Timeouts** (6 + 2 HIGH items)
   - Add timeout to blocking operations
   - Implement timeout enforcement
   - Timeout regression tests

### Gate: ThreadSanitizer PASS, deadlock-free verification

---

## Batch 4.3 — Resilience (Weeks 4-6: Sep 6-20)

### Focus Areas
1. **Retry Logic** (111 items)
   - Exponential backoff implementation
   - Circuit breaker patterns
   - Retry exhaustion tests

2. **Error Handling** (16 items)
   - Return value validation
   - Clear error semantics
   - Recovery path testing

### Gate: Resilience tests PASS, circuit breaker validated

---

## Batch 4.4 — C++ Safety (Weeks 7-8: Sep 21-Oct 4)

### Focus Areas
1. **Lifetime Management** (23 items)
   - Proper temporary object handling
   - Static analysis validation

2. **Move Semantics** (3 items)
   - Replace copies with moves
   - Performance improvement

3. **Endianness** (8 items)
   - Explicit endianness handling
   - Cross-platform compatibility

### Gate: Static analysis PASS, performance baseline stable

---

## Resource Summary

### Files to Modify
- High-impact: 8 files (6 CRITICAL issues each)
- Medium-impact: 7 files
- Low-impact: 15 files

### Lines of Code
- Estimated changes: ~500 LOC (additions/modifications)
- Test coverage: ~200 LOC (new test cases)
- Documentation: ~100 LOC (inline + API docs)

### Timeline
- **Total:** 8 weeks (Aug 16 - Oct 6, 2026)
- **Batch 4.1:** 1 week (CRITICAL path)
- **Batch 4.2:** 1.5 weeks (HIGH threading)
- **Batch 4.3:** 2 weeks (HIGH resilience)
- **Batch 4.4:** 2 weeks (HIGH safety)
- **Buffer:** 1.5 weeks (contingency)

### Capacity Requirements
- **Primary:** themisdb-implementer AI agent
- **Review:** AI code review agent
- **Escalation:** Human maintainer (on-demand)

---

## Decision Tree

```
┌─ Start Batch 4.1
│
├─ [ALL 19 FIXES PASS?]
│  ├─ YES → Proceed to Batch 4.2
│  └─ NO  → Investigate & escalate
│           Options: Fix immediately / Defer to 4.2+ / Technical debt decision
│
└─ [GA-Ready at Oct 6?]
   ├─ YES → Release v2.4.0 ✅
   └─ NO  → Extend by max 2 weeks OR defer HIGH items to v2.5.0
```

---

## Key Decisions & Rationale

| Decision | Rationale |
|----------|-----------|
| **Batch 4 (not earlier)** | Network module depends on Process/Failover/Updates support modules (Phase 1-6 complete) |
| **4-batch model** | Allows parallel CI/CD validation; reduces risk of large-batch regressions |
| **CRITICAL first** | Compilation blocker + memory safety issues must be fixed before HIGH items |
| **MEDIUM deferred** | 75% of gaps are documentation/optimization only; Wave B Phase 7+ adequate |
| **8-week timeline** | Aligns with Q3/Q4 2026 GA deadline; includes 1.5-week contingency buffer |

---

## Success Criteria

### ✅ GA-Ready Network Module (Oct 6)
1. All 29 CRITICAL gaps → FIXED (100%)
2. All 491 HIGH gaps → FIXED (100%)
3. All tests → PASS
4. All sanitizers → 0 alerts (ASan, UBSan, TSan)
5. Benchmarks → No regression
6. release_critical CI → GREEN
7. Documentation → Complete

### 📊 Metrics
- Code quality: Gap count 2,083 → 1,563 (tracked for Wave B)
- Safety: Memory/threading incidents → 0
- Performance: Baseline stable or improved
- Test coverage: +200+ LOC new tests

---

## Related Documents

**Planning:**
- `NETWORK_MODULE_GAPS_BATCH4_ANALYSIS.md` (full strategy)
- `NETWORK_BATCH4_1_CRITICAL_FIXES_CHECKLIST.md` (implementation guide)
- `NETWORK_MODULE_GAPS_BATCH4_EXECUTIVE_SUMMARY.md` (status tracking)

**Reference:**
- `src/network/MODULE_GAPS.md` (gap inventory)
- `src/network/ROADMAP.md` (module roadmap)
- `ROADMAP.md` (project-level GA gates)

---

## Next Action

→ **Proceed to Batch 4.1 Implementation with themisdb-implementer agent**

See: `NETWORK_BATCH4_1_CRITICAL_FIXES_CHECKLIST.md` for detailed implementation steps
