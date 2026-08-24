# Phase 2-3 Implementation Plan: user_storage_encrypted Module

**Status:** In Progress (2026-08-08)
**Priority:** Critical
**Target Completion:** Phase 2-3 Complete

## Components Implemented

### ✅ Component 0: Error Codes & Diagnostics
- **File:** `include/user_storage_encrypted/error_codes.hpp`
- **Status:** ✅ COMPLETE
- **Changes:**
  - Added ErrorCode enum with 32 distinct error codes (1xxx-5xxx categories)
  - DiagnosticEvent struct for structured logging
  - JSON serialization support
  - Global diagnostic event handler with spdlog integration

**Files Affected:**
- `include/user_storage_encrypted/error_codes.hpp` (NEW)
- `src/user_storage_encrypted/diagnostic_events.cpp` (NEW)
- `src/user_storage_encrypted/CMakeLists.txt` (UPDATED)

---

### ✅ Component 1: CommandTimeoutManager
- **File:** `include/user_storage_encrypted/command_timeout_manager.hpp`
- **Status:** ✅ COMPLETE
- **Changes:**
  - Timeout tracking using `std::chrono::steady_clock`
  - Graceful process termination (SIGTERM → SIGKILL)
  - Resource cleanup guarantees (via waitpid)
  - No silent hangs

**Files Affected:**
- `include/user_storage_encrypted/command_timeout_manager.hpp` (NEW)

---

## Components To Implement (Phase 2)

### Component 2.1: GoCryptFS Backend Hardening
**File:** `src/user_storage_encrypted/gocryptfs_backend.cpp`

**Tasks:**
1. **[PENDING]** Integrate CommandTimeoutManager into executeCommandWithStdin()
   - Replace raw waitpid with timeout-aware process management
   - Enforce mount timeout (30s), unmount timeout (30s), key delivery (5s)
   - Gracefully kill processes on timeout

2. **[PENDING]** FUSE Error Recovery
   - Enhance checkAvailability() for better error classification
   - Add /proc/mounts parsing for stale mount detection
   - Implement `fusermount -u` fallback logic
   - Map system errors to ErrorCode enum

3. **[PENDING]** Stdin Pipe Buffer Cleanup
   - Verify explicit_bzero() usage in deliverKeyViaStdin()
   - Ensure cleanup on exceptions (RAII wrapper)
   - Add Doxygen security properties documentation

**Estimated Lines of Code:** 150-200 new/modified lines

---

### Component 2.2: Key Rotation Scheduler Hardening
**File:** `src/user_storage_encrypted/key_rotation_scheduler.cpp`

**Tasks:**
1. **[PENDING]** Recovery Diagnostics
   - Add `DiagnosticEvent` emission for all rotation lifecycle events
   - Track attempt/success/failure counts per level
   - Implement state tracking in `Impl` struct

2. **[PENDING]** Callback Failure Handling
   - Wrap callback invocation in try-catch
   - Implement bounded retry logic (3 retries, exponential backoff)
   - Isolate failure to prevent cascade to other levels

3. **[PENDING]** State Persistence Verification
   - Verify IRotationStore integration in initialize()
   - Add error handling for corrupted store
   - Test restore-on-startup scenarios

**Estimated Lines of Code:** 100-150 new/modified lines

---

### Component 2.3: Multi-Level Storage Bounded Error Propagation
**File:** `src/user_storage_encrypted/multi_level_storage.cpp`

**Tasks:**
1. **[PENDING]** Per-Level Error Handling
   - Wrap initializeLevel() with per-level error tracking
   - Ensure one level's failure doesn't prevent others

2. **[PENDING]** Stale Mount Reconciliation
   - Verify reconcileStaleMounts() reads /proc/mounts correctly
   - Test fusermount -u fallback behavior
   - Non-fatal error handling (log but don't crash)

3. **[PENDING]** Error Semantics Documentation
   - Document failure domain boundaries
   - Add Doxygen error recovery procedures

**Estimated Lines of Code:** 100-150 new/modified lines

---

### Component 2.4: Key Derivation Service Validation
**File:** `src/user_storage_encrypted/key_derivation_service.cpp`

**Tasks:**
1. **[PENDING]** Input Validation
   - Validate master_key is not empty
   - Validate salt generation and persistence
   - Handle edge cases (empty passphrase, too long)

2. **[PENDING]** Determinism Verification
   - Document Argon2id parameters
   - Add performance budget checks (≤1s per container, p99)
   - Add salt file format documentation

3. **[PENDING]** Error Handling
   - Return explicit ErrorCode values
   - Secure memory cleanup on errors
   - Document KDF security properties

**Estimated Lines of Code:** 100-150 new/modified lines

---

## Components To Implement (Phase 3)

### Component 3.1: Fail-Safe Backend Behavior
**File:** `src/user_storage_encrypted/gocryptfs_backend.cpp` (enhancements)

**Tasks:**
- Map all failure modes to explicit ErrorCode values
- Ensure no silent fallbacks (all failures logged)
- Include error context: code, message, errno, remediation

---

### Component 3.2: Unified Diagnostics
**File:** Multiple (all components)

**Tasks:**
- Ensure all components emit DiagnosticEvent via emitDiagnosticEvent()
- Standardize event types and JSON serialization
- Enable operator observability without external tools

---

### Component 3.3: Path Validation Hardening
**File:** `src/user_storage_encrypted/gocryptfs_backend.cpp` (enhancements)

**Tasks:**
- Verify symlink traversal prevention in CommandArgumentValidator
- Add canonical path conversion (realpath)
- Verify permission checks before mount operations

---

## Build & Test Strategy

### Focused Build (Per-Component)
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset linux-release
cmake --build --preset linux-release --target themis_user_storage_encrypted --parallel 16
```

### Focused Tests
```bash
ctest --preset linux-release -L user_storage_encrypted -V
```

### Test Files Affected
- `tests/user_storage_encrypted/test_backend_io_timeout_focused.cpp`
- `tests/user_storage_encrypted/test_backend_command_injection_focused.cpp`
- `tests/user_storage_encrypted/test_user_storage_encrypted_contract_hardening_focused.cpp`

---

## Acceptance Criteria

Phase 2-3 is complete when:

1. ✅ Zero compiler warnings in new code
2. ✅ All 8 timeout findings from gap-verifier resolved
3. ✅ All focused tests passing (120s timeout)
4. ✅ No memory leaks detected (AddressSanitizer)
5. ✅ Public APIs fully documented (Doxygen)
6. ✅ Error codes mapped to all failure modes
7. ✅ Diagnostic events emitted for operator visibility
8. ✅ Production-ready code quality (modern C++, RAII, exception-safe)

---

## Implementation Order

**Phase 2 - Sequential (dependencies)**
1. ✅ Error codes & diagnostics foundation
2. ✅ CommandTimeoutManager utility
3. → **Next: Component 2.1** (gocryptfs backend uses both above)
4. → Component 2.2 (key rotation scheduler)
5. → Component 2.3 (multi-level storage)
6. → Component 2.4 (key derivation)

**Phase 3 - Integrated**
7. → Fail-safe behavior across all components
8. → Unified diagnostics integration
9. → Path validation hardening

---

## Notes

- All changes maintain backward compatibility
- No schema changes required
- No migration logic needed
- Focus on production hardening, not new features
- Each component tested independently before integration
