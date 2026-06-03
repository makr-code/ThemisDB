# Gap Validation Assessment Guide

## Overview
✅ **Representative sample generated:** 50 gaps stratified by severity and category  
📊 **Total gaps in dataset:** 18,795  
🎯 **Assessment required:** TP (True Positive) vs FP (False Positive)  
⏱️ **Time estimate:** 1-2 hours for manual review

---

## Files Generated

### 1. **SAMPLE_VALIDATION_TEMPLATE.md** (Main Assessment Document)
Contains all 50 gaps with:
- **File location** and line number
- **Gap description** (what the scanner detected)
- **Function context** (~15 lines of code around the issue)
- **Assessment checkboxes:** TP / FP / ? (uncertain)
- **Notes field:** Reasoning for your assessment

### 2. **sample_validation_metadata.json** (Sample Statistics)
```json
{
  "sample_size": 50,
  "total_gaps": 18795,
  "by_severity": {
    "CRITICAL": 8,    (16%)
    "HIGH": 20,       (40%)
    "MEDIUM": 22      (44%)
  },
  "by_category": {
    "llm_ai_safety": 6,
    "performance": 10,
    "data_race": 2,
    "...": "..."
  },
  "by_module": {
    "server": 9,
    "llm": 9,
    "query": 4,
    "...": "..."
  }
}
```

---

## Assessment Process

### Step 1: Open Template
Open `ai_working/SAMPLE_VALIDATION_TEMPLATE.md` in your editor.

### Step 2: For Each Gap (1-50)
For every gap section:

1. **Read the description** - What does the scanner claim?
2. **Review the function context** - Is the context sufficient? (±12 lines around the target)
3. **Assess correctness:**
   - **TP** = The code has a real issue (e.g., resource leak, race condition, null dereference)
   - **FP** = The code is safe/correct (scanner misidentified a pattern)
   - **?** = Uncertain; needs deeper investigation or more context
4. **Write reasoning** in Notes field (1-3 sentences):
   - Why you think it's TP or FP
   - Any missing context that would help
   - Relevant coding patterns you recognize

### Step 3: Example Assessment

```markdown
## [15/50] query - HIGH
**File:** `src\query\query_engine.cpp:4558`
**Category:** no_timeout
**Message:** semaphore_wait without timeout — can block indefinitely

### Function Context
```cpp
 4557 |     	}
 4558 | >>>     tg2.wait();
 4559 |         for (auto& b : buckets) { ...
```
### Assessment
- [X] **TP** - True Positive (real issue)
- [ ] **FP** - False Positive (code is safe)
- [ ] **?** - Uncertain / Needs investigation

**Notes:** _tg2 is TaskGroup with wait() that blocks indefinitely if task throws exception. Should add timeout or exception handler._ ✅
```

---

## Stratification Strategy

The 50 gaps represent the full dataset (18,795) proportionally:

| Severity | Count | % | Represents |
|----------|-------|---|-----------|
| CRITICAL | 8 | 16% | ~3,000 gaps |
| HIGH | 20 | 40% | ~7,500 gaps |
| MEDIUM | 22 | 44% | ~8,300 gaps |

**Categories:** Proportional sample across 30+ categories (llm_ai_safety, performance, data_race, etc.)

**Modules:** Top modules oversampled (server, llm, query, utils, security, etc.)

---

## Expected Outcomes

After assessment, we'll calculate:

| Metric | Calculation |
|--------|-----------|
| **TP Rate (%)** | (# of TP) / 50 |
| **FP Rate (%)** | (# of FP) / 50 |
| **Extrapolated FP in Dataset** | FP% × 18,795 |
| **Effective Gap Count** | TP% × 18,795 |

**Example:** If assessment yields:
- 35 TP (70%)
- 12 FP (24%)
- 3 ? (6%)

Then: **~13,157 effective gaps** in full dataset (70% of 18,795)

---

## Known Patterns from Gap Scanners

### High-Confidence Patterns (Likely TP)
- Mutex locks without timeout in loops (can deadlock/block)
- Resource leaks (acquire without release on all paths)
- Shared data access without synchronization (race conditions)
- Null pointer dereference without validation
- Exception-unsafe resource management (no RAII)

### Common False Positives (Likely FP)
- `Status s;` followed by assignment (flagged as "uninitialized" but is safe)
- Vector operations on well-known types (flagged as "missing reserve" when unnecessary)
- Temporary containers in range-for loops that are actually safe (std::string returns, etc.)
- Smart pointers with safe lifetimes (flagged as "potential misuse")
- Float operations in deterministic contexts (flagged as "non-determinism")

---

## Tips for Review

1. **Context gaps?** If ±12 lines insufficient, mark as `?` and note what's missing
2. **Type confusion?** If scanner misidentified the type, mark as **FP**
3. **Severity mismatch?** If issue is real but less critical than labeled, still mark **TP**
4. **Uncertain?** Mark as `?` and describe uncertainty in Notes
5. **Patterns?** If you see same false positive repeat, note pattern for scanner tuning

---

## Next Steps (After Assessment)

### Phase 1: Extrapolation
Calculate TP/FP rates from 50-sample:
```
Extrapolated FP count = (FP% / 100) × 18,795
Extrapolated TP count = (TP% / 100) × 18,795
```

### Phase 2: Pattern Analysis
Group FP cases by:
- Scanner category (llm_ai_safety, performance, etc.)
- Module (which codebases produce most FPs?)
- Root cause (type confusion, pattern overgeneralization, etc.)

### Phase 3: Scanner Tuning (If Needed)
If FP rate > 30%:
- Modify detection logic in `gap_scanner_v3_*.py` files
- Tighten pattern matching or add type validation
- Re-run pipeline and compare results

### Phase 4: Final Report
Generate validation report with:
- TP/FP breakdown by severity, category, module
- Confidence score interpretation
- Recommendations for production use

---

## Assessment Checklist

- [ ] Template opened in editor
- [ ] Read through all 50 gaps
- [ ] Marked each gap as TP / FP / ?
- [ ] Added notes for 90%+ of gaps
- [ ] Saved edited template
- [ ] Counted final TP/FP/? breakdown

---

## Questions?

If gaps lack sufficient context (function too long, complex call chains, etc.):
1. Mark as `?`
2. Note what context is missing
3. These insights help refine the validation process

**Goal:** Honest assessment of gap quality, not perfect accuracy. 80% confident > 100% uncertain!

