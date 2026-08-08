# Phase 3: Error Handling and Edge Cases - Implementation Guide

**Phase Status**: 0% → In Progress  
**Target Completion**: Q4 2026  
**Roadmap Link**: ROADMAP.md Phase 3

## Overview

Phase 3 standardizes fail-safe behavior and error handling contracts across all utils subsystems:

1. **Unified Error Framework** (error_contracts.h/cpp)
2. **Observability Error Contracts** (audit_logger, logger, tracing, saga_logger)
3. **Privacy Error Contracts** (pii_detector, detection engines)
4. **Crypto Error Contracts** (hkdf_helper, hkdf_cache, pki_client)
5. **Compression Error Contracts** (zstd_codec, lz4_codec)
6. **Runtime Services Error Contracts** (thread_pool_manager, rate_limiter, connection pools)
7. **Incident Categorization Framework** for operator diagnostics

## Deliverables

### 1. Unified Error Contract Framework ✅ COMPLETE

**Files Created**:
- `include/utils/error_contracts.h` - Header with error framework definitions
- `src/utils/error_contracts.cpp` - Implementation

**Key Components**:

#### ErrorCategory Enum
Maps errors to 18 subsystem categories:
- Observability: AuditLog, StructuredLogging, Tracing, SagaLogging
- Privacy: PrivacyDetection, PatternDetection, NERDetection, PrivacyFilter
- Crypto: KeyDerivation, KeyCache, PublicKeyInfra, LocalEncryption
- Compression: ZstdCodec, LZ4Codec, SerializationErr
- Concurrency: ThreadPool, RateLimiting, ConnectionPool

#### ErrorSeverity Enum
Four-level severity model:
- `Fatal (0)` - System corrupted; cascade risk; graceful shutdown
- `Error (1)` - Functional failure; manual intervention needed
- `Warning (2)` - Degradation; fallback available
- `Degraded (3)` - Performance impact; service continues

#### ErrorCode Enum (Range 9000-9099)
90 error codes organized by subsystem:
- 9000-9009: General utility errors (10 codes)
- 9010-9019: Audit logging errors (10 codes)
- 9020-9029: Structured logging errors (10 codes)
- 9030-9039: Tracing errors (6 codes)
- 9040-9049: Privacy detection errors (10 codes)
- 9050-9059: Cryptography errors (10 codes)
- 9060-9069: Compression errors (8 codes)
- 9070-9079: Concurrency errors (10 codes)
- 9080-9089: Serialization errors (5 codes)

#### ErrorContext Struct
Complete error context with:
- Error identification (code, category, severity)
- Timing information (timestamp, elapsed_ms)
- Diagnostic information (message, component, context)
- Recovery information (hint, recoverable flag, retry count)
- Resource state (limit, current usage)
- Methods: `toJSON()`, `toFormattedString()`

#### IncidentCategory Enum
15 operator-visible incident categories:
- BufferOverflow, MemoryExhaustion, ConnectionPoolExhausted, ThreadPoolOverload
- DetectionTimeout, OperationTimeout, KeyDerivationFailure
- PrivacyDetectionFailure, CompressionFailure, FallbackActivated
- RateLimitExhausted, InvalidConfiguration, DataCorruption
- ExternalServiceUnavailable, UnclassifiedIncident

#### Helper Functions
- `logErrorWithContext()` - Structured error logging with context
- `makeErrorContext()` - Factory for creating error contexts
- `categorizeIncident()` - Map error codes to incident categories
- `errorCodeName()` - Get human-readable error code names
- `categoryName()`, `severityName()`, `incidentName()` - Enum to string helpers

### 2. Doxygen Error Contract Template

All public APIs should document error contracts using this pattern:

