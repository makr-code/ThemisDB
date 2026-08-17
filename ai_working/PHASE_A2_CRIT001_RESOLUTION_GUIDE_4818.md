# PHASE A.2 - CRIT-001 Resolution Guide
## Error Code Range Collision Blocker

**Critical Issue:** Error codes [7300-7305] already exist in `UtilsError` enum  
**Status:** ⚠️ BLOCKING - Must resolve before A.2 implementation proceeds  
**Deadline:** 2026-08-09  
**Estimated Effort:** 2-3 hours  
**Owner:** TBD (Developer assignment needed immediately)

---

## THE PROBLEM

### Current State
The `UtilsError` enum in `include/utils/utils_api_contract.h` defines 6 error codes:

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

### Proposed State (from Phase A.2 task spec)
The task requires error codes [7300-7399] with sub-ranges:
- Audit/Logging errors [7300-7309] ← **COLLISION**
- Privacy detection errors [7310-7319]
- Compression errors [7320-7329]
- Concurrency errors [7330-7339]
- Crypto errors [7340-7349]

### The Collision
Codes [7300-7305] are **allocated twice**, causing:
- ❌ Compilation failure (duplicate enum values)
- ❌ Backward compatibility breakage
- ❌ Confusion in error code taxonomy

---

## THE SOLUTION

### Strategy: Extend Existing Enum

Instead of creating a new error taxonomy header, **extend the existing `UtilsError` enum** to include all new categories.

### Step 1: Extend `UtilsError` Enum

**File:** `include/utils/utils_api_contract.h`

**Action:** Replace the current 6-entry enum with extended enum (20+ entries):

