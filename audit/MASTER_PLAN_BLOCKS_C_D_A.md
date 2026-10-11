---
Author: "Copilot Task Agent"
Created: "2026-10-11"
Last Updated: "2026-10-11"
Status: "COMPLETE - All Blocks Ready"
---

# MASTER PLAN: 100% Test Coverage Implementation (Blocks C + D + A)

## 📊 Project Overview

**Objective:** Increase ThemisDB test coverage from baseline (1,562 binaries) to 100% (~2,050 binaries, 810-950 new tests)

**Scope:** 3 major implementation blocks
- **Block C:** Coverage Infrastructure (gates + CI/CD)
- **Block D:** Governance/Compliance Framework
- **Block A:** Phase 3 Batch 1 Implementation (80-90 tests)

**Total Effort:** 150-210 hours
**Total Duration:** 6-7 weeks
**New Tests:** 137-147 (C+D+A phase)

---

## 🎯 Blocks Summary Table

| Block | Component | Tests | Effort | Duration | Status |
|-------|-----------|-------|--------|----------|--------|
| **C** | Coverage Infrastructure | 0 | 7-10h | 1 week | ✅ Ready |
| **D** | Wave Criteria (7/8/9) | 30 | 45-50h | 1 week | ✅ Ready |
| **D** | Roadmap Alignment | 4 | 10-15h | 2-3d | ✅ Ready |
| **D** | Edition Compliance | 15 | 15-20h | 2-3d | ✅ Ready |
| **D** | Docs Governance | 8 | 10-15h | 2-3d | ✅ Ready |
| **A** | distributed_tensor | 20-25 | 12-15h | 3-4d | ✅ Ready |
| **A** | llama_cpp | 20-25 | 12-15h | 3-4d | ✅ Ready |
| **A** | stable_diffusion | 20-25 | 12-15h | 3-4d | ✅ Ready |
| **A** | execution | 20-25 | 12-15h | 3-4d | ✅ Ready |
| **TOTAL** | — | **137-147** | **150-210h** | **6-7w** | ✅ **READY** |

---

## 📋 Deliverables by Block

### Block C: Coverage Infrastructure
**Files Created:**
1. ✅ `audit/COVERAGE_INFRASTRUCTURE_ANALYSIS.md` (11.5 KB)
   - Current state assessment
   - Gap analysis (missing gcov/lcov setup)
   - C1-C4 implementation plan
   - Platform support (GCC, Clang, MSVC)

2. ✅ `cmake/CoverageConfiguration.cmake` (NEW)
   - `-DTHEMIS_ENABLE_COVERAGE=ON` CMake option
   - Compiler detection + flag configuration
   - Helper functions for coverage tracking

3. ✅ `scripts/generate_coverage_baseline.py` (NEW)
   - Parse lcov output
   - Extract per-module metrics
   - Output: baseline.txt + baseline.json
   - Deterministic tracking

4. ✅ `.github/workflows/coverage-report-nightly.yml` (NEW)
   - Nightly collection (02:00 UTC)
   - Runs focused + release_critical tests
   - Generates lcov + HTML reports
   - Auto-updates baseline on develop

**Integration Point:** Add `include(cmake/CoverageConfiguration.cmake)` to root CMakeLists.txt

**Success Metrics:**
- Coverage baseline established per module
- Nightly workflow collects + reports metrics
- CMake coverage option working
- No workflow linting errors

---

### Block D: Governance/Compliance Framework
**File Created:**
1. ✅ `audit/BLOCK_D_GOVERNANCE_IMPLEMENTATION_GUIDE.md` (20.4 KB)

**Components:**

#### D1: Wave Exit Criteria Tests (30 tests, 45-50h)
- Wave 7: Code review, test density, documentation
- Wave 8: Performance, security, compliance
- Wave 9: SLA, runbooks, support escalation
- Labels: `governance/maturity;governance/wave[7-9]`

#### D2: Roadmap→Test Mapping (4 tests, 10-15h)
- Validates ROADMAP.md items have tests
- Coverage distribution compliance
- Labels: `governance/roadmap`

#### D3: Edition Compliance Tests (15 tests, 15-20h)
- Community, Minimal, Enterprise, Military
- Per-edition feature flag validation
- Labels: `governance/edition`

#### D4: Documentation Governance (8 tests, 10-15h)
- Doxygen completeness (95%+)
- SOT consistency (MODULES_AND_APIS.md)
- Runbook existence (Tier A)
- Labels: `governance/docs`

**CMake Integration:**
```cmake
set_property(GLOBAL APPEND PROPERTY THEMIS_GOVERNANCE_MATURITY_TARGETS ...)
set_property(GLOBAL APPEND PROPERTY THEMIS_GOVERNANCE_ROADMAP_TARGETS ...)
set_property(GLOBAL APPEND PROPERTY THEMIS_EDITION_COMPLIANCE_TARGETS ...)
set_property(GLOBAL APPEND PROPERTY THEMIS_GOVERNANCE_DOCS_TARGETS ...)
```

