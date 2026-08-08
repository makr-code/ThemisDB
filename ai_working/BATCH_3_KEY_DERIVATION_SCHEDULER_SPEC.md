# BATCH 3: Key Derivation & Scheduler Hardening Specification

## Overview

**Target Duration:** 2 weeks  
**Priority:** HIGH  
**Depends On:** BATCH 1 (Complete ✓), BATCH 2 (In Progress ~)  
**Affected Components:** key_derivation_service.cpp, key_rotation_scheduler.cpp  
**Total Findings:** 22 (11 per file)  
**Quality Level:** High-severity blocking production readiness  

---

## Part A: Key Derivation Service Hardening (key_derivation_service.cpp)

### Current State Analysis

**File Location:** src/user_storage_encrypted/key_derivation_service.cpp (360 LOC)  
**Current Issues:**
- 1 critical: Uninitialized member field access
- 5 high-severity: Resource leaks in exception paths
- 5 medium-severity: Input validation gaps, missing error context

**Key Responsibilities:**
- Derives encryption keys using HKDF (SHA-256) + Argon2id
- Validates user credentials and tenure
- Stores derived keys in secure memory (using sodium_mlock for sensitivity)
- Provides key material to backend for mount operations

### Implementation Scope: Key Derivation Service

#### Task A.1: Fix Uninitialized Member Field Access (CRITICAL)

**Objective:** Ensure all class members are properly initialized in constructors

**Implementation Requirements:**

1. Audit all member fields in KDF classes
   - Identify any fields not initialized in default/parametrized constructors
   - Focus on fields used in error paths or early exits
   - Check parent classes for inherited uninitialized fields

2. Add explicit initializers to constructors
   ```cpp
   class KeyDerivationService {
       // BEFORE (undefined if early return):
       uint32_t derivation_rounds_;  // uninitialized
       
       // AFTER (always initialized):
       KeyDerivationService() : derivation_rounds_(0) {}
   };
   ```

3. Add unit tests for all error paths
   - Test that destroyed-early members are safe to access
   - Test exception-during-construction scenarios
   - Verify no use-after-move or double-free

**Test Cases (3-4 new tests):**
- Test: Uninitialized field access in error path
- Test: Destruction during partial construction
- Test: Move semantics don't leave uninitialized state

**Acceptance Criteria:**
- [ ] All member fields explicitly initialized in all constructors
- [ ] No code path accesses uninitialized members
- [ ] Constructor exception safety verified via tests
- [ ] No valgrind/ASan warnings on member access

---

#### Task A.2: Fix Resource Leaks in Exception Paths (HIGH)

**Objective:** Ensure all resources are cleaned up when exceptions occur during derivation

**Current Issues (5 locations identified):**
- Allocated memory not freed when errors occur
- File descriptors for random number generation not closed
- Lock guards or mutexes not released (should be automatic with RAII)

**Implementation Requirements:**

1. Identify all manual resource allocations in key derivation paths
   - malloc/new for key buffers
   - fopen/open for /dev/urandom or /dev/random
   - Mutexes or locks protecting derivation state

2. Replace manual cleanup with RAII wrappers where applicable
   - Use std::unique_ptr for heap allocations
   - Use RAII file handles or std::ifstream for /dev/urandom
   - Use std::lock_guard for mutex access (already best practice)

3. Test exception safety via RAII cleanup verification
   - Throw exception during derivation
   - Verify all resources cleaned up via valgrind/ASan
   - Verify no file descriptor leaks
   - Verify all mutexes released

**Example Pattern:**
```cpp
// BEFORE (leak on exception):
Result<KeyDerivation> derive(const std::string& password) {
    uint8_t* key_buffer = malloc(32);
    if (!randomBytes(...)) {
        free(key_buffer);
        return error(...);  // leak if randomBytes returns false mid-way
    }
    // ... more code that might throw
    free(key_buffer);
    return ok(KeyDerivation{...});
}

// AFTER (exception-safe):
Result<KeyDerivation> derive(const std::string& password) {
    auto key_buffer = std::make_unique<uint8_t[]>(32);
    if (!randomBytes(...)) {
        return error(...);  // no leak, unique_ptr auto-destructs
    }
    // ... more code
    return ok(KeyDerivation{...});
}
```

**Test Cases (4-5 new tests):**
- Test: Memory cleanup on derivation failure
- Test: Random source closed on exception
- Test: No fd leaks in exception paths
- Test: Mutex released even if derive throws

**Acceptance Criteria:**
- [ ] All heap allocations wrapped in std::unique_ptr or similar
- [ ] All file handles properly RAII-wrapped
- [ ] No valgrind/ASan reported leaks on any exception path
- [ ] Exception-safety tests verify cleanup

