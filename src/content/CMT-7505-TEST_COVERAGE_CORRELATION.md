# CMT-7505: Test Coverage Correlation Report
## Content Module Batch 1-4 Remediation & Test Coverage Mapping

**Status:** Phase 2 Preparation (In Progress)  
**Scope:** Aggregate all CRITICAL (48) and HIGH (402) findings from Batches 1-4, verify test coverage  
**Date:** 2026-08-15  
**Target:** >= 95% of fixed gaps have corresponding test coverage  

---

## Executive Summary

This document tracks the correlation between gap closure items from Batch 1-4 remediation work (450 total items: 48 CRITICAL + 402 HIGH) and corresponding test cases in `tests/content/`. 

**Key Metrics:**
- Total Batch 1-4 findings: ~450 (CRITICAL 48 + HIGH 402)
- Test files in content module: 20+ test files
- Target coverage: >= 95% gap-to-test correlation
- Tests marked with `release_critical` label for Wave A gate

---

## Batch 1: CRITICAL Findings (48 items)

### Category: braces_imbalance (48 CRITICAL gaps)

| Finding ID | File | Issue | Test File | Status |
|---|---|---|---|---|
| CMT-B1-001 | abuse_detector.cpp | Brace imbalance in filter validation | test_content_pipeline_hardening.cpp | [ ] Verify |
| CMT-B1-002 | archive_processor.cpp | Brace in archive header parsing | test_archive_processor_validation.cpp | [ ] Verify |
| CMT-B1-003 | office_processor.cpp | Brace in format detection logic | test_content_features.cpp | [ ] Verify |
| CMT-B1-004 | geo_processor.cpp | Brace in spatial query validation | test_content_contract_hardening_focused.cpp | [ ] Verify |
| CMT-B1-005 | content_logger.cpp | Brace in diagnostic path | test_content_logger.cpp | [ ] Verify |
| CMT-B1-006–048 | (39 more files) | Various brace imbalance findings | (mapped to tests) | [ ] Batch verify |

**Aggregation Status:** [~] Batch 1 findings consolidated; test mapping in progress  
**Verification:** All test files must pass with `ctest --preset community-release -L content`

---

## Batch 2: HIGH Severity Findings (402 items)

### Category Distribution (HIGH severity, 402 total)

| Severity | Category | Count | Primary Test Files | Status |
|---|---|---|---|---|
| HIGH | scope_mismatch | ~200 | test_content_contract_hardening_focused.cpp | [ ] Verify |
| HIGH | uninitialized_access | ~100 | test_content_con007_con012_remediations.cpp | [ ] Verify |
| HIGH | resource_leak | ~50 | test_content_errors.cpp | [ ] Verify |
| HIGH | logic_error | ~30 | test_content_pipeline_hardening.cpp | [ ] Verify |
| HIGH | Other | ~22 | (distributed across suite) | [ ] Verify |

**Sub-tasks (CMT-7505-01: Aggregate Batch 1-4 findings)**
- [ ] Extract all CRITICAL findings from Batch 1 module gap scan
- [ ] Extract all HIGH findings from Batch 2 module gap scan
- [ ] Classify by processor type (archive, office, geo, etc.)
- [ ] Produce master inventory with GitHub issue references

---

## Batch 3: MEDIUM Severity Findings (2770 items)

**Status:** Batch 3 remediation may exceed test correlation requirement if all items are scope_mismatch type.  
**Strategy:** Focus on HIGH + CRITICAL coverage; MEDIUM items optional for Phase 2.

---

## Batch 4: LOW Severity + Edge Cases

**Status:** Batch 4 items tracked separately; not required for Phase 2 test coverage gate.

---

## Test Coverage Analysis

### Current Test Suite Status

**Total test files:** 20+ in `tests/content/`  
**Test count:** 120+ test cases  
**Labels:**
- `release_critical` — required for GA gate
- `content` — content module filter

**Mapped Test Files (High Confidence):**

1. **test_content_contract_hardening_focused.cpp**
   - Purpose: Verify contract boundaries and fail-closed behavior
   - Coverage: scope_mismatch, validation edge cases
   - Tests: CMT-FIN-36..40 (scope validation, pointer safety, RAII)

2. **test_content_con007_con012_remediations.cpp**
   - Purpose: Address specific CON007/CON012 findings from historical audit
   - Coverage: uninitialized access, resource lifecycle
   - Tests: CMT-FIN-21..35

3. **test_archive_processor_validation.cpp**
   - Purpose: Archive format and edge case validation
   - Coverage: archive processor braces, boundary conditions
   - Tests: CMT-FIN-01..10

4. **test_content_pipeline_hardening.cpp**
   - Purpose: End-to-end pipeline hardening and error paths
   - Coverage: ingestion flow, error handling
   - Tests: CMT-FIN-11..20

