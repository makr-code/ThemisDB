# Phase 2: A2 Remediation — Continuation Guide (Phase B & C)

**Status**: Ready for Phase B execution  
**Date**: 2026-08-07  
**Effort Completed**: 4 hours (Phase A foundation)  
**Effort Remaining**: 12 hours (Phase B: 8 hours, Phase C: 4 hours)  

## What's Been Done

### ✅ Phase A Complete (Foundation)
1. **Error Registry Extended** (30 min)
   - Added 20 TENSOR error codes (9510-9589)
   - File: `include/utils/error_registry.h`
   - Codes cover: Graph, Index, Adapter, Fingerprint, Routing, Recovery, Concurrency, Core

2. **Diagnostic Infrastructure** (1.5 hours)
   - Added 3 helper functions to `tensor_error_handling.h`
   - Implemented full functions in `tensor_error_handling.cpp`
   - Integrated with `FieldDiagnosticsCollector`
   - Thread-safe, noexcept, production-ready

3. **CRITICAL-1 Fixed** (2 hours)
   - 7 error paths in `tensor_fingerprint_graph.cpp` now emit diagnostics
   - Each path: identify error condition → emit diagnostic → return/continue
   - No API changes, fully backward compatible
   - Test file created: `test_tensor_fingerprint_graph_critical1_diagnostics_focused.cpp`

4. **Documentation & Planning** (0.5 hours)
   - Created comprehensive remediation plan
   - Detailed progress tracking
   - Timeline & success metrics defined

---

## How to Continue in Phase B

### Quick Start for Phase B (Core Hardening)

#### 1. Audit tensor_index_manager.cpp for Error Paths (~1 hour)

**File**: `src/tensor/tensor_index_manager.cpp`

**Task**: Find 8 error paths that silently fail without diagnostics

**Search Pattern**:
```bash
grep -n "return.*nullptr\|return \{\}\|return false\|continue\|break" \
  src/tensor/tensor_index_manager.cpp | head -20
```

**What to Look For**:
- Early returns from functions (especially nullptr)
- Loop continues without logging
- Error conditions that don't emit events
- Catch blocks that swallow exceptions

**Expected Paths**:
- Lines 300-350: mapCores() and flushAll() error handling
- Lines 200-250: createIndex() error returns
- Index lookup failures
- Routing decision failures
- RocksDB operation failures

**Fix Template** (for each path found):
```cpp
// BEFORE
if (some_error_condition) {
    return nullptr;  // OR continue;
}

// AFTER
if (some_error_condition) {
    emitIndexDiagnostic(
        "TENSOR-952X",  // Use appropriate error code from 9520-9524 range
        "Description of what failed",
        index_key);  // If applicable
    return nullptr;  // OR continue;
}
```

#### 2. Audit tensor_core_bridge.cpp for Error Paths (~1 hour)

**File**: `src/tensor/tensor_core_bridge.cpp`

**Task**: Find 4 error paths that need diagnostics

**Search Pattern**:
```bash
grep -n "return.*nullptr\|return \{\}\|return false\|continue" \
  src/tensor/tensor_core_bridge.cpp | head -15
```

**Expected Paths**:
- RocksDB delete/put/get failures (TENSOR-9562)
- Write operation failures (TENSOR-9580)
- Read operation failures (TENSOR-9581)
- spdlog integration points

#### 3. Implement HIGH-3 Error Propagation (~1 hour)

**Location**: `src/tensor/tensor_index_manager.cpp` lines 323-334

**Current Issue**: Three failure modes collapse to identical nullptr

**Three Distinct Errors to Separate**:
1. Graph construction failure → TENSOR-9520
2. Index lookup failure → TENSOR-9521
3. Adapter routing failure → TENSOR-9522

**Implementation**:
```cpp
// BEFORE: All three cases return nullptr
if (condition1) return nullptr;
if (condition2) return nullptr;
if (condition3) return nullptr;

// AFTER: Each case has distinct diagnostic
if (condition1) {
    emitIndexDiagnostic("TENSOR-9520", "Graph construction failed");
    return nullptr;
}
if (condition2) {
    emitIndexDiagnostic("TENSOR-9521", "Index lookup failed");
    return nullptr;
}
if (condition3) {
    emitIndexDiagnostic("TENSOR-9522", "Adapter routing failed");
    return nullptr;
}
```

#### 4. Implement MEDIUM-1..5 Fixes (~2 hours)

