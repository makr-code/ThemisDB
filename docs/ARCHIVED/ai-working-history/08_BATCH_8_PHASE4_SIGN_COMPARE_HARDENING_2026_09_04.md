# Batch 8 Phase 4: Sign-Compare Hardening Execution Plan
**Date:** 2026-09-04 07:31 UTC  
**Scope:** Fix -Wsign-compare warnings (target: 19,849 instances across 1,698 files)  
**Target:** Q3 2026 hardening completion  
**Status:** EXECUTION IN PROGRESS

---

## Executive Summary

Signed/unsigned integer comparison warnings (-Wsign-compare) are a common source of latent type-mismatch bugs in C/C++. This batch systematically resolves all instances following semantic-correctness-first principles:

| Category | Pattern | Count | Approach |
|----------|---------|-------|----------|
| **A (Primary)** | Loop counter vs `.size()` | ~8,000 (40%) | Convert loop counter to `size_t` |
| **B (Secondary)** | Unsigned literal suffixes | ~3,000 (15%) | Remove `U`/`UL`/`ULL` suffix |
| **C (Tertiary)** | Explicit static_cast | ~6,000 (30%) | Add narrowing cast where needed |
| **D (Edge cases)** | Mixed-type arithmetic | ~2,849 (15%) | Range checks + conversions |

---

## Phase 4 Execution Roadmap

### Stage 1: Pre-Execution Validation (2-3 hours)
- [x] Document target pattern types and counts
- [ ] Quick-build and collect actual warning sample (50-100 instances)
- [ ] Create automated pattern-matching scripts for Categories A–D
- [ ] Validate script output on sample set

### Stage 2: Automated Fixes (4-6 hours)
- [ ] **Commit A:** Loop counter conversions (~8,000 warnings)
- [ ] **Commit B:** Literal suffix removal (~3,000 warnings)
- [ ] **Commit C:** Static cast additions (~6,000 warnings)
- [ ] **Commit D:** Mixed-type handling (~2,849 warnings)

### Stage 3: Verification (1.5–2 hours)
- [ ] Build after each commit
- [ ] Verify no new errors introduced
- [ ] Spot-check semantic correctness of top 50 changes per commit
- [ ] Document any manual overrides or exceptions

### Stage 4: Final Validation (0.5–1 hour)
- [ ] Full build without -Wsign-compare warnings (quiet build)
- [ ] Compare warning count before/after
- [ ] Verify no behavior changes (logic-only fixes)

---

## Expected Deliverables

1. **4 Commits:**
   - `fix: loop counters to size_t (Category A, ~8k warnings)`
   - `fix: remove unsigned literal suffixes (Category B, ~3k warnings)`
   - `fix: add static_cast for mixed-type comparisons (Category C, ~6k warnings)`
   - `fix: range-check and convert mixed arithmetic (Category D, ~2.8k warnings)`

2. **Manifest:**
   - List of 1,698 modified files with change summary
   - Before/after warning counts per file

3. **Build Evidence:**
   - Compilation log (clean build)
   - Warning reduction summary

4. **Semantic Validation Report:**
   - Sampled correctness checks
   - Any identified exceptions or false-positive patterns

---

## Execution Notes

- **Safe Patterns:** Loop counters to `size_t` are lowest-risk (semantically correct 99%+ of cases)
- **Risky Patterns:** Mixed-type arithmetic requires manual review to avoid unintended narrowing
- **No Behavior Changes:** All fixes preserve original comparison logic and range semantics
- **Rollback Plan:** Each commit is independent and can be reverted if issues found

---

## Status Tracking

| Stage | Target Time | Actual Time | Status |
|-------|-------------|-------------|--------|
| Pre-Execution Validation | 2-3 hrs | | STARTING |
| Automated Fixes (A+B+C+D) | 4-6 hrs | | QUEUED |
| Verification | 1.5–2 hrs | | QUEUED |
| Final Validation | 0.5–1 hr | | QUEUED |

---

## Next Steps

1. Create warning extraction scripts
2. Run quick validation build to quantify actual warnings
3. Implement Commit A (loop counter conversions)
4. Proceed stage-by-stage with verification after each commit
