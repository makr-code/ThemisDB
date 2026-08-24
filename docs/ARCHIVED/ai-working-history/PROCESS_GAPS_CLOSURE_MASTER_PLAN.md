# Process Module Gap Closure — Master Coordination Plan

**Date:** 2026-08-16  
**Status:** EXECUTING  
**Scope:** Process module 177 findings (53 CRITICAL+HIGH actionable)  
**Model:** 3-agent parallel execution with per-file isolation

---

## Executive Summary

The Process module contains **177 total findings** (CRITICAL: 5, HIGH: 48, MEDIUM: 111, LOW: 13) across **19 files**. This plan executes **3 parallel sub-agents** with strict **no-file-overlap** to deliver production-ready source code with hardened error handling, memory safety, and performance optimization.

**Estimated Timeline:**
- Agent execution: 5–6 hours (parallel)
- Integration + validation: 2–3 hours
- **Total: 7–9 hours** (vs. ~15+ hours sequential)

---

## Scope & Severity Breakdown

| Severity | Count | Status | Assignee |
|---|---|---|---|
| **CRITICAL** | 5 | Sub-agent-critical-batch-1 | Agent 1 |
| **HIGH (Group A)** | 27 | Sub-agent-high-batch-2a | Agent 2 |
| **HIGH (Group B)** | 21 | Sub-agent-high-batch-2b | Agent 3 |
| **MEDIUM + LOW** | 124 | Deferred (v2.4.1) | — |

---

## Agent Execution Map

### 🔴 Agent 1: CRITICAL Findings + Top-Risk HIGH
**Target Files:** process_graph_rag.cpp, dmn_evaluator.cpp, vcc_vpb_importer.cpp (CRITICAL only)

**Deliverables:**
- ✅ Iterator invalidation fix (lines 367–368 in process_graph_rag.cpp)
- ✅ Pointer arithmetic bounds validation (lines 244–245)
- ✅ Thread-safety hardening in dmn_evaluator.cpp
- ✅ Resource lifecycle fixes in vcc_vpb_importer.cpp (CRITICAL: line 1 resource_leaked_in_exception)

**Gate Criteria:**
- Zero new compiler warnings
- All CRITICAL findings fixed or marked `[STUB/SIMULATION NOTE]` with removal plan
- Focused test suite passes (15+ tests, min P23-01..P23-05)

---

### 🟢 Agent 2: HIGH Findings Group A
**Target Files:** process_agentic_rag.cpp, vcc_vpb_importer.cpp (HIGH remainder)

**Deliverables:**
- ✅ Fix 12 HIGH in process_agentic_rag.cpp (string_concat_loop, unordered_container_iter patterns)
- ✅ Fix 8 remaining HIGH in vcc_vpb_importer.cpp (pointer_arithmetic_unbounded, nested_loop_find)
- ✅ Resource cleanup and bounds validation
- ✅ Performance optimization (O(n²) → O(n log n) patterns)

**Gate Criteria:**
- All HIGH findings in scope fixed or deferred (documented)
- Focused test suite passes (20+ tests, min EXS-01..EXS-20)
- Benchmark regression < 5%

---

### 🔵 Agent 3: HIGH Findings Group B
**Target Files:** process_linker.cpp, process_community_detector.cpp, ocel_exporter.cpp, epk_serializer.cpp, process_graph_rag.cpp (HIGH remainder), bpmn_serializer.cpp (HIGH only)

**Deliverables:**
- ✅ Fix 5 HIGH in process_linker.cpp
- ✅ Fix 3 HIGH in process_community_detector.cpp
- ✅ Fix 2 HIGH in ocel_exporter.cpp
- ✅ Fix 1 HIGH in epk_serializer.cpp
- ✅ Fix 12 remaining HIGH in process_graph_rag.cpp (non-CRITICAL)
- ✅ Fix 3 HIGH in bpmn_serializer.cpp

**Gate Criteria:**
- All HIGH findings in scope fixed
- Focused test suite passes (25+ tests, min OBJ-01..OBJ-25)
- Security sanitizer pass (no new data races)

---

## Execution Timeline

### T+0 to T+60 min: Parallel Agent Startup
- [ ] Agent 1: Pull repo, analyze CRITICAL scope, plan fixes
- [ ] Agent 2: Pull repo, analyze HIGH-A scope, plan fixes
- [ ] Agent 3: Pull repo, analyze HIGH-B scope, plan fixes

