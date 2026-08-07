# Phase 3: Test Hardening & Edge Case Coverage Plan

**Purpose:** Expand test suite with stress testing and platform parity verification  
**Dependencies:** Phase 1 completion (base fixes)  
**Target Start:** Parallel with Phase 2 (after Phase 1)  
**Duration:** 8-10 hours total (split across 2 agents)

---

## Overview

Phase 3 focuses on two critical test suites:

1. **Wire Protocol & Session Stress Testing** (Agent 5)
   - Concurrent wire sessions
   - Connection lifecycle edge cases
   - Malformed message handling
   - Resource cleanup under load

2. **Loader Platform Parity Testing** (Agent 6)
   - Linux vs Windows behavior alignment
   - Trust material configuration scenarios
   - Dependency resolution edge cases
   - Fail-safe signature/dependency verification

---

## Agent 5: Wire Protocol & Session Stress Testing

### Target Files

**Primary Test:** `tests/network/test_themis_wire_protocol_stress.cpp`  
**Integration:** Linked to wire protocol subsystem  
**Dependencies:** 
- `include/network/wire_protocol_server.h`
- `include/network/wire_protocol.h`
- `src/network/wire_protocol_server.cpp`

### Test Scenarios

#### 1. Concurrent Wire Sessions

**Objective:** Verify thread safety under high concurrency

```cpp
TEST(WireProtocolStress, ConcurrentSessions) {
    // Setup: Create wire protocol server
    // Spawn 100+ concurrent client threads
    // Each thread:
    //   - Open connection
    //   - Send batch messages (1000 per thread)
    //   - Verify response integrity
    //   - Close connection
    // Verify:
    //   - No memory leaks
    //   - No data corruption
    //   - Consistent message ordering
    //   - No thread safety violations (TSAN)
}
```

**Metrics:**
- Throughput: messages/sec under concurrent load
- P95/P99 latency under load
- Memory growth pattern
- Thread pool utilization

---

#### 2. Connection Lifecycle Edge Cases

**Objective:** Verify robust connection state management

```cpp
TEST(WireProtocolStress, ConnectionLifecycleEdgeCases) {
    // Case 1: Rapid connect/disconnect
    //   - Open connection, immediately close (no messages)
    //   - Verify cleanup, no resource leaks
    
    // Case 2: Partial message on disconnect
    //   - Send incomplete message header
    //   - Disconnect abruptly
    //   - Verify recovery, no corruption
    
    // Case 3: Timeout during message
    //   - Start message, pause mid-send
    //   - Wait for timeout
    //   - Verify connection state
    
    // Case 4: Reconnect after timeout
    //   - Timeout occurs
    //   - Client reconnects
    //   - Verify session state recovery
    
    // Case 5: Out-of-order messages
    //   - Send messages in reverse order
    //   - Verify error handling
    
    // Case 6: Duplicate message receipt
    //   - Send same message twice
    //   - Verify idempotency or duplicate detection
}
```

**Metrics:**
- Connection recovery time
- Resource cleanup completeness
- Message reordering tolerance
- Duplicate detection accuracy

---

#### 3. Malformed Message Handling

**Objective:** Verify robustness against protocol violations

```cpp
TEST(WireProtocolStress, MalformedMessages) {
    // Case 1: Invalid header
    //   - Send corrupted message header
    //   - Verify error response
    //   - Verify connection remains viable
    
    // Case 2: Wrong message length
    //   - Send length header != actual payload
    //   - Verify detection and recovery
    
    // Case 3: Invalid checksum
    //   - Send correct format but wrong checksum
    //   - Verify detection
    
    // Case 4: Oversized message
    //   - Send message exceeding max size
    //   - Verify rejection
    
    // Case 5: Undersized message
    //   - Send incomplete message (missing required fields)
    //   - Verify error handling
    
    // Case 6: Version mismatch
    //   - Send message with incompatible protocol version
    //   - Verify negotiation or rejection
}
```

**Metrics:**
- Detection accuracy
- Connection resilience after error
- Error message clarity
- Recovery time

---

#### 4. Resource Cleanup Under Load

**Objective:** Verify complete resource cleanup during high-stress scenarios

