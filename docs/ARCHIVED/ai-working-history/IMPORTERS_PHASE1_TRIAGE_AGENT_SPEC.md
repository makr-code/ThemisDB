# Phase 1 Triage Agent Specification

**Agent Type:** `gap-verifier` (read-only semantic analysis)  
**Target Artifact:** `IMPORTERS_PHASE1_GAP_TRIAGE.md`  
**Deadline:** 2026-08-22 (End of Week 1)

---

## Objective

Analyze all 282 gaps from `/home/runner/work/ThemisDB/ThemisDB/src/importers/MODULE_GAPS.md` to:
1. Eliminate false positives with high confidence (>90%)
2. Classify true positives by severity (CRITICAL, HIGH, MEDIUM, LOW)
3. Categorize by fix complexity (Tier-1: <100 LOC, Tier-2: 100-300 LOC, Tier-3: >300 LOC)
4. Identify dependencies and blockers
5. Prepare prioritized fix list for Phase 2-5 execution

---

## Input Data

**Source:** `/home/runner/work/ThemisDB/ThemisDB/src/importers/MODULE_GAPS.md`

Current snapshot:
- Total findings: 282
- Critical: 44
- High: 151
- Medium: 82
- Low: 5
- Affected files: 27

---

## Analysis Tasks

### Task 1: False Positive Elimination

For each gap category, assess likelihood of true positive:

| Category | Confidence Threshold | Assessment Approach |
|----------|----------------------|-------------------|
| null_dereference | >90% | Verify handle lifecycle and ownership model |
| data_race | >80% | Check for actual concurrent access patterns |
| blocking_no_timeout | >95% | Inspect mutex/condition_variable usage |
| uninitialized_access | >85% | Validate container bounds and initialization |
| resource_leaked_in_exception | >90% | Check RAII wrapping in exception paths |
| string_concat_loop | >95% | Verify O(n²) pattern in actual code |
| map_vs_unordered_map | >75% | Assess hash distribution impact |
| nested_loop_find | >85% | Check actual O(n²) occurrence in loops |
| hardcoded_path | >90% | Verify path is not parameterized |
| smart_ptr_misuse | >95% | Confirm raw new/delete not wrapped |

**Output Format:**
- For each gap: `CATEGORY | LINE | SEVERITY | TRUE/FALSE/DEFERRED | CONFIDENCE | NOTES`

### Task 2: Severity Reassessment

For TRUE_POSITIVE gaps, validate scanner severity or reassess:

- **CRITICAL** (runtime safety): null_dereference, data_race, blocking_no_timeout, resource_leak, smart_ptr_misuse, smart_ptr_misuse
  - Affects: immediate crash, deadlock, memory corruption, or exception-unsafe behavior
  - Impact: production reliability
  
- **HIGH** (correctness/performance): uninitialized_access, nested_loop_find, pointer_arithmetic_unbounded, string_concat_loop, hardcoded_path
  - Affects: correctness under load, performance regression, maintainability
  - Impact: correctness or p99 latency
  
- **MEDIUM** (refactoring/optimization): map_vs_unordered_map, repeated_search, copy_overhead, iterator_invalidation, hardcoded_output
  - Affects: performance or maintainability without immediate correctness impact
  - Impact: benchmark gates, developer experience
  
- **LOW** (documentation/style): module_doc_linkset_drift, stale_doc_section_reference, unstructured_log, explicit_delete, delete_no_nullptr
  - Affects: documentation accuracy or code style
  - Impact: operability and maintainability

**Output Format:** 
- For each gap: `CATEGORY | LINE | SCANNER_SEVERITY | REASSESSED_SEVERITY | JUSTIFICATION`

### Task 3: Complexity Matrix

For each TRUE_POSITIVE gap, classify fix complexity:

**Tier-1 (Simple):** Single file, <100 LOC change, no cross-module impact
- Examples: single null check, add timeout to isolated mutex_lock, replace raw new with make_unique

**Tier-2 (Moderate):** 2-8 files, 100-300 LOC, moderate cross-module coordination
- Examples: standardize error handling across 3 connectors, add mutex to shared state

**Tier-3 (Complex):** 8+ files or >300 LOC, high cross-module impact, requires design review
- Examples: refactor data structure across all connectors, introduce global registry for timeout semantics

**Output Format:**
- For each gap: `CATEGORY | LINE | TIER | ESTIMATED_LOC | AFFECTED_FILES | BLOCKING_DEPS`