```cpp
/**
 * @brief Function description
 * 
 * Detailed behavior description.
 * 
 * @error_contract
 * | Condition | ErrorCode | Severity | Logging | Recovery |
 * |-----------|-----------|----------|---------|----------|
 * | Input validation fails | PRIVACY_INVALID_INPUT | Warning | Input details + limits | Return error code |
 * | Pattern too complex | PRIVACY_PATTERN_OVERFLOW | Error | Input size + limit | Fallback to basic detection |
 * | Timeout (pattern eval) | PRIVACY_DETECTION_TIMEOUT | Error | Elapsed time + budget | Return partial results |
 * | Resource exhaustion | PRIVACY_BUFFER_OVERFLOW | Fatal | Current usage + limit | Cascade failure upstream |
 * 
 * @see error_contracts.h - ErrorCode, ErrorContext, ErrorSeverity
 * @return ErrorCode on failure; 0 or SUCCESS on success
 * @throws May throw std::exception if not marked noexcept
 */
```

## Implementation Checklist

### Phase 3A: Observability Error Contracts

**Files to Update**:
- [ ] `include/utils/audit_logger.h` - Add @error_contract tags
- [ ] `src/utils/audit_logger.cpp` - Implement error handling with logging
- [ ] `include/utils/logger.h` - Add @error_contract tags
- [ ] `src/utils/logger.cpp` - Implement error handling with logging
- [ ] `include/utils/tracing.h` - Add @error_contract tags
- [ ] `src/utils/tracing.cpp` - Implement error handling with logging
- [ ] `include/utils/saga_logger.h` - Add @error_contract tags
- [ ] `src/utils/saga_logger.cpp` - Implement error handling with logging

**Key Tasks**:
1. Document all possible error conditions
2. Define audit-specific error codes (9010-9019)
3. Implement logging for each error path
4. Add Doxygen @error_contract documenting:
   - When error occurs
   - Diagnostic information logged
   - Recovery/fallback behavior
   - Impact on downstream consumers
5. Verify audit buffer overflow has explicit fallback behavior

**Error Codes to Use**:
- `AUDIT_BUFFER_OVERFLOW` - Queue/buffer full
- `AUDIT_WRITE_FAILED` - Event write failed
- `AUDIT_PERSISTENCE_FAILED` - Storage write failed
- `AUDIT_ROTATION_FAILED` - Log rotation failed
- `AUDIT_FORMAT_ERROR` - Event format invalid
- `AUDIT_ENCRYPTION_FAILED` - Encryption failed
- `AUDIT_SIGNATURE_FAILED` - Signing failed
- `AUDIT_VALIDATION_FAILED` - Validation failed
- `AUDIT_QUEUE_FULL` - Bounded queue at capacity
- `AUDIT_FLUSH_FAILED` - Flush operation failed

**Bounded Resource Constraints**:
- Audit queue: Max 10,000 events or configurable limit
- Log buffer: Max 1MB per write or configurable
- Pattern/message: Max 4KB with truncation
- Fallback: When buffer full, drop oldest or fail with error

### Phase 3B: Privacy & Key Error Contracts

**Files to Update**:
- [ ] `include/utils/pii_detector.h` - Add @error_contract tags
- [ ] `src/utils/pii_detector.cpp` - Implement error handling
- [ ] `include/utils/pii_detection_engine.h` - Add @error_contract tags
- [ ] `src/utils/pii_detection_engine.cpp` - Implement error handling
- [ ] `include/utils/ner_detection_engine.h` - Add @error_contract tags
- [ ] `src/utils/ner_detection_engine.cpp` - Implement error handling
- [ ] `include/utils/regex_detection_engine.h` - Add @error_contract tags
- [ ] `src/utils/regex_detection_engine.cpp` - Implement error handling
- [ ] `include/utils/hkdf_helper.h` - Add @error_contract tags
- [ ] `src/utils/hkdf_helper.cpp` - Implement error handling
- [ ] `include/utils/hkdf_cache.h` - Add @error_contract tags
- [ ] `src/utils/hkdf_cache.cpp` - Implement error handling
- [ ] `include/utils/pki_client.h` - Add @error_contract tags
- [ ] `src/utils/pki_client.cpp` - Implement error handling