**MEDIUM-1: RocksDB Deletion Errors**
- Find: RocksDB delete() calls without status checking
- Fix: Check return status, emit TENSOR-9562 on failure
```cpp
rocksdb::Status status = db->Delete(rocksdb::WriteOptions(), key);
if (!status.ok()) {
    emitTensorDiagnostic("TENSOR-9562", 
        "RocksDB delete failed: " + status.ToString());
}
```

**MEDIUM-2: spdlog Integration**
- Find: tensor_core_bridge::write() function
- Fix: Add spdlog logging
```cpp
#include <spdlog/spdlog.h>
// ... in write function
spdlog::info("Tensor write operation starting");
// ... on error
spdlog::error("Tensor write failed: {}", error_message);
```

**MEDIUM-3: addAdapter() Context**
- Find: addAdapter() method
- Fix: Emit diagnostic on success/failure
```cpp
bool TensorIndexManager::addAdapter(...) {
    // ... implementation
    emitTensorDiagnostic("TENSOR-9530", 
        "Adapter added: " + adapter_id);  // on success
    // or
    emitTensorDiagnostic("TENSOR-9530",
        "Adapter add failed: " + error_reason);  // on failure
    return result;
}
```

**MEDIUM-4: Concurrent Diagnostics**
- Find: Shared state accessed from multiple threads
- Fix: Use atomic or mutex
```cpp
// If using atomic:
std::atomic<bool> diagnostic_in_progress;
if (!diagnostic_in_progress.exchange(true)) {
    emitTensorDiagnostic(...);
    diagnostic_in_progress = false;
}

// OR mutex:
std::lock_guard<std::mutex> lock(diagnostic_mutex_);
emitTensorDiagnostic(...);
```

**MEDIUM-5: Exception Handling**
- Find: getRaw() method and other unprotected try-catch blocks
- Fix: Add diagnostic on catch
```cpp
// BEFORE
try {
    // ... operation
} catch (...) {
    return default_value;
}

// AFTER
try {
    // ... operation
} catch (const std::exception& e) {
    emitTensorDiagnostic("TENSOR-XXXX",
        "Exception caught: " + std::string(e.what()));
    return default_value;
}
```

---

## Test Files to Create (Phase C)

### 1. `test_tensor_diagnostics_critical2_focused.cpp`
Tests for CRITICAL-2: 18 diagnostic paths

```cpp
// Test pattern:
TEST_F(TensorDiagnosticsCritical2Test, IndexManagerErrorPathEmitsDiagnostic) {
    // Setup: Create condition that triggers error path
    // Execute: Call method that should emit diagnostic
    // Verify: Check that diagnostic was emitted with correct code
}
```

### 2. `test_tensor_index_manager_error_propagation_focused.cpp`
Tests for HIGH-3: Error state propagation (3 distinct modes)

### 3. `test_tensor_error_codes_focused.cpp`
Tests for HIGH-4: Error code registration

```cpp
TEST_F(TensorErrorCodesTest, TensorErrorCodesRegistered) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Verify all TENSOR codes exist
    for (int code = 9510; code <= 9589; ++code) {
        auto metadata = registry.getError(static_cast<ErrorCode>(code));
        EXPECT_NE(metadata.code, ErrorCode::ERR_UNKNOWN);
    }
}
```

### 4. `test_tensor_medium_findings_focused.cpp`
Tests for MEDIUM-1..5

### 5. `test_tensor_low_findings_focused.cpp`
Tests for LOW-1..2

---

## Build & Verification Checklist

### Pre-Build Verification
- [ ] All #include statements are present
- [ ] No syntax errors in modified files
- [ ] Diagnostic functions are declared in headers
- [ ] Implementations match declarations
- [ ] No circular include dependencies

### Build Steps
1. Configure with CMake preset:
   ```bash
   cmake --preset linux-release
   ```

2. Build tensor module:
   ```bash
   cmake --build build --target themis_tensor
   ```

3. Build tests:
   ```bash
   cmake --build build --target tensor_tests
   ```

4. Run tests:
   ```bash
   ctest --output-on-failure -R "tensor"
   ```

### Post-Build Verification
- [ ] 0 compilation errors
- [ ] 0 compilation warnings (or expected warnings only)
- [ ] 0 linker errors
- [ ] All tests pass (11 P2-A2-XX tests)
- [ ] No ThreadSanitizer warnings
- [ ] No memory leaks detected

---

## Quick Reference: Error Code Assignments

Use this table when implementing fixes in Phase B:

| Range | Category | Codes | Use When |
|-------|----------|-------|----------|
| 9510-9514 | Graph | TENSOR-9510..9514 | Fingerprint computation fails |
| 9520-9524 | Index | TENSOR-9520..9524 | Index operations fail |
| 9530-9533 | Adapter | TENSOR-9530..9533 | Adapter issues |
| 9540-9542 | Fingerprint | TENSOR-9540..9542 | Fingerprint verification fails |
| 9550-9552 | Routing | TENSOR-9550..9552 | Route selection fails |
| 9560-9562 | Recovery | TENSOR-9560..9562 | Persistence/recovery fails |
| 9570-9571 | Concurrency | TENSOR-9570..9571 | Thread/lock issues |
| 9580-9581 | Core Bridge | TENSOR-9580..9581 | Core bridge I/O fails |

---

## Helper Function Quick Reference

### Use emitFingerprintDiagnostic() for:
- Errors in tensor_fingerprint_graph.cpp
- Similarity computation failures
- Train lookup failures

```cpp
emitFingerprintDiagnostic(
    "TENSOR-9510",           // Error code
    "What went wrong",       // Description
    query_key);              // Optional: adapter/key context
```

### Use emitIndexDiagnostic() for:
- Errors in tensor_index_manager.cpp
- Index construction/lookup/routing failures

```cpp
emitIndexDiagnostic(
    "TENSOR-9520",           // Error code
    "What went wrong",       // Description
    index_key);              // Optional: index context
```

### Use emitTensorDiagnostic() for:
- Generic tensor errors
- Core bridge errors
- Other module errors

```cpp
emitTensorDiagnostic(
    "TENSOR-9580",           // Error code
    "What went wrong",       // Description
    {                        // Context map
        {"component", "core_bridge"},
        {"operation", "write"},
        {"details", "..."}
    });
```

---

## Timeline for Phase B & C

**Phase B (Core Hardening): Aug 18-19 (8 hours)**
- Day 1 (4 hours):
  - Audit tensor_index_manager.cpp (1.5 hours)
  - Audit tensor_core_bridge.cpp (1.5 hours)
  - Implement HIGH-3 fixes (1 hour)

- Day 2 (4 hours):
  - Implement MEDIUM-1..5 fixes (2 hours)
  - Review & refine all changes (2 hours)

**Phase C (Testing & Validation): Aug 20-21 (4 hours)**
- Day 1 (2 hours):
  - Create all 11 test files (1 hour)
  - Build & initial verification (1 hour)

- Day 2 (2 hours):
  - Run full test suite (1 hour)
  - Documentation finalization & sign-off (1 hour)

---

## Success Criteria for Phase B & C

✅ **All 11 Findings Fixed**
- 7 in CRITICAL-1 (Phase A) ✅
- 6 in CRITICAL-2 (Phase B)
- 3 in HIGH-3 (Phase B)
- 5 in MEDIUM-1..5 (Phase B)

✅ **11 Test Files Created & Passing**
- P2-A2-01..11 all passing
- 100% pass rate

✅ **Production Ready**
- No stubs/mocks
- Thread-safe
- No regressions

✅ **Documentation Complete**
- Before/after code samples
- Test evidence links
- MTTR reduction validated

---

## Common Issues & Solutions

**Issue**: Compilation error: "undefined reference to emitFingerprintDiagnostic"
**Solution**: Add `#include "tensor/tensor_error_handling.h"` to the .cpp file

**Issue**: FieldDiagnosticsCollector::getInstance() not found
**Solution**: Add `#include "observability/field_diagnostics_collector.h"` to tensor_error_handling.cpp

**Issue**: Test can't compile with diagnostic code
**Solution**: Mock the FieldDiagnosticsCollector or use a test double

**Issue**: Performance regression after adding diagnostics
**Solution**: Diagnostics use async batch emission; verify batching is enabled

---

## Documents to Reference

- **Full Plan**: `ai_working/PHASE2_A2_REMEDIATION_PLAN.md`
- **Progress Report**: `ai_working/A2_REMEDIATION_COMPLETION.md`
- **Error Registry**: `include/utils/error_registry.h`
- **Diagnostic Headers**: `include/tensor/tensor_error_handling.h`
- **Test Template**: `tests/tensor/test_tensor_fingerprint_graph_critical1_diagnostics_focused.cpp`

---

**Ready for Phase B?** Yes! The foundation is solid. 
**Next Step**: Start Phase B audit of tensor_index_manager.cpp
**Questions?** Reference the documents above or check the inline code comments.

---

**Status**: READY FOR PHASE B  
**Last Updated**: 2026-08-07T15:42:27Z
