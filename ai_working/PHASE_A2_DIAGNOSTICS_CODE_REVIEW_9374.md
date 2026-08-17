# CODE REVIEW: Tighten Diagnostics Consistency Across Utils Helpers (Phase A.2)

**Review Date:** 2026-08-08  
**Reviewer:** ThemisDB Code Review Specialist  
**Status:** Pre-implementation Review  
**Severity:** Multiple Findings (Critical→Low)

---

## FINDINGS ORDERED BY SEVERITY

### CRITICAL: Error Code Range Collision Risk

**Finding ID:** CRIT-001  
**Severity:** CRITICAL  
**Evidence:**

1. **Existing allocation in `utils_api_contract.h`:** Error codes [7300-7305] already defined:
   ```cpp
   enum class UtilsError : int32_t {
       kAuditOverflow      = 7300,
       kBatchRollback      = 7301,
       kBatchSizeExceeded  = 7302,
       kRetryExhausted     = 7303,
       kDeserInvalid       = 7304,
       kPoolExhausted      = 7305,
   };
   ```

2. **Task proposes overlapping allocation:** Spec requires [7300-7399] with sub-ranges:
   - Audit/Logging [7300-7309] ← **COLLISION with existing 7300-7305**
   - Privacy detection [7310-7319]
   - Compression [7320-7329]
   - Concurrency [7330-7339]
   - Crypto [7340-7349]

**Impact:**
- Duplicate error code definitions will cause **compilation failure**
- Breaks backward compatibility with existing `UtilsError` enum
- Cannot implement as specified without refactor

**Recommendation:**
1. **Extend existing `UtilsError` enum** in `utils_api_contract.h` rather than creating new taxonomy header
2. **Map new codes to existing ranges:**
   - Audit/Privacy → [7306-7319]
   - Compression → [7320-7329]
   - Concurrency → [7330-7339]
   - Crypto → [7340-7349]
3. **Update error_registry.h** to reserve [7306-7349] for expanded utils taxonomy
4. Document frozen allocation in API contract to prevent future collisions

---

### HIGH: Inconsistent Error Handling Patterns Across Helpers

**Finding ID:** HIGH-001  
**Severity:** HIGH  
**Evidence:**

1. **PII Detection Engine** (`src/utils/pii_detection_engine.cpp:72-100`):
   ```cpp
   spdlog::error("PluginSignature: Config hash mismatch. Expected: {}, Computed: {}", 
                 config_hash, computed_hash);
   // Returns false; does not emit diagnostic events
   ```
   - Error logged to spdlog **only**
   - No structured event emission
   - No operator-actionable context (no severity level, no incident category)

2. **Audit Logger** (`src/utils/audit_logger.cpp` header):
   - Uses CEF (Common Event Format)
   - **Not integrated with diagnostics listener pattern**
   - Designed for compliance logging, not operator observability

3. **Rate Limiter** (`include/utils/rate_limiter.h:38-77`):
   - No error codes exposed
   - No diagnostics on exhaustion
   - Silent failure on acquire timeout

4. **Compression Codecs** (lz4, zstd):
   - Returns empty vector on error (legacy API)
   - New safe API returns `Result<T>` but **no structured diagnostic events**
   - No incident categorization (buffer overflow vs. decompression bomb)

5. **Thread Pool Manager** (`include/utils/thread_pool_manager.h`):
   - `submit()` returns bool
   - No error context (queue full vs. shutdown)
   - No diagnostic broadcast on resource exhaustion

**Impact:**
- Operators cannot correlate diagnostics across utils helpers
- Incident response requires grepping logs for different formats
- No unified severity/category mapping for SLA tracking