5. **test_content_logger.cpp**
   - Purpose: Diagnostic and logging consistency
   - Coverage: diagnostic path correctness
   - Tests: CMT-FIN-41..50

6. **test_content_errors.cpp**
   - Purpose: Error code and exception safety
   - Coverage: resource cleanup, exception paths
   - Tests: CMT-FIN-51..65

**Additional Test Files:**
- test_content_audio_processor.cpp (audio processor tests)
- test_content_embedding_pipeline.cpp (enrichment path tests)
- test_content_features.cpp (feature flag tests)
- test_content_policy_manual.cpp (policy gate tests)
- test_content_processor_chain.cpp (multi-processor coordination)
- test_content_fulltext_index.cpp (search/index consistency)
- (14 more test files in suite)

### Test Coverage Matrix

#### Sub-task (CMT-7505-02: Verify test existence)

| Finding Category | Batch | Count | Test File(s) | Coverage? |
|---|---|---|---|---|
| braces_imbalance | 1 | 48 | test_archive_processor_validation.cpp, test_content_contract_hardening_focused.cpp | [ ] Verify |
| scope_mismatch | 2 | 200 | test_content_contract_hardening_focused.cpp | [ ] Verify |
| uninitialized_access | 2 | 100 | test_content_con007_con012_remediations.cpp, test_content_errors.cpp | [ ] Verify |
| resource_leak | 2 | 50 | test_content_errors.cpp | [ ] Verify |
| logic_error | 2 | 30 | test_content_pipeline_hardening.cpp | [ ] Verify |
| (MEDIUM scope_mismatch) | 3 | 2770 | (distributed, optional for Phase 2) | [ ] Defer |

---

## Phase 2 Checklist (CMT-7505 Foundation)

### CMT-7505-01: Aggregate Batch 1-4 Items
- [ ] Query gap scan results from MODULE_GAPS_BATCH5.md § CMT-7500–7503 implementation status
- [ ] For each finding: extract file, line range, error code, severity
- [ ] Group by processor type (archive, office, geo, mime, text, pdf, etc.)
- [ ] Create master CSV/JSON with GitHub issue references
- [ ] Store in `src/content/CMT-7505-BATCH14_INVENTORY.json`

### CMT-7505-02: Verify Test Coverage
- [ ] For each CRITICAL (48) item: identify mapping to test file
- [ ] For each HIGH (402) item: identify mapping to test file
- [ ] Mark as `[ ] Verify` in test coverage matrix above
- [ ] Create missing test stubs if coverage gap identified
- [ ] Document in `src/content/CMT-7505-TEST_MAPPING.md`

### CMT-7505-03: Validate Test Suite (Phase 3)
- [ ] Run `ctest --preset community-release -L content` locally
- [ ] Collect PASS/FAIL counts per test file
- [ ] Identify failing tests that correlate to gap fixes
- [ ] Generate test output log to `src/content/CMT-7505-TEST_RUN_LOG.txt`

### CMT-7505-04: Coverage Report (Phase 4)
- [ ] Aggregate pass rates per batch (Batch 1: 100%, Batch 2: 95%+, etc.)
- [ ] Generate summary report showing gap-to-test mapping
- [ ] Calculate overall coverage percentage
- [ ] Verify >= 95% target met
- [ ] Publish to `src/content/CMT-7505-COVERAGE_REPORT.md`

---

## Acceptance Criteria (Phase 2)

**CMT-7505-01 complete when:**
- [ ] All 450 Batch 1-4 findings listed with file/line/severity/issue reference
- [ ] Findings grouped by processor type
- [ ] Inventory file stored at `src/content/CMT-7505-BATCH14_INVENTORY.json`

**CMT-7505-02 complete when:**
- [ ] All 48 CRITICAL findings mapped to test file
- [ ] All 402 HIGH findings mapped to test file
- [ ] Coverage matrix updated with `[x]` or `[ ]` status
- [ ] Any identified test gaps documented
- [ ] Mapping file stored at `src/content/CMT-7505-TEST_MAPPING.md`

---

## References

- **MODULE_GAPS_BATCH5.md** — CMT-7500–7506 task definitions
- **Test suite baseline** — `tests/content/CMakeLists.txt` with `release_critical` label
- **Batch 1-4 status** — Parallel remediation work tracked in GitHub issues
- **GA gate reference** — `docs/governance/GA_PROMOTION_SIGN_OFF.md` § Section 3 (Promotion Checklist)

---

**Phase 2 Status:** [ ] In Progress  
**Phase 3 Status:** [ ] Awaiting  
**Phase 4 Status:** [ ] Awaiting  

**Last Updated:** 2026-08-15  
**Owner:** CMT-7505 Implementation  
**Target Completion:** Sept 22, 2026 (v2.4.0 GA)
