# Phase 3: Subagent Coordination & Delegation Strategy

**Status:** 🟡 **READY FOR EXECUTION** (2026-07-01)  
**Owner Coordination:** @makr-code (coordinator) + subagent team  
**Timeline:** 6 weeks (2026-07-15 to 2026-08-27)  
**Target:** v2.5 release

---

## Overview

Phase 3 is a **large-scale remediation effort** with 107 findings across 6 categories. To execute efficiently, we will use **parallel subagent teams** to handle different finding categories while maintaining synchronization and quality gates.

### Subagent Types Available

| Agent | Specialty | Use Case |
|-------|-----------|----------|
| **explore** | Codebase analysis, symbol search, impact assessment | Finding analysis, scope mapping |
| **task** | Build/test execution, CI validation | Running test suites, benchmarks |
| **general-purpose** | Complex multi-step work, high-quality reasoning | Deep refactoring, optimization |
| **code-review** | Security/quality analysis, bug detection | Vulnerability review, pattern validation |
| **themisdb-implementer** | ThemisDB-specific code fixes, guided implementation | Primary fix implementation |
| **themisdb-reviewer** | ThemisDB change review, architecture validation | Code quality & correctness review |
| **gap-verifier** | AI code review, false-positive elimination | Gap remediation verification |

---

## Parallel Execution Strategy

### Coordination Model

```
┌─────────────────────────────────────────────────────────────────┐
│                      HUMAN COORDINATOR                          │
│                       (@makr-code)                              │
│  - Assigns work to agents                                       │
│  - Monitors progress & gates                                    │
│  - Integrates results                                           │
│  - Makes go/no-go decisions                                     │
└──────────────────────────┬──────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
    ┌────────┐         ┌────────┐        ┌────────┐
    │ ANALYSIS│         │ BUILDER │        │ IMPL   │
    │ TEAM    │         │ TEAM    │        │ TEAM   │
    └────────┘         └────────┘        └────────┘
       Agent:             Agent:            Agent:
      explore         task + general      themisdb-
      gap-verifier    purpose             implementer
```

### Team Composition

**Team 1: Analysis & Categorization (Sprint 1)**
- **Agent:** explore (primary) + gap-verifier (secondary)
- **Mission:** Categorize 107 findings, identify quick-wins, create playbooks
- **Output:** PHASE_3_FINDING_ANALYSIS.md

**Team 2: Quick-Win Implementation (Sprints 2–3)**
- **Agent:** themisdb-implementer (parallel batch executors × 2–3)
- **Mission:** Implement 25–30 MEDIUM findings (4 batches, <2 hrs each)
- **Output:** 4 commits, batch results

**Team 3: HIGH Remediation (Sprint 4)**
- **Agent:** themisdb-implementer (focused) + themisdb-reviewer
- **Mission:** Resolve all 19–25 blocking issues
- **Output:** All HIGH findings resolved, documented

**Team 4: MEDIUM Optimization (Sprint 5)**
- **Agent:** general-purpose (architecture focus) + themisdb-implementer
- **Mission:** Performance & memory improvements (35 findings)
- **Output:** Benchmark results, optimization documentation

**Team 5: Stability & Release (Sprint 6)**
- **Agent:** task (test runner) + code-review (security/quality)
- **Mission:** 100× test runs, memory profiling, rc prep
- **Output:** Stability report, release candidate

---

## Sprint-by-Sprint Execution Plan

### Sprint 1: Analysis & Categorization (Week 1–2)

**Agent Assignment:** explore + gap-verifier

**Tasks:**
1. **explore agent (Phase 3.1):**
   - Query Phase 2.4 audit output for 107 findings
   - Categorize by: scope, cache, memory, concurrency, performance, distributed, error handling
   - Map findings to source files and code locations
   - Identify cross-file dependencies

   **Prompt Template:**
   ```
   Use explore agent to analyze Phase 2.4 audit findings:
   - Parse the 107 findings from PHASE_2_4_L1_CONFORMANCE_AUDIT_REPORT.md
   - Categorize each finding into: (scope/lifetime, cache/concurrency, memory/RAII, 
     performance/algo, distributed, error/robustness)
   - For each category, create a list of top 5–10 quick-win candidates (fix <2 hrs)
   - Output: JSON structure with finding IDs, categories, quick-win flags, file paths
   ```

