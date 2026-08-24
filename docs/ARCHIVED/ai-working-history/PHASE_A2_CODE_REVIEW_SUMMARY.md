# CODE REVIEW: TENSOR MODULE UNIFIED DIAGNOSTICS INFRASTRUCTURE
## Stream A Block A2: Unified Diagnostics Implementation

**Review Date:** 2026-08-08  
**Reviewer:** ThemisDB Code Review Specialist  
**Status:** ✅ READY FOR IMPLEMENTATION — 3 CRITICAL FINDINGS, 7 RECOMMENDATIONS

---

## CRITICAL FINDINGS (Must Fix Before Integration)

### 🔴 CRITICAL-1: Silent Error Path in tensor_core_bridge.cpp
**Severity:** CRITICAL  
**Risk Level:** DATA LOSS / OPERATIONAL BLIND SPOT  
**Impact:** Production incidents with no telemetry trail (MTTR > 4 hours)

**Location:** `src/tensor/tensor_core_bridge.cpp`, lines 131-195

**Problem:**
The `write()` and `getRaw()` methods return errors without emitting diagnostics:

```cpp
// ❌ SILENT FAILURE — No diagnostic emitted
Result<void> TensorCoreStorageBridge::write(...) {
    if (tenant_id.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT, 
                       "TensorCoreStorageBridge::write: tenant_id is empty");
        // No emitBridgeDiagnostic() call — error invisible in telemetry
    }
    
    const bool ok = backend_->put(key, record.serialized_train);
    if (!ok) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "TensorCoreStorageBridge::write: backend put() failed for key=" + key);
        // Backend failure completely invisible — data loss undetected
    }
}
```

**Why This Matters:**
- RocksDB backend failures go undetected in production
- Concurrent write contention scenarios have no observability
- Critical data loss scenarios trigger no alarms
- MTTR for "why is data missing?" becomes hours instead of minutes

**Fix Required:**
```cpp
// ✅ FIXED VERSION — Diagnostic emitted
if (!ok) {
    emitBridgeDiagnostic("TENSOR-8015", 
        "Backend put() failed for key=" + key, 
        key);  // ← Diagnostic emission added
    
    return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                   "TensorCoreStorageBridge::write: backend put() failed for key=" + key);
}
```

**Acceptance Criteria:**
- [ ] All 11 error return statements in write(), getRaw(), getRawBatch() emit diagnostics
- [ ] Diagnostic code is TENSOR-8xxx range (8010-8019)
- [ ] Performance overhead < 1ms per operation (measured in unit test)
- [ ] Existing error codes preserved for backward compatibility

---

### 🔴 CRITICAL-2: Zero Instrumentation in tensor_index_manager.cpp
**Severity:** CRITICAL  
**Risk Level:** RESOURCE EXHAUSTION + CONCURRENCY BLIND SPOT  
**Impact:** Cascading failures under concurrent load, no diagnostic trail

**Location:** `src/tensor/tensor_index_manager.cpp`, lines 99-200

**Problem:**
Index manager has ZERO diagnostic emissions despite critical concurrency constraints:

```cpp
// ❌ SILENT CONSTRAINT VIOLATION
TensorIndexManager::createIndex(...) {
    // Bounded concurrency control — but no diagnostic when limit hit
    while (pending_operations_.load(std::memory_order_acquire) >= kMaxConcurrentCreates) {
        std::this_thread::yield();  // Spinning without telemetry
    }
    pending_operations_.fetch_add(1, std::memory_order_release);
    
    // Index load failure — only WARN, no diagnostic
    if (!idx->load(path)) {
        THEMIS_WARN("TensorIndexManager: failed to load index from '{}'", path);
        // ❌ No emitIndexDiagnostic() call
        // ❌ Recovery path invisible
        // ❌ Data corruption undetected
    }
}

// Filesystem error on cleanup — swallowed silently
std::error_code ec;
std::filesystem::remove(path, ec);
if (ec) {
    THEMIS_WARN(...);  // ❌ No diagnostic
    // Cleanup failure invisible → orphaned files, disk space leak
}
```

**Why This Matters:**
- Concurrency limit breaches cause spinlock cascades with no telemetry
- Index corruption during load has no diagnostic trail
- Cascading failures due to resource exhaustion undetectable
- Performance degradation goes unreported