```cpp
TEST(WireProtocolStress, ResourceCleanupUnderLoad) {
    // Setup: Monitor initial resource state
    //   - File descriptors
    //   - Memory usage
    //   - Thread count
    
    // Load: Generate sustained high throughput
    //   - 1000+ concurrent sessions
    //   - 100,000+ messages total
    //   - Run for 5+ seconds
    
    // Stress: Interleave disconnects during load
    //   - Random connection closes
    //   - Session timeouts
    //   - Partial message aborts
    
    // Verify (after shutdown):
    //   - All file descriptors closed
    //   - Memory returned to baseline (+/- 5%)
    //   - All threads joined
    //   - No ASAN/TSAN/Valgrind errors
}
```

**Metrics:**
- Memory leak detection (ASAN)
- Thread safety violations (TSAN)
- Resource handle leaks (valgrind)
- Cleanup time

---

### Expected Test Metrics

| Metric | Target | Threshold |
|--------|--------|-----------|
| Concurrent sessions | 100+ | Pass at 100, warn >120 |
| Messages/sec throughput | 50,000+ | Pass if >40,000 |
| P95 latency | <10ms | Warn if >15ms |
| P99 latency | <50ms | Warn if >75ms |
| Memory growth | <5% over baseline | Fail if >10% |
| Thread cleanup | 100% | Fail if any join timeout |
| Malformed message handling | 100% | Fail if any pass through |

---

## Agent 6: Loader Platform Parity Testing

### Target Files

**Primary Test:** `tests/security/test_themis_loader_platform_parity.cpp`  
**Integration:** Linked to module loader subsystem  
**Dependencies:**
- `include/security/module_loader.h`
- `src/security/module_loader.cpp`
- `include/security/trust_material.h`

### Test Scenarios

#### 1. Linux vs Windows Loader Behavior

**Objective:** Verify consistent behavior across platforms

```cpp
TEST(LoaderPlatformParity, BasicLoadingBehavior) {
    // Scenario 1: Load valid signed module
    //   - Linux: Use dlopen + signature verification
    //   - Windows: Use LoadLibrary + signature verification
    //   - Verify: Same error codes, same behavior
    
    // Scenario 2: Load unsigned module
    //   - Both platforms: Should reject
    //   - Verify: Consistent error message
    
    // Scenario 3: Load corrupted module
    //   - Both platforms: Should detect corruption
    //   - Verify: Same error codes
    
    // Scenario 4: Load from various paths
    //   - Absolute path
    //   - Relative path
    //   - System library path
    //   - Custom library path
    //   - Verify: Platform-agnostic behavior
}
```

**Metrics:**
- Behavior consistency score
- Error code alignment
- Performance parity (allow 10% variance)

---

#### 2. Trust Material Configuration

**Objective:** Verify trust material handling across platforms

```cpp
TEST(LoaderPlatformParity, TrustMaterialConfiguration) {
    // Scenario 1: Load with certificate store
    //   - Linux: Use /etc/ssl/certs or custom path
    //   - Windows: Use Windows certificate store or custom
    //   - Verify: Both find and validate trusted certs
    
    // Scenario 2: Load with custom CA bundle
    //   - Both platforms: Support custom PEM file
    //   - Verify: Consistent validation results
    
    // Scenario 3: Expired certificate
    //   - Both platforms: Should reject
    //   - Verify: Consistent error
    
    // Scenario 4: Revoked certificate (CRL)
    //   - Both platforms: Support CRL checking
    //   - Verify: Consistent behavior
    
    // Scenario 5: Missing trust material
    //   - Both platforms: Should fail safely
    //   - Verify: Don't fall back to insecure defaults
}
```

**Metrics:**
- Certificate validation consistency
- Trust store detection accuracy
- Error handling uniformity

---

#### 3. Dependency Resolution Edge Cases

**Objective:** Verify robust dependency handling

```cpp
TEST(LoaderPlatformParity, DependencyResolution) {
    // Scenario 1: Circular dependencies
    //   - Module A depends on B, B depends on A
    //   - Verify: Detected and rejected on both platforms
    
    // Scenario 2: Missing dependency
    //   - Module depends on non-existent library
    //   - Verify: Clear error on both platforms
    
    // Scenario 3: Version mismatch
    //   - Module depends on lib v1.0, only v2.0 installed
    //   - Verify: Consistent handling
    
    // Scenario 4: Dependency chain (A→B→C→D)
    //   - Verify: All levels loaded correctly
    //   - Verify: Cleanup order (reverse of load)
    
    // Scenario 5: Diamond dependency (A→{B,C}, B→D, C→D)
    //   - Verify: D loaded only once
    //   - Verify: No double-initialization
    
    // Scenario 6: Symbol shadowing
    //   - Multiple modules export same symbol
    //   - Verify: Predictable resolution on both platforms
}
```

