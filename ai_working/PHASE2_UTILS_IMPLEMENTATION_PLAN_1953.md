# Phase 2: Utils Module Core Implementation Plan

**Status**: In Progress
**Target Completion**: Q4 2026
**Scope**: Hardening 5 core utility planes with production-ready error handling

## Overview

Phase 2 focuses on hardening shared utility hotspots with broad module fan-out. All implementations must:
- Have explicit error handling (no silent failures)
- Include error codes mapped to unified taxonomy (range 7300-7399 for utils-specific)
- Implement degradation paths for external service failures
- Include Doxygen comments documenting error contracts
- Contain no stubs or placeholder code

## 5 Implementation Areas

### Area 1: Observability Plane (Files: 4)
- **audit_logger.cpp**: Audit buffer overflow handling, external service degradation
- **logger.cpp**: Logging failure fallback, graceful degradation
- **tracing.cpp**: Tracing degradation under load, resource exhaustion
- **saga_logger.cpp**: Saga event failure handling, event loss protection

**Hardening Needs**:
- Add explicit error codes for audit failures (7300-7309)
- Implement bounded buffer checks with overflow handling
- Add degradation paths for logging system failures
- Document error contracts with Doxygen

### Area 2: Privacy & Key Plane (Files: 7)
- **pii_detector.cpp**: Edge case handling (Unicode, malformed input)
- **pii_detection_engine.cpp**: Detection failure contracts, graceful degradation
- **ner_detection_engine.cpp**: NER detection edge cases, resource bounds
- **regex_detection_engine.cpp**: Regex failure handling, timeout contracts
- **hkdf_helper.cpp**: Key derivation error contracts, validation
- **hkdf_cache.cpp**: Cache expiration/refresh logic, TTL enforcement
- **pki_client.cpp**: PKI service degradation, retry logic

**Hardening Needs**:
- Add explicit error codes for privacy failures (7310-7329)
- Add error codes for key management failures (7330-7339)
- Implement edge case handling for Unicode and malformed input
- Add cache refresh and expiration logic
- Document PKI service degradation paths

### Area 3: Compression/Encoding Plane (Files: 3)
- **zstd_codec.cpp**: Compression failure behavior, resource bounds
- **lz4_codec.cpp**: Decompression error handling, output validation
- **serialization.cpp**: Serialization contract clarity, error propagation

**Hardening Needs**:
- Add explicit error codes for compression failures (7340-7349)
- Ensure all compression operations have error paths
- Add bounded buffer checks
- Document compression error contracts

### Area 4: Runtime Services Plane (Files: 3)
- **thread_pool_manager.cpp**: Overload shedding, explicit feedback
- **rate_limiter.cpp**: Rate limit failure contracts, queue depth bounds
- **grpc_channel_pool.cpp**: Connection pool degradation, fallback strategies

**Hardening Needs**:
- Add explicit error codes for runtime failures (7350-7369)
- Implement queue overflow handling with explicit feedback
- Add rate limit rejection handling
- Document concurrency safety guarantees

### Area 5: Error Taxonomy Alignment
- **error_registry.h/cpp**: Define unified taxonomy for utils subsystem

**Hardening Needs**:
- Define error code ranges: 7300-7399 for utils
- Document categories: audit (7300-7309), privacy (7310-7329), key mgmt (7330-7339), compression (7340-7349), runtime (7350-7369)
- Add descriptions and resolution steps
- Update error registry with all codes

## Error Code Taxonomy (7300-7399)

### Audit Errors (7300-7309)
- 7300: Audit buffer overflow
- 7301: Audit log write failed
- 7302: Audit event serialization failed
- 7303: Audit external service unreachable
- 7304: Audit format invalid
- 7305: Audit permission denied
- 7306: Audit disk full
- 7307: Audit rotation failed
- 7308: Audit service degraded
- 7309: Audit cleanup failed

### Privacy/Detection Errors (7310-7329)
- 7310: PII detection failed (general)
- 7311: PII engine initialization failed
- 7312: PII detection timeout
- 7313: PII Unicode handling error
- 7314: PII malformed input
- 7315: PII regex compilation failed
- 7316: PII NER engine error
- 7317: PII detection resource exhausted
- 7318: PII policy not found
- 7319: PII pseudonymization failed

