# LLM Module Gaps - Implementation Checklist

**Document:** Detailed step-by-step checklist for implementing gap fixes  
**Status:** TEMPLATE (to be populated after gap-verifier completes)  
**Created:** 2026-08-15  

---

## Phase 1: Gap Verification Status

**Agent:** gap-verifier-llm  
**Status:** In Progress (tool calls: 18+)  
**Expected Duration:** 10-30 minutes for 12,474 findings  

**Task:** Verify raw gaps, classify, re-assess severity, eliminate false-positives  
**Expected Output:** 
- `ai_working/gap_scanner_verified_llm.json` (structured findings)
- `ai_working/gap_verifier_report_llm.md` (human-readable analysis)

---

## Phase 2: Post-Verification Action Items

Once gap-verifier completes, execute in order:

### Step 1: Review Verified Findings
- [ ] Read `ai_working/gap_verifier_report_llm.md` (summary + key insights)
- [ ] Parse `ai_working/gap_scanner_verified_llm.json` (structured data)
- [ ] Identify severity changes (downgrades, upgrades)
- [ ] Note false-positives removed (audit scanner assumptions)

### Step 2: Cluster Gaps by Impact Category
- [ ] Extract CRITICAL verified gaps (target: ~50–100)
- [ ] Extract HIGH verified gaps (target: ~500–800)
- [ ] Group by pattern: thread-safety | resource-mgmt | memory-safety | performance | error-handling
- [ ] Map to top affected files (rank by gap density)

### Step 3: Create Implementation Batches
- [ ] **Batch 1:** CRITICAL + high-risk HIGH gaps (focus files: model_loader.cpp, async_inference_engine.cpp)
- [ ] **Batch 2:** Remaining HIGH gaps + medium-risk patterns
- [ ] **Batch 3:** MEDIUM + cleanup (lower priority, Phase N+1 candidates)

### Step 4: Implementation Planning per Batch
For each batch:
- [ ] Identify affected files (5–10 files per batch)
- [ ] Review existing tests for each file
- [ ] Draft fix approach (use implementation patterns from LLM_GAPS_IMPLEMENTATION_PLAN.md)
- [ ] Estimate line count + complexity

---

## Phase 3: Batch 1 Implementation (CRITICAL + High-Risk)

### Execution Sequence

**File 1: model_loader.cpp** (expected ~80–120 gaps)
```
1. Review gap_verifier findings for this file
2. Identify top patterns: [list after verification]
3. Apply fixes in order:
   - [ ] Resource leak fixes (db_connection_leak, resource_leaked_in_exception)
   - [ ] Null dereference guards
   - [ ] Move semantics for performance
4. Validate against existing tests
5. Add regression tests for fixed patterns
6. Commit: "fix(llm/model_loader): close resource mgmt gaps + null checks"
```

**File 2: async_inference_engine.cpp** (expected ~80–120 gaps)
```
1. Review gap_verifier findings for this file
2. Identify top patterns: [list after verification]
3. Apply fixes in order:
   - [ ] Thread-safety (mutex guards, atomic operations)
   - [ ] Circular lock ordering (consistent lock order)
   - [ ] Exception safety (RAII wrappers)
4. Validate against existing tests (esp. concurrency tests)
5. Add regression tests for fixed patterns
6. Commit: "fix(llm/async_inference_engine): close thread-safety gaps"
```

**File 3-5: [Other high-gap files]**
```
Repeat pattern for each file until all CRITICAL gaps closed
```

### Acceptance Criteria per File
- [ ] All CRITICAL gaps for the file addressed
- [ ] Existing tests still PASS
- [ ] New regression tests added for fixed patterns
- [ ] Code review completed (style, comments, safety)
- [ ] No new compiler/sanitizer warnings

### Build & Test After Each File Fix
```bash
# Configure and build
cmake --preset windows-release -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build --preset windows-release --parallel 16

# Test module-specific
ctest --preset windows-release --output-on-failure -j 1 -k "llm" --timeout 120

# Test release-critical gate
ctest --preset windows-release --output-on-failure -j 1 -k "release_critical" --timeout 300
```

---

## Phase 4: Batch 2 Implementation (HIGH)

**Similar execution to Batch 1, but for HIGH-severity gaps**

- [ ] Work through top HIGH-gap files in priority order
- [ ] Apply fixes per category (copy_overhead, pointer_arithmetic, uninitialized_access, etc.)
- [ ] Build + test after each file
- [ ] Add regression tests
- [ ] Commit with detailed message

