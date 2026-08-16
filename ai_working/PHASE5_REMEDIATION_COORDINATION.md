# Phase 5 Blocker Remediation — Master Coordination (2026-08-15)

**Status:** 🟡 DISPATCHING NOW (2026-08-15 15:20 UTC)  
**Target Deadline:** 2026-08-20 18:00 UTC (48 hours before hard deadline 2026-08-22)  
**Total Effort:** 3 coordinated agents, 18-24 hours total effort

---

## Executive Summary

3 remediation agents dispatched to fix 6 CRITICAL/HIGH/MEDIUM findings blocking Phase 5 CP-1 checkpoint validation:

| Agent | Blockers | Type | ETA | Dependency |
|-------|----------|------|-----|-----------|
| **Agent 1** | #1, #2, #3, #4 (C++ Fixes) | themisdb-implementer | 2026-08-20 12:00 UTC | None |
| **Agent 2** | #5 (CMake Presets) | task | 2026-08-15 18:00 UTC | None |
| **Agent 3** | #6 (Tests) | themisdb-implementer | 2026-08-21 18:00 UTC | Agent 1 |

**Key Insight:** Agents 1 & 2 run in parallel (independent). Agent 3 waits for Agent 1.

---

## Agent Dispatch Order

### ✅ DISPATCH 1: Agent 2 (CMake Infrastructure) — IMMEDIATE

**Agent Name:** `index-phase5-cmakepreset-remediation`  
**Type:** task  
**Spec File:** `ai_working/PHASE5_REMEDIATION_AGENT2_SPEC.md`  
**Blocker:** #5 (HIGH)  
**Timeline:** 2-4 hours → target 2026-08-15 18:00 UTC  
**Dependencies:** None

**Rationale:** Independent from code changes; can execute immediately and unlock validation infrastructure.

**Acceptance Criteria:**
- [ ] 3 presets added (develop-strict, develop-asan, develop-tsan)
- [ ] CMakePresets.json valid JSON (python3 -m json.tool passes)
- [ ] All presets configure successfully
- [ ] Commit with msg: "build(cmake): Add missing sanitizer presets"

---

### ✅ DISPATCH 2: Agent 1 (C++ Code Fixes) — IMMEDIATE

**Agent Name:** `index-phase5-code-remediation`  
**Type:** themisdb-implementer  
**Spec File:** `ai_working/PHASE5_REMEDIATION_AGENT1_SPEC.md`  
**Blockers:** #1, #2, #3, #4 (3 CRITICAL, 1 HIGH)  
**Timeline:** 8-12 hours → target 2026-08-20 12:00 UTC  
**Dependencies:** None (but can use Agent 2 presets once available)

**Scope:**
- Exception-in-destructor (VectorIndexManager) → noexcept + try-catch
- Unsafe raw delete (2 locations) → std::unique_ptr
- GPU Vector Index destructor → noexcept + explicit cleanup
- Iterator invalidation → size-cached + bounds guards

**Acceptance Criteria:**
- [ ] All 4 fixes in single commit
- [ ] Destructors marked noexcept with exception wrapping
- [ ] All new/delete → std::unique_ptr/std::make_unique
- [ ] Iterator patterns refactored
- [ ] Compiles without warnings
- [ ] ASan: 0 leaks/errors
- [ ] UBSan: 0 undefined behavior
- [ ] TSan: 0 data races
- [ ] Existing tests pass (no regressions)
- [ ] Commit with msg: "fix(index): Phase 5 blocker resolution — destructors, RAII, iterator safety"

---

### ⏳ DISPATCH 3: Agent 3 (Test Suite) — AFTER AGENT 1

**Agent Name:** `index-phase5-test-remediation`  
**Type:** themisdb-implementer  
**Spec File:** `ai_working/PHASE5_REMEDIATION_AGENT3_SPEC.md`  
**Blocker:** #6 (MEDIUM)  
**Timeline:** 6-8 hours (after Agent 1) → target 2026-08-21 18:00 UTC  
**Dependencies:** Agent 1 must merge first

**Scope:**
- test_index_destructor_safety.cpp (5 tests)
- test_index_iterator_validity.cpp (3 tests)
- test_index_gpu_memory_safety.cpp (3 tests)
- CMakeLists.txt integration

**Trigger:** After Agent 1 commits code fixes

**Acceptance Criteria:**
- [ ] 3 test files created in tests/index/
- [ ] All tests compile without errors
- [ ] All tests PASS with default config
- [ ] ASan: 0 memory errors
- [ ] TSan: 0 data races (iterator tests)
- [ ] No test regressions
- [ ] Commit with msg: "test(index): Add Phase 5 blocker validation test suite"

---

## Critical Path & Timeline

```
2026-08-15 15:20 UTC: DISPATCH Agents 1 & 2 (parallel)
  ├─ Agent 2 (CMake): 2026-08-15 15:20 → 2026-08-15 18:00 (2-4 hrs) ✅
  │  └─ Commit: develop-strict, develop-asan, develop-tsan presets
  │
  └─ Agent 1 (C++ Code): 2026-08-15 15:20 → 2026-08-20 12:00 (8-12 hrs) ✅
     └─ Commit: noexcept destructors, unique_ptr, iterator fix
     
2026-08-20 12:00 UTC: Agent 1 COMPLETES & MERGES
  └─ Agent 3 (Tests): 2026-08-20 12:00 → 2026-08-21 18:00 (6-8 hrs) ⏳
     └─ Commit: 3 test files + CMakeLists.txt update

2026-08-21 18:00 UTC: Agent 3 COMPLETES & MERGES

2026-08-22 18:00 UTC: HARD DEADLINE (7 days buffer)
  └─ Expected: ALL FIXES MERGED (with 4 days buffer)

2026-08-28 10:00 UTC: Phase 5 Re-Validation Starts
  └─ themisdb-reviewer re-checks all 6 findings
  └─ If PASS: approve CP-1 checkpoint restart
  └─ Expected: 2-4 hours review
```