**Recommendation:**
1. Define **DiagnosticEvent structure** with mandatory fields:
   ```cpp
   struct DiagnosticEvent {
       UtilsErrorCode code;           // [7300-7399]
       std::string module;             // "audit", "pii", "compression", "concurrency", "crypto"
       DiagnosticSeverity severity;    // CRITICAL, HIGH, MEDIUM, LOW
       IncidentCategory category;      // resource_exhaustion, degradation, etc.
       std::string action;             // Operator-actionable guidance
       std::string context;            // Context key=value pairs
       std::chrono::system_clock::time_point timestamp;
   };
   ```

2. Implement **DiagnosticsEmitter** with listener subscription (see MEDIUM-001)

3. Update **all 5 helpers** to emit diagnostic events at error sites

---

### HIGH: Thread-Unsafe Diagnostics Listener Pattern (if implemented naively)

**Finding ID:** HIGH-002  
**Severity:** HIGH  
**Evidence:**

Task specification shows:
```cpp
class DiagnosticsEmitter {
public:
    void Subscribe(std::shared_ptr<DiagnosticListener> listener);
    void Emit(const DiagnosticEvent& event);
};
```

**Problems with naive implementation:**

1. **No lock protection documented** for subscriber list
2. **Concurrency pattern undefined:**
   - Can `Subscribe` be called from multiple threads?
   - Can listeners be notified while new subscriptions arrive?
   - What if listener throws exception?

3. **Audit logger is high-volume** → diagnostics emitter will be called frequently from multiple threads

4. **Race conditions in listener lifecycle:**
   - Thread A: emits event to listener L1
   - Thread B: removes L1 from subscriber list
   - Thread A: → **use-after-free on L1**

**Impact:**
- Data corruption in diagnostics metadata
- Crashed operators miss critical events
- Non-deterministic test failures

**Recommendation:**
1. Use **reader-writer lock** for subscriber list:
   ```cpp
   class DiagnosticsEmitter {
   private:
       mutable std::shared_mutex subscribers_lock_;
       std::vector<std::weak_ptr<DiagnosticListener>> subscribers_;
   
   public:
       void Subscribe(std::shared_ptr<DiagnosticListener> listener);
       void Emit(const DiagnosticEvent& event);
   };
   ```

2. **Clean dead listeners** during emit (weak_ptr check)

3. Document threading guarantee: "All methods are thread-safe. Listeners are notified asynchronously; listeners MUST be thread-safe."

4. Add integration test DG-05 that validates concurrent emits + subscriptions

---

### HIGH: Incomplete Incident Categorization Framework

**Finding ID:** HIGH-003  
**Severity:** HIGH  
**Evidence:**

Task lists incident categories without definition:
```
"Define incident categories (resource_exhaustion, degradation, external_dependency_loss, etc.)"
```

**Problems:**

1. **No enum defined** in spec
2. **No mapping document** showing which utils errors → which categories
3. **Vague category names:**
   - "degradation" — performance? functionality?
   - "external_dependency_loss" — crypto validation? PII detection timeout?

4. **No SLA implications** — operators don't know which incidents warrant escalation

5. **Test DG-06 cannot be written** until categories are concrete

**Impact:**
- Operators cannot filter for critical incidents
- SLA dashboards cannot be built
- Incident response runbooks incomplete

**Recommendation:**
1. Define **IncidentCategory enum**:
   ```cpp
   enum class IncidentCategory : uint8_t {
       kResourceExhaustion,       // Queue full, memory exhausted, thread limit hit
       kDependencyFailure,        // Crypto key derivation, PKI unavailable
       kPerformanceDegradation,   // Timeout, slow path triggered
       kDataIntegrity,            // Decompression bomb, invalid format
       kSecurityViolation,        // Signature verification failure
       kConfigurationError,       // Invalid policy, missing plugin
   };
   ```

2. **Create mapping table** in diagnostics_guide.md:
   ```
   Code  | Module      | Category                | SLA Impact
   ------|-------------|-------------------------|-------------
   7310  | pii         | DependencyFailure       | P1 (30 min)
   7320  | compression | DataIntegrity           | P1 (15 min)
   7330  | rate_limit  | ResourceExhaustion      | P2 (2 hr)
   7340  | crypto      | SecurityViolation       | P1 (15 min)
   ```