**Example commits:**
```
fix(llm/multi_lora_manager): eliminate copy_overhead via move semantics + const-refs
fix(llm/streaming_handler): add bounds checks for pointer arithmetic
fix(llm/wiki_index_store): close db connection leak + resource mgmt gaps
```

---

## Phase 5: Batch 3 + Validation

**MEDIUM gaps, code cleanup, documentation sync**

- [ ] Fix MEDIUM gaps (likely lower complexity)
- [ ] Add comments for legacy/placeholder patterns
- [ ] Update MODULE_GAPS.md with closure evidence
- [ ] Synchronize ROADMAP.md + ARCHITECTURE.md
- [ ] Final full test run

### Final Verification
```bash
# Full module test
ctest --preset windows-release --output-on-failure -k "llm" --timeout 300

# Release-critical gate
ctest --preset windows-release --output-on-failure -k "release_critical" --timeout 600

# Benchmark validation (optional, if regression detected)
./benchmarks/runner --module llm --baseline develop
```

---

## Commit Message Template

```
fix(llm/<file>): close <gap-category> gaps

Fixes from gap-verifier verification (Phase 1, 2026-08-15):
- Category: [thread-safety | resource-mgmt | memory-safety | performance | error-handling]
- Files affected: <list>
- Patterns fixed:
  - <pattern 1>
  - <pattern 2>
  ...

Acceptance criteria:
- [x] All CRITICAL gaps for <file> addressed
- [x] Existing tests still PASS
- [x] New regression tests added (test_*.cpp)
- [x] Code reviewed + documented

Tests run:
- module_llm_test_* PASS (N tests, N/A)
- release_critical PASS
- No sanitizer alerts (ASan/UBSan/TSan)
```

---

## Risk Mitigation

### If a Fix Breaks Existing Tests
1. [ ] Revert the specific fix
2. [ ] Review the test failure reason
3. [ ] Check if the fix introduced a real issue or if the test needs updating
4. [ ] Either:
   - Adjust the fix to preserve existing behavior
   - Update the test to reflect new correct behavior
5. [ ] Re-test and re-commit

### If a Fix Causes Performance Regression
1. [ ] Check if the fix is necessary for correctness (if yes, keep it)
2. [ ] Optimize the fixed code (move semantics, reduce allocations)
3. [ ] Document the regression in commit message
4. [ ] Consider Phase N+1 optimization pass

### If Gap-Verifier Produces Unexpected Results
1. [ ] Review `gap_verifier_report_llm.md` for classification rationale
2. [ ] Cross-check a sample of 10-20 findings against source code
3. [ ] If systematic issue found, document and adjust verification strategy
4. [ ] Proceed with fixes for verified findings

---

## Sign-Off Checklist

Before creating PR and requesting review:

- [ ] All Batch 1 (CRITICAL) fixes complete and tested
- [ ] All Batch 2 (HIGH) fixes complete and tested  
- [ ] Batch 3 (MEDIUM) fixes complete OR deferred to Phase N+1
- [ ] MODULE_GAPS.md updated with closure evidence
- [ ] ROADMAP.md updated to reflect completion
- [ ] ARCHITECTURE.md reviewed and synchronized
- [ ] All module tests PASS (ctest ... -k "llm" ... PASSED)
- [ ] Release-critical gate PASS
- [ ] No sanitizer alerts on full module
- [ ] Benchmark regressions <5% or documented with justification
- [ ] Code review completed
- [ ] PR description includes gap-verifier phase summary + fix summary

---

## Timeline (Post-Verification)

| Phase | Task | Duration | Status |
|-------|------|----------|--------|
| 2 | Impact analysis & batch planning | 2–4 hours | *pending* |
| 3 | Batch 1 implementation + test | 8–16 hours | *pending* |
| 4 | Batch 2 implementation + test | 8–16 hours | *pending* |
| 5 | Batch 3 + validation + sign-off | 4–8 hours | *pending* |
| **TOTAL** | | **22–44 hours** | |

**Target completion:** 2026-08-22 (7 days from start)

---

## References

- Verified gaps JSON: (pending) `ai_working/gap_scanner_verified_llm.json`
- Verified gaps report: (pending) `ai_working/gap_verifier_report_llm.md`
- Implementation patterns: `ai_working/LLM_GAPS_IMPLEMENTATION_PLAN.md`
- Module roadmap: `src/llm/ROADMAP.md`
- Module architecture: `src/llm/ARCHITECTURE.md`

---

**Document Status:** TEMPLATE (awaiting gap-verifier completion)  
**Next Update:** When gap-verifier agent completes