---

#### Task A.3: Implement Fail-Safe Derivation Behavior (HIGH)

**Objective:** Validate all input parameters and return explicit error codes for invalid inputs

**Implementation Requirements:**

1. Input validation for all public derivation methods
   - Password: non-empty, max length enforced
   - User tenure: valid range (not negative, reasonable upper bound)
   - Nonce: correct size (should be 16+ bytes for security)
   - Derivation rounds: within configured safe range (10,000-1,000,000 typical)

2. Return explicit error codes instead of silent behavior
   - InvalidPasswordLength → explain min/max requirements
   - InvalidTenure → explain valid ranges
   - InvalidNonceSize → explain expected size
   - InvalidRoundsValue → explain configuration constraints

3. Add boundary condition tests
   - Empty password → explicit error
   - Excessive password length → explicit error
   - Negative tenure → explicit error
   - Nonce too short → explicit error
   - Derivation rounds out of bounds → explicit error

**Example Pattern:**
```cpp
// BEFORE (silent behavior):
Result<Key> derive(string_view pwd, uint32_t rounds) {
    if (pwd.empty()) return error(...);  // OK
    // But what if rounds == 0? Might derive silently with 0 rounds
    // This is a fail-unsafe behavior
}

// AFTER (fail-safe):
Result<Key> derive(string_view pwd, uint32_t rounds) {
    // Validate password length
    if (pwd.empty() || pwd.size() > kMaxPasswordLength) {
        return error("Password must be non-empty and < 64KB");
    }
    // Validate derivation rounds
    if (rounds < kMinRounds || rounds > kMaxRounds) {
        return error(format("Derivation rounds must be in range [{}, {}]", 
                           kMinRounds, kMaxRounds));
    }
    // Now derive with explicit validated parameters
    // ...
}
```

**Test Cases (6-8 new tests):**
- Test: Empty password rejected
- Test: Excessive password length rejected
- Test: Negative tenure rejected
- Test: Out-of-bounds rounds rejected
- Test: Invalid nonce size rejected
- Test: All boundary conditions produce explicit errors
- Test: Error messages are clear and actionable

**Acceptance Criteria:**
- [ ] All public methods validate inputs before processing
- [ ] No silent behavior for invalid inputs
- [ ] Explicit error messages explain constraints
- [ ] Boundary condition tests all pass
- [ ] Operator can understand why derivation failed from error message

---

#### Task A.4: Add Correlation ID to Key Operations (HIGH)

**Objective:** Link key derivation to backend mount operations via correlation ID

**Implementation Requirements:**

1. Accept correlation ID in key derivation API
   - Add optional parameter: `derive(..., string_view correlation_id)`
   - Pass through all internal derivation steps
   - Log correlation ID with each step

2. Add structured logging for key operations
   - Log at derivation start (with correlation ID)
   - Log at each major step (nonce generation, HKDF, Argon2id)
   - Log at derivation end (success or error, with correlation ID)

3. Enable end-to-end tracing from mount → derive → backend
   - mount() generates correlation ID
   - Passes to derive()
   - Logs include same correlation ID
   - Backend can match logs to understand flow

**Test Cases (2-3 new tests):**
- Test: Correlation ID appears in all log messages
- Test: End-to-end mount→derive flow logs same correlation ID
- Test: No performance regression from logging (< 1ms overhead)

**Acceptance Criteria:**
- [ ] Key derivation accepts correlation ID parameter
- [ ] All log messages include correlation ID
- [ ] End-to-end tracing visible in logs with consistent ID
- [ ] No performance regression

---

## Part B: Key Rotation Scheduler Hardening (key_rotation_scheduler.cpp)

### Current State Analysis

**File Location:** src/user_storage_encrypted/key_rotation_scheduler.cpp (306 LOC)  
**Current Issues:**
- 1 critical: Uninitialized access pattern
- 5 high-severity: Callback failure handling gaps
- 5 medium-severity: Missing diagnostics, incomplete error logging

**Key Responsibilities:**
- Schedules periodic key rotation events
- Manages rotation callbacks and event listeners
- Handles rotation failures with retry logic
- Tracks rotation history and metrics

### Implementation Scope: Key Rotation Scheduler

#### Task B.1: Fix Uninitialized Access Patterns (CRITICAL)

**Objective:** Ensure all rotation scheduler state is properly initialized

**Implementation Requirements:**

1. Audit rotation scheduler state machine
   - Identify states that may not be explicitly initialized (e.g., last_rotation_time_)
   - Ensure all schedule events start with known state
   - Check timer/event queue initialization