3. Update DG-06 test to validate **all codes have entries** in mapping

---

### MEDIUM: Missing Diagnostic Message Format Template

**Finding ID:** MED-001  
**Severity:** MEDIUM  
**Evidence:**

Task specifies message format:
```
"[<SEVERITY>] <MODULE>.<ERROR_CODE>: <ACTION> | <CONTEXT>"
```

But provides no:
1. **Helper class/function** to construct messages
2. **Validation that messages are operator-actionable**
3. **Examples for each error code**
4. **Rules for context key-value formatting**

Current code uses free-form spdlog:
```cpp
spdlog::error("PluginSignature: Config hash mismatch. Expected: {}, Computed: {}", 
             config_hash, computed_hash);
```

This is **not operator-actionable** — operators must read code to understand fix.

**Impact:**
- Messages inconsistent across helpers
- Difficult to parse in log aggregation (Splunk, ELK)
- Operators cannot search by error code

**Recommendation:**
1. Create **DiagnosticMessageFormatter**:
   ```cpp
   class DiagnosticMessageFormatter {
   public:
       static std::string format(
           DiagnosticSeverity severity,
           std::string_view module,
           UtilsErrorCode code,
           std::string_view action,
           const std::unordered_map<std::string, std::string>& context
       );
   };
   
   // Usage:
   auto msg = DiagnosticMessageFormatter::format(
       DiagnosticSeverity::ERROR,
       "pii",
       UtilsErrorCode::kPrivacyInvalidInput,
       "Cannot scan input; input exceeds max size",
       {{"max_size", "1000"}, {"actual_size", "1500"}}
   );
   // Output: "[ERROR] pii.7312: Cannot scan input; input exceeds max size | max_size=1000 actual_size=1500"
   ```

2. Define **message templates** in `diagnostic_messages.h`:
   ```cpp
   namespace messages {
       struct Template {
           std::string action;
           std::vector<std::string> required_context_keys;
       };
       
       static const Template kPrivacyInvalidInput{
           "Cannot scan input; exceeds maximum size",
           {"max_size", "actual_size"}
       };
   }
   ```

3. **Add validation test:** Message must contain action + all required context keys

---

### MEDIUM: Documentation Drift Risk — Diagnostics Guide Not Versioned

**Finding ID:** MED-002  
**Severity:** MEDIUM  
**Evidence:**

Task deliverable: "docs/utils/DIAGNOSTICS_GUIDE.md" with:
- Common failure scenarios
- Diagnostic message meanings
- Runbook for common failures
- Prometheus metric queries

**Problem:**

1. **No version field** — how do operators know if guide matches their build?
2. **No update trigger policy** — when must guide be updated?
3. **No automated sync test** — code and docs can drift (error codes added but guide not updated)
4. **No metrics defined** — "Prometheus metric queries" section incomplete

**Impact:**
- Operators follow outdated runbooks
- Troubleshooting steps don't match error codes
- On-call team wastes time cross-referencing commits

**Recommendation:**
1. Add **version field to guide** that matches code:
   ```markdown
   # ThemisDB Utils Diagnostics Guide
   Version: 0.0.47 (matches utils module maturity version)
   Last Updated: 2026-08-08
   Build Compatibility: v2.x.x and later
   ```

2. Create **automated sync test** (DG-03 enhancement):
   ```cpp
   TEST(UtilsDiagnosticsGuide, AllErrorCodesDocumented) {
       std::ifstream guide("docs/utils/DIAGNOSTICS_GUIDE.md");
       std::string content((std::istreambuf_iterator<char>(guide)),
                          std::istreambuf_iterator<char>());
       
       for (auto code : kAllUtilsErrorCodes) {
           EXPECT_THAT(content, HasSubstr(fmt::format("{}.", static_cast<int>(code))))
               << fmt::format("Error code {} not documented in guide", static_cast<int>(code));
       }
   }
   ```