**Success Metrics:**
- 57 governance tests implemented
- Wave exit criteria automated
- Roadmap→Test mapping validated
- Edition compliance enforced
- Documentation governance gates functional

---

### Block A: Phase 3 Batch 1 Implementation
**File Created:**
1. ✅ `audit/BLOCK_A_PHASE3_BATCH1_IMPLEMENTATION.md` (18.9 KB)

**4 Minimal Modules: 80-90 Tests**

#### distributed_tensor (20-25 tests)
- Function-level (10-12): Core tensor ops
- Module-level (4-5): Tensor lifecycle
- Cross-module (2-3): Server/storage integration
- Happy-path (2-3): Neural network forward pass
- Error (2): Mismatch handling
- Edge (1-2): Large tensors, edge cases

#### llama_cpp (20-25 tests)
- Function: Model loading, tokenization, inference
- Module: Session lifecycle, caching
- Cross-module: RPC, metrics export
- Happy-path: Chat, completion, embeddings
- Error: OOM, invalid model
- Edge: Long context, concurrent

#### stable_diffusion (20-25 tests)
- Function: Encoding, generation, decoding
- Module: Pipeline orchestration
- Cross-module: GPU allocation, storage
- Happy-path: Text2img, img2img, upscaling
- Error: Invalid prompt, CUDA OOM
- Edge: Long prompts, extreme ratios

#### execution (20-25 tests)
- Function: Query parsing, plan generation
- Module: Transaction management
- Cross-module: Storage backend
- Happy-path: OLTP, OLAP, mixed
- Error: Invalid SQL, exhaustion
- Edge: Complex joins, large results

**All Tests:**
- Labeled: `focused` + CTest category (function/module/cross/happy/error/edge)
- 100% pass locally before PR
- CMake registration complete
- Code review: 2-3 approvals per module
- Timeline: 2-3 weeks (60-90 hours)

**Success Metrics:**
- 80-90 tests pass locally
- All labeled `focused`
- CTest distribution correct
- Code review approved
- gate-pr-core.yml integration verified
- Build <30 min, tests <15 min

---

## 🚀 Execution Roadmap

### Phase 1: Week 1 (Block C - Infrastructure)
**Tasks:**
1. Add `include(cmake/CoverageConfiguration.cmake)` to root CMakeLists.txt
2. Test coverage flags: `cmake -DTHEMIS_ENABLE_COVERAGE=ON`
3. Validate generate_coverage_baseline.py with sample lcov
4. Lint coverage-report-nightly.yml (actionlint)
5. Dry-run nightly workflow (workflow_dispatch)

**Deliverables:**
- ✅ CMake coverage option functional
- ✅ Baseline generation working
- ✅ Nightly workflow lint-clean
- ✅ Coverage infrastructure ready

### Phase 2: Week 2-3 (Block D - Governance)
**Tasks:**
1. Implement D1 Wave exit criteria tests (30 tests)
2. Implement D2 Roadmap alignment validator (4 tests)
3. Implement D3 Edition compliance tests (15 tests)
4. Implement D4 Docs governance tests (8 tests)
5. CMake registration + aggregate targets
6. Code review + iteration

**Deliverables:**
- ✅ 57 governance tests merged
- ✅ Wave exit criteria automated
- ✅ Compliance gates functional

### Phase 3: Week 3-5 (Block A - Implementation)
**Tasks (per module):**
1. Create function-level tests (10-12 per module)
2. Create module-level tests (4-5 per module)
3. Create cross-module tests (2-3 per module)
4. Create happy-path tests (2-3 per module)
5. Create error handling tests (2 per module)
6. Create edge-case tests (1-2 per module)
7. CMake registration (THEMIS_FOCUSED_TARGETS)
8. Code review + iteration

**Sequence:**
- Days 1-4: distributed_tensor (20-25 tests)
- Days 5-8: llama_cpp (20-25 tests)
- Days 9-12: stable_diffusion (20-25 tests)
- Days 13-16: execution (20-25 tests)
- Days 17-21: Code review + iteration

**Deliverables:**
- ✅ 80-90 Phase 3 Batch 1 tests merged
- ✅ All labeled `focused`
- ✅ Full CTest taxonomy coverage

---

## 📊 Impact After Blocks C+D+A

### Before (Current)
- 1,562 test binaries
- ~1 test per 2.7K LOC (sparse)
- 4 modules with 1 test each (critical gap)
- No coverage baseline
- No governance gates

