# Sprint 8 Wave 3: Complex Control Flow Move Semantics Remediation

**Date:** 2026-07-05  
**Wave 3 Scope:** 20 Tier 3 Medium gaps (Complex control flow)  
**Status:** 🟡 ANALYSIS & KICKOFF  
**Target:** 18 gaps fixed + 2 documented FP (90% fix rate)

---

## Wave 3 Overview

### Previous Waves Status
- **Wave 1:** ✅ 8/8 fixed (Category A: `.clear()` after move)
- **Wave 2:** ✅ 4 fixed + 16 documented FP (Category B: Member access after move)
- **Total Fixed So Far:** 12 gaps, **80% accuracy** on identification

### Wave 3 Characteristics
- **Complexity:** HIGH (control flow analysis required)
- **Pattern Type:** Conditional moves, loops, state machines, exception paths
- **False Positive Risk:** MEDIUM-HIGH (30-40% estimated)
- **Modules:** sharding, storage, training, gpu, analytics, other

---

## Strategy: Smart Analysis Before Fixing

### Step 1: Analyze Control Flow
For each gap, analyze:
- Is the move conditional (if/else)?
- Is the moved-from variable ever actually accessed after move?
- Is the access safe due to scoping or control flow?
- What's the loop/exception context?

### Step 2: Categorize as
- **TRUE GAP:** Definite moved-from usage (fix immediately)
- **CONDITIONAL:** Move happens in branch, access outside (analyze carefully)
- **FALSE POSITIVE:** Control flow ensures safety (document)

### Step 3: Implement Conservative Fixes
- Restructure control flow if needed
- Add guards or conditions
- Document moved-from state explicitly if necessary

---

## Wave 3 Expected Gaps (20 total)

Based on categories from Gap Report:

### Category C: Complex Flow After Move (18 gaps, primary focus)
**Pattern:** Move in complex control flow, then use variable
- Conditional moves (if/else branches)
- Loop-related patterns
- Exception path patterns
- State machine patterns
- Async/callback patterns

### Category D: Partial False Positives (2 gaps expected)
**Pattern:** Valid reuse patterns that look like moved-from

---

## Analysis Methodology

### For Each Flagged Gap:
1. View 15+ lines of context
2. Trace variable through control flow
3. Identify all use sites of moved variable
4. Check scoping (are variables in same scope?)
5. Determine: TRUE / CONDITIONAL / FALSE POSITIVE

### Classification Rules:
- **Move and use in same unrelated statements** → TRUE GAP
- **Move in branch, use only in that branch** → Usually SAFE
- **Move followed by reassignment** → SAFE
- **Lambda capture** → Usually SAFE (check carefully)
- **Loop fresh variables** → SAFE (new var each iteration)

---

## Implementation Priorities

### Priority 1: Loop-Related Patterns (5-7 gaps)
- Tokenization loops with moved variables
- Container iteration with move semantics
- **Fix:** Simple restructuring or control flow analysis

### Priority 2: Exception Path Patterns (3-5 gaps)
- try/catch with move in one branch
- Exception handlers accessing moved objects
- **Fix:** Guard with conditional checks

### Priority 3: State Machine Patterns (3-5 gaps)
- State changes with conditional moves
- Operator overloads with move semantics
- **Fix:** Careful analysis of state transitions

### Priority 4: Async/Callback Patterns (3-5 gaps)
- Lambda captures with moved variables
- Async operations referencing moved objects
- **Fix:** Understand capture semantics (usually safe)

---

## Expected Outcome

After Wave 3 (combining all waves):
- **Wave 1:** 8/8 ✅ (100% true gaps)
- **Wave 2:** 4/25 ✅ (16% true gaps, 16 FP documented)
- **Wave 3:** 8-10/20 (40-50% true gaps, rest analyzed)
- **Total Fixed:** 20-22/97 (21-23%)
- **Total Analyzed/Documented:** 30-35/97 (31-36%)
- **Remaining:** 62-67/97 (64-69%, need semantic analysis)

---

## Quality Gate

✅ Only fix gaps that are TRULY moved-from usage violations
❌ Avoid over-correction

**Better to have:**
- 10 true fixes + 10 documented as safe/false positives
- Than 20 fixes where half are unnecessary refactoring

---

## Files to Analyze (Priority Order)

Per Gap Report, Wave 3 targets:
1. `src/sharding/cross_shard_transaction.cpp` - Complex transaction flow
2. `src/storage/wom_tree.cpp` - Tree structure manipulation
3. Training/optimization modules - Conditional moves
4. GPU modules - Async patterns
5. Other miscellaneous patterns

---

## Execution Plan

1. **Phase 1: Gap Discovery & Analysis** (1-2 hours)
   - Identify exact gap locations
   - View code context (15+ lines)
   - Trace control flow
   - Document initial classification

2. **Phase 2: Detailed Classification** (2-4 hours)
   - Analyze each gap deeply
   - Build decision tree for each gap
   - Categorize: TRUE / CONDITIONAL / FALSE POSITIVE
   - Document rationale

3. **Phase 3: Implementation** (2-4 hours)
   - Fix TRUE gaps with minimal changes
   - Add comments/documentation for CONDITIONAL gaps
   - Document FALSE POSITIVES with explanation

4. **Phase 4: Verification** (1-2 hours)
   - Build and test affected modules
   - Review fixes for safety
   - Generate final report

---

## Next Steps

1. ✅ Create this kickoff plan
2. 📋 Discover Wave 3 gaps from Gap Report data
3. 🔍 View code context for each gap
4. 📊 Create detailed gap analysis spreadsheet
5. 🛠️ Implement fixes for TRUE gaps
6. ✔️ Generate Wave 3 Final Report

---

**Status:** Ready to begin Phase 1 - Gap Discovery
