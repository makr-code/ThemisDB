# Batch A-0: TODO/FIXME Cleanup Execution Plan

**Date:** 2026-08-15T11:02:13Z  
**Status:** 🟡 INITIATED  
**Phase:** 2 (Revised Scope)  
**Priority:** PRIMARY  
**Effort:** 3-5 days  

---

## 📋 Overview

**Scope Change:** Phase 1 verification identified 27 false positives in original Batches A-1..4. **NEW priority: Batch A-0 TODO/FIXME cleanup** (79 items).

**Task:** Remove all TODO/FIXME markers from index module production code.

**Success Criteria:**
- ✅ All 79 TODO/FIXME items identified and categorized
- ✅ Remediation strategy documented for each
- ✅ All TODOs removed from production code paths
- ✅ No functionality changes (cleanup only)
- ✅ All focused tests PASS
- ✅ Code review approval

---

## 🎯 Batch A-0 Scope

### Target: 79 TODO/FIXME Items Across Index Module

**Affected Files:** ~41 files in src/index/ directory (every .cpp file has pattern-based TODOs)

**Types of TODOs Identified:**
1. **Pattern-based TODOs** — Metadata comments in file headers
   - `@note Gap Summary: total=N; TODO=1, ...`
   - Found in ~41 files
   - Type: Auto-generated metadata (safe to clean)

2. **Code Comment TODOs** — Actual code-level TODO/FIXME markers
   - Type: Implementation guidance comments
   - Need to identify and remove

3. **Legacy Compatibility Notes** — LEGACY_COMPAT markers
   - May contain TODO/FIXME implications
   - Need to audit

---

## 🔍 Phase 1: TODO/FIXME Inventory

### Step 1: Complete Inventory Scan

**Procedure:**
```bash
# Find all TODO/FIXME markers (excluding metadata)
find src/index -name "*.cpp" -o -name "*.h" | while read f; do
  grep -Hn "TODO\|FIXME" "$f" | grep -v "@note" | grep -v "Gap Summary"
done > /tmp/todo_inventory.txt
```

**Expected Output:** ~79 TODO/FIXME lines total

### Step 2: Categorize by Type

**Categories to Use:**
1. **Type A: Code Completion** — `// TODO: implement feature X`
2. **Type B: Optimization** — `// TODO: optimize this algorithm`
3. **Type C: Error Handling** — `// TODO: handle error case Y`
4. **Type D: Testing** — `// TODO: add test for Z`
5. **Type E: Documentation** — `// TODO: document API`
6. **Type F: Cleanup** — `// FIXME: remove workaround`
7. **Type G: Refactoring** — `// TODO: refactor for readability`

### Step 3: Create Categorization Report

**Output Format:**
```markdown
## TODO/FIXME Inventory (79 items)

### Type A: Code Completion (N items)
- file.cpp:LINE — description

### Type B: Optimization (N items)
- file.cpp:LINE — description

... (other types)
```

---

## 🔧 Phase 2: Remediation Strategy

### For Each TODO/FIXME Item

**Decision Tree:**

```
Is this TODO still relevant?
├─ YES: Implementation still needed
│  └─ Can it be completed in Phase 2?
│     ├─ YES: Implement and remove TODO
│     └─ NO: Document and defer to Phase 3
├─ NO: Obsolete, remove marker
└─ UNCLEAR: Review with team, then decide
```

### Remediation Options

#### Option 1: Remove (Obsolete)
- For outdated or no-longer-relevant TODOs
- Action: Delete the TODO comment entirely
- Impact: Clean code, no functional change

#### Option 2: Implement (If Simple)
- For simple, scoped improvements
- Action: Implement the suggested change + remove TODO
- Impact: Better code, potential functional improvement
- Timeline: 1-2 days per item (if many are simple)

#### Option 3: Document & Defer (If Complex)
- For large features or complex refactoring
- Action: Move to Phase 3 backlog with clear issue/ticket
- Impact: Tracked for future work, cleanup comment
- Timeline: Phase 3+

#### Option 4: Clarify Comment (If Unclear)
- For ambiguous TODOs
- Action: Rewrite as clear inline comment or remove
- Impact: Better code maintainability
- Timeline: <1 hour per item

---

## 📊 Phase 3: Implementation

### Breakdown by Effort