```cpp
/// @brief Error codes for utils module failures.
/// @remarks Error codes in range [7300-7399]. Ranges by category:
///   - Audit/Logging:     [7300-7309] (6 existing + 4 new)
///   - Privacy detection: [7310-7319] (10 new)
///   - Compression:       [7320-7329] (10 new)
///   - Concurrency:       [7330-7339] (10 new)
///   - Crypto/Key:        [7340-7349] (10 new)
enum class UtilsError : uint32_t {  // NOTE: Change to uint32_t for future expansion
    // Audit/Logging errors [7300-7309]
    kAuditOverflow          = 7300,  ///< Audit log sink full
    kBatchRollback          = 7301,  ///< Batch failed; rolled back
    kBatchSizeExceeded      = 7302,  ///< Batch exceeds max size
    kRetryExhausted         = 7303,  ///< Max retry attempts reached
    kDeserInvalid           = 7304,  ///< Malformed deserialisation input
    kPoolExhausted          = 7305,  ///< Thread/connection pool exhausted
    kAuditQueueFull         = 7306,  ///< Audit event queue at capacity
    kAuditWriteFailed       = 7307,  ///< Failed to write audit event
    kAuditSerializationErr  = 7308,  ///< Audit event serialization failed
    kAuditFlushTimeout      = 7309,  ///< Audit flush operation timed out
    
    // Privacy detection errors [7310-7319]
    kPrivacyScanTimeout     = 7310,  ///< PII scan exceeded time limit
    kPrivacyInvalidInput    = 7311,  ///< Invalid input for privacy scan
    kPrivacyRegexError      = 7312,  ///< Regex compilation/execution error
    kPrivacyBufferTooSmall  = 7313,  ///< Output buffer insufficient
    kPrivacyAllocFailed     = 7314,  ///< Memory allocation failed
    kPrivacyStreamClosed    = 7315,  ///< Stream scanner stream is closed
    kPrivacyPatternNotFound = 7316,  ///< Expected pattern not found
    kPrivacyOverflow        = 7317,  ///< Privacy detection overflowed
    kPrivacyInternalError   = 7318,  ///< Internal privacy engine error
    kPrivacyDeprecated      = 7319,  ///< Feature deprecated (legacy)
    
    // Compression errors [7320-7329]
    kCompressionBufTooSmall = 7320,  ///< Output buffer too small
    kCompressionDecompFailed= 7321,  ///< Decompression failed
    kCompressionBomb        = 7322,  ///< Decompression bomb detected
    kCompressionAllocFailed = 7323,  ///< Codec memory allocation failed
    kCompressionUnsupported = 7324,  ///< Compression codec not supported
    kCompressionStreamError = 7325,  ///< Stream codec error
    kCompressionFormatError = 7326,  ///< Invalid compression format
    kCompressionLevelError  = 7327,  ///< Invalid compression level
    kCompressionInternalErr = 7328,  ///< Internal compression error
    kCompressionDeprecated  = 7329,  ///< Compression method deprecated
    
    // Concurrency errors [7330-7339]
    kRateLimiterFull        = 7330,  ///< Rate limiter token quota exhausted
    kThreadPoolFull         = 7331,  ///< Thread pool queue at capacity
    kThreadPoolShutdown     = 7332,  ///< Thread pool is shutting down
    kMutexLockTimeout       = 7333,  ///< Mutex lock acquisition timed out
    kDeadlockDetected       = 7334,  ///< Potential deadlock detected
    kConcurrencyLimitError  = 7335,  ///< Concurrent operation limit reached
    kSyncError              = 7336,  ///< Synchronization primitive error
    kCondVarTimeout         = 7337,  ///< Condition variable wait timed out
    kBarrierError           = 7338,  ///< Barrier synchronization error
    kConcurrencyDeprecated  = 7339,  ///< Concurrency feature deprecated
    
    // Crypto/Key-management errors [7340-7349]
    kCryptoKeyDerivationFailed = 7340,  ///< HKDF key derivation failed
    kCryptoInvalidKeyMaterial  = 7341,  ///< Invalid key material provided
    kCryptoKeyTooShort         = 7342,  ///< Key size below minimum
    kCryptoKeyExpired          = 7343,  ///< Key has expired
    kCryptoKeyRevoked          = 7344,  ///< Key has been revoked
    kCryptoRotationFailed      = 7345,  ///< Key rotation operation failed
    kCryptoAllocFailed         = 7346,  ///< Crypto memory allocation failed
    kCryptoOperationUnsupported= 7347,  ///< Crypto operation not supported
    kCryptoRandomGenFailed     = 7348,  ///< Random number generation failed
    kCryptoDeprecated          = 7349,  ///< Crypto method deprecated
};

// Backward compatibility alias (if needed)
using UtilsErrorCode = UtilsError;
```

**Key Changes:**
- ✅ Change from `int32_t` to `uint32_t` (allows future expansion)
- ✅ Preserve existing 6 codes [7300-7305]
- ✅ Add new codes [7306-7349] organized by category
- ✅ Keep backward compatibility with existing code

### Step 2: Verify Backward Compatibility

**File:** Check all files that use `UtilsError`:

```bash
grep -r "UtilsError::" include/utils src/utils tests/utils | grep -v ".o:" | wc -l
```

**Expected:** All existing usages (kAuditOverflow, kBatchRollback, etc.) remain valid.

**Action:** Run compiler to verify no breaking changes:
```bash
cmake --preset linux-release
cmake --build build-linux-release --target module_utils_test_contract_hardening_focused 2>&1 | grep -i error
```

### Step 3: Update Error Registry (if exists)

**File:** `include/utils/error_registry.h` or similar

**Action:** Ensure error registry reserves range [7300-7399] and documents sub-ranges:

```cpp
// In error_registry.h or similar
namespace themis::utils::error_codes {
    // Reserved ranges for utils module errors [7300-7399]
    constexpr uint32_t kUtilsErrorRangeMin = 7300;
    constexpr uint32_t kUtilsErrorRangeMax = 7399;
    
    // Sub-ranges
    constexpr uint32_t kAuditLoggingMin    = 7300;  // [7300-7309]
    constexpr uint32_t kPrivacyDetectionMin= 7310;  // [7310-7319]
    constexpr uint32_t kCompressionMin     = 7320;  // [7320-7329]
    constexpr uint32_t kConcurrencyMin     = 7330;  // [7330-7339]
    constexpr uint32_t kCryptoKeyMin       = 7340;  // [7340-7349]
}
```

