# Phase A.1: Privacy and Audit Helpers Hardening Implementation Plan

**Date**: 2026-08-08  
**Scope**: Hardening of utils module for edge cases and overload scenarios  
**Target**: Q3 2026 release-gate completion  
**Status**: In Progress

## Summary

This document outlines the hardening requirements and implementation strategy for 8 focused tests (PH-01..08) validating edge-case robustness and overload scenario handling in ThemisDB's privacy and audit helpers.

---

## Test Coverage Matrix

| Test ID | Component | Scenario | Input | Expected Outcome |
|---------|-----------|----------|-------|-----------------|
| **PH-01** | PII Detection | Unicode Normalization | Combining chars, BOM, emoji, surrogates | No crashes; correct detection |
| **PH-02** | Regex Engine | Multibyte UTF-8 Edge Cases | Emoji, mixed scripts, malformed UTF-8 | Graceful handling or explicit error |
| **PH-03** | Audit Logger | Buffer Overflow | 10K+ rapid audit entries | Documented fallback (drop/reject/block) |
| **PH-04** | Rate Limiter | Sustained Load | 1000+ scans/sec | Graceful degradation; no crashes |
| **PH-05** | Pseudonymizer | Edge Cases | Null, empty, >1MB inputs | Consistent behavior or clear error |
| **PH-06** | Saga Logger | Concurrent Writes | 10+ threads writing simultaneously | No corruption; audit trail integrity |
| **PH-07** | Audit Logger | Timeout/Cancellation | Long-running operation + cancel signal | Graceful stop; final entry logged |
| **PH-08** | Regex Engine | Catastrophic Backtracking Prevention | ReDoS patterns: (a+)+b, (a*)*b | Timeout or bounded computation |

---

## Implementation Roadmap

### Phase 1: Test Infrastructure (COMPLETED)
- [x] Create `test_utils_privacy_audit_hardening.cpp` with 8 focused tests
- [x] Register test in `tests/utils/CMakeLists.txt`
- [x] Each test includes:
  - Clear @brief describing scenario
  - Input/Expected outcome documented
  - No silent failures; explicit error handling

### Phase 2: Code Hardening (IN PROGRESS)

#### 2.1: Regex Detection Engine (PH-02, PH-08)
**Files**: `src/utils/regex_detection_engine.cpp`, `include/utils/regex_detection_engine.h`

**Changes**:
1. Add timeout protection for regex matching
   - Implement pattern complexity analysis
   - Detect known ReDoS patterns
   - Enforce timeout for match operations
   
2. Add explicit error codes for hardening violations
   - `REGEX_TIMEOUT = 7350`
   - `REGEX_COMPLEXITY_EXCEEDED = 7351`
   - `REGEX_PATTERN_MALFORMED = 7352`

3. Unicode handling improvements
   - Validate UTF-8 input before regex matching
   - Handle combining characters gracefully
   - Implement UnicodeDetectionEngine fallback

4. Implementation approach:
   ```cpp
   // Add to RegexDetectionEngine class:
   struct RegexTimeoutPolicy {
       std::chrono::milliseconds timeout{100};
       size_t max_backtrack_depth{1000};
       bool detect_redos_patterns{true};
   };
   
   [[nodiscard]] bool validateRegexPattern(const std::string& pattern);
   [[nodiscard]] std::optional<UtilsError> matchWithTimeout(
       const std::string& text,
       const std::regex& pattern,
       std::vector<PIIFinding>& findings);
   ```

#### 2.2: PII Detection Engine (PH-01)
**Files**: `src/utils/pii_detection_engine.cpp`, `include/utils/pii_detection_engine.h`

**Changes**:
1. Add Unicode normalization support
   - Implement NFD/NFC normalization
   - Handle combining characters
   - Gracefully skip invalid UTF-8 sequences

2. Input validation layer
   - Bounds check all input strings
   - Validate UTF-8 byte sequences
   - Reject oversized inputs (>10MB)

3. Error handling improvements
   - Explicit error returns for Unicode errors
   - Audit trail entry for skipped invalid sequences
   - Configurable fallback behavior

#### 2.3: Audit Logger (PH-03, PH-07)
**Files**: `src/utils/audit_logger.cpp`, `include/utils/audit_logger.h`

**Changes**:
1. Buffer overflow handling
   - Configurable queue size with clear limits
   - Document overflow policy (drop oldest/reject new)
   - Log overflow events themselves
   
2. Timeout and cancellation support
   - Add `cancellation_token` parameter to long-running ops
   - Ensure final audit entry logged on cancellation
   - No resource leaks (graceful shutdown)

3. Backpressure mechanism
   - Return status indicating if entry accepted
   - Signal when queue is full
   - Provide queue availability info to caller

#### 2.4: Saga Logger (PH-06)
**Files**: `src/utils/saga_logger.cpp`, `include/utils/saga_logger.h`

**Changes**:
1. Thread-safe batch assembly
   - Use lock-free ring buffer or mutex-protected deque
   - Atomic batch counter
   - Deterministic batch ordering