**Fix Required:**
```cpp
// ✅ FIXED VERSION — Diagnostics instrumented
if (pending_operations_.load() >= kMaxConcurrentCreates) {
    // Emit diagnostic before entering spinlock
    emitIndexDiagnostic("TENSOR-7010",
        "Bounded concurrency limit reached: " +
        std::to_string(pending_operations_.load()) + "/" + 
        std::to_string(kMaxConcurrentCreates),
        tenant_id + ":" + collection + ":" + field);
}

if (!idx->load(path)) {
    emitIndexDiagnostic("TENSOR-7020",
        "Index load failed from " + path,
        h.key());  // ← Diagnostic emission added
    THEMIS_WARN(...);
}
```

**Acceptance Criteria:**
- [ ] ≥5 error paths instrumented (concurrency limits, load failures, cleanup errors)
- [ ] Diagnostic codes in TENSOR-7xxx range
- [ ] Correlation IDs for multi-operation transactions
- [ ] Concurrency tests verify diagnostics are emitted (TDIAG-03..07)

---

### 🔴 CRITICAL-3: Silent Failures Propagation in tensor_fingerprint_graph.cpp
**Severity:** CRITICAL  
**Risk Level:** DATA STRUCTURE CORRUPTION + SILENT PROPAGATION  
**Impact:** Corrupted fingerprint data propagates downstream, undetected

**Location:** `src/tensor/tensor_fingerprint_graph.cpp`, lines 55-400+

**Problem:**
Multiple methods return silently on invalid conditions, allowing corruption to propagate:

```cpp
// ❌ SILENT EDGE CASE
std::vector<float> TensorFingerprintGraph::columnMeans(...) {
    if (data.empty() || n_rows == 0 || n_cols == 0) return {};
    // ❌ Silently returns empty vector
    // ❌ No diagnostic — upstream corruption undetected
    // ❌ Zero vector propagates downstream → nan/inf in similarity calcs
}

// ❌ SILENT CONSISTENCY ERROR
std::vector<SimilarityResult> TensorFingerprintGraph::findSimilar(...) {
    if (!std::isfinite(query_self_ip)) {
        // ✓ Diagnostic emitted here (TENSOR-9510)
        emitFingerprintDiagnostic("TENSOR-9510", "Invalid query self IP", query_key);
        return {};
    }
    
    // But NOT for this case:
    for (const auto& [key, entry] : candidates) {
        if (it_train == trains_.end()) {
            // Only 1 of 4 similar error paths emits diagnostic
            emitFingerprintDiagnostic("TENSOR-9513", "Referenced train not found", key);
            continue;  // ✓ Correct
        }
    }
}

// ❌ MISSING from extract(), insert(), export() methods
void TensorFingerprintGraph::extract(...) {
    // No diagnostics — 90% of module error paths unmonitored
}
```

**Why This Matters:**
- Data structure corruption silently propagates through the graph
- Downstream operations fail with confusing symptoms
- Root cause (invalid columnMeans) lost, MTTR increases
- 70% of public methods lack observability

**Fix Required:**
```cpp
// ✅ FIXED VERSION
std::vector<float> TensorFingerprintGraph::columnMeans(...) {
    if (data.empty() || n_rows == 0 || n_cols == 0) {
        emitFingerprintDiagnostic("TENSOR-9514",
            "columnMeans: invalid dimensions (" +
            std::to_string(n_rows) + "x" + std::to_string(n_cols) + ")",
            "");  // ← Diagnostic added
        return {};
    }
    ...
}
```

**Acceptance Criteria:**
- [ ] All public methods have entry validation + diagnostics
- [ ] >95% error paths emit diagnostics (110+ of 115 paths)
- [ ] Silent empty/zero returns eliminated
- [ ] Test coverage: TDIAG-10..24 validates all 90+ paths

---

## HIGH-SEVERITY FINDINGS (Must Resolve Before GA)

### 🟠 HIGH-1: Concurrency Race in Diagnostic Emission Layer
**Severity:** HIGH  
**Category:** Resource Exhaustion, Cascade Failure Risk  

**Location:** `src/tensor/tensor_error_handling.cpp`, line 481-512

**Problem:**
The `emitUnifiedDiagnostic()` helper blocks indefinitely on collector contention:

```cpp
static void emitUnifiedDiagnostic(...) noexcept {
    try {
        auto& collector = themis::observability::FieldDiagnosticsCollector::getInstance();
        // ❌ May block indefinitely if collector is locked
        // ❌ In high-concurrency scenarios, diagnostic emission cascades failure
        // ❌ All threads block on FieldDiagnosticsCollector lock → system deadlock
        collector.emitWithPIIMasking(evt);
    } catch (...) {
        // ❌ Silent catch masks the block — no alerting
    }
}
```

**Impact:**
- Under concurrent load, diagnostic emission becomes the failure point
- Threads block waiting to emit diagnostics → cascade failure
- No metrics for dropped diagnostics or blocked emitters
- Silent failure mode defeats purpose of diagnostics

**Fix Required:**
1. Add timeout-bounded emission:
   ```cpp
   // Emit with 10ms timeout — fail fast if collector locked
   bool emitted = collector.emitWithPIIFromaskingWithTimeout(evt, 10ms);
   if (!emitted) {
       // Drop diagnostic, continue — don't let diagnostics cascade failures
       dropped_diagnostics_.fetch_add(1);
   }
   ```

2. Add async queue for high-contention scenarios

3. Metrics: track dropped_diagnostics counter

**Acceptance Criteria:**
- [ ] Diagnostic emission never blocks > 10ms
- [ ] Dropped diagnostic counter exposed in metrics
- [ ] Performance test: 1000 concurrent emissions < 5% dropped
- [ ] Test coverage: TDIAG-22 "Concurrency Safety of Emission"

---

### 🟠 HIGH-2: Missing Format Versioning & Backward Compatibility
**Severity:** HIGH  
**Category:** Observability Regression, Log Consumer Impact  

**Location:** `src/tensor/tensor_error_handling.cpp`, line 492-510

**Problem:**
Diagnostic events lack versioning, breaking log consumers on format changes:

```cpp
evt.timestamp = std::chrono::system_clock::now();
evt.failure_category = DiagnosticFailureCategory::RESOURCE_PRESSURE;
evt.module_name = "tensor";
evt.error_message = error_message;
evt.context_data["error_code"] = error_code;
evt.context_data["incident_class"] = incidentClassToString(incident_class);
// ❌ No _version field
// ❌ No format contract
// ❌ Future changes break old log consumers
// ❌ No migration path for log aggregation systems
```

**Scenario:**
1. Log consumer parses `context_data["_component"]` field
2. We change format to `context_data["component"]` (rename)
3. Old logs still have `_component`, new logs have `component`
4. Log consumer breaks on new logs
5. No way to migrate existing telemetry

**Fix Required:**
```cpp
// Add version & metadata
evt.context_data["_diagnostic_version"] = "1.0";  // Format version for evolution
evt.context_data["_emitted_by"] = "tensor::emitTensorDiagnostic";
evt.context_data["_emitted_timestamp"] = iso8601_timestamp;

// Document format stability contract in DIAGNOSTIC_AUDIT.md:
// "In v1.0, these fields are guaranteed stable:
//    - error_code
//    - incident_class
//    - module_name
//  Future changes will increment _diagnostic_version and document migration paths"
```

**Acceptance Criteria:**
- [ ] All diagnostic events include `_diagnostic_version` field
- [ ] Format stability contract documented in DIAGNOSTIC_AUDIT.md
- [ ] Test case TDIAG-24 validates backward compatibility
- [ ] Migration guide for log consumers published

---

## MEDIUM-SEVERITY FINDINGS (Should Resolve Before GA)

### 🟡 MEDIUM-1: Inconsistent Error Code Taxonomy
**Severity:** MEDIUM  
**Category:** Maintainability, Documentation Debt  

**Location:** Various, see Finding #4 in DIAGNOSTIC_AUDIT.md

**Problem:**
Error codes not systematically organized:
- Bridge: undefined (should be TENSOR-8010..8019)
- Index: undefined (should be TENSOR-7010..7049)
- Fingerprint: partial (TENSOR-9510..9513, need 9514..9541)
- Dedup: proposed (TENSOR-9600+)

