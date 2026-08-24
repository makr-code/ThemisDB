# Wave A Block 2: Replication + Voice Fail-Closed Hardening — Implementation Plan

**Date**: 2026-08-18  
**Target**: Wave A Exit Criteria Closure  
**Scope**: Replication (async WAL + lag alerts + lock ordering) + Voice (stream validation + anti-spoof + teardown + audit)  

## Executive Summary

This document provides the complete implementation plan for **Block 2 of Wave A — Runtime Reliability First**. The work addresses 210 gaps in Replication (16 CRITICAL + 194 HIGH) and 45 gaps in Voice (13 CRITICAL + 32 HIGH).

**Key Insight**: Both modules are 95-99% code-complete. Remaining work is focused on:
- **Replication**: Async timeout wrapping, lock ordering verification, diagnostics polish
- **Voice**: Audit logging closure, exception/thread safety fixes, iterator safety

**Execution Model**: 4 parallel subagents + main integration agent  
**Timeline**: 2-3 hours for implementation + 1 hour for verification  

---

## Detailed Work Breakdown

### Phase 1: Replication Module Hardening

#### 1.1 Async Cross-Region WAL Shipping with Lag Limits

**Files**:
- `src/replication/async_wal_shipper.cpp` (modify)
- `src/replication/lag_alert_manager.cpp` (modify)
- `include/replication/async_wal_shipper.h` (review)
- `tests/replication/test_replication_async_wal_lag_alerts.cpp` (new)

**Requirements**:

1. **Configuration Keys**:
   ```c++
   const char* REPLICATION_WAL_SHIPPING_MAX_LAG_MS = "replication.wal_shipping.max_lag_ms";
   const uint64_t DEFAULT_WAL_LAG_LIMIT_MS = 1000;  // 1 second
   ```

2. **Async WAL Shipping Implementation**:
   - `AsyncWALShipper::shipToRemoteDC(remote_dc_endpoint, wal_segment)`
     * Zero-copy serialization (std::string_view)
     * Backpressure control (bounded queue size)
     * Timeout wrapping on network operations (executeWithTimeout)
   - Worker thread pattern with configurable parallelism
   - Throughput target: ≥ 80 MB/s on GbE link

3. **Lag Alert Integration**:
   - Hook `LagAlertManager::checkLagThreshold()` into shipping loop
   - Emit Prometheus `replication_wal_lag_ms` histogram on each shipment
   - Alert callback fires when lag > configured limit (default 2× lag window)

4. **Fail-Closed Behavior**:
   - On network stall: emit alert + degrade to local-only WAL
   - On serialization error: fail-close the shipping thread, alert operator
   - On timeout: emit metric + retry with exponential backoff

**Acceptance Criteria**:
- [ ] Zero unresolved TODO/STUB in async_wal_shipper.cpp
- [ ] All blocking I/O wrapped with executeWithTimeout()
- [ ] Throughput ≥ 80 MB/s measured in benchmark
- [ ] Lag alert fires within 2× lag window (2 seconds default)
- [ ] Tests cover: normal load, lag spike, network failure, concurrent DCs

---

#### 1.2 Failover Diagnostics Improvements

**Files**:
- `src/replication/replication_manager.cpp` (lines 5486+)
- `src/replication/observability.cpp` (fix braces)
- `src/replication/policy.cpp` (fix braces)

**Requirements**:

1. **Error Message Enhancement**:
   - Replace stub at line 5486 with production event processing logic
   - Add multi-region context to all failover transitions
   - Enhance conflict resolution diagnostics consistency

2. **Brace Balance Fixes**:
   - observability.cpp: Fix opening/closing brace count (currently 27 open, 28 close)
   - policy.cpp: Fix opening/closing brace count

3. **Diagnostic Events**:
   - Failover event: { timestamp, source_replica, target_replica, reason, lag_ms }
   - Conflict event: { timestamp, key, source_value, target_value, strategy_applied }
   - Lag spike event: { timestamp, replica_id, current_lag_ms, threshold_ms }