2. Add explicit initialization for all state
   - Initial rotation state: kNotScheduled or kIdle
   - Last rotation time: explicit minimum value (not garbage)
   - Rotation counter: initialized to 0
   - Event queue: must be empty on start

3. Test state initialization
   - Constructor must set all state consistently
   - State immediately after construction must be valid
   - No uninitialized reads possible

**Test Cases (2-3 new tests):**
- Test: Scheduler state valid immediately after construction
- Test: No uninitialized reads in rotation logic
- Test: State consistent after first schedule

**Acceptance Criteria:**
- [ ] All scheduler state explicitly initialized in constructor
- [ ] No code path reads uninitialized state
- [ ] State immediately after construction is consistent
- [ ] ASan/valgrind confirm no uninitialized reads

---

#### Task B.2: Implement Callback Failure Recovery (HIGH)

**Objective:** Add retry logic and fallback behavior for rotation callback failures

**Current Problem:**
- If callback fails, rotation stops (no retry)
- No exponential backoff or jitter
- Operators don't know why rotation failed

**Implementation Requirements:**

1. Implement retry mechanism with exponential backoff
   - First retry: 5 seconds
   - Second retry: 25 seconds (5 * 5)
   - Third retry: 125 seconds (25 * 5)
   - Max retries: 3 before declaring failure
   - Add jitter to prevent thundering herd