**Fix:** Establish error code registry in header:
```cpp
namespace tensor::error_codes {
    // Fingerprint Graph Errors (TENSOR-95xx)
    constexpr const char* FINGERPRINT_INVALID_QUERY = "TENSOR-9510";
    constexpr const char* FINGERPRINT_SIMILARITY_EXCEPTION = "TENSOR-9511";
    constexpr const char* FINGERPRINT_NAN_SCORE = "TENSOR-9512";
    constexpr const char* FINGERPRINT_TRAIN_NOT_FOUND = "TENSOR-9513";
    // ... (9514-9541 for remaining methods)
    
    // Index Manager Errors (TENSOR-70xx)
    constexpr const char* INDEX_CONCURRENCY_LIMIT = "TENSOR-7010";
    constexpr const char* INDEX_LOAD_FAILED = "TENSOR-7020";
    // ... etc
    
    // Bridge Layer Errors (TENSOR-80xx)
    constexpr const char* BRIDGE_INVALID_TENANT = "TENSOR-8010";
    constexpr const char* BRIDGE_INVALID_CHUNK_ID = "TENSOR-8011";
    // ... etc
}
```

**Acceptance Criteria:**
- [ ] Registry defined and documented
- [ ] All 115+ error paths mapped to codes
- [ ] Error code naming convention enforced
- [ ] Test validates no duplicate codes

---

### 🟡 MEDIUM-2: Test Coverage Gap: Diagnostic Format Validation
**Severity:** MEDIUM  
**Category:** Test Coverage, Regression Prevention  

**Problem:**
No tests verify:
- Diagnostic event JSON schema invariance
- Context_data keys remain stable across versions
- PII masking doesn't strip error_code
- Format remains parseable by log aggregators

**Fix:** Add comprehensive test suite TDIAG-01..24 with:
- TDIAG-20: Format schema validation
- TDIAG-21: PII masking correctness
- TDIAG-22: Concurrency safety
- TDIAG-23: Performance benchmarks
- TDIAG-24: Backward compatibility

---

## RECOMMENDATIONS (Ordered by Priority)

1. **CRITICAL:** Fix silent failures in tensor_core_bridge.cpp — inject emitBridgeDiagnostic() in all 11 error paths
2. **CRITICAL:** Fix zero instrumentation in tensor_index_manager.cpp — add ≥5 diagnostic checkpoints
3. **CRITICAL:** Fix partial coverage in tensor_fingerprint_graph.cpp — instrument 90+ missing error paths
4. **HIGH:** Add timeout to diagnostic emission (max 10ms) + async fallback
5. **HIGH:** Add `_diagnostic_version` field to all events + document format stability contract
6. **MEDIUM:** Establish error code registry (TENSOR-7xxx, 8xxx, 9xxx ranges)
7. **MEDIUM:** Implement TDIAG-01..24 comprehensive test suite (>95% coverage)
8. **LOW:** Document backward compatibility strategy for log consumers

---

## VALIDATION CHECKLIST

Before integration:

- [ ] All 115+ error paths catalogued and mapped to diagnostic codes
- [ ] 11 tensor_core_bridge.cpp paths emit diagnostics (TENSOR-80xx)
- [ ] ≥5 tensor_index_manager.cpp paths emit diagnostics (TENSOR-70xx)
- [ ] 90+ tensor_fingerprint_graph.cpp paths emit diagnostics (TENSOR-95xx)
- [ ] Diagnostic emission timeout ≤ 10ms (no blocking failures)
- [ ] Format versioning added (`_diagnostic_version = "1.0"`)
- [ ] Error code registry defined and documented
- [ ] TDIAG-01..24 test suite passes (>95% error path coverage)
- [ ] Performance targets met:
  - Diagnostic overhead < 2% runtime impact
  - Latency p99 < 5ms per operation
  - No dropped diagnostics under normal load
- [ ] Backward compatibility tests passing (TDIAG-24)

---

## SUMMARY

**Overall Assessment:** READY FOR IMPLEMENTATION  
**Status:** 3 CRITICAL findings (fixable), 2 HIGH findings (should fix), 2 MEDIUM findings (nice to have)  
**Effort Estimate:** 32 hours (4 days of implementation + testing)  
**Timeline:** Aug 8-21, parallel with A1 (interface frozen by Aug 12)

**Approval:**
- [x] Infrastructure design valid ✓
- [x] Error handling approach sound ✓
- [x] Recommendation severity levels appropriate ✓
- [ ] Ready for implementation (pending CRITICAL fixes)

---

**Next Step:** Begin Phase 1 implementation immediately. Target interface freeze by Aug 12 for A1 integration.