**Acceptance Criteria**:
- [ ] observability.cpp braces balanced (grep verification)
- [ ] policy.cpp braces balanced (grep verification)
- [ ] Failover event processing produces structured diagnostics
- [ ] All multi-region scenarios have explicit error context
- [ ] No silent error swallowing (all errors logged + metrics emitted)

---

#### 1.3 Circular Lock Ordering Verification & Hardening

**Files**:
- `src/replication/replication_slot.cpp` (96 HIGH gaps)
- `src/replication/raft_v2.cpp`
- `src/replication/event_stream.cpp`
- `tests/replication/test_replication_lock_ordering_focused.cpp` (new)

**Requirements**:

1. **Lock Hierarchy Documentation**:
   - Level 1 (outer): `slots_mutex_` (global slot list protection)
   - Level 2 (inner): `state_mutex_` (per-slot state protection)
   - Document at line 34-81 in replication_slot.cpp + add to ARCHITECTURE.md

2. **Deadlock Prevention Audit**:
   - Verify all lock acquisition follows Level 1 → Level 2 order
   - No reverse ordering (Level 2 → Level 1) anywhere in codebase
   - Add `LOCK_HIERARCHY` documentation comment at each critical section
   - Implement deadlock detector test (acquire Level 2 first, verify panic)

3. **Timeout Wrapping** (no_timeout: 8 gaps):
   ```c++
   // Pattern:
   {
     std::unique_lock<std::mutex> lock(state_mutex_, 
       std::chrono::milliseconds(config_.state_lock_timeout_ms));
     if (!lock.owns_lock()) {
       // Timeout: fail-closed
       metrics_.record_lock_timeout();
       return Status::Timeout("State lock acquisition timeout");
     }
     // ... critical section ...
   }
   ```

4. **Move Semantics Hardening** (raft_v2.cpp):
   - Verify all move constructors/operators marked `noexcept`
   - No move operations in exception-unsafe contexts

**Acceptance Criteria**:
- [ ] Lock hierarchy documented in 3+ files
- [ ] Zero reverse-order lock acquisition
- [ ] All 8 no-timeout gaps wrapped with executeWithTimeout()
- [ ] Deadlock detector test passes (intentional ABBA acquisition fails)
- [ ] Move operations verified noexcept (grep verification)
- [ ] No TODO/STUB in lock/timeout paths

---

#### 1.4 Range Temporary Lifetime & Iterator Safety

**Files**:
- `src/replication/event_stream.cpp` (21 range_temporary gaps)
- `src/replication/replication_manager.cpp` (2 iterator_invalidation gaps at 2769, 4052)

**Requirements**:

1. **Range Temporary Fixes**:
   ```c++
   // BAD: range temporary use-after-free
   for (auto& event : getEventBatch()) { ... }  // getEventBatch() returns temporary
   
   // GOOD: bind temporary to variable
   auto events = getEventBatch();
   for (auto& event : events) { ... }
   ```

2. **Iterator Invalidation Fixes** (lines 2769, 4052):
   - Replace unsafe iterator loops with safe alternatives
   - Use container.erase(iterator) return value for next iteration
   - Or rebuild container after modifications

**Acceptance Criteria**:
- [ ] Zero range_temporary warnings in event_stream.cpp
- [ ] Zero iterator_invalidation crashes in tests
- [ ] All container mutations safe under concurrent iteration

---

### Phase 2: Voice Module Hardening

#### 2.1 Stream Validation Fail-Closed Behavior

**Files**:
- `src/voice/voice_assistant.cpp` (stream validation)
- `include/voice/voice_stream_validator.h` (new interface)
- `tests/voice/test_voice_stream_validation.cpp` (new)

**Requirements**:

1. **Stream Validation Gates**:
   - Empty stream rejection: `if (stream.size() == 0) return Status::InvalidInput(...)`
   - Oversized payload rejection: `if (stream.size() > MAX_VOICE_PAYLOAD_MB * 1024 * 1024) return Status::InvalidInput(...)`
   - Malformed frame header: checksum/magic validation, return on mismatch
   - Invalid session state transition: check state machine before processing
   - Encoding validation: UTF-8 or audio format correctness

2. **Fail-Closed Behavior**:
   - All validation failures trigger explicit error code (e.g., `VOICE_ERR_INVALID_STREAM`)
   - Emit diagnostic with reason (empty, oversized, malformed, invalid_state, encoding_error)
   - Emit audit log on failure (subject to 2.4 audit logging)
   - Graceful teardown on validation failure (no resource leak)

3. **Configuration**:
   ```c++
   const uint64_t MAX_VOICE_PAYLOAD_MB = 100;  // Configurable
   const size_t VOICE_FRAME_HEADER_SIZE = 16;  // Validate magic + checksum
   ```

**Acceptance Criteria**:
- [ ] Zero unresolved TODO/STUB in stream validation
- [ ] All validation gates exercised in ≥8 focused tests
- [ ] Fail-closed verification under malformed/oversized/invalid inputs
- [ ] No resource leaks on validation failure
- [ ] Diagnostics emit reason code for each failure type

---

#### 2.2 Adversarial Anti-Spoof Hardening

**Files**:
- `src/voice/voice_anti_spoof_engine.cpp` (strengthen liveness)
- `src/voice/voice_authenticator.cpp` (replay-resistance)
- `include/voice/voice_anti_spoof_engine.h` (interface review)
- `tests/voice/test_voice_adversarial_anti_spoof.cpp` (new)

**Requirements**:

1. **Liveness Detection Robustness**:
   - Challenge-response mechanism: server sends random challenge, client echoes + signs
   - Liveness score calculation: account for audio artifacts, compression, network jitter
   - Adversarial hardening: detect pre-recorded replay, detect low-quality forgery

2. **Replay-Resistance**:
   - Timestamp validation: reject replays > 5 seconds old (configurable)
   - Nonce validation: ensure nonce changes on each challenge
   - Session binding: replay proof tied to specific session_id, not transferable

3. **Adversarial Regression Matrix**:
   - **Live speaker**: genuine liveness challenge → PASS
   - **Replay attack**: pre-recorded audio from same speaker → FAIL
   - **Speaker mismatch**: audio from different speaker → FAIL
   - **Noisy audio**: background noise, compression artifacts → score degradation but no false positives

4. **Detection Quality Metrics**:
   ```c++
   struct DetectionMetrics {
     double false_positive_rate;  // Should be < 0.5%
     double false_negative_rate;  // Should be < 2%
     double latency_p95_ms;       // Should be < 200ms
   };
   ```

**Acceptance Criteria**:
- [ ] Liveness detection passes ≥8 adversarial regression tests
- [ ] Replay attack detection fails consistently
- [ ] Speaker mismatch detection fails consistently
- [ ] Noisy audio handled without false positives
- [ ] Metrics published: false_positive_rate, false_negative_rate, latency

---

#### 2.3 Multi-Session Teardown Safety

**Files**:
- `src/voice/voice_assistant.cpp` (session lifecycle)
- `src/voice/voice_session_manager.cpp` (teardown logic)
- `include/voice/voice_session_manager.h` (interface review)
- `tests/voice/test_voice_multi_session_teardown.cpp` (new)

**Requirements**:

1. **Session State Machine**:
   ```
   RUNNING → CLOSING → CLOSED
      ↓         ↓        ↓
   (Active) (Cleanup) (Freed)
   ```
   - No transitions outside this sequence
   - No references retained after CLOSED state
   - Atomic state transitions (no race conditions)