**Low Effort (< 30 min per item):**
- Remove obsolete TODOs
- Clarify unclear comments
- Update outdated references
- **Estimate:** ~50-60 items (3-5 hours total)

**Medium Effort (30 min - 2 hours per item):**
- Implement simple improvements
- Add missing error handling
- Complete partial implementations
- **Estimate:** ~15-20 items (1-2 days total)

**High Effort (> 2 hours per item):**
- Major refactoring
- Complex optimizations
- Defer to Phase 3
- **Estimate:** ~5-10 items (defer)

### Timeline Estimation

```
Phase 1 (Inventory & Categorization):    1-2 hours
Phase 2 (Strategy & Decision Making):    2-3 hours
Phase 3 (Implementation):                 2-3 days
                                         ──────────
Total Effort:                            3-5 days
```

---

## ✅ Quality Gate: Code Review

### Review Checklist

For each TODO/FIXME removal:
- [ ] TODO marker removed from code
- [ ] Rationale documented (why removed)
- [ ] No unintended code changes
- [ ] Related tests still pass
- [ ] Comments updated for clarity
- [ ] No new technical debt introduced

---

## 🚀 Execution Steps

### Week 1 (2026-08-15..16): Planning & Inventory

- [ ] **Day 1 (Today):**
  - [ ] Complete inventory scan (all 79 items)
  - [ ] Categorize by type (A-G)
  - [ ] Create categorization report
  
- [ ] **Day 2 (Tomorrow):**
  - [ ] Review categorization with team
  - [ ] Create decision matrix for each item
  - [ ] Identify high/medium/low effort items
  - [ ] Create detailed remediation plan

### Week 1 (2026-08-17..19): Implementation

- [ ] **Day 3:**
  - [ ] Remove/clarify low-effort items (30-40 items)
  - [ ] Create Phase 3 backlog for deferred items
  
- [ ] **Day 4:**
  - [ ] Implement/complete medium-effort items (15-20 items)
  - [ ] Run focused tests after each batch
  
- [ ] **Day 5:**
  - [ ] Code review all changes
  - [ ] Final validation testing
  - [ ] Update ROADMAP.md

### Completion (2026-08-20)

- [ ] **Day 5-6:**
  - [ ] All 79 TODOs addressed
  - [ ] No TODOs in production paths
  - [ ] All tests passing
  - [ ] Ready for Phase 3

---

## 📝 Documentation to Produce

### During Planning
1. `BATCH_A0_TODO_INVENTORY.md` — Complete list of all 79 items
2. `BATCH_A0_CATEGORIZATION_REPORT.md` — Breakdown by type and effort
3. `BATCH_A0_REMEDIATION_DECISIONS.md` — Decision for each item

### During Implementation
4. `BATCH_A0_COMPLETION_LOG.md` — Progress tracking
5. `BATCH_A0_PHASE3_BACKLOG.md` — Deferred items for Phase 3

### After Completion
6. `BATCH_A0_DELIVERY_SUMMARY.md` — Final delivery report

---

## 🎯 Success Metrics

| Metric | Target | Method |
|--------|--------|--------|
| TODOs identified | 79 | Grep scan |
| TODOs removed | 100% | Code review |
| Tests passing | 100% | ctest |
| Productivity | 15-20 items/day | Time tracking |
| Quality | No regressions | Test suite |

---

## 🔄 Risk Mitigation

### Risk 1: Removing Important TODOs
**Mitigation:** Review all TODOs with original authors before removal

### Risk 2: Introducing Regressions
**Mitigation:** Run full test suite after each batch of changes

### Risk 3: Incomplete Implementation
**Mitigation:** Implement only simple/scoped items; defer complex work

### Risk 4: Timeline Overrun
**Mitigation:** Prioritize low-effort removals first; defer medium/high effort to Phase 3

---

## 📞 Next Steps

1. ✅ Complete inventory scan today
2. ✅ Categorize by type today
3. ✅ Create decision matrix tomorrow
4. ✅ Begin implementation 2026-08-17
5. ✅ Target completion 2026-08-20

---

**Status:** Ready to begin Batch A-0 execution  
**Timeline:** 3-5 days (2026-08-15 to 2026-08-20)  
**Expected Result:** 100% of TODOs removed from production code

---