### Task 4: Dependency & Blocker Analysis

Identify:
- **Shared state conflicts:** Gaps affecting the same global/shared variable
- **Sequential dependencies:** Gap X must be fixed before Gap Y
- **Cross-module boundaries:** Gaps that require coordination with other modules (Failover, Updates, etc.)
- **Testing blockers:** Mock/stub connector requirements for validation
- **Build/environment blockers:** RocksDB availability, feature gates, CI/CD constraints

**Output Format:**
- Dependency graph: `GAP_X -> [GAP_Y, GAP_Z] (reason: shared state / sequential / cross-module)`
- Blockers list: `BLOCKER_NAME | IMPACT | MITIGATION | STATUS`

### Task 5: Prioritization and Batching Proposal

Recommend batching for Phase 2-5 execution based on:
- **Parallelizability:** Can fixes run in parallel or must they be sequential?
- **Risk:** How many other fixes depend on this one?
- **Throughput:** Tier-1 (fast) vs Tier-2 (moderate) vs Tier-3 (slow)

**Output Format:**
- Batch proposal: `BATCH_NAME | GAP_COUNT | FILES | ESTIMATED_DURATION | DEPENDENCIES | AGENT`

---

## Acceptance Criteria

✅ **Phase 1 Complete When:**

1. **Coverage:** All 282 gaps classified into TRUE_POSITIVE / FALSE_POSITIVE / DEFERRED
   - Acceptance: ≤5 gaps marked as UNCLEAR (>95% confidence threshold met)

2. **Severity:** All gaps have reassessed severity with justification
   - Acceptance: Severity distribution aligns with scanner output ±10% (allowing for reassessment)

3. **Complexity:** All true positives have Tier classification
   - Acceptance: Tier distribution documented with file/LOC estimates

4. **Dependencies:** Dependency graph and blockers documented
   - Acceptance: No missing inter-gap dependencies, blockers mapped to mitigations

5. **Batching:** Phase 2-5 batches proposed with duration estimates
   - Acceptance: All batches span ≤3 weeks, parallelizable work identified

6. **Quality:** Artifact is ready for Phase 2 dispatch without further clarification
   - Acceptance: All gaps have unambiguous fix strategy or explicit deferral reason

---

## Output Format

**Artifact Name:** `IMPORTERS_PHASE1_GAP_TRIAGE.md`

**Structure:**
```markdown
# Phase 1 Triage Report – Importers Module

**Date:** 2026-08-15  
**Agent:** gap-verifier  
**Duration:** W1 (Aug 15-22)  

## Executive Summary
- Total gaps analyzed: 282
- True Positives: N
- False Positives: M
- Deferred: K
- Confidence threshold met: >95%

## Gap Classification Summary
| Severity | Count | TRUE | FALSE | DEFERRED | Avg Confidence |
|----------|-------|------|-------|----------|----------------|

## Tier Distribution
| Tier | Count | Files | Est. LOC | Est. Duration |
|------|-------|-------|---------|---------------|

## Dependency Graph
- [Textual representation of dependencies]

## Blockers & Mitigations
- [Blocker list]

## Phase 2-5 Batch Proposals
- Batch P2-A: ...
- Batch P2-B: ...
- Batch P3-A1: ...
- etc.

## Detailed Gap Analysis
[Per-gap rows with confidence, severity, complexity, dependencies]

## Recommendations for Phase 2-6
- High-priority items
- Risk mitigations
- Success metrics
```

---

## Success Indicators

✅ **Triage is successful when:**
- All gaps have >90% confidence classification (TRUE/FALSE/DEFERRED)
- No re-triage work needed in Phase 2 (agents can proceed directly to fixes)
- Batch proposals are detailed enough for Agent dispatch without clarification
- Estimated throughput is realistic (<2 weeks per batch)
- All CRITICAL gaps are identified and prioritized for Phase 2 Week 1

---

## Notes for Agent

1. **Focus on semantics, not just syntax:** Verify patterns are real bugs, not false positives
2. **Confidence matters:** Mark uncertain findings as DEFERRED rather than guessing
3. **Blockers are critical:** Missing a blocker could derail Phase 2-5 work
4. **Batch sizing:** Each batch should be ~3-5 days of work for an implementer agent
5. **Parallelizability:** Identify which batches can run in parallel without conflicts