2. **Dangling Reference Elimination**:
   - All session references cleared on state → CLOSED
   - Cleanup order: Sessions → Authenticator → Storage → Cache
   - No back-references from cleaned components
   - Reverse-dependency cleanup verified via destructor guards

3. **Timeout Handling**:
   - Teardown timeout: 5 seconds (configurable)
   - Force-terminate on timeout: release resources, emit alert
   - No hung threads on teardown timeout

4. **Iterator Safety** (voice_liveness_detector.cpp):
   - Rewrite cleanup loops to avoid iterator invalidation
   - Use `container.erase(iterator)` return value or rebuild container

**Acceptance Criteria**:
- [ ] Zero dangling session references after CLOSED state
- [ ] Concurrent teardown of 10+ sessions produces no crashes
- [ ] Timeout expires after exactly 5s ± 100ms
- [ ] Force-terminate frees all resources (sanitizer clean)
- [ ] Reverse-dependency cleanup verified in ≥5 tests

---

#### 2.4 Audit Logging Closure

**Files**:
- `src/voice/voice_assistant.cpp` (authenticate at 144, 264, 659)
- `src/voice/voice_authenticator.cpp` (authorize)
- `src/voice/voice_audit_logger.cpp` (wire audit events)
- `include/voice/voice_audit_logger.h` (interface review)
- `tests/voice/test_voice_audit_logging.cpp` (new)

**Requirements**:

1. **Security Function Audit Calls**:
   ```c++
   // At line 144 in voice_assistant.cpp:
   auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
   audit_logger_.log({
     .timestamp = SystemClock::now(),
     .user_id = uid,
     .action = "authenticate",
     .result = auth_result.ok() ? "SUCCESS" : "FAILURE",
     .reason_code = auth_result.error_code(),
     .context = "voice_session_start"
   });
   ```

2. **Audit Event Schema**:
   ```c++
   struct AuditEvent {
     std::chrono::system_clock::time_point timestamp;
     std::string user_id;
     std::string uid;  // voice biometric ID
     std::string action;  // authenticate, authorize, createSession, terminateSession
     std::string result;  // SUCCESS, FAILURE, PARTIAL
     int reason_code;  // Error code if FAILURE
     std::string context;  // e.g., "voice_session_start", "voice_command_execution"
   };
   ```

3. **Audit Persistence**:
   - Audit logs persisted (file + optional remote syslog)
   - Always-on for production (cannot be disabled)
   - Retention: 90 days (configurable)
   - Immutable logs (append-only, no tampering)

4. **Audit Coverage**:
   - Voice assistant authenticate: 3 locations (144, 264, 659)
   - Voice authenticator authorize: ≥2 locations
   - Session create/terminate: 2+ locations
   - Error paths: all must emit audit

**Acceptance Criteria**:
- [ ] All 4 CRITICAL audit gaps closed (authenticate, authorize, session create/terminate)
- [ ] Audit events captured in all code paths
- [ ] Audit logs survive process restart (persistence verified)
- [ ] Audit cannot be disabled in production (compile-time check)
- [ ] ≥6 audit logging tests pass (coverage verification)

---

#### 2.5 Exception & Thread Safety Fixes

**Files**:
- `src/voice/voice_browser_streaming.cpp` (exception safety)
- `src/voice/voice_error_handler.cpp` (destructor noexcept)
- `src/voice/voice_telephony.cpp` (data race fixes)

**Requirements**:

1. **Exception Safety**:
   - Mark destructors with `noexcept` (no exceptions in cleanup)
   - RAII for all resource acquisition (locks, file handles, buffers)
   - No new/delete in exception paths (use smart pointers)

2. **Thread Safety**:
   - Add mutex guards for shared state (voice_telephony.cpp)
   - Use std::atomic for counters/flags
   - Lock guards scoped to minimal critical section
   - No deadlock scenarios (use std::lock for multi-lock acquisition)

