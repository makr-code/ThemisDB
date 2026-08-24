# Scanner Feedback Loop Report

**Generated:** 2026-06-02T13:43:25.544292
**Scan Date:** 2026-06-02
**Sample Size:** 50 gaps

## Overall Metrics
| Metric | Current | Previous | Delta |
|--------|---------|----------|-------|
| TP Rate | 24.0% | 24.0% | +0.0% |
| FP Rate | 36.0% | - | - |
| Total Effort | 53.9 days | - | - |

## Scanner Analysis

### DB_CONNECTION_LEAK
**Priority:** HIGH | **Effort:** 4.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- RAII-wrapped connections not recognized
- ConnectionPool pattern not detected
- Explicit close() calls outside context window
- Smart pointer cleanup not visible in ±5 line context

**Recommended Fixes:**
- ✓ Recognize smart_ptr wrapped connections
- ✓ Whitelist ConnectionPool patterns
- ✓ Expand context window from 5 to 20 lines (catch cleanup)
- ✓ Check for explicit .close() calls
- ✓ RAII pattern detection in destructor
- ✓ Add exception-safety context check

**Expected Impact:** 0% -> 50% TP

### NO_HEALTH_CHECK
**Priority:** HIGH | **Effort:** 3.0 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- Internal utilities flagged
- Non-handler functions flagged
- Data processors flagged (not entry points)
- Functions without critical path marker flagged

**Recommended Fixes:**
- ✓ Limit to HTTP/gRPC handler functions only
- ✓ Skip internal utilities (detail::, _impl)
- ✓ Require critical path marker or known patterns
- ✓ Skip data processor functions
- ✓ Whitelist common safe patterns
- ✓ Scope to entry point functions only

**Expected Impact:** 0% -> 60% TP

### HARDCODED_PATH
**Priority:** HIGH | **Effort:** 2.4 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- Compile-time constants flagged
- Configuration sources flagged
- Environment variables flagged
- Test code flagged

**Recommended Fixes:**
- ✓ Distinguish compile-time vs runtime paths
- ✓ Whitelist constexpr paths
- ✓ Whitelist configuration sources
- ✓ Whitelist environment variable sources
- ✓ Skip test code
- ✓ Add source tracking

**Expected Impact:** 0% -> 50% TP

### NO_TIMEOUT
**Priority:** HIGH | **Effort:** 2.4 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- Sync operations incorrectly flagged
- Operations with natural timeouts flagged
- Context windows too small to see cancellation

**Recommended Fixes:**
- ✓ Identify async operations requiring timeout
- ✓ Whitelist sync-only operations
- ✓ Check context for cancellation support
- ✓ Expand scope to critical paths

**Expected Impact:** 0% -> 50% TP