2. Concurrent write safety
   - All writes protected by sync primitives
   - Batch assembly is atomic (no partial signatures)
   - Verify no log entry loss under stress

3. Audit trail completeness
   - Persist completed batches to disk
   - Maintain checksum of batch contents
   - Recover on restart if needed

#### 2.5: Rate Limiter Integration (PH-04)
**Files**: `include/utils/rate_limiter.h`, `src/utils/` (if implementation exists)

**Changes**:
1. Add configurable rate limits for PII scans
   - Max scans per second (default: 10,000)
   - Burst allowance (e.g., 1000 tokens)
   - Graceful backpressure when exceeded

2. Monitoring/metrics
   - Current throughput statistics
   - Queue depth tracking
   - Overflow events counter

3. Configuration
   ```yaml
   rate_limiting:
     max_scans_per_sec: 10000
     burst_allowance: 1000
     overflow_policy: "drop_oldest" # or "reject_new"
   ```

#### 2.6: Pseudonymizer (PH-05)
**Files**: `src/utils/pii_pseudonymizer.cpp`, `include/utils/pii_pseudonymizer.h`

**Changes**:
1. Input bounds validation
   - Reject inputs > 10MB
   - Validate not null
   - Handle empty strings (return as-is or error)

2. Consistent pseudonymization
   - HMAC-SHA256 must be deterministic for same input
   - Configurable pseudonym format (hex, base64)
   - Audit trail includes pseudonym mapping (encrypted)

3. Error handling
   - Clear error return for oversized inputs
   - Explicit handling of null/empty
   - Special characters treated consistently

---

## Error Codes (Range 7300-7399)

### Existing Codes
- `UTILS_AUDIT_OVERFLOW = 7300`
- `UTILS_BATCH_ROLLBACK = 7301`
- `UTILS_BATCH_SIZE_EXCEEDED = 7302`
- `UTILS_RETRY_EXHAUSTED = 7303`
- `UTILS_DESER_INVALID = 7304`
- `UTILS_POOL_EXHAUSTED = 7305`

### New Codes (Phase A.1)
- `REGEX_TIMEOUT = 7350` - Pattern match exceeded timeout
- `REGEX_COMPLEXITY_EXCEEDED = 7351` - Pattern too complex (ReDoS risk)
- `REGEX_PATTERN_MALFORMED = 7352` - Regex compilation failed
- `UNICODE_NORMALIZATION_ERROR = 7353` - Unicode validation failed
- `INPUT_TOO_LARGE = 7354` - Input exceeds size limit
- `RATE_LIMIT_EXCEEDED = 7355` - Scan rate limit exceeded
- `CONCURRENCY_VIOLATION = 7356` - Thread-safety violation detected

---

## Build and Test Commands

```bash
# Configure
cmake --preset community-release

# Build test
cmake --build build-community-release \
  --target module_utils_test_privacy_audit_hardening_focused

# Run tests
cd build-community-release
ctest -R "PrivacyAuditHardening" -V --output-on-failure

# Run specific test
ctest -R "PH0[18]_" -V --output-on-failure
```

---

## Implementation Status

### ✅ COMPLETED
- [x] Test file: `test_utils_privacy_audit_hardening.cpp` (8 tests)
- [x] CMakeLists.txt registration
- [x] Test IDs: PH-01 through PH-08

### 🔄 IN PROGRESS
- [ ] Regex timeout implementation (PH-02, PH-08)
- [ ] Unicode validation (PH-01)
- [ ] Audit buffer overflow policy (PH-03)
- [ ] Rate limiter integration (PH-04)
- [ ] Pseudonymizer bounds checks (PH-05)
- [ ] Saga logger concurrent safety (PH-06)
- [ ] Audit logger timeout/cancellation (PH-07)

### 📋 TO DO
- [ ] Update error_registry.h with new error codes
- [ ] Add operator-actionable error messages
- [ ] Create benchmarks for load testing
- [ ] Document fallback behaviors for operators

---

## Acceptance Criteria

### Code Quality
- [ ] All 8 tests pass (PH-01..08)
- [ ] No regressions in existing utils tests
- [ ] C++ best practices (RAII, const-correctness, [[nodiscard]])
- [ ] No silent failures; explicit error returns
- [ ] Audit trail consistency validated

### Documentation
- [ ] Public API updated (if signatures changed)
- [ ] Operator-actionable error messages
- [ ] Fallback behaviors documented
- [ ] Performance expectations documented

### Hardening
- [ ] Unicode edge cases handled
- [ ] Regex timeout protection active
- [ ] Rate limiting configurable
- [ ] Concurrent writes safe
- [ ] No resource leaks under load

---

## References

- `include/utils/utils_api_contract.h` - Error taxonomy
- `.github/instructions/cpp-best-practices.instructions.md` - C++ guidelines
- `CLAUDE.md` - Stub/mock governance rules
- `src/utils/ROADMAP.md` - Phase 1 gate criteria

---

## Contacts & Approval

- **Prepared by**: AI Implementation Agent
- **Date**: 2026-08-08
- **Status**: Ready for implementation