**Acceptance Criteria**:
- [ ] All destructors marked `noexcept`
- [ ] Zero data race reports in sanitizer runs
- [ ] All shared state protected by mutex/atomic
- [ ] No deadlock scenarios (deadlock detector test passes)

---

## Integration & Wave A Exit (Phase 3)

### 3.1 Cross-Module Verification

**Verification Checklist**:
- [ ] Replication + Voice work independently (unit tests pass)
- [ ] Replication + Voice work with Failover module (integration tests pass)
- [ ] Replication + Voice work with Updates module (coordinated rollout tests pass)
- [ ] No regressions in Process module (dependency verification)

### 3.2 Release Gate Closure

**Gates**:
1. [ ] All CRITICAL + HIGH gaps resolved in production code (not stubs)
2. [ ] Lock ordering: zero deadlock scenarios (deadlock detector passes)
3. [ ] Timeout bounds: all blocking I/O wrapped (timeout test suite passes)
4. [ ] Fail-closed: all adversarial inputs handled (fuzzing + chaos injection)
5. [ ] `release_critical` CI: green on `develop` (CI run verification)
6. [ ] p95/p99 baselines: representative hardware (benchmark run + documentation)

### 3.3 Documentation & Sign-Off

**Artifacts to Prepare**:
1. Update `ROADMAP.md` with completion evidence + timestamps
2. Update `PERFORMANCE_EXPECTATIONS.md` with p95/p99 baselines
3. Update `PRODUCTION_REQUIREMENTS.md` with configuration keys
4. Create `WAVE_A_BLOCK2_CLOSURE_2026_08_18.md` with consolidated evidence
5. Prepare human sign-off at `docs/governance/GA_PROMOTION_SIGN_OFF.md` §9

---

## Success Criteria

**Wave A Exit Requirements**:
- ✅ Geographic placement policy: feature complete + tested
- ✅ Async WAL shipping: throughput ≥ 80 MB/s, lag alerts working
- ✅ Failover diagnostics: multi-region error messages improved
- ✅ Lock ordering: deadlock-free verified
- ✅ Timeout bounds: all blocking I/O wrapped
- ✅ Stream validation: fail-closed under all adversarial inputs
- ✅ Anti-spoof: adversarial regression matrix passing
- ✅ Teardown safety: zero dangling references under concurrency
- ✅ Audit logging: all security operations logged
- ✅ Exception/thread safety: sanitizer + deadlock detector clean
- ✅ `release_critical` CI: green on `develop`
- ✅ Representative p95/p99 baselines: locked in documentation

---

## Timeline

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| Analysis | ✅ 1h | Gap inventory + plan (this document) |
| Implementation | ⏳ 2-3h | 4 subagents execute in parallel |
| Verification | ⌛ 1h | Evidence artifacts + CI green |
| Sign-off | ⌛ 0.5h | Wave A exit criteria met |
| **Total** | **~4-5h** | **Wave A Block 2 complete** |

---

## Risk Mitigation

| Risk | Severity | Mitigation |
|------|----------|-----------|
| WAL throughput <80 MB/s | HIGH | Zero-copy serialization + flamegraph profiling |
| Circular lock deadlock | HIGH | Deadlock detector test + lock hierarchy audit |
| Fail-closed regression | HIGH | Adversarial regression suite + chaos injection |
| Audit logging missed cases | MEDIUM | Pre-merge gap scanner + audit coverage tests |
| Performance regression | MEDIUM | Benchmark baseline comparison + regression gates |

---

## Execution Notes

- **Parallel Execution**: 4 subagents work independently on different modules
- **Commit Strategy**: Per-phase commits with focused, reviewable changes
- **Testing**: Each phase includes dedicated test suite with focused regressions
- **Documentation**: Evidence artifacts collected in `ai_working/` for sign-off
- **Dependencies**: Process/Failover/Updates modules are production-ready prerequisites

---

**Status**: ✓ Plan Complete | ⏳ Implementation In Progress