### NULL_DEREFERENCE
**Priority:** HIGH | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### POINTER_ARITHMETIC
**Priority:** HIGH | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### NO_RETRY_LOGIC
**Priority:** HIGH | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### O_N_SQUARED
**Priority:** HIGH | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### REPEATED_LOOKUP
**Priority:** HIGH | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### MANUAL_CLEANUP
**Priority:** HIGH | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 100.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### LEGACY_DUPLICATION
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 100.0% (1 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 100% -> 100% TP

### SMART_PTR_MISUSE
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 100.0% (1 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 40% -> 80% TP

### STRING_CONCAT_LOOP
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 100.0% (1 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 60% -> 95% TP

### REPEATED_SEARCH
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 100.0% (1 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 50% -> 90% TP

### MEMORY_ORDER
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 100.0% (1 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 0

**Common FP Patterns:**
- Conservative atomic patterns flagged
- Legacy code with simple atomics
- Platform-specific ordering assumptions

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 25% -> 70% TP

### MISSING_DTOR
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 100.0% (1 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 100% -> 100% TP

### UNCAUGHT_EXCEPTION
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 3
- TP Rate: 66.7% (2 true positives)
- FP Rate: 33.3% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 67% -> 97% TP

### DATA_RACE
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 2
- TP Rate: 50.0% (1 true positives)
- FP Rate: 50.0% (1 false positives)
- Uncertain: 0

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 50% -> 80% TP

### AUDIT_LOGGING
**Priority:** LOW | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 2
- TP Rate: 50.0% (1 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 1

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 50% -> 80% TP

### LLM_AI_SAFETY
**Priority:** MEDIUM | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 6
- TP Rate: 16.7% (1 true positives)
- FP Rate: 33.3% (2 false positives)
- Uncertain: 3

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 17% -> 47% TP

### PERFORMANCE
**Priority:** MEDIUM | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 10
- TP Rate: 10.0% (1 true positives)
- FP Rate: 40.0% (4 false positives)
- Uncertain: 5

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 10% -> 40% TP

### OBSERVABILITY
**Priority:** MEDIUM | **Effort:** 3.6 days

**Current Metrics:**
- Total Gaps: 2
- TP Rate: 0.0% (0 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 2

**Common FP Patterns:**
- Internal functions flagged (detail::, _impl)
- Trivial getters/setters flagged
- Private member functions flagged
- Functions < 5 lines flagged as missing metrics

**Recommended Fixes:**
- ✓ Skip functions in internal/detail/impl namespaces
- ✓ Skip trivial functions (< 5 lines code)
- ✓ Skip getters/setters/operators/destructors
- ✓ Require public API marker (THEMIS_API)
- ✓ Skip private member functions
- ✓ Confidence threshold for weak signals

**Expected Impact:** 0% -> 70% TP

### COPY_OVERHEAD
**Priority:** MEDIUM | **Effort:** 3.0 days

**Current Metrics:**
- Total Gaps: 3
- TP Rate: 0.0% (0 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 3

**Common FP Patterns:**
- make_shared/make_unique detected but whitelisted
- POD types (int, float) flagged as complex
- std::move() calls flagged as overhead
- Short loop iterations (< 5 iterations) flagged

**Recommended Fixes:**
- ✓ Whitelist make_shared / make_unique in PATTERNS
- ✓ Add POD type detection (int, float, bool, size_t)
- ✓ Require loop context (±15 lines minimum)
- ✓ Check for std::move in context (safe optimization)
- ✓ Expand context window from 5 to 15 lines
- ✓ Add confidence threshold (require 2+ signals)

**Expected Impact:** 0% -> 60% TP

### LOCK_CONTENTION
**Priority:** MEDIUM | **Effort:** 2.2 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 1

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Identify hot locks (high contention)
- ✓ Suggest fine-grained locking
- ✓ Flag long critical sections

**Expected Impact:** 0% -> 50% TP

### DELETE_NO_NULLPTR
**Priority:** MEDIUM | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 1

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### DETERMINISM
**Priority:** MEDIUM | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 1

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### UNINITIALIZED_ACCESS
**Priority:** MEDIUM | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 1

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### SIZE_ASSUMPTION
**Priority:** MEDIUM | **Effort:** 1.5 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 1

**Common FP Patterns:**
- High-frequency patterns not yet analyzed
- More context needed for precise classification
- Category-specific tuning pending

**Recommended Fixes:**
- ✓ Expand context window for better pattern matching
- ✓ Add category-specific whitelists
- ✓ Implement scope awareness (public vs internal)

**Expected Impact:** 0% -> 50% TP

### RANGE_TEMPORARY
**Priority:** MEDIUM | **Effort:** 1.2 days

**Current Metrics:**
- Total Gaps: 1
- TP Rate: 0.0% (0 true positives)
- FP Rate: 0.0% (0 false positives)
- Uncertain: 1

**Common FP Patterns:**
- POD temporaries flagged as unsafe
- Short-lived temporaries with explicit lifetime
- Safe binding patterns not recognized

**Recommended Fixes:**
- ✓ Detect temporary objects in range-for
- ✓ Flag only complex types (not POD)
- ✓ Check for explicit lifetime extension

**Expected Impact:** 0% -> 50% TP

## Implementation Timeline

Total Effort: **53.9 days** (~2 weeks including validation)

Recommended Sequence:

**Step 1: DB_CONNECTION_LEAK** (4.5 days)
- Impact: 0% -> 50% TP
- Fixes: 6 changes

**Step 2: NO_HEALTH_CHECK** (3.0 days)
- Impact: 0% -> 60% TP
- Fixes: 6 changes

**Step 3: HARDCODED_PATH** (2.4 days)
- Impact: 0% -> 50% TP
- Fixes: 6 changes

**Step 4: NO_TIMEOUT** (2.4 days)
- Impact: 0% -> 50% TP
- Fixes: 4 changes

**Step 5: NULL_DEREFERENCE** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 6: POINTER_ARITHMETIC** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 7: NO_RETRY_LOGIC** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 8: O_N_SQUARED** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 9: REPEATED_LOOKUP** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 10: MANUAL_CLEANUP** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 11: LEGACY_DUPLICATION** (1.5 days)
- Impact: 100% -> 100% TP
- Fixes: 3 changes

**Step 12: SMART_PTR_MISUSE** (1.5 days)
- Impact: 40% -> 80% TP
- Fixes: 3 changes

**Step 13: STRING_CONCAT_LOOP** (1.5 days)
- Impact: 60% -> 95% TP
- Fixes: 3 changes

**Step 14: REPEATED_SEARCH** (1.5 days)
- Impact: 50% -> 90% TP
- Fixes: 3 changes

**Step 15: MEMORY_ORDER** (1.5 days)
- Impact: 25% -> 70% TP
- Fixes: 3 changes

**Step 16: MISSING_DTOR** (1.5 days)
- Impact: 100% -> 100% TP
- Fixes: 3 changes

**Step 17: UNCAUGHT_EXCEPTION** (1.5 days)
- Impact: 67% -> 97% TP
- Fixes: 3 changes

**Step 18: DATA_RACE** (1.5 days)
- Impact: 50% -> 80% TP
- Fixes: 3 changes

**Step 19: AUDIT_LOGGING** (1.5 days)
- Impact: 50% -> 80% TP
- Fixes: 3 changes

**Step 20: LLM_AI_SAFETY** (1.5 days)
- Impact: 17% -> 47% TP
- Fixes: 3 changes

**Step 21: PERFORMANCE** (1.5 days)
- Impact: 10% -> 40% TP
- Fixes: 3 changes

**Step 22: OBSERVABILITY** (3.6 days)
- Impact: 0% -> 70% TP
- Fixes: 6 changes

**Step 23: COPY_OVERHEAD** (3.0 days)
- Impact: 0% -> 60% TP
- Fixes: 6 changes

**Step 24: LOCK_CONTENTION** (2.2 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 25: DELETE_NO_NULLPTR** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 26: DETERMINISM** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 27: UNINITIALIZED_ACCESS** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 28: SIZE_ASSUMPTION** (1.5 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

**Step 29: RANGE_TEMPORARY** (1.2 days)
- Impact: 0% -> 50% TP
- Fixes: 3 changes

## High-Priority Fixes (Start Here)
- [no_health_check] ✓ Limit to HTTP/gRPC handler functions only
- [no_health_check] ✓ Skip internal utilities (detail::, _impl)
- [no_health_check] ✓ Require critical path marker or known patterns
- [no_health_check] ✓ Skip data processor functions
- [no_health_check] ✓ Whitelist common safe patterns
- [no_health_check] ✓ Scope to entry point functions only
- [no_timeout] ✓ Identify async operations requiring timeout
- [no_timeout] ✓ Whitelist sync-only operations
- [no_timeout] ✓ Check context for cancellation support
- [no_timeout] ✓ Expand scope to critical paths