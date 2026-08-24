# Phase A.2 Diagnostics - Code Review Resolution Tracker

**Review Date:** 2026-08-08  
**Status:** Pre-Implementation Review  
**Target Implementation Start:** 2026-08-12 (Week 1)

---

## CRITICAL FINDINGS - MUST RESOLVE BEFORE IMPLEMENTATION

### [CRIT-001] Error Code Range Collision Risk

**Status:** ⬜ NOT STARTED

**Blocking:** Entire implementation cannot proceed until resolved

**Resolution Steps:**
- [ ] Step 1: Rename `UtilsError` → `UtilsErrorCode` in utils_api_contract.h
- [ ] Step 2: Change type from `int32_t` to `uint32_t`
- [ ] Step 3: Verify existing codes [7300-7305] compile with new enum name
- [ ] Step 4: Add new codes [7306-7349] without collision
- [ ] Step 5: Update error_registry.h to reserve [7306-7349]
- [ ] Step 6: Create backward compat typedef: `using UtilsError = UtilsErrorCode;`
- [ ] Step 7: Run full utils test suite to verify no regressions

**Expected Completion:** 2026-08-09  
**Owner:** @[TBD - lead developer]  
**Sign-off:** @[TBD - tech lead]

**Testing:**
```bash
cd build-linux-release
cmake --build . --target themis_core
cmake --build . --target module_utils_test_interfaces_focused
ctest -R "test_utils_" --output-on-failure
```

---

## HIGH SEVERITY FINDINGS - MUST RESOLVE BEFORE PR SUBMISSION

### [HIGH-001] Inconsistent Error Handling Across 5 Helpers

**Status:** ⬜ NOT STARTED

**Blocking:** Cannot accept PR that doesn't address inconsistency

**Resolution Steps:**
- [ ] Step 1: Define DiagnosticEvent struct:
  ```cpp
  struct DiagnosticEvent {
      UtilsErrorCode code;
      std::string module;              // "audit", "pii", "compression", "concurrency", "crypto"
      DiagnosticSeverity severity;     // CRITICAL, HIGH, MEDIUM, LOW
      IncidentCategory category;       // See HIGH-003
      std::string action;              // Operator-actionable guidance
      std::map<std::string, std::string> context;  // key=value pairs
      std::chrono::system_clock::time_point timestamp;
  };
  ```
- [ ] Step 2: Implement DiagnosticsEmitter class (see HIGH-002 for thread-safety)
- [ ] Step 3: Update PII detection engine to emit on timeout/invalid/regex error
- [ ] Step 4: Update audit logger to emit on queue full/write failure
- [ ] Step 5: Update rate limiter to emit on exhaustion
- [ ] Step 6: Update compression codecs to emit on buffer/decompression/bomb errors
- [ ] Step 7: Update thread pool to emit on queue full/shutdown timeout
- [ ] Step 8: Verify all 5 helpers call emitter.Emit() at error sites

**Expected Completion:** 2026-08-20  
**Owner:** @[TBD - developers A, B, C]  
**Sign-off:** @[TBD - tech lead]

**Testing:**
```bash
# After all 5 helpers updated
ctest -R "test_utils_interfaces" --output-on-failure
# Load test: 1000 events/sec for 10 seconds
```

---

### [HIGH-002] Thread-Unsafe Diagnostics Listener Pattern

**Status:** ⬜ NOT STARTED

**Blocking:** Cannot accept PR with unsafe concurrency

**Resolution Steps:**
- [ ] Step 1: Design DiagnosticsEmitter with shared_mutex:
  ```cpp
  class DiagnosticsEmitter {
  private:
      mutable std::shared_mutex subscribers_lock_;
      std::vector<std::weak_ptr<DiagnosticListener>> subscribers_;
  
  public:
      void Subscribe(std::shared_ptr<DiagnosticListener> listener);
      void Unsubscribe(std::shared_ptr<DiagnosticListener> listener);
      void Emit(const DiagnosticEvent& event);
  private:
      void CleanDeadListeners();
  };
  ```
- [ ] Step 2: Implement Subscribe (acquire write lock, add to vector)
- [ ] Step 3: Implement Unsubscribe (acquire write lock, remove from vector)
- [ ] Step 4: Implement Emit (acquire read lock, notify all valid listeners, clean dead)
- [ ] Step 5: Add exception handling in Emit (catch listener exceptions, continue)
- [ ] Step 6: Document threading guarantees in header comments
- [ ] Step 7: Add RAII ListenerSubscription class for automatic unsubscribe

**Expected Completion:** 2026-08-15  
**Owner:** @[TBD - concurrency expert]  
**Sign-off:** @[TBD - tech lead]

**Testing:**
```cpp
// Must be included in DG-05 test:
TEST(UtilsDiagnosticsThreadSafety, ConcurrentSubscribeEmit) {
    // 10 threads subscribe, 10 threads emit, 10 threads unsubscribe
    // Expected: no crashes, no use-after-free, all listeners notified correctly
}
```