### Step 4: Update Documentation

**Files to update:**
1. `include/utils/utils_api_contract.h` - Add doxygen comments above enum
2. `src/utils/ROADMAP.md` - Add note about error code expansion
3. `docs/utils/ARCHITECTURE.md` - Document error taxonomy
4. `PHASE_A2_DIAGNOSTICS_CODE_REVIEW.md` - Note the resolution

**Documentation Addition:**

```markdown
## Error Code Taxonomy

The utils module uses error codes in the range [7300-7399], organized by category:

| Category | Range | Count | Description |
|----------|-------|-------|-------------|
| Audit/Logging | [7300-7309] | 10 | Audit logging and batch operation errors |
| Privacy Detection | [7310-7319] | 10 | PII/NER/regex detection errors |
| Compression | [7320-7329] | 10 | Compression codec errors |
| Concurrency | [7330-7339] | 10 | Thread pool, rate limiter, sync errors |
| Crypto/Keys | [7340-7349] | 10 | Key derivation, rotation, crypto errors |

All error codes must be mapped through the DiagnosticsEmitter for operator visibility.
```

---

## VERIFICATION CHECKLIST

- [ ] Edit `include/utils/utils_api_contract.h` with extended enum
- [ ] Change `int32_t` to `uint32_t` for UtilsError
- [ ] Add all 50 new error codes with doxygen comments
- [ ] Add backward compatibility alias `using UtilsErrorCode = UtilsError;`
- [ ] Build and verify no compilation errors:
  ```bash
  cmake --preset linux-release
  cmake --build build-linux-release --target module_utils_test_contract_hardening_focused
  ```
- [ ] Run existing tests to verify backward compatibility:
  ```bash
  cd build-linux-release && ctest -R "test_utils_contract" --output-on-failure
  ```
- [ ] Update `include/utils/error_registry.h` with error range constants
- [ ] Update documentation files with error taxonomy table
- [ ] Update `PHASE_A2_DIAGNOSTICS_CODE_REVIEW.md` with resolution note
- [ ] Commit with message: "CRIT-001: Extend UtilsError enum to [7300-7399], consolidate error taxonomy"

---

## BLOCKERS UNBLOCKED AFTER THIS FIX

Once CRIT-001 is resolved, the following high-priority items can proceed:

1. **Phase A.2 Implementation** (themisdb-reviewer)
   - Redesign DiagnosticEvent to use extended UtilsError enum
   - Implement DiagnosticsEmitter with listener pattern
   - Create DG-01..06 integration tests

2. **Phase A.3 Benchmarks** (task Agent)
   - Execute privacy scan benchmarks
   - Measure p95/p99 latencies
   - Create BE-01..08 benchmark gates

3. **Phase B.1 Regressions** (themisdb-implementer)
   - Targeted regression expansion using new error codes
   - Unicode, multibyte, overload scenarios

---

## ROLLBACK PLAN

If compilation issues arise after the change:

1. **Revert to `int32_t`** if size is a concern (unlikely)
2. **Check for conflicting allocations** in other modules
3. **Verify no typedef collisions** with UtilsError in consumer code

**Rollback command:**
```bash
git checkout include/utils/utils_api_contract.h
```

---

## SIGN-OFF

- [ ] Developer assigned and acknowledged
- [ ] CRIT-001 fix completed by 2026-08-09
- [ ] Compilation verified (0 errors, 0 warnings)
- [ ] Tests pass (100% pass rate)
- [ ] Tech lead sign-off
- [ ] Ready for Phase A.2 implementation to begin
