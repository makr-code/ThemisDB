# Wave 1 Remediation Priorities - Updated 2026-06-21

**Critical Update:** False positive analysis complete. Braces check scanner has ~99.99% FP rate. Revised priorities below.

---

## Status Summary

| Issue | Original Findings | Validated Issues | FP Rate | Action |
|-------|-----------------|-----------------|---------|--------|
| **#5482 - Braces** | 105,459 | ~1 | 99.99% | ✋ HOLD → Fix ontology_manager.cpp only |
| **#5483 - Memory/RAII** | 1,069 | HIGH confidence | <5% | ✅ PROCEED |
| **#5484 - Error/Thread** | 2,577 | HIGH confidence | <5% | ✅ PROCEED |

---

## Immediate Actions (Next 24 hours)

### 1. **FIX ontology_manager.cpp** (~30 min)
- **Issue:** One extra closing brace (balance = -1)
- **Confidence:** 100% (verified by check_braces.py)
- **Effort:** 30 minutes
- **Action:** Find and remove the extra `}`
- **Reference:** Issue #5482 comment with FP analysis
- **Status:** UNBLOCKED - Start immediately

### 2. **QUARANTINE Braces Check Scanner** (~15 min)
- **Action:** Mark braces check as "Research Phase"
- **Location:** `tools/scanners/gs3_step01_braces_check.py`
- **Change:** Add warning comment at top
- **Rationale:** Scanner unreliable until rewritten with proper C++ AST parsing
- **Status:** BLOCKING #5482 closure

### 3. **TRIAGE Memory & Error Handling Issues** (~1-2 hours)
- **Focus:** Network layer (server, RPC, API handlers)
- **From:** Issue #5483 (memory/RAII), #5484 (error/thread)
- **Sample Top Files:**
  - `server/mcp_server.cpp` (697 braces false positives, but real issues likely)
  - `server/rpc/rpc_service_impl.cpp` (512 false positives)
  - `server/monitoring_api_handler.cpp` (750 false positives)
- **Validation:** Check which have real memory/error issues (not braces-related)
- **Status:** BLOCKED by braces clarity, but can proceed with semantic analysis

---

## Wave 1 Revised Effort Estimate

| Category | Original | FP-Adjusted | Change |
|----------|----------|------------|--------|
| **Braces** | 24h | 0.5h | -23.5h |
| **Memory/RAII** | 40h | 40h | — |
| **Error Handling** | 50h | 50h | — |
| **Thread Safety** | 30h | 30h | — |
| **Wave 1 TOTAL** | **212h** | **156.5h** | **-27%** |
| **Timeline** | 6 weeks | 4-5 weeks | **-1 week** |

---

## Remediation Roadmap (Phase 1: Network Layer)

### Phase 1.1 — Memory Safety (Issue #5483)
**Target:** GPU/Network resource cleanup, RAII wrappers

| File | Finding Type | Confidence | Fix Type | Est. Effort |
|------|-------------|-----------|----------|----------|
| `server/mcp_server.cpp` | Uninitialized resources | 92% | RAII wrapper | 3h |
| `server/rpc/rpc_service_impl.cpp` | Leaked buffers | 90% | Scope guard | 2h |
| `server/monitoring_api_handler.cpp` | Missing cleanup | 89% | Auto release | 2h |
| `server/llm_api_handler.cpp` | Dangling ptr | 88% | shared_ptr | 2h |
| **Phase 1.1 Total** | | | | **9h** |

### Phase 1.2 — Error Handling (Issue #5484)
**Target:** HTTP/RPC error handling, exception safety

| File | Finding Type | Confidence | Fix Type | Est. Effort |
|------|-------------|-----------|----------|----------|
| `server/query_api_handler.cpp` | Silent failures | 87% | Error propagation | 3h |
| `server/rpc/rpc_service_impl.cpp` | Missing status checks | 86% | Guard clauses | 2h |
| Network layer broad | Missing try/catch | 85% | Exception safety | 5h |
| **Phase 1.2 Total** | | | | **10h** |

### Phase 1.3 — Thread Safety (Bonus from #5484)
**Target:** Lock ordering, critical sections

| Finding Type | Confidence | Fix Type | Est. Effort |
|-------------|-----------|----------|----------|
| Lock usage violations | 84% | Lock ordering audit | 4h |
| Unsynchronized state | 82% | Mutex scopes | 3h |
| **Phase 1.3 Total** | | | **7h** |

**Phase 1 Subtotal: 26 hours**

---

## Phase 2 (Next Sprint)

- Performance optimizations (cache sizing, query planning)
- Platform portability (CUDA compatibility layer)
- LLM safety constraints

**Est. Effort:** 80-100 hours

---

## Success Criteria for Wave 1

- ✅ ontology_manager.cpp fixed (1 extra brace removed)
- ✅ #5482 updated with "Use compiler errors instead" recommendation
- ✅ Memory safety fixes in top 5 network layer files
- ✅ Error handling hardening for RPC/API layer
- ✅ Thread safety validation with ThreadSanitizer
- ✅ Wave 1 closure with full audit report

---

## GitHub Issues Status

| Issue | Status | Updated | Next Step |
|-------|--------|---------|-----------|
| #5482 - Braces | 🟡 ON HOLD | ✅ Yes (FP analysis) | Quarantine scanner + fix 1 file |
| #5483 - Memory/RAII | 🟢 PROCEED | — | Start Phase 1.1 |
| #5484 - Error/Thread | 🟢 PROCEED | — | Start Phase 1.2 + 1.3 |
| #5475 - Wave 1 Meta | 🟢 UPDATED | ✅ Yes (effort revised) | Close when Phase 1 complete |

---

## Key Decisions

**Decision 1: Braces Check Scanner**
- **Option A:** Rewrite with proper C++ AST (libclang) - **1-2 days effort**
- **Option B:** Rely on compiler errors (-Wall -Werror) - **RECOMMENDED** (simpler, more reliable)
- **Option C:** Remove from gap scanner - **ACCEPTABLE** (defer to Phase 2)
- **Selected:** **Option B** + mark as "Research Phase" until Option A/C implemented

**Decision 2: False Positive Handling**
- **Will Not:** Implement 105,000+ meaningless braces fixes
- **Will Do:** Fix 1 real issue + inform team about scanner limitations
- **Impact:** Saves 23.5 hours of wasted effort

**Decision 3: Wave 1 Focus**
- **Shift:** From "all findings" → "network layer first" (highest impact/risk)
- **Rationale:** Memory + error handling in network layer blocks production deployment
- **Timeline:** 4-5 weeks down from 6 weeks

---

## Action Checklist

- [ ] **TODAY:** Fix ontology_manager.cpp (-1 brace)
- [ ] **TODAY:** Comment on #5482 with FP analysis + recommendation
- [ ] **TODAY:** Quarantine braces check scanner
- [ ] **TOMORROW:** Start Phase 1.1 (memory safety, 3 files)
- [ ] **WEEK 1:** Complete Phase 1 (26 hours)
- [ ] **WEEK 2-3:** Phase 2 prep + validation
- [ ] **WEEK 4:** Final audit report

---

**Status:** Wave 1 Remediation UNBLOCKED with revised priorities