2. **gap-verifier agent (Phase 3.2):**
   - Verify categorization accuracy
   - Eliminate false-positive findings
   - Validate quick-win estimates
   - Rank findings by impact

   **Prompt Template:**
   ```
   Use gap-verifier to validate Phase 3 finding analysis:
   - Review explore agent's categorization
   - Verify each quick-win candidate has realistic <2 hr fix estimate
   - Check for false positives (defensive patterns vs. real bugs)
   - Output: Verified finding list with quick-win confidence levels (high/medium/low)
   ```

3. **Deliverable:** `PHASE_3_FINDING_ANALYSIS.md`

**Human Checkpoint:**
- Review categorization logic
- Approve quick-win list
- Validate category priorities
- Go/no-go for Sprint 2

---

### Sprints 2–3: Quick-Win Batches 1–4 (Weeks 2–4)

**Agent Assignment:** themisdb-implementer (parallel × 2–3)

**Batch Structure:**
- **QW-P3-1:** Memory & RAII patterns (8–10 findings)
- **QW-P3-2:** Scope & lifetime cleanup (7–9 findings)
- **QW-P3-3:** Cache consistency basics (8–12 findings)
- **QW-P3-4:** Error path hardening (4–6 findings)

**Execution (per batch):**

1. **themisdb-implementer agent (4 parallel agents, one per batch):**

   **Prompt Template (per batch):**
   ```
   Use themisdb-implementer to implement QW-P3-[N]:
   - Source findings from PHASE_3_FINDING_ANALYSIS.md (category: [CATEGORY])
   - For each finding, implement focused fix (< 2 hrs):
     * [Finding 1] - [Fix description]
     * [Finding 2] - [Fix description]
     * ...
   - Apply consistent patterns across all fixes in this batch
   - Run: cmake --build --preset community-release --target themis_graph
   - Run: ctest --preset community-release -R "test_graph" --output-on-failure
   - Output: Commit with batch results, no regressions
   ```

2. **task agent (post-batch validation):**
   - Run full 326-test suite
   - Capture benchmark baseline
   - Report regressions (if any)

   **Prompt Template:**
   ```
   Use task agent to validate QW-P3-[N]:
   - Build: cmake --preset community-release --target themis_graph
   - Test: ctest --preset community-release -R "test_graph" -j 4 --output-on-failure
   - Benchmark: ./build-community-release/benchmarks/graph/graph_benchmarks
   - Output: Test results, benchmark data, any regressions flagged
   ```

3. **Deliverable (per batch):**
   - ✅ Commit with batch fixes
   - ✅ Test results (326 tests PASS)
   - ✅ Benchmark baseline captured
   - ✅ No regressions

**Human Checkpoint (after each batch):**
- Review commit quality
- Verify test results
- Check for unexpected side effects
- Approve or request changes

---

### Sprint 4: HIGH-Severity Remediation (Week 4–5)

**Agent Assignment:** themisdb-implementer (focused) + themisdb-reviewer

**Categories:**
- **HIGH-1:** Blocking scope mismatches (5–7 findings)
- **HIGH-2:** Resource leaks (3–4 findings)
- **HIGH-3:** Concurrent access violations (4–6 findings)
- **HIGH-4:** Critical error paths (2–3 findings)

**Execution:**

1. **themisdb-implementer agent (focused on HIGH findings):**

   **Prompt Template:**
   ```
   Use themisdb-implementer to fix HIGH-severity findings:
   - Source findings from PHASE_3_FINDING_ANALYSIS.md (severity: HIGH)
   - For each category (scope, resource, concurrency, error):
     * Analyze root cause
     * Implement production-quality fix (not stub/workaround)
     * Add RAII guards where applicable
     * Document fix rationale
   - Build and test continuously (after each fix)
   - Output: Clean commits, no test regressions
   ```

2. **themisdb-reviewer agent (code quality gate):**

   **Prompt Template:**
   ```
   Use themisdb-reviewer to validate HIGH fixes:
   - Review each themisdb-implementer commit
   - Check: RAII correctness, error handling, resource cleanup, concurrency safety
   - Verify: No memory leaks (ASAN potential), no data races (TSAN potential)
   - Flag: Any incomplete fixes or edge cases
   - Output: Approval or revision requests
   ```

3. **task agent (regression testing):**
   - Run full test suite after each fix
   - Flag any regressions

4. **Deliverable:**
   - ✅ All HIGH findings resolved
   - ✅ Reviewed and approved
   - ✅ 326 tests PASS

**Human Checkpoint:**
- Review themisdb-reviewer feedback
- Approve all HIGH fixes
- Go/no-go for Sprint 5

---

### Sprint 5: MEDIUM Optimization Pass (Week 5–6)