**Buffer Time:** 6 days (2026-08-22 to 2026-08-28) for:
- Unexpected test failures
- Edge case fixes
- Code review feedback
- Re-validation cycles

---

## Coordination Checklist

### Pre-Dispatch (NOW)

- [x] Read and understand blocker report (6 findings)
- [x] Create 3 agent specs (Agent1/Agent2/Agent3)
- [x] Define success criteria per agent
- [x] Timeline validation (fits within 2026-08-20 48-hour target)

### During Execution

- [ ] Monitor Agent 1 & 2 progress (parallel, 1st 6 hours)
- [ ] Verify Agent 2 CMake presets work with Agent 1 builds
- [ ] Track Agent 1 compiler/sanitizer issues
- [ ] Queue Agent 3 spec for dispatch after Agent 1 merges
- [ ] Update live status doc: `INDEX_GAP_CLOSURE_LIVE_STATUS_2026-08-15.md`

### Post-Completion

- [ ] All 3 agents report SUCCESS (fixes merged)
- [ ] Re-run full CI/CD validation (ASan/UBSan/TSan for all fixes)
- [ ] Prepare re-validation brief for Phase 5 reviewer
- [ ] Update ROADMAP.md: Phase 5 blocker RESOLVED
- [ ] Notify Phase 5 reviewer: ready for CP-1 re-validation

---

## Success Metrics

### Code Quality

- Destructors: 100% marked noexcept
- RAII: 100% new/delete → smart pointers
- Iterators: 100% bounds-checked access
- Exceptions: ASan/UBSan/TSan: 0 alerts
- Tests: All tests PASS, no regressions

### Timeline

- Agent 1: ≤12 hours (target 2026-08-20 12:00)
- Agent 2: ≤4 hours (target 2026-08-15 18:00)
- Agent 3: ≤8 hours (target 2026-08-21 18:00)
- **Total: ≤24 hours, on schedule for 2026-08-22 deadline**

### Documentation

- [ ] 3 agent specs complete (yes, done now)
- [ ] 3 commits with clear messages
- [ ] Master coordination doc (this file)
- [ ] Final status report for Phase 5 reviewer

---

## Agent Specifications (Reference Links)

1. **Agent 1 (C++ Code Fixes):** `ai_working/PHASE5_REMEDIATION_AGENT1_SPEC.md`
   - 4 blockers, 8-12 hours, themisdb-implementer
   
2. **Agent 2 (CMake Presets):** `ai_working/PHASE5_REMEDIATION_AGENT2_SPEC.md`
   - 1 blocker, 2-4 hours, task agent
   
3. **Agent 3 (Test Suite):** `ai_working/PHASE5_REMEDIATION_AGENT3_SPEC.md`
   - 1 blocker, 6-8 hours, themisdb-implementer (after Agent 1)

---

## Escalation Contacts

**If Agent 1 (Code Fixes) Fails:**
1. Review blocker report for missing details
2. Check compile errors in themis-dev Slack
3. Escalate to human reviewer if > 2 hours blocked

**If Agent 2 (CMake) Fails:**
1. Verify CMake version ≥ 3.20
2. Check CMakePresets.json syntax (python3 -m json.tool)
3. Retry with corrected JSON

**If Agent 3 (Tests) Fails:**
1. Verify Agent 1 code fixes merged
2. Check GTest installation
3. Verify GPU/CUDA environment (if GPU tests fail)

---

## Expected Outcomes

### After Agent 1 Completes:
- 4 CRITICAL/HIGH findings fixed
- All destructors noexcept + exception-safe
- All manual memory management → RAII
- All iterator patterns safe
- ASan/UBSan/TSan validation gates pass
- **Status:** Unblocks Agent 3 (tests)

### After Agent 2 Completes:
- CI/CD validation infrastructure ready
- 3 new sanitizer presets available for Agent 1
- Developers can run: `cmake --preset develop-asan && ctest`
- **Status:** Unblocks future CI automation

### After Agent 3 Completes:
- 3 new test files ensure regression protection
- 10+ tests validate destructor/iterator/GPU safety
- All tests pass with ASan/TSan/UBSan
- **Status:** Full blocker remediation complete

### After Phase 5 Re-Validation:
- All 6 findings addressed and verified
- CP-1 checkpoint validation APPROVED
- Phase 5 proceeds to formal code review
- Timeline impact: 7-day delay absorbed (2026-10-07 still achievable)

---

## Communication Plan

**Agent 1 (Code Fixes):** Expected to provide updates every 4 hours
- Hour 0-4: File identification, initial edits
- Hour 4-8: Destructor + delete fixes
- Hour 8-12: Iterator fixes, sanitizer validation

**Agent 2 (CMake Presets):** Expected to provide final report within 4 hours
- Presets added and tested
- Commit ready for merge

**Agent 3 (Test Suite):** Queued for dispatch after Agent 1
- Will provide status updates hourly during execution

**Phase 5 Reviewer:** Awaits blocker completion
- Re-validation starts 2026-08-28
- Expected 2-4 hour turnaround

---

**Document Created:** 2026-08-15 15:20 UTC  
**Prepared By:** Coordinating Agent  
**Status:** AGENTS READY FOR DISPATCH

---

**Next Step:** Dispatch Agents 1 & 2 immediately (parallel execution)