### After Week 5 (Blocks C+D+A)
- 1,562 + 137-147 = **1,699-1,709 test binaries**
- Tier C modules: 1 test per 500 LOC
- Coverage baseline established (nightly)
- Governance gates functional (57 tests)
- Phase 3 Batch 1: 80-90 tests in production

### After Full Phase 3+4 (Month 3-4)
- 1,562 + 490 = **~2,050 test binaries**
- All modules: 1 test per 500 LOC
- Coverage baseline + delta tracking
- Release gates automated (22+ workflows)
- 810-950 total tests with explicit labels

---

## 🔗 Documentation Structure

**Master Planning Documents:**
- PHASE1_COVERAGE_BASELINE_2026-10-11.md — Audit baseline
- PHASE2_PRIORITIZATION_2026-10-11.md — CTest taxonomy
- CTEST_QUALITY_STANDARD.md — Quality reference
- ADDITIONAL_TEST_CATEGORIES_RELEASE_GATES.md — Gate alignment
- QUICK_REFERENCE_TEST_CATEGORIES.md — 1-page summary
- PHASE3_BATCH1_IMPLEMENTATION_CHECKLIST.md — Original checklist

**Block-Specific Guides (NEW):**
- COVERAGE_INFRASTRUCTURE_ANALYSIS.md — Block C detailed plan
- BLOCK_D_GOVERNANCE_IMPLEMENTATION_GUIDE.md — Block D detailed guide
- BLOCK_A_PHASE3_BATCH1_IMPLEMENTATION.md — Block A implementation guide

**Code Artifacts (NEW):**
- cmake/CoverageConfiguration.cmake — Coverage CMake module
- scripts/generate_coverage_baseline.py — Baseline generator
- .github/workflows/coverage-report-nightly.yml — Nightly workflow

---

## ✅ Quality Checklist

**Before Starting:**
- [ ] All three block guides reviewed
- [ ] Timeline confirmed with team
- [ ] Resource allocation approved
- [ ] No critical production issues pending

**During Execution:**
- [ ] Daily progress tracking (burndown chart)
- [ ] Code review: 2-3 approvals per PR
- [ ] Build validation: <30 min for full suite
- [ ] Test suite: <15 min for full execution
- [ ] No regressions in existing tests

**After Completion:**
- [ ] All 137-147 new tests passing
- [ ] Coverage baseline stable (±2% variance)
- [ ] Governance gates functional
- [ ] Code review approved
- [ ] No workflow linting errors
- [ ] Artifacts documented

---

## 🎯 Success Criteria (Summary)

**Block C Success:**
✅ CMake coverage option functional
✅ Baseline generation working
✅ Nightly workflow lint-clean
✅ Coverage infrastructure ready

**Block D Success:**
✅ 57 governance tests implemented
✅ Wave exit criteria automated
✅ Compliance gates functional
✅ Tests labeled `governance/*`

**Block A Success:**
✅ 80-90 tests pass locally
✅ All labeled `focused`
✅ CTest distribution correct
✅ Code review approved
✅ Baseline collection working

**Overall Success:**
✅ 137-147 new tests merged
✅ Coverage infrastructure operational
✅ Governance framework automated
✅ Phase 3 Batch 1 complete
✅ No blockers for Batch 2-4

---

## 🚀 Ready to Execute

**All three blocks have:**
✅ Detailed specifications
✅ Implementation templates
✅ CMake integration patterns
✅ Test naming conventions
✅ Success metrics
✅ Timeline estimates
✅ Code review checklists

**Recommended Sequence:**
1. **Week 1:** Block C (infrastructure first)
2. **Week 2-3:** Block D (governance framework)
3. **Week 3-5:** Block A (implementation)

**Alternative:**
- Start Block A immediately (test coding)
- Block C+D infrastructure in parallel
- All three streams converge Week 3

---

## 📞 Next Steps

**For User:**
1. Review all three block guides
2. Approve timeline and resource allocation
3. Designate developer + reviewer for each block
4. Schedule kickoff meeting (Week 1)
5. Setup CI/CD for pull request automation

**For Developer (Block C):**
1. Add CMakeLists.txt include
2. Test coverage flags locally
3. Validate baseline generation
4. Lint nightly workflow
5. Dry-run workflow_dispatch

**For Developer (Block D):**
1. Create tests/governance directory
2. Implement D1-D4 test files
3. CMake registration
4. Code review + iteration

**For Developer (Block A):**
1. Create tests/MODULE_NAME directories
2. Draft test file templates
3. Implement function/module/cross/happy/error/edge tests
4. CMake registration
5. Code review + iteration

---

**STATUS: ✅ READY FOR EXECUTION**
**Estimated Project Completion: 3-4 months to 100% coverage (810-950 tests)**

Generated: 2026-10-11 09:47 UTC