**Agent Assignment:** general-purpose (optimization focus) + themisdb-implementer

**Optimization Categories:**
- **PERF-OPT-1:** Query optimization (10–12 findings)
- **PERF-OPT-2:** Memory efficiency (8–10 findings)
- **PERF-OPT-3:** Cache strategies (5–7 findings)

**Execution:**

1. **general-purpose agent (optimization analysis):**

   **Prompt Template:**
   ```
   Use general-purpose agent to design MEDIUM optimizations:
   - Analyze Phase 3 MEDIUM findings (performance category)
   - For each optimization (query, memory, cache):
     * Design implementation approach
     * Estimate expected improvement (5–20% for latency, 10–20% for memory)
     * Identify any compatibility concerns
     * Create code change patterns
   - Output: Optimization design doc with implementation steps
   ```

2. **themisdb-implementer agent (optimization implementation):**

   **Prompt Template:**
   ```
   Use themisdb-implementer to implement MEDIUM optimizations:
   - Source: general-purpose optimization designs
   - For each optimization:
     * Implement changes
     * Add performance-sensitive tests
     * Benchmark pre/post improvement
   - Build and test
   - Output: Commits with optimization + benchmarks
   ```

3. **task agent (performance validation):**
   - Run benchmark suite (5 iterations)
   - Compare to Phase 2.4 baseline
   - Calculate improvement percentages

4. **Deliverable:**
   - ✅ 25–30 MEDIUM findings resolved
   - ✅ Performance benchmarks (improvement documented)
   - ✅ No regressions

**Human Checkpoint:**
- Review optimization designs
- Validate benchmark results
- Check for acceptable tradeoffs

---

### Sprint 6: Stability & Release Prep (Week 6)

**Agent Assignment:** task (test runner) + code-review (security/quality)

**Tasks:**

1. **task agent (stability verification):**

   **Prompt Template:**
   ```
   Use task agent for Phase 3 stability validation:
   - Run 100× full graph test suite: for i in {1..100}; do ctest ...; done
   - Capture any flakes or transient failures
   - Run memory profiling: valgrind --leak-check=full tests/test_graph*
   - Run ASAN: ASAN_OPTIONS=detect_leaks=1 tests/test_graph*
   - Run TSAN (if applicable): TSAN_OPTIONS=halt_on_error=1 tests/test_graph*
   - Output: Stability report (flakes, memory leaks, data races)
   ```

2. **code-review agent (security/quality gate):**

   **Prompt Template:**
   ```
   Use code-review to validate Phase 3 quality:
   - Review all Phase 3 commits for:
     * Security vulnerabilities
     * Memory safety issues
     * Concurrency bugs
     * Error handling gaps
   - Check for intentional stubs/mocks (must be documented)
   - Verify no legacy paths introduced without approval
   - Output: Security findings + quality assessment
   ```

3. **task agent (final benchmark + rc prep):**

   **Prompt Template:**
   ```
   Use task agent for Phase 3 release preparation:
   - Run final benchmark suite with baseline comparison
   - Generate performance improvement report
   - Verify version bump in VERSION file
   - Create git tag for release/v2.5-rc1
   - Output: Benchmark report + rc ready signal
   ```

4. **Deliverable:**
   - ✅ Stability report (100× runs clean)
   - ✅ Memory profile (clean)
   - ✅ Security review (clean)
   - ✅ Performance benchmarks (improvements documented)
   - ✅ Release candidate created

**Human Checkpoint (Final):**
- Review all reports
- Approve Phase 3 completion
- Authorize v2.5-rc1 promotion

---

## Parallel Execution Timeline

```
Timeline (6 weeks):

Week 1–2  Sprint 1: Analysis
          ├─ explore: Finding categorization
          └─ gap-verifier: Verification
          
Week 2–4  Sprints 2–3: Quick-Wins (Parallel × 4 batches)
          ├─ themisdb-implementer (×4): QW-P3-1..4
          ├─ task: Test & benchmark (continuous)
          └─ Human checkpoints: After each batch

Week 4–5  Sprint 4: HIGH Remediation
          ├─ themisdb-implementer: HIGH fixes (continuous)
          ├─ themisdb-reviewer: Quality gate (continuous)
          ├─ task: Regression testing
          └─ Human checkpoint: After completion

Week 5–6  Sprint 5: MEDIUM Optimization
          ├─ general-purpose: Design
          ├─ themisdb-implementer: Implementation
          ├─ task: Benchmark
          └─ Human checkpoint: After completion

Week 6    Sprint 6: Stability & Release
          ├─ task: 100× runs + memory profiling
          ├─ code-review: Security/quality
          ├─ task: Final benchmarks + rc prep
          └─ Human checkpoint: Final sign-off
```

