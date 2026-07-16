# Scanner Improvement TODO — Gap Scan v4 Enhancement

**Created:** 2026-06-02  
**Owner:** Copilot Scanner Enhancement Task Force  
**Based on:** QW-28/QW-29 False-Positive Analysis  
**Goal:** Reduce false-positive rate from ~25% to <10% while maintaining true-positive sensitivity

---

## Executive Summary

During fail-closed guard implementation (QW-28, QW-29), discovered **systematic false-positive patterns**:

| Pattern | False-Positive Rate | Root Cause | Priority |
|---------|---|---|---|
| Multi-parameter aggregation | ~20% | Methods with N parameters scored as single "validation missing" | P0 |
| Struct/non-trivial-type validation | ~40% | LSN, Config structs lack empty()/default detection | P0 |
| Logging-only parameters | ~20% | 'collection', 'context' tagged as requiring validation | P1 |
| Reference vs. value distinction | ~15% | const T& param treated same as T param (copy semantics differ) | P1 |
| Smart pointer null-checks | ~10% | `.get() == nullptr` vs `!ptr` syntax variations missed | P2 |

**Action:** Implement 5 targeted refinements below.

---

## P0: Multi-Parameter Validation Disaggregation

### Problem
```cpp
// Scanner finds 1 finding: "recordAcknowledgment lacks validation"
// Actual: replica_id needs guard (CRITICAL), lsn is opaque struct (MEDIUM)

void recordAcknowledgment(const std::string& replica_id, const LSN& lsn);
```

Current scoring: `[1 finding] = input_validation missing`  
Correct scoring: `[2 findings] = string param (HIGH), struct param (MEDIUM)`

### Task
- [ ] **T1-P0.1** Refactor scanner to decompose `input_validation` category by parameter **type**
  - Input: `findall(func, param_N)` → find N distinct parameters requiring validation
  - Output: Generate N separate findings, each with parameter-specific severity
  - **Effort:** 4-6 hours (signature analysis + decomposition logic)
  - **File:** `tools/gap_scanner/scanners/input_validation_scanner.py`
  - **Success Criterion:** Multi-parameter methods generate separate findings per param; test on `recordAcknowledgment(replica_id: string, lsn: LSN)` → 2 findings

- [ ] **T1-P0.2** Implement parameter-type classification
  - Classify params as: `builtin_string`, `builtin_numeric`, `struct`, `class`, `pointer`, `smart_ptr`, `container`
  - Assign validation rules per type:
    - `builtin_string` → `empty()` guard (confidence: high)
    - `struct` → requires `T == T{}` or explicit validator (confidence: medium)
    - `numeric` → range/bounds check (confidence: medium)
  - **Effort:** 3-4 hours (type inference + classification rules)
  - **File:** `tools/gap_scanner/lib/type_classifier.py`
  - **Success Criterion:** `getLSN(lsn: LSN)` correctly identifies `LSN` as struct vs string

- [ ] **T1-P0.3** Re-scan high-confidence input_validation findings with new decomposition
  - Target: "input_validation" category from gap_scan_v3_confidence_review.json
  - Re-run on subset (first 100 findings) with new scanner
  - Compare results: expect ~30-40% of aggregated findings to split into per-param breakdown
  - **Effort:** 1-2 hours (test harness + re-scan)
  - **File:** `tools/gap_scanner/tests/test_multi_param_disaggregation.py`
  - **Success Criterion:** 100 "input_validation missing" findings → ~130-140 separated findings (1.3-1.4x expansion, per-parameter granularity)

---

## P0: Struct/Non-Trivial-Type Validation Detection

### Problem
```cpp
// Scanner miss: LSN has no trivial empty() → assumed "no validation needed"
// Actual: LSN should validate segment/offset bounds or default-construct equality check

void recordAcknowledgment(const std::string& replica_id, const LSN& lsn) {
    if (replica_id.empty()) { /* guard found ✓ */ }
    // lsn validation: MISSING (struct type, no empty() method)
}
```

Current rule: `validate if type.has_method("empty")` → fails for LSN  
Correct rule: `validate if type is non-builtin AND passed by const-ref AND parameter is read`

### Task
- [ ] **T2-P0.1** Enumerate custom types (struct/class) that require validation
  - Scan include/sharding/, include/replication/, include/llm/ for struct/class definitions
  - Extract: name, fields, default-constructor, comparison operators
  - **Effort:** 2-3 hours (AST walk + struct catalog build)
  - **File:** `tools/gap_scanner/lib/struct_validator_catalog.py`
  - **Success Criterion:** Catalog contains LSN, Config, ShardInfo, ReplicaInfo; flags each with "fields to validate" and "default state"

- [ ] **T2-P0.2** Implement semantic validation rules for struct parameters
  - Rule: If struct S passed as `const S& param`, check for:
    - Explicit validator call: `if (!param.isValid())`, `if (param == S{})`, `if (param.id.empty())`
    - OR struct has `operator bool()` / `is_valid()` method
    - OR struct fields are validated individually
  - **Effort:** 3-4 hours (pattern matching + field-level validation checks)
  - **File:** `tools/gap_scanner/scanners/struct_param_validation_scanner.py`
  - **Success Criterion:** Correctly identifies LSN struct param as "missing validation" with confidence 0.7+