**Key Tasks for Privacy Components**:
1. Document all possible error conditions (input, timeout, resource)
2. Define privacy-specific error codes (9040-9049)
3. Add bounded input validation:
   - String length limits (e.g., max 1MB input)
   - Pattern complexity limits (e.g., max 10,000 patterns)
   - Unicode/encoding edge cases (invalid UTF-8 handling)
4. Document degradation behavior (fallback to basic detection, etc.)
5. Add diagnostic logging for each error class
6. Doxygen @error_contract for each public method

**Key Tasks for Crypto Components**:
1. Document key derivation failure modes (invalid params, timeout)
2. Define crypto-specific error codes (9050-9059)
3. Add key expiration checking and documented recovery
4. Add cache failure handling (miss + derivation failure)
5. Add certificate validation error handling
6. Add explicit timeout and retry limits

**Bounded Resource Constraints**:
- Input text: Max 10MB per detection call
- Pattern set: Max 10,000 patterns
- NER model: Max 500MB in memory
- Detection timeout: 5 second default
- Cache size: Max 1000 keys or configurable
- Key derivation: Max 3 retries, 30 second timeout

**Error Codes to Use (Privacy)**:
- `PRIVACY_INVALID_INPUT` - Input validation failed
- `PRIVACY_PATTERN_OVERFLOW` - Pattern complexity exceeded
- `PRIVACY_DETECTION_TIMEOUT` - Detection timeout
- `PRIVACY_BUFFER_OVERFLOW` - Result buffer full
- `PRIVACY_ENGINE_LOAD_FAILED` - Engine load failed
- `PRIVACY_CONFIG_INVALID` - Config validation failed
- `PRIVACY_UNICODE_ERROR` - Unicode handling failed
- `PRIVACY_MEMORY_EXCEEDED` - Memory limit exceeded
- `PRIVACY_NO_ENGINE` - No engine available
- `PRIVACY_ENGINE_FAILED` - Engine threw exception

**Error Codes to Use (Crypto)**:
- `CRYPTO_KEY_DERIVATION_FAILED` - HKDF derivation failed
- `CRYPTO_KEY_INVALID` - Key validation failed
- `CRYPTO_KEY_EXPIRED` - Key expired
- `CRYPTO_KEY_NOT_FOUND` - Key lookup failed
- `CRYPTO_CACHE_MISS` - Cache miss + derivation failed
- `CRYPTO_CERT_LOAD_FAILED` - Certificate load failed
- `CRYPTO_CERT_INVALID` - Certificate invalid
- `CRYPTO_CERT_EXPIRED` - Certificate expired
- `CRYPTO_ENCRYPTION_FAILED` - Encryption failed
- `CRYPTO_DECRYPTION_FAILED` - Decryption failed

### Phase 3C: Compression/Encoding Error Contracts

**Files to Update**:
- [ ] `include/utils/zstd_codec.h` - Add @error_contract tags
- [ ] `src/utils/zstd_codec.cpp` - Implement error handling
- [ ] `include/utils/lz4_codec.h` - Add @error_contract tags
- [ ] `src/utils/lz4_codec.cpp` - Implement error handling
- [ ] `include/utils/serialization.h` - Add @error_contract tags
- [ ] `src/utils/serialization.cpp` - Implement error handling

**Key Tasks**:
1. Document all possible compression/decompression failures
2. Define codec-specific error codes (9060-9069)
3. Implement bounded resource consumption checks:
   - Input size limits (e.g., max 1GB)
   - Compression level constraints (0-22 for Zstd)
   - Decompression bomb detection (output vs input ratio)
4. Add Doxygen @error_contract documenting:
   - Compression level constraints
   - Max input/output buffer sizes
   - Decompression failure handling
   - Recovery strategies (retry, different level, fallback)