---

## Handoff Protocol

### Between Sprints

1. **Output Preparation:** Completing agent summarizes work in JSON/markdown
2. **Human Review:** @makr-code reviews and approves outputs
3. **Input Preparation:** @makr-code prepares inputs for next sprint
4. **Agent Kickoff:** New agent receives inputs + coordination data

### Example Handoff (Sprint 1 → Sprint 2)

```markdown
## Handoff: Sprint 1 (Analysis) → Sprint 2 (Quick-Wins)

### Sprint 1 Outputs (from explore + gap-verifier)
- PHASE_3_FINDING_ANALYSIS.md (categorization + quick-wins)
- 107 findings classified into 6 categories
- 28 quick-win findings identified (< 2 hrs each)

### Human Review & Approval
- ✅ Categorization approved
- ✅ Quick-wins approved
- ✅ Priority ranking accepted

### Sprint 2 Inputs (for themisdb-implementer × 4)
- Category: Memory & RAII (QW-P3-1)
  - Finding M-1.1 @ file/line
  - Finding M-1.2 @ file/line
  - ...
- Shared build/test commands
- Success criteria: All tests PASS, no regressions
```

---

## Success Metrics

### Per-Sprint Metrics

| Sprint | Success Criteria |
|--------|-----------------|
| **1** | 107/107 findings categorized; 25–30 quick-wins identified |
| **2–3** | 100/100 quick-win fixes implemented; 326 tests PASS × 4 batches |
| **4** | 19–25/19–25 HIGH findings resolved; reviews clean |
| **5** | 25–30/25–30 MEDIUM findings resolved; 5–20% improvements measured |
| **6** | 100 test runs clean; memory profiling clean; rc ready |

### Phase 3 Gate

- ✅ 100% of HIGH findings resolved
- ✅ 100% of quick-win MEDIUM findings resolved
- ✅ 326 tests PASS on 100 consecutive runs
- ✅ Zero memory leaks (valgrind + ASAN)
- ✅ Performance >= baseline
- ✅ L1 audit 4/4 PASS (scanner re-run)
- ✅ Release candidate v2.5-rc1 created

---

## Communication & Escalation

### Daily Check-in (Sprint 2–6)
- **Time:** 10:00 UTC
- **Participants:** @makr-code (coordinator), lead agent (if stuck)
- **Purpose:** Blockers, progress update, any adjustments needed

### Weekly Sync (All Sprints)
- **Time:** Monday 14:00 UTC
- **Participants:** @makr-code, all active agents (read-only)
- **Agenda:** Sprint summary, metrics, risk assessment

### Escalation Path
1. **Agent blocked:** Notify @makr-code immediately (task timeout > 2 hrs)
2. **Test regression:** Pause; flag for human investigation
3. **Architecture question:** Escalate to tech lead for decision
4. **Go/no-go decision:** @makr-code makes final call

---

## Resource Allocation

### Recommended Subagent Deployment

| Phase | Agent Type | Count | Duration |
|-------|-----------|-------|----------|
| **Sprint 1** | explore + gap-verifier | 2 | 2 days |
| **Sprint 2–3** | themisdb-implementer + task | 5 (4 impl + 1 test) | 2 weeks |
| **Sprint 4** | themisdb-implementer + themisdb-reviewer + task | 3 | 1.5 weeks |
| **Sprint 5** | general-purpose + themisdb-implementer + task | 3 | 1 week |
| **Sprint 6** | task + code-review | 2 | 1 week |

### Estimated Agent Hours
- **Total:** ~60–80 agent-hours across 6 weeks
- **Peak:** Week 2–5 (12–16 hours/week)
- **Taper:** Week 6 (8–10 hours/week)

---

## Next Steps (Immediate)

1. **✅ Phase 3 planning complete** (DONE — 2026-07-01)
2. **⏳ Assign Phase 3 owner** (@makr-code recommended)
3. **⏳ Schedule Sprint 1 kickoff** (2026-07-08)
4. **⏳ Brief explore agent** on finding analysis task
5. **⏳ Prepare Phase 2.4 audit outputs** as input for Sprint 1

---

**Document Created:** 2026-07-01  
**Next Update:** After Sprint 1 kickoff (2026-07-08)  
**Contact:** @makr-code (Phase 3 Coordinator)