3. Define **metrics schema** in guide:
   ```markdown
   ### Prometheus Metrics
   
   - `themis_utils_diagnostic_events_total{module,severity}` — diagnostic events emitted
   - `themis_utils_error_rate{module,error_code}` — errors per module
   - `themis_audit_queue_utilization` — audit log queue fill %
   - `themis_pii_scan_timeout_total` — PII scan timeouts
   ```

---

### MEDIUM: Test Coverage Gap — No Listener Lifecycle Tests

**Finding ID:** MED-003  
**Severity:** MEDIUM  
**Evidence:**

Task specifies test DG-04:
```
"Listener subscription and emission"
- Diagnostics can be subscribed to and emitted correctly
```

But missing test cases for:
1. **Multiple listeners** — do all receive events?
2. **Listener removal** — can listeners be unsubscribed?
3. **Listener exceptions** — what if listener throws?
4. **Emitter shutdown** — what if emitter is destroyed while emitting?

**Impact:**
- Listener pattern has latent bugs
- Production outages when listener crashes
- Memory leaks from unremoved listeners

**Recommendation:**
1. Expand DG-04 test suite:
   ```cpp
   TEST(UtilsDiagnosticsListeners, MultipleListenersReceiveEvents) {
       auto emitter = std::make_shared<DiagnosticsEmitter>();
       auto listener1 = std::make_shared<MockDiagnosticListener>();
       auto listener2 = std::make_shared<MockDiagnosticListener>();
       
       emitter->Subscribe(listener1);
       emitter->Subscribe(listener2);
       
       DiagnosticEvent event{/*.code=*/UtilsErrorCode::kAuditQueueFull};
       emitter->Emit(event);
       
       EXPECT_CALL(*listener1, OnDiagnostic).Times(1);
       EXPECT_CALL(*listener2, OnDiagnostic).Times(1);
   }
   
   TEST(UtilsDiagnosticsListeners, ListenerExceptionDoesNotCrashEmitter) {
       auto emitter = std::make_shared<DiagnosticsEmitter>();
       auto bad_listener = std::make_shared<BadListener>();  // throws
       auto good_listener = std::make_shared<MockDiagnosticListener>();
       
       emitter->Subscribe(bad_listener);
       emitter->Subscribe(good_listener);
       
       EXPECT_NO_THROW({
           DiagnosticEvent event{/*.code=*/UtilsErrorCode::kPrivacyScanTimeout};
           emitter->Emit(event);
       });
       
       // good_listener should still receive event despite bad_listener
       EXPECT_CALL(*good_listener, OnDiagnostic).Times(1);
   }
   ```

2. Add unsubscribe capability:
   ```cpp
   class DiagnosticsEmitter {
   public:
       std::shared_ptr<ListenerSubscription> Subscribe(std::shared_ptr<DiagnosticListener> listener);
   };
   
   // Usage:
   auto sub = emitter->Subscribe(listener);
   // ... use ...
   sub->Unsubscribe();  // RAII removes listener
   ```

---

### MEDIUM: Compression Error Codes Not Aligned with Implementation

**Finding ID:** MED-004  
**Severity:** MEDIUM  
**Evidence:**

1. **Existing compression codes** in `error_registry.h`:
   ```cpp
   ERR_COMPRESSION_FAILED = 7000,
   ERR_COMPRESSION_BUFFER_TOO_SMALL = 7001,
   ERR_COMPRESSION_INVALID_FORMAT = 7002,
   ```

2. **Task proposes new range** [7320-7329]:
   ```cpp
   kCompressionBufTooSmall = 7320,
   kCompressionDecompFailed = 7321,
   kCompressionBomb = 7322,
   ```