### T+60 to T+300 min: Implementation Phase
- [ ] Agent 1: Implement & test CRITICAL + top-risk HIGH fixes
- [ ] Agent 2: Implement & test HIGH Group A fixes
- [ ] Agent 3: Implement & test HIGH Group B fixes

### T+300 to T+360 min: Integration & Validation
- [ ] Merge all agent branches onto develop (sequential)
- [ ] Full build verification (linux-release, community-release, windows-release presets)
- [ ] Complete test suite execution + benchmarks
- [ ] CodeQL + sanitizer verification

### T+360 to T+420 min: Sign-off & Documentation
- [ ] ROADMAP.md § Findings Closure update
- [ ] Evidence bundle consolidation
- [ ] Final gate verification

---

## File Assignment (No Overlap)

| File | Agent | Findings | CRITICAL | HIGH | Notes |
|---|---|---|---|---|---|
| process_graph_rag.cpp | 1 (CRITICAL) + 3 (HIGH) | 28 | 2 | 12 | Split: Iterator inv → A1, O(n²) patterns → A3 |
| dmn_evaluator.cpp | 1 | 13 | 2 | 0 | Thread-safety + LOW issues |
| vcc_vpb_importer.cpp | 1 (CRITICAL) + 2a (HIGH) | 14 | 1 | 9 | Split: resource_leak → A1, pointer/string patterns → A2 |
| process_agentic_rag.cpp | 2 | 14 | 0 | 12 | String concat + container iter patterns |
| process_linker.cpp | 3 | 6 | 0 | 5 | Nested loop + string patterns |
| process_community_detector.cpp | 3 | 10 | 0 | 3 | String + iterator patterns |
| ocel_exporter.cpp | 3 | 8 | 0 | 2 | Hardcoded path + regex patterns |
| epk_serializer.cpp | 3 | 5 | 0 | 1 | String + container patterns |
| bpmn_serializer.cpp | 3 | 22 | 0 | 3 | String + resource patterns; MEDIUM/LOW deferred |
| Other files (10 remaining) | Deferred | 57 | 0 | 0 | MEDIUM + LOW; v2.4.1 target |

---

## Success Criteria

### Phase Completion Gate (All Must PASS)

✅ **Code Quality:**
- Zero new compiler warnings on any preset
- Clang-tidy pass (all agents must run locally)
- No CRITICAL findings remaining in scope

✅ **Testing:**
- All focused test suites pass (P23-01..P23-XX, EXS-01..EXS-XX, OBJ-01..OBJ-XX)
- Process module full test suite passes (72+ tests minimum)
- 42 benchmark gates passing

✅ **Security & Performance:**
- Sanitizer run clean (no new data races, leaks, or undefined behavior)
- Benchmark regression < 5% on representative baseline
- Doxygen 100% on all modified public APIs

✅ **Governance:**
- All ROADMAP.md acceptance criteria for Phase 1-6 remain satisfied
- Evidence bundle complete + linked
- No new medium/high blocking issues

---

## Blockers & Mitigation

| Risk | Probability | Mitigation |
|---|---|---|
| File merge conflicts during integration | Medium | Sequential merge with conflict resolution checklist |
| Build/CMake dependency changes | Low | Verify CMakePresets.json unchanged; pre-test on all presets |
| Test flakiness on high-concurrency paths | Medium | Rerun failing tests 3x; escalate if persistent |
| Benchmark regression > 5% | Low | Profile hot paths; defer O(n²) fixes if unacceptable |

---

## Agent Coordination

All agents operate under:
1. **Master source:** develop branch (current HEAD)
2. **Base commit:** Latest commit on develop at T+0
3. **Merge strategy:** Sequential merge to avoid conflicts; each agent rebases before final push
4. **Communication:** Weekly sync; escalate blockers in real-time to orchestrator

---

## References

- **Gap Source:** `/home/runner/work/ThemisDB/ThemisDB/src/process/MODULE_GAPS.md`
- **Acceptance Criteria:** `/home/runner/work/ThemisDB/ThemisDB/src/process/ROADMAP.md`
- **Test Coverage:** `/home/runner/work/ThemisDB/ThemisDB/tests/process/`
- **Build:** `cmake --preset linux-release`, `cmake --preset community-release`, `cmake --preset windows-release`

---

**Next Step:** Launch Agents 1, 2, 3 in parallel.