**Metrics:**
- Dependency detection accuracy
- Circular dependency detection rate
- Resolution order consistency

---

#### 4. Fail-Safe Signature & Dependency Verification

**Objective:** Verify security-critical verification paths

```cpp
TEST(LoaderPlatformParity, FailSafeVerification) {
    // Scenario 1: Signature verification cannot proceed
    //   - Missing key material
    //   - Corrupted signature
    //   - Unreadable module file
    //   - Verify: FAIL CLOSED (no fallback to insecure)
    
    // Scenario 2: Dependency verification cannot proceed
    //   - Cannot reach trust material
    //   - Cannot verify dependent module
    //   - Verify: FAIL CLOSED
    
    // Scenario 3: Partial verification success
    //   - Some signatures verify, some don't
    //   - Verify: Overall rejection
    
    // Scenario 4: Timing-sensitive verification
    //   - Certificate validation expires during load
    //   - Verify: Consistent handling
    
    // Scenario 5: Recovery after verification failure
    //   - Load fails due to invalid signature
    //   - Fix issue (update cert, re-sign module)
    //   - Retry load
    //   - Verify: Successful on retry
    
    // Scenario 6: Concurrent verification
    //   - Multiple threads attempt concurrent loads
    //   - Verify: No race conditions in verification
}
```

**Metrics:**
- Fail-safe behavior accuracy (100% required)
- Race condition detection (TSAN)
- Security property preservation

---

### Platform-Specific Considerations

#### Linux-Specific
- [ ] Verify dlopen with RTLD_DEEPBIND
- [ ] Verify LD_LIBRARY_PATH handling
- [ ] Verify /etc/ld.so.conf.d/ precedence
- [ ] Verify ELF header parsing
- [ ] Verify symbol table consistency

#### Windows-Specific
- [ ] Verify LoadLibrary search order
- [ ] Verify DLL manifest parsing
- [ ] Verify side-by-side assembly handling
- [ ] Verify PE header validation
- [ ] Verify Windows certificate store integration

### Expected Test Metrics

| Metric | Target | Threshold |
|--------|--------|-----------|
| Platform behavior consistency | 100% | No deviations allowed |
| Trust material detection | 100% | Must find cert store on both platforms |
| Dependency resolution accuracy | 100% | All scenarios handled correctly |
| Signature verification safety | 100% | Fail-closed in all error cases |
| Performance parity | Linux ≈ Windows | Allow ±10% variance |

---

## Integration with Phase 1 Fixes

These tests validate the Phase 1 fixes:

| Phase 1 Fix | Phase 3 Validation |
|------------|-------------------|
| Thread join timeouts | Test connection timeout scenarios |
| Resource cleanup (RAII) | Test resource cleanup under load (ASAN/Valgrind) |
| Exception safety | Test malformed message exception paths |
| Signature validation hardening | Test fail-safe verification across platforms |

---

## Acceptance Criteria

- [ ] All 4 wire protocol stress test scenarios pass
- [ ] Concurrent session limit ≥100 without degradation
- [ ] Memory leak detection clean (ASAN)
- [ ] Thread safety violations clean (TSAN)
- [ ] All 4 loader platform parity scenarios pass
- [ ] Linux/Windows behavior 100% consistent
- [ ] Trust material configuration works on both platforms
- [ ] Dependency resolution edge cases handled
- [ ] Fail-safe verification behavior verified
- [ ] Performance benchmarks established and documented

---

## Files to Create

- [ ] `tests/network/test_themis_wire_protocol_stress.cpp` (~500 LOC)
- [ ] `tests/security/test_themis_loader_platform_parity.cpp` (~400 LOC)
- [ ] `benchmarks/network/bench_wire_protocol_stress.cpp` (~200 LOC)
- [ ] Supporting test fixtures and utilities

---

**Status:** Ready for Agent 5 & 6 (Test agents) implementation  
**Dependencies:** Phase 1 completion  
**Can Execute:** In parallel with Phase 2 (after Phase 1)