3. **Overlapping semantics:**
   - `7001 (BUFFER_TOO_SMALL)` vs `7320 (kCompressionBufTooSmall)` — same error?
   - `7002 (INVALID_FORMAT)` vs `7321 (kCompressionDecompFailed)` — related?

**Impact:**
- Compression code migration unclear
- Old code may use 7000-7002; new code uses 7320-7329
- Operators confused by duplicate error meanings

**Recommendation:**
1. **Reuse existing codes 7320-7329** for fine-grained compression errors
2. **Deprecate codes 7000-7002** with clear migration path
3. Update compression codecs (lz4, zstd) to return new error codes from safe APIs
4. Add migration note in CHANGELOG

---

### LOW: UtilsError Enum Naming Inconsistency

**Finding ID:** LOW-001  
**Severity:** LOW  
**Evidence:**

Existing enum in `utils_api_contract.h`:
```cpp
enum class UtilsError : int32_t {
    kAuditOverflow,      // "k" prefix (Google C++ style)
    kBatchRollback,
    ...
};
```

Task proposes:
```cpp
enum class UtilsErrorCode : uint32_t {
    kAuditQueueFull,     // "k" prefix (consistent)
    kPrivacyScanTimeout,
    ...
};
```

**Problems:**
1. **Naming inconsistency:** `UtilsError` vs `UtilsErrorCode`
2. **Type inconsistency:** `int32_t` vs `uint32_t`
3. **Value consistency:** Both use "k" prefix (good)

**Impact:**
- API consumers must remember two enum names
- Type mismatch in error comparisons
- Low code maintainability

**Recommendation:**
1. **Standardize on single enum name:** `UtilsErrorCode` (more semantic)
2. **Use uint32_t** (error codes should not be negative)
3. Update `utils_api_contract.h` to rename `UtilsError` → `UtilsErrorCode`
4. Create typedef for backward compatibility: `using UtilsError = UtilsErrorCode;`

---

## OPEN QUESTIONS & ASSUMPTIONS

### Q1: Diagnostics Listener Broadcast — Sync vs Async?
**Issue:** If audit logger emits events at high frequency (100s/sec), synchronous listener notification could block compression/PII pipelines.

**Assumption:** Spec implies synchronous notification. Need clarification on:
- Should listeners be notified in background thread?
- Should events be queued if listener is slow?
- What backpressure strategy if listener queue overflows?

**Recommendation:** Add async listener support with bounded queue:
```cpp
class AsyncDiagnosticListener : public DiagnosticListener {
    AsyncDiagnosticListener(std::shared_ptr<DiagnosticListener> inner, size_t queue_size = 1000);
    // Events queued and notified async; overflow triggers circuit breaker
};
```

---

### Q2: Error Code Allocation — Should We Reserve More?
**Issue:** Task allocates [7300-7349] for utils (50 codes). Current expansion already tight:
- Audit/Logging: 6 codes → 10 (4 new)
- Privacy: 0 → 10 (10 new)
- Compression: 3 existing → 10 total (7 new)
- Concurrency: 0 → 10 (10 new)
- Crypto: 0 → 10 (10 new)

**Risk:** Future features (e.g., vector caching, compression bomb detection v2) may need codes.

**Recommendation:** Allocate [7300-7399] (100 codes) with plan to split ranges as features mature:
- [7300-7319]: Audit/Logging/Saga
- [7320-7339]: Privacy/PII
- [7340-7359]: Compression
- [7360-7379]: Concurrency/Rate Limiting
- [7380-7399]: Crypto/Key Management

---

### Q3: Backward Compatibility — How to Handle Existing Code?
**Issue:** Audit logger and other helpers already deployed. Changing error codes breaks existing error handling code.

**Assumption:** Task assumes new infrastructure can coexist with old.

**Recommendation:**
1. **Phase 1 (this task):** Implement new `DiagnosticsEmitter` + error codes [7306-7349]
2. **Phase 2 (future):** Add shim layer that emits new events when old error codes hit
3. **Phase 3:** Deprecate old error codes with 2-version notice period