**Bounded Resource Constraints**:
- Max input size: 1GB
- Max output size: 1.5x input or configurable
- Zstd level: 0-22
- LZ4 level: 0-12
- Decompression bomb ratio: Max 255x expansion
- Timeout: 30 seconds for large data

**Error Codes to Use**:
- `COMPRESSION_FAILED` - Compression failed
- `DECOMPRESSION_FAILED` - Decompression failed
- `COMPRESSION_BUFFER_SMALL` - Output buffer too small
- `COMPRESSION_INPUT_INVALID` - Invalid compressed input
- `COMPRESSION_BOMB_DETECTED` - Decompression bomb detected
- `COMPRESSION_RATIO_EXCEEDED` - Compression ratio limit exceeded
- `CODEC_INITIALIZATION_FAILED` - Codec setup failed
- `CODEC_NOT_SUPPORTED` - Codec not available

### Phase 3D: Runtime Services Error Contracts

**Files to Update**:
- [ ] `include/utils/thread_pool_manager.h` - Add @error_contract tags
- [ ] `src/utils/thread_pool_manager.cpp` - Implement error handling
- [ ] `include/utils/rate_limiter.h` - Add @error_contract tags
- [ ] `src/utils/rate_limiter.cpp` - Implement error handling
- [ ] `include/utils/grpc_channel_pool.h` - Add @error_contract tags
- [ ] `src/utils/grpc_channel_pool.cpp` - Implement error handling
- [ ] `include/utils/http_client_pool.h` - Add @error_contract tags
- [ ] `src/utils/http_client_pool.cpp` - Implement error handling

**Key Tasks**:
1. Document all possible failure modes (queue full, timeout, exhaustion)
2. Define concurrency/pool-specific error codes (9070-9079)
3. Implement explicit overload shedding:
   - Queue depth limits with explicit rejection
   - Timeout handling with backpressure
   - Graceful degradation under load
4. Add Doxygen @error_contract for:
   - Queue full behavior (reject vs queue)
   - Timeout behavior and limits
   - Connection pool exhaustion
   - Cascading failure prevention

**Bounded Resource Constraints**:
- Thread pool: Min 1, Max CPU count * 4, default CPU count
- Task queue: Max 100,000 or configurable; backpressure when full
- Connection pool: Max 1000 or configurable
- Request timeout: 30 seconds default
- Rate limit window: 1-60 seconds
- Lock timeout: 1-30 seconds

**Error Codes to Use**:
- `THREADPOOL_QUEUE_FULL` - Task queue at capacity
- `THREADPOOL_SHUTDOWN` - Pool is shutting down
- `THREADPOOL_INVALID_STATE` - Invalid state
- `RATELIMIT_EXCEEDED` - Rate limit quota exhausted
- `RATELIMIT_WINDOW_ERROR` - Window computation failed
- `CONNECTION_POOL_EXHAUSTED` - Pool at capacity
- `CONNECTION_POOL_TIMEOUT` - Connection timeout
- `LOCK_ACQUISITION_FAILED` - Lock acquisition failed
- `LOCK_TIMEOUT` - Lock timeout
- `CONCURRENT_MODIFICATION` - Concurrent modification detected

### Phase 3E: Documentation Updates

**Files to Create/Update**:
- [ ] `src/utils/README.md` - Add error handling section
- [ ] `src/utils/ARCHITECTURE.md` - Add error contract patterns
- [ ] `src/utils/ROADMAP.md` - Update Phase 3 status
- [ ] Include/utils/README.md (if exists) - Error contract guide

**Documentation Content**:
1. Error handling best practices for utils components
2. Bounded resource constraints and defaults
3. Fallback and degradation patterns
4. Incident categorization guide for operators
5. Testing strategy for error paths (reference to Phase 4)

## Testing Strategy (Phase 4 Reference)

