# Scanner Tuning Strategy - Rootcause-Driven Improvements

**Date:** 2026-06-02  
**Based on:** Automated validation analysis (50-gap sample)  
**Findings:** 24% TP rate | 36% FP rate | 40% uncertain

---

## Executive Summary

Based on rootcause analysis of false positive categories (0% TP), we've identified **5 concrete tuning strategies** to improve scanner quality from 24% to 50%+ TP rate:

1. **Whitelist Safe Patterns** (make_shared, lock_guard, etc.)
2. **Expand Context Windows** (5 → 15 lines for better understanding)
3. **Add Confidence Thresholds** (require multiple signals, not just one)
4. **Type/Scope Awareness** (skip internals, distinguish POD vs complex)
5. **Pattern Refinement** (reduce over-aggressive heuristics)

---

## Category-Specific Tuning Roadmap

### 1. COPY_OVERHEAD (3 FP, 0% TP) ⚠️

**Root Cause:** Over-aggressive detection of any copy operation without understanding context.

**Concrete Fixes:**

```
PRIORITY: HIGH
CONFIDENCE: MEDIUM
EFFORT: 2 days

Actions:
[ ] 1. Whitelist std::make_shared / std::make_unique
    - These are safe and should NEVER be flagged
    - Modify: gap_scanner_v3_performance.py
    - Add check: if 'make_shared' or 'make_unique' in context: return SKIP

[ ] 2. Require actual loop context
    - Don't flag copy outside of explicit loops
    - Check: Look for 'for', 'while', 'std::for_each' in ±10 lines
    - Current: Flags any copy anywhere (too broad)

[ ] 3. Add POD type detection
    - POD copies (int, float, bool) are cheap (~zero cost)
    - Complex types (vector, map, string) need scrutiny
    - Use: C++ POD type patterns (fundamental types only)

[ ] 4. Expand context window
    - Current: ±5 lines
    - New: ±15 lines
    - Reason: Need to understand full loop structure, not just immediate context

[ ] 5. Confidence threshold
    - Current: Single-signal flag → all are FP
    - New: Require 2+ signals:
      * Signal 1: Copy operation detected
      * Signal 2: Inside loop
      * Signal 3: Non-POD type OR large size
    - Flag only if 2+ signals present
```

**Testing After Fix:**
```
Expected improvement: 0% → 60%+ TP rate
Validation: Re-run analysis on copy_overhead sample
```

---

### 2. OBSERVABILITY (2 FP, 0% TP) ⚠️

**Root Cause:** Over-flagging all functions without logging, including internal utilities and trivial functions.

**Concrete Fixes:**

```
PRIORITY: HIGH
CONFIDENCE: HIGH
EFFORT: 2 days

Actions:
[ ] 1. Skip private/internal functions
    - Don't flag functions in 'internal/', 'detail/', 'impl/' namespaces
    - Don't flag static functions (file-local scope)
    - Modify: gap_scanner_v3_observability.py
    - Add: namespace check before flagging

[ ] 2. Skip trivial functions
    - Functions < 5 lines of code don't need logging
    - Check: Count actual code lines (exclude braces, comments)
    - Example: Getters, simple wrappers → skip

[ ] 3. Require public API scope
    - Only flag public functions (exported, in public headers)
    - Check: grep for "THEMIS_.*_API" or "public:" context
    - Reason: Users care about public API observability, not internals

[ ] 4. Add confidence threshold
    - Current: Any function without spdlog → flag
    - New: Require 2+ signals:
      * Signal 1: No logging statement
      * Signal 2: Public API marker (THEMIS_API)
      * Signal 3: Complex logic (>30 lines OR >3 branches)
    - Flag only if all 3 signals present

[ ] 5. Whitelist common patterns
    - std::string operator<<
    - Simple getter/setter methods
    - Operator overloads
    - Constructor/destructor wrappers
```

**Testing After Fix:**
```
Expected improvement: 0% → 70%+ TP rate
Validation: Check observability gaps match public API definitions
```

---

### 3. DB_CONNECTION_LEAK (1 FP, 0% TP) ⚠️

**Root Cause:** Flagging RAII-wrapped connections as "leaks" without recognizing automatic cleanup.