Example shim:
```cpp
void legacy_emit_error(errors::ErrorCode old_code, const std::string& msg) {
    auto new_code = map_legacy_to_utils_error_code(old_code);
    auto event = DiagnosticEvent{
        .code = new_code,
        .severity = infer_severity(old_code),
        ...
    };
    GlobalDiagnosticsEmitter::instance()->Emit(event);
}
```

---

## CHANGE SUMMARY

**Scope:** Expand existing utils error taxonomy ([7300-7305]) to ([7300-7349]) with new infrastructure

**Key Changes Required:**

1. **File: `include/utils/utils_api_contract.h`**
   - Extend `UtilsError` enum → `UtilsErrorCode` with new codes [7306-7349]
   - Update documentation and contract

2. **New File: `include/utils/diagnostic_event.h`**
   - `DiagnosticEvent` struct
   - `IncidentCategory` enum
   - `DiagnosticSeverity` enum

3. **New File: `include/utils/diagnostics_emitter.h`**
   - `DiagnosticListener` interface
   - `DiagnosticsEmitter` class (thread-safe)
   - Global singleton accessor

4. **New File: `include/utils/diagnostic_messages.h`**
   - `DiagnosticMessageFormatter` class
   - Message templates for each error

5. **Updated: `src/utils/` (5 helpers)**
   - Audit logger: emit events on queue full, write failure
   - PII engine: emit events on timeout, invalid input, regex error
   - Compression (lz4/zstd): emit events on buffer too small, decompression failure, bomb detected
   - Rate limiter: emit events on exhaustion
   - Thread pool: emit events on queue full, shutdown timeout

6. **New File: `tests/utils/test_utils_interfaces.cpp` (extend)**
   - Add diagnostics section with tests DG-01 through DG-06

7. **New File: `docs/utils/DIAGNOSTICS_GUIDE.md`**
   - Error code reference
   - Common failure scenarios
   - Runbooks and metric queries

---

## SUGGESTED VALIDATION ADDITIONS

### Pre-Implementation Validation
- [ ] Confirm error code allocation doesn't conflict with other modules
- [ ] Review with operator oncall team for actionability feedback
- [ ] Prototype async listener pattern with audit logger (high-volume test)

### Post-Implementation Validation
- [ ] Load test: emit 10k diagnostics/sec for 1 minute, verify no corruption
- [ ] Listener stress test: 100 listeners all receiving events simultaneously
- [ ] Documentation review: example runbook for each error category with ops team
- [ ] Backward compatibility: verify legacy error codes still work with shim layer
- [ ] Metrics validation: confirm Prometheus queries in guide work

### Acceptance Checklist
- [ ] All 6 integration tests (DG-01..06) pass
- [ ] Error codes consolidated in single location (utils_api_contract.h)
- [ ] All diagnostic messages follow standard format
- [ ] Listener pattern thread-safe under concurrent load
- [ ] All error codes documented in DIAGNOSTICS_GUIDE.md
- [ ] Zero regressions in existing test suite

---

## RECOMMENDATIONS FOR IMPLEMENTATION ORDER

1. **Week 1:** Resolve Critical findings
   - Consolidate error codes into utils_api_contract.h
   - Fix HIGH-002 (thread-safe listener implementation)
   - Define HIGH-003 (incident categories)

2. **Week 2:** Implement core infrastructure
   - Create `diagnostic_event.h`, `diagnostics_emitter.h`, `diagnostic_messages.h`
   - Implement `DiagnosticsEmitter` with thread-safe listener pattern
   - Global singleton accessor

3. **Week 3:** Integrate with helpers
   - Update all 5 helpers (audit, PII, compression, rate limiter, thread pool)
   - Emit events at error sites
   - Add context to all messages

4. **Week 4:** Testing & Documentation
   - Write integration tests DG-01..06
   - Write diagnostics guide
   - Load testing and backward compatibility validation

---