### Key Management Errors (7330-7339)
- 7330: HKDF derivation failed
- 7331: HKDF invalid parameters
- 7332: HKDF cache miss
- 7333: HKDF cache expired
- 7334: PKI cert load failed
- 7335: PKI key load failed
- 7336: PKI service unavailable
- 7337: PKI validation failed
- 7338: Key derivation timeout
- 7339: Key cache refresh failed

### Compression Errors (7340-7349)
- 7340: Compression failed (general)
- 7341: Compression buffer too small
- 7342: Decompression failed
- 7343: Invalid compression format
- 7344: Compression resource exhausted
- 7345: Compression timeout
- 7346: Serialization failed
- 7347: Serialization buffer overflow
- 7348: Codec initialization failed
- 7349: Codec not available

### Runtime Service Errors (7350-7369)
- 7350: Thread pool overflow
- 7351: Thread pool task rejected
- 7352: Thread pool timeout
- 7353: Rate limiter exceeded
- 7354: Rate limiter internal error
- 7355: Connection pool exhausted
- 7356: Connection pool timeout
- 7357: Queue depth exceeded
- 7358: Resource exhaustion (general)
- 7359: Concurrency conflict
- 7360-7369: Reserved for future runtime errors

## Implementation Strategy

### Phase 2A: Error Taxonomy (Subtask 1-2 days)
1. Add all error codes to error_registry.h (7300-7399)
2. Update error_registry.cpp with descriptions and resolution steps
3. Validate compilation and Doxygen generation

### Phase 2B: Observability Hardening (Subtask 3-4 days)
1. **audit_logger.cpp**: Add overflow handling, error codes, degradation paths
2. **logger.cpp**: Add logging failure fallback, graceful shutdown
3. **tracing.cpp**: Add load-based degradation, sampling fallback
4. **saga_logger.cpp**: Add event loss protection, degradation handling

### Phase 2C: Privacy & Key Hardening (Subtask 5-8 days)
1. **pii_detector.cpp**: Add edge case handling, malformed input validation
2. **pii_detection_engine.cpp**: Add failure contracts, explicit error returns
3. **ner_detection_engine.cpp**: Add resource bounds, timeout handling
4. **regex_detection_engine.cpp**: Add timeout, compilation error handling
5. **hkdf_helper.cpp**: Add parameter validation, error contracts
6. **hkdf_cache.cpp**: Add TTL enforcement, refresh logic
7. **pki_client.cpp**: Add service degradation, retry policies

### Phase 2D: Compression/Runtime Hardening (Subtask 9-11 days)
1. **zstd_codec.cpp**: Add bounded checks, compression failure handling
2. **lz4_codec.cpp**: Add decompression error validation
3. **serialization.cpp**: Add contract documentation, error propagation
4. **thread_pool_manager.cpp**: Add overload shedding, explicit feedback
5. **rate_limiter.cpp**: Add queue bounds, rejection handling
6. **grpc_channel_pool.cpp**: Add fallback strategies, degradation paths

### Phase 2E: Documentation & Verification (Subtask 12 days)
1. Add Doxygen comments to all modified functions
2. Update ROADMAP.md with Phase 2 completion status
3. Run full build and test
4. Generate Doxygen and verify all comments render correctly

## Verification Steps

After each phase:
1. **Compile**: `cmake --build build --target utils`
2. **No Warnings**: Verify no new compiler warnings
3. **Doxygen**: Generate docs and verify comments render
4. **Tests**: Run focused utils tests (Phase 4 will add unit tests)
5. **Code Review**: Verify error handling is explicit and documented

## Key Success Criteria

- [x] 5 implementation areas identified
- [ ] All error codes defined (7300-7399)
- [ ] All hotspots have explicit error handling
- [ ] All external service failures have degradation paths
- [ ] All public APIs have Doxygen comments with error contracts
- [ ] ROADMAP.md updated with Phase 2 completion
- [ ] No stubs or TODO-only code
- [ ] Clean build with no new warnings

## Related Documents

- ROADMAP.md: Overall utils module roadmap
- ARCHITECTURE.md: Utils subsystem architecture
- PRODUCTION_REQUIREMENTS.md: Production quality standards
- error_registry.h: Error code definitions
