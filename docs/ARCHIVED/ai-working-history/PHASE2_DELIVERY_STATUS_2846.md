# Phase 2: Utils Module Core Implementation - Delivery Status

**Status**: Implementation In Progress
**Completion Target**: Q4 2026
**Scope**: Error handling hardening, error code taxonomy, Doxygen documentation

## Completed Tasks

### 1. Error Code Taxonomy (✅ COMPLETE)
**Files Updated**:
- include/utils/error_registry.h
- src/utils/error_registry.cpp

**Work Done**:
- Added 64 new error codes (7300-7363) for Phase 2 utils subsystem
- Organized into 5 categories:
  - Audit errors (7300-7309): Buffer overflow, write failures, service degradation
  - Privacy/Detection errors (7310-7329): PII detection, engine failures, resource exhaustion
  - Key Management errors (7330-7339): HKDF, PKI, cache management
  - Compression/Encoding errors (7340-7349): Compression failures, codec issues
  - Runtime Service errors (7350-7363): Thread pool, rate limiting, connection pool, gRPC

**Error Registry Registrations**:
- All 64 errors registered in error_registry.cpp with:
  - Full error messages with format templates
  - Category and severity classification
  - Root cause descriptions
  - Resolution/troubleshooting steps
  - Relevant documentation links
  - Searchable keywords

**Verification**:
- ✅ Syntax validated (clang++ -fsyntax-only)
- ✅ Error codes align with repository ranges
- ✅ No conflicts with existing error codes
- ✅ Comprehensive documentation for all codes

## Remaining Tasks

### 2. Observability Plane Hardening (IN PROGRESS)
**Target Files**:
1. audit_logger.cpp - Buffer overflow handling, write failure contracts
2. logger.cpp - Logging failure fallback, graceful degradation
3. tracing.cpp - Load-based degradation, sampling fallback
4. saga_logger.cpp - Event loss protection, buffering guarantees

**Implementation Pattern**:
For each file:
1. Audit current error handling
2. Add bounded resource checks (buffers, queues, memory)
3. Add explicit error returns with Phase 2 error codes
4. Add Doxygen @error_contract documentation
5. Implement graceful degradation for external service failures

### 3. Privacy & Key Plane Hardening (QUEUED)
**Target Files**:
1. pii_detector.cpp - Unicode/malformed input handling
2. pii_detection_engine.cpp - Engine failure contracts
3. ner_detection_engine.cpp - NER resource bounds
4. regex_detection_engine.cpp - Regex timeout handling
5. hkdf_helper.cpp - Parameter validation, error contracts
6. hkdf_cache.cpp - TTL enforcement, refresh logic
7. pki_client.cpp - Service degradation, retry policies

### 4. Compression/Runtime Plane Hardening (QUEUED)
**Target Files**:
1. zstd_codec.cpp - Compression bounded checks
2. lz4_codec.cpp - Decompression error validation
3. serialization.cpp - Contract documentation
4. thread_pool_manager.cpp - Overload shedding
5. rate_limiter.cpp - Queue bounds, rejection handling
6. grpc_channel_pool.cpp - Fallback strategies

### 5. Documentation & Acceptance (QUEUED)
1. Add Doxygen @error_contract to all hardened functions
2. Update ROADMAP.md with Phase 2 completion status
3. Verify full build with no new warnings
4. Generate Doxygen and verify rendering

## Key Implementation Standards

### Error Handling Pattern
```cpp
/**
 * @brief Brief description of operation
 * @param param Description
 * @return Detailed return description
 * @throws No exceptions
 * 
 * @error_contract
 * - Returns ErrorCode::X on validation failure
 * - Returns ErrorCode::Y on resource exhaustion  
 * - Returns ErrorCode::Z on external service failure
 * 
 * @note Thread-safe: uses lock_guard for internal state
 * @note Bounded resources: queue capped at config.max_depth
 */
ErrorCode doWork(const Parameters& params) {
    // 1. Validate inputs
    if (!params.valid()) {
        THEMIS_WARN("Invalid parameters: {}", params.error_msg);
        return ErrorCode::ERR_XXXX_INVALID_PARAMS;
    }
    
    // 2. Check resource bounds
    if (queue_.size() >= config_.max_depth) {
        THEMIS_WARN("Queue depth exceeded: {} >= {}", queue_.size(), config_.max_depth);
        return ErrorCode::ERR_QUEUE_DEPTH_EXCEEDED;
    }
    
    // 3. Perform operation with error handling
    try {
        auto result = performOperation();
        if (!result.ok()) {
            THEMIS_ERROR("Operation failed: {}", result.error());
            return ErrorCode::ERR_XXXX_OPERATION_FAILED;
        }
        return ErrorCode::SUCCESS; // or return result
    } catch (const std::exception& e) {
        THEMIS_ERROR("Unexpected exception: {}", e.what());
        return ErrorCode::ERR_XXXX_INTERNAL_ERROR;
    }
}
```