**Concrete Fixes:**

```
PRIORITY: MEDIUM
CONFIDENCE: MEDIUM
EFFORT: 1.5 days

Actions:
[ ] 1. Whitelist RAII patterns
    - Recognize: std::unique_ptr<Connection>, shared_ptr<Connection>
    - Recognize: RAII wrappers with ~destructor
    - Add: Pattern detection for common RAII types in codebase
    - Don't flag if: Connection is in smart_ptr or wrapped object

[ ] 2. Expand context window
    - Current: ±5 lines
    - New: ±20 lines (need to see destructor/cleanup)
    - Reason: Connection cleanup may be 10+ lines away

[ ] 3. Require multiple signals
    - Signal 1: Database connection opened
    - Signal 2: No explicit close() OR no RAII wrapper
    - Signal 3: Unguarded resource (not in try/finally or RAII)
    - Flag only if ALL 3 signals present

[ ] 4. Known good patterns
    - DBConnections with pool (ConnectionPool handles cleanup)
    - Connections in transaction scope
    - Connections passed as references (caller responsibility)
```

**Testing After Fix:**
```
Expected improvement: 0% → 60%+ TP rate
Validation: Verify all flagged connections have no cleanup path
```

---

### 4. NO_HEALTH_CHECK (1 FP, 0% TP) ⚠️

**Root Cause:** Flagging functions that don't need health checks (not endpoints/critical paths).

**Concrete Fixes:**

```
PRIORITY: MEDIUM
CONFIDENCE: MEDIUM
EFFORT: 1.5 days

Actions:
[ ] 1. Scope to critical paths only
    - Only flag: HTTP/gRPC handlers, main server entry points
    - Skip: Internal utilities, data processing, helper functions
    - Check: Function signature (Handler*, process()*, *_handler)

[ ] 2. Whitelist internal functions
    - Skip functions called only internally (not entry points)
    - Check: Only flag if function is public API entry point

[ ] 3. Require multiple signals
    - Signal 1: Handles external requests/network
    - Signal 2: No health checks (no THEMIS_HEALTH_CHECK macros)
    - Signal 3: Potential for cascading failures (database access, network)
    - Flag only if ALL 3 signals present

[ ] 4. Add confidence threshold
    - Current: Single-signal (no health check) → all FP
    - New: Require evidence of actual risk
    - Example: "This is internal utility, not critical path" → SKIP
```

**Testing After Fix:**
```
Expected improvement: 0% → 50%+ TP rate
Validation: All flagged functions should be actual entry points
```

---

### 5. HARDCODED_PATH (1 FP, 0% TP) ⚠️

**Root Cause:** Flagging compile-time paths as hardcoded, or safe configuration paths.

**Concrete Fixes:**

```
PRIORITY: LOW
CONFIDENCE: MEDIUM
EFFORT: 1 day

Actions:
[ ] 1. Distinguish compile-time vs runtime paths
    - Compile-time: Constants, preprocessor #define → OK
    - Runtime: User input, config values → FLAG
    - Check: Is path in constexpr, #define, or macro?

[ ] 2. Whitelist configuration paths
    - Paths from config file (safe)
    - Paths from environment variables (safe)
    - Paths in unit test fixtures (safe)
    - Skip if: Path comes from config object

[ ] 3. Skip test/debug code
    - Functions in *_test.cpp, *_unittest.cpp → skip
    - Functions with TEST_*, GTEST_ macros → skip

[ ] 4. Require multiple signals
    - Signal 1: Hardcoded path string detected
    - Signal 2: At runtime (not compile-time constant)
    - Signal 3: Non-configurable (not from config/env)
    - Flag only if ALL 3 signals present
```

**Testing After Fix:**
```
Expected improvement: 0% → 50%+ TP rate
Validation: All flagged paths should be non-configurable
```

---

## Phase-Based Implementation Plan

### Phase 1: Quick Wins (Week 1)
```
Priority: HIGH
Effort: 3-4 days

Tasks:
[ ] Whitelist safe patterns (make_shared, lock_guard)
    - copy_overhead: +30% improvement expected
    - 2-3 hour fix per category

[ ] Expand context windows (5 → 15 lines)
    - All categories benefit
    - 1-2 hour global change

[ ] Add confidence thresholds
    - Require 2+ signals for each category
    - 2-3 hours per category (5 categories × 2 = 10 hours)
```