2. Add fallback behavior for permanent failures
   - After max retries: emit diagnostic event (don't cascade failure)
   - Operator-facing alert: "Key rotation callback failed after 3 retries"
   - Continue scheduling (don't stop scheduler)
   - Next rotation cycle will retry again

3. Log detailed failure information
   - Callback name and parameters
   - Error code from callback
   - Retry attempt number
   - Time until next retry

**Example Pattern:**
```cpp
// Rotation callback execution with retry
struct RotationRetry {
    std::function<Result<void>()> callback;
    int attempts = 0;
    static constexpr int kMaxAttempts = 3;
    static constexpr std::array<int, 3> kBackoffSeconds = {5, 25, 125};
    
    Result<void> executeWithRetry() {
        while (attempts < kMaxAttempts) {
            auto result = callback();
            if (result.isOk()) return result;
            
            attempts++;
            if (attempts >= kMaxAttempts) {
                // Permanent failure - emit diagnostic
                diagnostics_.emit("rotation_callback_failed", {
                    {"callback_name", callback.name},
                    {"attempts", attempts},
                    {"error", result.error()}
                });
                return error("Max retries exceeded");
            }
            
            // Schedule retry with backoff
            int delay_sec = kBackoffSeconds[attempts - 1];
            scheduleRetry(delay_sec);
            return Ok();  // Will retry asynchronously
        }
    }
};
```

**Test Cases (5-6 new tests):**
- Test: First callback failure triggers retry
- Test: Exponential backoff correct (5, 25, 125 seconds)
- Test: Max retries stops retrying and emits diagnostic
- Test: Successful retry unblocks scheduler
- Test: Jitter prevents synchronization issues
- Test: Failed callback doesn't stop other rotations

**Acceptance Criteria:**
- [ ] Callback failures trigger exponential backoff retry
- [ ] Retry succeeds at any attempt → rotation completes
- [ ] Max retries → diagnostic emitted, no cascade
- [ ] Scheduler continues running after callback failure
- [ ] Operators see clear error message in diagnostics
- [ ] No resource leaks in retry loops

---

#### Task B.3: Enhance Scheduler Diagnostics (HIGH)

**Objective:** Add detailed logging and metrics for rotation events

**Implementation Requirements:**

1. Log rotation events with correlation ID
   - Rotation scheduled: `[ROTATION_SCHEDULED] correlation_id=<id> next_rotation=<time>`
   - Rotation started: `[ROTATION_START] correlation_id=<id> user_id=<uid>`
   - Rotation completed: `[ROTATION_COMPLETE] correlation_id=<id> duration_ms=<ms>`
   - Rotation failed: `[ROTATION_FAILED] correlation_id=<id> reason=<reason> retry_in=<seconds>`

2. Add rotation metrics
   - Total rotations scheduled
   - Total rotations completed
   - Total rotations failed
   - Average rotation duration
   - Last rotation timestamp

3. Implement DiagnosticEmitter integration
   - Emit rotation events as structured diagnostics
   - Include all metadata (user ID, duration, status)
   - Link to backend operations via correlation ID

**Test Cases (3-4 new tests):**
- Test: Rotation events logged with correlation ID
- Test: Metrics updated correctly on success/failure
- Test: Diagnostic events include all required fields
- Test: End-to-end: mount→derive→rotate logs consistent correlation ID

**Acceptance Criteria:**
- [ ] All rotation events logged with correlation ID
- [ ] Metrics accessible via scheduler.getMetrics()
- [ ] DiagnosticEmitter receives all rotation events
- [ ] Operator can trace full rotation flow via logs

---

## Integration: Key Derivation ↔ Scheduler ↔ Backend

### Dependency Chain

```
Backend (gocryptfs mount)
    ↓
    Requests key material from KeyDerivationService
    ↓
Key Derivation (derives encryption key)
    ↓
    Encrypts user data, delivers key via stdin
    ↓
Backend (mount succeeds)
    ↓
Scheduler (schedules key rotation)
    ↓
    Timer fires at rotation interval
    ↓
Scheduler (calls rotation callbacks)
    ↓
    Callback re-derives key with new parameters
    ↓
Key Rotation Complete
```

### Cross-Component Error Handling

**Error Propagation Rule:** Errors must be explicit and bounded
- Key derivation error → mount fails with clear message (no silent fallback)
- Callback failure → logged and retried (doesn't stop scheduler)
- Rotation failure → diagnostic emitted (doesn't affect existing mount)

### Correlation ID Threading

All components log same correlation ID:
```
mount(user_id=alice, mount_path=/mnt/alice) → correlation_id=abc123
  → derive(correlation_id=abc123) [logs with abc123]
    → backend.deliverKey(correlation_id=abc123) [logs with abc123]
  → schedule_rotation(correlation_id=abc123)
    → rotation_timer(correlation_id=abc123) [logs with abc123]
      → callback_execute(correlation_id=abc123)
        → derive(correlation_id=abc123) [logs with abc123]
```

Operator can grep logs for `abc123` and see complete flow.

---

## Quality Gates: BATCH 3 Acceptance Criteria

**All must pass before PR merge:**

| Criterion | Acceptance | Evidence |
|-----------|-----------|----------|
| Uninitialized member fields fixed (2 files) | 2/2 fixed | valgrind/ASan clean |
| Resource leaks fixed (5 locations) | 5/5 fixed | valgrind/ASan clean |
| Input validation hardened | All public methods validate | Boundary tests pass |
| Callback retry logic implemented | 3-retry exponential backoff | 6 retry tests pass |
| Scheduler diagnostics enhanced | All events logged + correlation ID | End-to-end tracing works |
| Correlation ID threading complete | All 3 components use same ID | Integration tests pass |
| Exception safety verified | No leaks on any exception path | ASan/valgrind + 12 tests |
| Build succeeds | No new warnings | Compiler output clean |
| All tests pass | 70+ contract + 20+ new BATCH 3 tests | CTest reports 90+ pass |
| No performance regression | Derivation + scheduling unchanged | Benchmark gates pass |

---

## Deliverables Summary

**New Files (2):**
1. tests/user_storage_encrypted/test_key_derivation_hardening_focused.cpp (250+ LOC, 12 tests)
2. tests/user_storage_encrypted/test_scheduler_reliability_focused.cpp (280+ LOC, 12 tests)

**Modified Files (3):**
1. src/user_storage_encrypted/key_derivation_service.cpp
   - Fix uninitialized fields (constructor initialization)
   - Add input validation with explicit error codes
   - Wrap resource allocations in RAII
   - Add correlation ID parameter to public methods
   - Add structured logging

2. src/user_storage_encrypted/key_rotation_scheduler.cpp
   - Fix uninitialized scheduler state
   - Implement callback retry with exponential backoff
   - Add rotation metrics and tracking
   - Implement DiagnosticEmitter integration
   - Add correlation ID threading

3. tests/user_storage_encrypted/CMakeLists.txt
   - Register 2 new focused test targets

**Verification Files:**
- benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp
  - Verify no performance regression

---

## Implementation Timeline

**Estimated Duration:** 2 weeks (10 business days)

- **Week 1:** Tasks A.1-A.4 (Key Derivation Service)
  - 3-4 days: Fix uninitialized + resource leaks + input validation
  - 2-3 days: Add correlation ID + logging
  - 1 day: Integration testing

- **Week 2:** Tasks B.1-B.3 (Key Rotation Scheduler)
  - 2-3 days: Fix uninitialized state + callback retry logic
  - 2 days: Enhance diagnostics + correlation ID
  - 2 days: Integration testing + end-to-end verification
  - 1 day: Performance verification + documentation

---

## Success Metrics

**Production Ready When:**
1. All 11 issues per file fixed (22 total)
2. All acceptance criteria above met
3. Zero critical or high-severity findings remain
4. All 20+ new tests pass
5. End-to-end tracing visible in logs
6. Operator can debug rotation failures using diagnostics + correlation IDs
7. PR reviewed and approved