Each error code must have corresponding test coverage:

1. **Happy Path**: Verify normal operation doesn't trigger error
2. **Condition Test**: Trigger specific error condition
3. **Recovery Test**: Verify recovery behavior works
4. **Logging Test**: Verify error is logged with context
5. **Concurrency Test**: Verify thread-safety under error conditions

Example test structure:
```cpp
TEST(AuditLoggerErrors, BufferOverflow) {
    // Setup with small buffer
    AuditLogger logger(config_with_small_buffer);
    
    // Fill buffer
    for (int i = 0; i < buffer_size + 100; ++i) {
        auto result = logger.logEvent(event);
        // Last entries should return AUDIT_QUEUE_FULL or AUDIT_BUFFER_OVERFLOW
    }
    
    // Verify error context logged
    auto logs = getLogOutput();
    EXPECT_THAT(logs, ContainsSubstring("AUDIT_QUEUE_FULL"));
    EXPECT_THAT(logs, ContainsSubstring("recovery_hint"));
}
```

## Integration with Existing Code

### Step 1: Add error_contracts to CMakeLists.txt

```cmake
# In src/utils/CMakeLists.txt, add to utils_SOURCES:
src/utils/error_contracts.cpp

# Add header to install
install(FILES include/utils/error_contracts.h DESTINATION include/utils)
```

### Step 2: Update Existing Headers

Each component (audit_logger.h, pii_detector.h, etc.) should:
1. Include `#include "utils/error_contracts.h"`
2. Add @error_contract tags to public APIs
3. Document return types (use ErrorCode, expected<T, ErrorCode>, or exceptions)
4. Update implementation to use error codes and logging

### Step 3: Build Verification

```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset develop
cmake --build build --target utils
# Should compile with no new warnings
```

## Quality Gates

Phase 3 completion requires:

1. ✅ error_contracts.h/cpp implemented
2. All public APIs documented with @error_contract tags
3. All subsystems use error codes instead of silent failures
4. Incident categorization working for all error codes
5. Bounded resource constraints documented
6. Fallback/degradation paths explicit and tested (Phase 4)
7. No silent failures anywhere in utils module
8. CMakeLists.txt updated with new files
9. Doxygen builds without new warnings
10. README.md updated with Phase 3 completion notes

## Timeline

- **Week 1**: Observability error contracts (audit_logger, logger, tracing, saga_logger)
- **Week 2**: Privacy & key error contracts (pii_detector, crypto helpers)
- **Week 3**: Compression & serialization error contracts
- **Week 4**: Runtime services error contracts (thread_pool, rate_limiter, pools)
- **Week 5**: Documentation updates, integration verification, build validation
- **Week 6**: Reserve for unforeseen issues and refinement

## Links & References

- [error_contracts.h](include/utils/error_contracts.h) - Framework header
- [error_contracts.cpp](src/utils/error_contracts.cpp) - Framework implementation
- [error_registry.h](include/utils/error_registry.h) - Global error registry
- [ROADMAP.md](ROADMAP.md) - Utils module roadmap
- [ARCHITECTURE.md](ARCHITECTURE.md) - Utils module architecture
- [README.md](README.md) - Utils module overview

## Dependencies & Prerequisites

- Phase 2 completion (API contracts, resource bounds defined)
- Doxygen 1.9+
- spdlog library (for logging)
- nlohmann/json library (for structured logging)
- C++17 or later

## Notes

- Error contracts are **machine-readable** (ErrorCode enums)
- Doxygen **@error_contract** tags are **human-readable** (in documentation)
- Together they enable automated testing and operator dashboards
- No new features; focus on standardization and diagnosability
- Maintain backward compatibility; add new error codes without removing old ones
- All error codes must be unique within the 9000-9099 range
- Use ErrorContext for rich diagnostic information, not just error codes
- Categorization enables quick incident response (operator focus)