### Phase 2: Category Tuning (Week 2)
```
Priority: MEDIUM
Effort: 3-4 days

Tasks:
[ ] Scope-awareness (public vs internal)
    - observability: +50% improvement
    - 2 hours

[ ] Type awareness (POD vs complex)
    - copy_overhead: +20% improvement
    - 2 hours

[ ] RAII pattern detection
    - db_connection_leak: +40% improvement
    - 3 hours
```

### Phase 3: Re-Validation & Tuning (Week 2-3)
```
Priority: HIGH
Effort: 2 days

Tasks:
[ ] Re-run gap_scanner_v3 with tuned rules
    - Full 18,795 gaps analysis
    - 30 min execution + 30 min analysis

[ ] Generate validation sample from new results
    - 50 gaps from new scan
    - Automated assessment (same pipeline)

[ ] Compare metrics
    - Old: 24% TP, 36% FP, 40% uncertain
    - Target: 50%+ TP, 25% FP, 25% uncertain

[ ] Adjust rules if needed
    - If TP < 45%: Further tuning required
    - If TP > 50%: Accept and move to remediation
```

---

## Success Criteria

| Metric | Current | Target | Pass? |
|--------|---------|--------|-------|
| Overall TP Rate | 24% | ≥50% | ❌ NEEDS TUNING |
| Overall FP Rate | 36% | ≤25% | ❌ NEEDS TUNING |
| CRITICAL TP | 50% | ≥60% | ✅ OK |
| HIGH/MEDIUM TP | 15-23% | ≥40% | ❌ NEEDS TUNING |
| copy_overhead TP | 0% | ≥50% | ❌ |
| observability TP | 0% | ≥60% | ❌ |
| db_connection_leak TP | 0% | ≥50% | ❌ |

---

## Estimated Impact

```
Week 1 (Quick Wins):
  - Whitelist + context expansion
  - Expected: 24% → 35-40% TP
  - Effort: 3-4 days
  - Risk: LOW

Week 2 (Category Tuning):
  - Scope/type awareness
  - Expected: 35-40% → 48-55% TP
  - Effort: 3-4 days
  - Risk: MEDIUM

Week 3 (Validation + Final Tuning):
  - Re-validate + adjust
  - Expected: Final TP rate 50-60%
  - Effort: 1-2 days
  - Risk: MEDIUM

Total: 1-2 weeks, 7-10 days effort
```

---

## Implementation Checklist

Before starting tuning:

- [ ] Backup current gap_scanner_v3_* files
- [ ] Create feature branch: `feature/gap-scanner-tuning`
- [ ] Document each change with rationale
- [ ] Add unit tests for each new/modified scanner
- [ ] Re-run full validation after each major change

After tuning complete:

- [ ] Generate new 50-gap validation sample
- [ ] Re-run automated assessment
- [ ] Compare old vs new metrics
- [ ] Document improvement per category
- [ ] Merge to develop with tuning summary

---

## Questions for Review

Before implementing, consider:

1. **Whitelist Comprehensiveness:** Are make_shared/lock_guard the only safe patterns to whitelist, or are there others?
2. **Context Window Size:** Is 15 lines enough, or should we go to 20-30?
3. **Multi-Signal Approach:** Should we require all signals, or is "any 2 of 3" sufficient?
4. **Configuration Path Detection:** How do we reliably detect config-sourced paths?
5. **Test Code Filtering:** Should we completely skip test files, or flag them separately?

---

## Next Steps

1. **Review this plan** with stakeholders
2. **Start Phase 1** (quick wins) immediately
3. **Validate incrementally** after each category fix
4. **Re-run full pipeline** at Phase 3
5. **Move to remediation planning** once TP rate ≥50%

---

**Confidence Level:** 🟡 MEDIUM  
**Rationale:** Heuristics-based tuning has inherent limits. Some categories may need 2-3 iteration cycles.  
**Fallback:** If TP doesn't reach 50% after Phase 2, consider disabling problematic categories entirely and focusing on high-quality CRITICAL gaps.