- [ ] **T2-P0.3** Generate "struct_param_validation" as separate category (distinct from string input_validation)
  - New category: `struct_param_validation` (severity: medium, pattern: "struct_lacks_semantic_guard")
  - Separate from `input_validation` (which is for strings/built-ins)
  - **Effort:** 1-2 hours (JSON schema update + category registration)
  - **File:** `tools/gap_scanner/lib/finding_categories.py`
  - **Success Criterion:** `recordAcknowledgment` generates 1x input_validation (replica_id) + 1x struct_param_validation (lsn)

- [ ] **T2-P0.4** Test on QW-28 sample methods
  - Re-run scanner on `RecordAcknowledgment`, `getShardForKey`, `waitForReplication` methods
  - Verify findings split correctly between string and struct params
  - **Effort:** 1 hour (manual verification)
  - **File:** `tools/gap_scanner/tests/test_struct_validation_detection.py`
  - **Success Criterion:** QW-28 methods show <10% false-positive rate (vs. current 20%)

---

## P1: Logging-Only Parameter Exclusion

### Problem
```cpp
// Scanner flags: "collection param lacks validation"
// Actual: collection is logging-only, not security-critical

std::string getShardForKey(const std::string& collection, const std::string& key) {
    // collection used only in: spdlog::info("collection={}", collection);
    // key is used in: hash_ring_->getNode(key) ← security-critical
}
```

Current: Both params scored equally for validation  
Correct: Exclude logging-only params from input_validation findings

### Task
- [ ] **T3-P1.1** Detect logging-only parameters via data-flow analysis
  - Trace parameter usage: does it reach:
    - Functional code path? (hash_ring→getNode, database→query) → validation needed
    - Logging only? (spdlog, fmt::format, cout) → skip validation
  - **Effort:** 4-5 hours (data-flow graph + usage classification)
  - **File:** `tools/gap_scanner/lib/param_usage_classifier.py`
  - **Success Criterion:** Correctly identifies `collection` as logging-only in 90%+ of cases

- [ ] **T3-P1.2** Implement param classification metadata
  - Classify each param as: `functional`, `logging_only`, `mixed`, `unknown`
  - Store in AST annotation or separate metadata file
  - **Effort:** 2-3 hours (metadata schema + storage)
  - **File:** `tools/gap_scanner/lib/ast_annotations.py`
  - **Success Criterion:** `getShardForKey` params classified: key=functional, collection=logging_only

- [ ] **T3-P1.3** Filter input_validation findings by param classification
  - Rule: Skip finding if param classified as `logging_only` or `mixed` with low functional ratio (<30%)
  - **Effort:** 1-2 hours (filter logic)
  - **File:** `tools/gap_scanner/scanners/input_validation_scanner.py` (filter stage)
  - **Success Criterion:** `collection` param no longer generates input_validation finding

- [ ] **T3-P1.4** Test on voice module (high logging-only param density)
  - Re-scan `src/voice/` module (identified as having high false-positive rate)
  - Expected: ~15-20% reduction in input_validation findings
  - **Effort:** 1 hour (test harness + manual spot-check)
  - **File:** `tools/gap_scanner/tests/test_logging_param_exclusion.py`
  - **Success Criterion:** Voice module false-positive rate drops from 30% to <15%

---

## P1: Reference vs. Value Parameter Distinction

### Problem
```cpp
// Both should trigger validation, but have different semantics:

void method1(const std::string& key) {    // caller responsible for lifetime
    // guard needed: if (key.empty()) { }
}

void method2(std::string key) {            // method owns copy
    // guard needed: if (key.empty()) { }
}

// Current scanner: treats both identically ✓ (good)
// But missing: some analyzers skip guards for "by-value" params
```

Risk: By-value params sometimes incorrectly skip validation (copy overhead excuse)

### Task
- [ ] **T4-P1.1** Enforce validation for both reference and value parameters
  - Rule: If param T (ref or value) is used in functional code, validate
  - Do NOT skip validation for by-value params
  - **Effort:** 2-3 hours (rule review + test cases)
  - **File:** `tools/gap_scanner/scanners/input_validation_scanner.py` (rule clause)
  - **Success Criterion:** By-value string params generate findings equally to const-ref

- [ ] **T4-P1.2** Add test cases for both parameter passing styles
  - Create test methods: `foo(const T& t)`, `bar(T t)` (both need guards)
  - Verify both generate identical findings
  - **Effort:** 1-2 hours (test suite expansion)
  - **File:** `tools/gap_scanner/tests/test_param_passing_styles.py`
  - **Success Criterion:** Both const-ref and by-value params trigger findings equally

---

## P2: Smart Pointer Null-Check Detection

### Problem
```cpp
// Scanner may miss these variations:

if (!ptr) { }                          // typical, often detected
if (ptr == nullptr) { }                // sometimes missed
if (ptr.get() == nullptr) { }          // often missed
if (!ptr.get()) { }                    // sometimes missed
auto raw = ptr.get(); if (!raw) { }    // usually missed (cross-statement)
```