---

### [HIGH-003] Incomplete Incident Categorization Framework

**Status:** ⬜ NOT STARTED

**Blocking:** Cannot write test DG-06 without concrete categories

**Resolution Steps:**
- [ ] Step 1: Define IncidentCategory enum:
  ```cpp
  enum class IncidentCategory : uint8_t {
      kResourceExhaustion,       // Queue full, memory exhausted, thread limit
      kDependencyFailure,        // Crypto key derivation, PKI unavailable
      kPerformanceDegradation,   // Timeout, slow path triggered
      kDataIntegrity,            // Decompression bomb, invalid format
      kSecurityViolation,        // Signature verification failure, policy denial
      kConfigurationError,       // Invalid policy, missing plugin, malformed config
  };
  ```
- [ ] Step 2: Create error → category mapping (e.g., 7310→kDependencyFailure)
- [ ] Step 3: Create incident → SLA mapping:
  - ResourceExhaustion: P2 (2-hour response)
  - DependencyFailure: P1 (30-minute response)
  - PerformanceDegradation: P3 (4-hour response)
  - DataIntegrity: P1 (15-minute response)
  - SecurityViolation: P0 (immediate response)
  - ConfigurationError: P2 (2-hour response)
- [ ] Step 4: Add mapping table to DIAGNOSTICS_GUIDE.md
- [ ] Step 5: Verify all [7306-7349] error codes have category assignment
- [ ] Step 6: Create automated test DG-06 to validate mapping

**Expected Completion:** 2026-08-13  
**Owner:** @[TBD - ops/SRE liaison]  
**Sign-off:** @[TBD - product owner]

**Testing:**
```bash
# DG-06 test must verify:
# - All error codes have category assignment
# - No error code mapped to multiple categories
# - SLA implies incident severity
```

---

## MEDIUM SEVERITY FINDINGS - MUST ADDRESS IN PR

### [MED-001] Missing Diagnostic Message Format Template

**Status:** ⬜ NOT STARTED

**Expected Completion:** 2026-08-22  
**Owner:** @[TBD - developer]

**Resolution Steps:**
- [ ] Create DiagnosticMessageFormatter class
- [ ] Implement static format() method
- [ ] Define message templates for each error with required context keys
- [ ] Add validation: must contain action + all required context
- [ ] Include example messages in DIAGNOSTICS_GUIDE.md

---

### [MED-002] Documentation Drift Risk

**Status:** ⬜ NOT STARTED

**Expected Completion:** 2026-08-25  
**Owner:** @[TBD - documentation owner]

**Resolution Steps:**
- [ ] Version DIAGNOSTICS_GUIDE.md to match utils module (0.0.47)
- [ ] Add update trigger policy (error code addition → guide update required)
- [ ] Create automated sync test (verify all codes documented in guide)
- [ ] Define Prometheus metric schema with concrete queries
- [ ] Add metrics section to guide

---

### [MED-003] Insufficient Listener Lifecycle Testing

**Status:** ⬜ NOT STARTED

**Expected Completion:** 2026-08-20  
**Owner:** @[TBD - QA engineer]

**Resolution Steps:**
- [ ] Expand DG-04 test to cover multiple listeners
- [ ] Add test: listener exception doesn't crash emitter
- [ ] Add test: unsubscribe removes listener (RAII pattern)
- [ ] Add stress test: concurrent subscribe/emit/unsubscribe

---

### [MED-004] Compression Error Codes Not Aligned

**Status:** ⬜ NOT STARTED

**Expected Completion:** 2026-08-18  
**Owner:** @[TBD - compression module owner]

**Resolution Steps:**
- [ ] Decide: migrate 7000-7002 → 7320-7329 or coexist?
- [ ] Update compression codecs (lz4, zstd) to return new codes
- [ ] Document deprecation path in CHANGELOG
- [ ] Add migration guide for compression error handling

---

## LOW SEVERITY FINDINGS - NICE-TO-HAVE

### [LOW-001] Enum Naming Inconsistency

**Status:** ⬜ NOT STARTED

**Expected Completion:** 2026-08-09 (same as CRIT-001)  
**Owner:** @[TBD - lead developer]

**Resolution Steps:**
- [ ] Standardize on UtilsErrorCode (uint32_t)
- [ ] Create backward compat typedef: `using UtilsError = UtilsErrorCode;`

---

## OPEN QUESTIONS - REQUIRE STAKEHOLDER CLARIFICATION

### Q1: Listener Notification - Sync or Async?

**Status:** 🔴 REQUIRES STAKEHOLDER DECISION

**Clarifications Needed:**
- Should listeners be notified in background thread?
- Should events be queued if listener is slow?
- What backpressure strategy if listener queue overflows?