### Doxygen Error Contract Example
```cpp
/**
 * @brief Process audit event with bounded buffer
 * @param event Event to audit
 * @return Error code on failure, SUCCESS on success
 * 
 * @error_contract
 * - ERR_AUDIT_BUFFER_OVERFLOW: Buffer capacity exceeded (check max_buffer_size config)
 * - ERR_AUDIT_LOG_WRITE_FAILED: Storage write failed (check disk space, permissions)
 * - ERR_AUDIT_SERVICE_DEGRADED: External service unreachable (will use local logging)
 * - ERR_AUDIT_SERIALIZATION_FAILED: Event format invalid
 * 
 * @thread_safety Thread-safe; uses internal mutex for buffer access
 * @bounded_resources Buffer capped at cfg.max_buffer_size (default 1GB)
 * 
 * @see ErrorCode for full error taxonomy
 * @see ROADMAP.md for Phase 2 implementation status
 */
```

## Metrics & Success Criteria

### Error Code Coverage
- [x] 64 error codes defined (Phase 2)
- [x] 5 error categories organized
- [x] All errors registered with descriptions
- [ ] All hotspot functions updated with error codes
- [ ] 100% of public APIs document error contracts

### Documentation Quality
- [ ] All hotspot functions have @error_contract in Doxygen
- [ ] All error codes have resolution steps
- [ ] All bounded resources documented
- [ ] No TODO/FIXME in implementations

### Code Quality
- [ ] Zero new compiler warnings
- [ ] All implementations production-ready (no stubs)
- [ ] All external service failures have degradation paths
- [ ] All resource limits are bounded and documented

## Next Steps (Priority Order)

1. **Immediate** (Today): Complete observability plane hardening
   - audit_logger.cpp: Add bounded checks and error codes
   - logger.cpp: Add failure fallback paths
   - tracing.cpp and saga_logger.cpp: Add degradation handling

2. **Next** (This week): Complete privacy & key plane
   - Add edge case handling to PII detection files
   - Add TTL enforcement to HKDF cache
   - Add service degradation to PKI client

3. **Following** (This week): Complete compression/runtime
   - Add bounded checks to codec files
   - Add queue overflow handling to runtime services
   - Add connection pool degradation

4. **Final** (Before merge): Documentation and acceptance
   - Generate Doxygen and verify rendering
   - Update ROADMAP.md
   - Run full test suite
   - Code review for error handling completeness

## Files Modified So Far

1. include/utils/error_registry.h
   - Added 64 new error codes (7300-7363)
   
2. src/utils/error_registry.cpp
   - Registered all 64 new errors with complete documentation
   
## Files To Modify Next

**High Priority** (Observability):
- src/utils/audit_logger.cpp - Add bounded buffer checks, overflow handling
- src/utils/logger.cpp - Add logging failure fallback
- src/utils/tracing.cpp - Add degradation under load
- src/utils/saga_logger.cpp - Add event loss protection

**Medium Priority** (Privacy & Keys):
- src/utils/pii_detector.cpp
- src/utils/pii_detection_engine.cpp  
- src/utils/hkdf_cache.cpp
- src/utils/pki_client.cpp

**Lower Priority** (Compression & Runtime):
- src/utils/zstd_codec.cpp
- src/utils/thread_pool_manager.cpp
- src/utils/rate_limiter.cpp
- src/utils/grpc_channel_pool.cpp

## Testing Strategy

**Phase 2 focuses on code quality, not unit tests**:
- Unit tests will be added in Phase 4
- For now, ensure implementations:
  - Have explicit error codes for all failure paths
  - Implement bounded resource checks
  - Include Doxygen error contracts
  - Have no stubs or placeholder code

**Verification methods**:
1. Syntax validation (clang++ -fsyntax-only)
2. Compilation check (cmake --build build --target utils)
3. Doxygen rendering check
4. Code review for completeness
5. Integration with error_registry verified

## References

- ROADMAP.md: Phase 2 timeline and objectives
- PHASE2_UTILS_IMPLEMENTATION_PLAN.md: Detailed work breakdown
- error_registry.h: Error code taxonomy
- Architecture guidelines in repository