### Task
- [ ] **T5-P2.1** Expand null-check pattern matching for smart pointers
  - Extend regex/AST rules to cover: `.get()`, `== nullptr`, `!ptr.get()`
  - Test on examples above
  - **Effort:** 2-3 hours (pattern expansion + test)
  - **File:** `tools/gap_scanner/lib/null_check_patterns.py`
  - **Success Criterion:** Detects all 5 variants above with confidence >0.8

- [ ] **T5-P2.2** Add cross-statement null-check tracking
  - If `auto raw = ptr.get()` followed by `if (!raw)` on next statement, link them
  - **Effort:** 3-4 hours (data-flow linking)
  - **File:** `tools/gap_scanner/lib/dataflow_linker.py`
  - **Success Criterion:** Detects multi-statement null patterns (cross-line)

- [ ] **T5-P2.3** Test on ReplicationCoordinator, URNResolver modules
  - Check for smart pointer usage patterns
  - Verify null-checks are detected
  - **Effort:** 1 hour (spot-check)
  - **File:** `tools/gap_scanner/tests/test_smart_ptr_null_checks.py`
  - **Success Criterion:** <5% false-positive rate on sharding module null-checks

---

## Global Integration & Testing

### P0 Integration
- [ ] **T6-GLOBAL.1** Update scanner pipeline to use disaggregated input_validation + struct_param_validation categories
  - Modify: `tools/gap_scanner/gap_scan_orchestrator.py` or equivalent entry point
  - Ensure T1, T2 refinements are chained before confidence scoring
  - **Effort:** 2-3 hours (pipeline integration)
  - **Success Criterion:** New scanner config passes integration tests

- [ ] **T6-GLOBAL.2** Re-baseline gap_scan_v4 metrics
  - Run full scanner on codebase with new rules
  - Collect: total gaps, by-category breakdown, confidence distribution
  - Compare to v3: expect ~15-20% overall reduction (false-positives removed), maintain true-positive count
  - **Effort:** 3-4 hours (full scan + analysis)
  - **File:** `ai_working/gap_scan_v4_summary.json` (new output)
  - **Success Criterion:** False-positive-adjusted true findings increase or hold steady; total gaps decrease

- [ ] **T6-GLOBAL.3** Validate on QW-28, QW-29 implementation (ground truth)
  - Known true positives: replica_id (QW-28), key (QW-29)
  - Known false positives: lsn struct param, collection logging param
  - Re-scan these methods with v4; verify true positives flagged, false positives reduced
  - **Effort:** 1-2 hours (manual validation)
  - **File:** `tools/gap_scanner/tests/test_qw28_qw29_ground_truth.py`
  - **Success Criterion:** 100% true-positive detection, <5% false-positives

- [ ] **T6-GLOBAL.4** Document scanner improvements & update scanner README
  - Write: "Scanner v4 Improvements" section
  - Include: examples, category definitions, confidence scoring rules
  - **Effort:** 1-2 hours (documentation)
  - **File:** `tools/gap_scanner/README.md` (update)
  - **Success Criterion:** README clearly explains new categories and patterns

---

## Success Metrics

| Metric | Target | Baseline (v3) | Expected (v4) |
|--------|--------|---|---|
| False-positive rate (input_validation) | <10% | 20-25% | <10% |
| Multi-param findings (per-parameter) | >90% disaggregation | 0% (aggregated) | >90% |
| Struct param detection | >85% recall | ~40% | >85% |
| Logging-only exclusion | >80% precision | N/A | >80% |
| Smart-ptr null-checks | >90% recall | ~70% | >90% |
| **Overall gap count** | -15% false-positives | 32,327 | ~27,500 (less noise) |
| **True-positive count** | Maintain or increase | 14,912 actionable | ≥14,912 |

---

## Handoff Checklist for Copilot Scanner Enhancement Team

- [ ] Review this TODO with focus on **P0 items first** (multi-param, struct validation)
- [ ] Clone/branch: `tools/gap_scanner/` with task-specific branch names (e.g., `scanner/multi-param-disagg`, `scanner/struct-validation`)
- [ ] Implement T1, T2 in parallel (no dependencies); T3-T5 after T1 baseline
- [ ] Test each refinement with unit tests + integration on QW-28/QW-29 methods
- [ ] Weekly sync on progress; P0 target: **1 week**, full v4: **2 weeks**
- [ ] Final validation: Re-run full gap scan, compare metrics, document results
- [ ] Update ROADMAP.md once v4 baseline is stable

---

## Contact & Questions

For clarifications on false-positive patterns or technical implementation:
- Reference: `ai_working/gap_scan_v3_confidence_review.json` (top 2000 findings)
- Validation ground truth: QW-28/QW-29 implementations in `src/sharding/replication_coordinator.cpp` and `src/sharding/urn_resolver.cpp`
- Existing scanner code: `tools/gap_scanner/scanners/` directory

**Last Updated:** 2026-06-02 | **Status:** Ready for handoff