**Recommendation:** Add async listener support with bounded queue:
```cpp
class AsyncDiagnosticListener : public DiagnosticListener {
    AsyncDiagnosticListener(std::shared_ptr<DiagnosticListener> inner, size_t queue_size = 1000);
};
```

**Decision Required By:** 2026-08-12  
**Stakeholders:** @[TBD - ops], @[TBD - performance team]  
**Resolution Owner:** @[TBD - tech lead]

---

### Q2: Error Code Budget - Enough Codes?

**Status:** 🔴 REQUIRES STAKEHOLDER DECISION

**Issue:** Task allocates [7300-7349] (50 codes). Expansion is tight.

**Recommendation:** Allocate [7300-7399] (100 codes) with planned sub-ranges:
- [7300-7319]: Audit/Logging/Saga
- [7320-7339]: Privacy/PII
- [7340-7359]: Compression
- [7360-7379]: Concurrency/Rate Limiting
- [7380-7399]: Crypto/Key Management

**Decision Required By:** 2026-08-12  
**Stakeholders:** @[TBD - architect], @[TBD - error management owner]  
**Resolution Owner:** @[TBD - tech lead]

---

### Q3: Backward Compatibility - How to Handle Old Code?

**Status:** 🔴 REQUIRES STAKEHOLDER DECISION

**Issue:** Changing error codes breaks existing error handling code.

**Recommendation:** 3-phase approach:
1. Phase 1 (this task): Implement new DiagnosticsEmitter + codes [7306-7349]
2. Phase 2 (future): Add shim layer for old error codes
3. Phase 3: Deprecate old codes with 2-version notice

**Decision Required By:** 2026-08-12  
**Stakeholders:** @[TBD - product owner], @[TBD - maintenance team]  
**Resolution Owner:** @[TBD - tech lead]

---

## IMPLEMENTATION TIMELINE

```
Week 1 (Aug 12-16):
  Mon: CRIT-001 resolution + stakeholder clarification
  Tue: HIGH-002 design + HIGH-003 category definition
  Wed: HIGH-003 SLA mapping + test DG-06 design
  Thu: Begin HIGH-001 (audit logger integration)
  Fri: HIGH-001 (PII detection) + code review checkpoint

Week 2 (Aug 19-23):
  Mon: HIGH-001 (compression codecs integration)
  Tue: HIGH-001 (rate limiter) + HIGH-001 (thread pool)
  Wed: MED-001 (message formatter) + MED-004 (compression codes alignment)
  Thu: Write integration tests DG-01..06
  Fri: Load testing + code review checkpoint

Week 3 (Aug 26-30):
  Mon: MED-002 (documentation + automation)
  Tue: MED-003 (listener lifecycle tests)
  Wed: DIAGNOSTICS_GUIDE.md creation
  Thu: Backward compatibility validation
  Fri: Operator review + final refinements

Week 4 (Sep 2-6):
  Mon: Resolve operator feedback
  Tue: Final load testing (10k events/sec)
  Wed: Documentation finalization
  Thu: Pre-submission gate validation
  Fri: Code submission + merge
```

---

## SIGN-OFF CHECKLIST

**Pre-Implementation Sign-Off** (required before Week 1):
- [ ] Tech Lead: CRIT-001, HIGH-002, HIGH-003 resolved
- [ ] Architect: Q1, Q2, Q3 clarifications completed
- [ ] Ops/SRE: IncidentCategory + SLA mapping approved

**Code Submission Sign-Off** (required before Week 3):
- [ ] All 6 integration tests (DG-01..06) pass
- [ ] Error codes unique and non-overlapping
- [ ] Listener pattern thread-safe under load
- [ ] All diagnostic messages follow standard format
- [ ] Code review approval (2+ reviewers)

**Merge Sign-Off** (required before Week 4):
- [ ] DIAGNOSTICS_GUIDE.md complete and operator-reviewed
- [ ] Backward compatibility validated
- [ ] Load testing successful (10k events/sec)
- [ ] Zero regressions in full test suite
- [ ] Product owner sign-off

---

## METRICS & SUCCESS CRITERIA

**Code Quality Metrics:**
- ✓ All 9 findings addressed (0 open)
- ✓ 100% test coverage for new diagnostics infrastructure
- ✓ 0 compiler warnings
- ✓ 0 regressions in existing tests

**Performance Metrics:**
- ✓ Diagnostic emit latency < 1ms (p99)
- ✓ Listener notification latency < 5ms (p99)
- ✓ No memory growth under sustained load (100k events/sec)
- ✓ No listener queue overflow at 10k events/sec

**Documentation Metrics:**
- ✓ All error codes documented (100% coverage)
- ✓ All runbooks operator-tested
- ✓ All Prometheus queries validated
- ✓ Guide versioning automated

**Operator Satisfaction Metrics:**
- ✓ Operator usability review: >= 8/10
- ✓ Message clarity feedback: >= 90% actionable
- ✓ Runbook comprehensiveness: >= 95% scenarios covered

