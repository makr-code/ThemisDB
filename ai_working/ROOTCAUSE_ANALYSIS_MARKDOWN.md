# Root-Cause Analysis: False Positive Categories

**Date:** 2026-06-02  
**Focus:** Categories with 0% TP rate (complete false positives)

---

## Summary

False positive categories indicate that the scanner is over-aggressive in these areas.
This analysis extracts why gaps are false positives and generates concrete tuning recommendations.

---

## COPY_OVERHEAD (FP Count: None)

### WHITELIST_PATTERN
- **Pattern:** std::make_shared / std::make_unique
- **Reason:** These are safe and should never be flagged
- **Confidence:** HIGH

### REQUIRE_CONTEXT
- **Pattern:** Copy in loop detection
- **Reason:** Need actual loop context, not just function calls
- **Confidence:** HIGH

### ADD_TYPE_CHECK
- **Pattern:** Distinguish POD vs complex types
- **Reason:** POD copies are cheap, complex copies are expensive
- **Confidence:** MEDIUM

### ADD_CONFIDENCE_THRESHOLD
- **Pattern:** Require multiple signals
- **Reason:** Single-signal flags have high FP rate
- **Confidence:** HIGH

### EXPAND_CONTEXT
- **Pattern:** Increase context window from 5->15 lines
- **Reason:** Current context too small for safe detection
- **Confidence:** MEDIUM

### TYPE_SAFE_PATTERNS
- **Pattern:** Whitelist type-safe containers/patterns
- **Reason:** Many flags are on type-safe code
- **Confidence:** HIGH

## OBSERVABILITY (FP Count: None)

### SKIP_INTERNAL
- **Pattern:** Private/internal functions
- **Reason:** Internal utils don't need observability
- **Confidence:** HIGH

### SKIP_TRIVIAL
- **Pattern:** Functions < 5 lines
- **Reason:** Trivial functions don't need logging
- **Confidence:** MEDIUM

### REQUIRE_PUBLIC_API
- **Pattern:** Only flag public/exported functions
- **Reason:** Users care about public API observability
- **Confidence:** HIGH

### ADD_CONFIDENCE_THRESHOLD
- **Pattern:** Require multiple signals
- **Reason:** Single-signal flags have high FP rate
- **Confidence:** HIGH

### EXPAND_CONTEXT
- **Pattern:** Increase context window from 5->15 lines
- **Reason:** Current context too small for safe detection
- **Confidence:** MEDIUM

### TYPE_SAFE_PATTERNS
- **Pattern:** Whitelist type-safe containers/patterns
- **Reason:** Many flags are on type-safe code
- **Confidence:** HIGH

## DB_CONNECTION_LEAK (FP Count: None)

### ADD_CONFIDENCE_THRESHOLD
- **Pattern:** Require multiple signals
- **Reason:** Single-signal flags have high FP rate
- **Confidence:** HIGH

### EXPAND_CONTEXT
- **Pattern:** Increase context window from 5->15 lines
- **Reason:** Current context too small for safe detection
- **Confidence:** MEDIUM

### TYPE_SAFE_PATTERNS
- **Pattern:** Whitelist type-safe containers/patterns
- **Reason:** Many flags are on type-safe code
- **Confidence:** HIGH

## NO_HEALTH_CHECK (FP Count: None)

### ADD_CONFIDENCE_THRESHOLD
- **Pattern:** Require multiple signals
- **Reason:** Single-signal flags have high FP rate
- **Confidence:** HIGH

### EXPAND_CONTEXT
- **Pattern:** Increase context window from 5->15 lines
- **Reason:** Current context too small for safe detection
- **Confidence:** MEDIUM

### TYPE_SAFE_PATTERNS
- **Pattern:** Whitelist type-safe containers/patterns
- **Reason:** Many flags are on type-safe code
- **Confidence:** HIGH

## HARDCODED_PATH (FP Count: None)

### ADD_CONFIDENCE_THRESHOLD
- **Pattern:** Require multiple signals
- **Reason:** Single-signal flags have high FP rate
- **Confidence:** HIGH

### EXPAND_CONTEXT
- **Pattern:** Increase context window from 5->15 lines
- **Reason:** Current context too small for safe detection
- **Confidence:** MEDIUM

### TYPE_SAFE_PATTERNS
- **Pattern:** Whitelist type-safe containers/patterns
- **Reason:** Many flags are on type-safe code
- **Confidence:** HIGH

