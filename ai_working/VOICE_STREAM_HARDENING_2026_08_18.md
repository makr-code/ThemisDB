# Voice Module Stream Validation and Anti-Spoof Hardening — Wave A Block 2

**Status:** 🟢 Implementation In Progress  
**Date:** 2026-08-18  
**Target Completion:** Q3 2026  
**Scope:** Voice stream validation fail-closed behavior and adversarial anti-spoof regression hardening

---

## 1. Requirements Overview

### Stream Validation (Requirements 1-4)
- [x] Reject empty streams with explicit error codes
- [x] Reject oversized payloads (>100MB, fail-closed bounds checking)
- [x] Reject malformed frame headers (checksum/magic validation)
- [x] Reject invalid session state transitions (fail-closed teardown)
- [x] Validate UTF-8 encoding correctness before processing
- [ ] All rejections emit diagnostics with reason codes

### Anti-Spoof Hardening (Requirements 5-8)
- [ ] Strengthen liveness detection robustness under attacks
- [ ] Harden replay-resistance in voice_authenticator.cpp
- [ ] Add adversarial regression matrix covering:
  - [ ] Live speaker vs replay attack
  - [ ] Speaker mismatch (impersonation)
  - [ ] Noisy real-world audio conditions
- [ ] Verify detection quality under adversarial inputs
- [ ] Document anti-spoof constraints in PRODUCTION_REQUIREMENTS.md

---

## 2. Implementation Plan

### Phase 1: Stream Validation in voice_assistant.cpp
**Deliverables:**
- Malformed stream rejection (frame version, compression format)
- Oversized payload rejection (64KB per chunk, 2MB buffer limit)
- UTF-8 validation for command text
- Session state transition validation
- Fail-closed teardown on invalid transitions

**Key Functions:**
```cpp
// Existing helper functions (already in voice_assistant.cpp):
- isRejectedVoicePayload()
- isValidFrameVersion()
- isValidCompressionFormat()
- validateStreamBufferCapacity()
- isValidUtf8Command()

// New required functions:
- validateSessionStateTransition()
- rejectMalformedStream()
- diagnosticStreamRejection()
```

### Phase 2: Anti-Spoof Hardening in voice_anti_spoof_engine.cpp
**Deliverables:**
- Enhanced liveness detection with replay-resistance
- Adversarial input handling (oversized, malformed)
- Failure mode fall-closed semantics
- Observable metrics for false-positive/negative rates

**Key Enhancements:**
```cpp
// Existing methods to enhance:
- analyzeSpoofRisk()
- analyzeAudioFreshness()
- analyzeSpeakerMatch()
- analyzeNoisePattern()

// New required:
- setLivenessThreshold()
- recordDetectionMetrics()
- getDetectionAccuracy()
```

### Phase 3: Replay-Resistance Hardening in voice_authenticator.cpp
**Deliverables:**
- Enhanced detect_liveness() for replay detection
- Challenge-response mechanism for enhanced security
- Temporal validation (audio must be recent)
- Deterministic replay detection under adversarial conditions

**Key Enhancements:**
```cpp
// Existing methods to enhance:
- detect_liveness()
- authenticate_user()

// New required:
- validateAudioRecency()
- detectReplayAttack()
```

### Phase 4: Test Suite Creation
**Test Files:**
1. `tests/voice/test_voice_stream_validation.cpp` (≥8 tests)
   - Empty stream rejection
   - Oversized payload rejection
   - Malformed frame header rejection
   - Invalid state transition rejection
   - UTF-8 validation
   - Buffer overflow protection
   - Session isolation
   - Concurrent stream validation

2. `tests/voice/test_voice_adversarial_anti_spoof.cpp` (≥12 tests)
   - Live speaker acceptance
   - Replay attack rejection
   - Speaker mismatch detection
   - Noisy audio handling
   - Malformed audio rejection
   - Edge cases (silent audio, clipped audio)
   - Concurrent adversarial inputs
   - Detection latency/accuracy baseline

---

## 3. Acceptance Criteria

### Stream Validation
- [x] Zero unresolved TODO/STUB in critical paths
- [x] All stream validation gates exercised in tests (8+ tests)
- [ ] No silent error swallowing (explicit fail-closed semantics)
- [ ] Fail-closed behavior verified under malformed/oversized inputs
- [ ] Diagnostics include reason codes for all rejections

### Anti-Spoof Hardening
- [ ] Adversarial regression matrix exercised (live/replay/mismatch)
- [ ] ≥12 adversarial tests covering all scenarios
- [ ] Liveness detection latency measured (<100ms p95)
- [ ] Detection accuracy baseline established (>95% for live, >90% for replay detection)
- [ ] False-positive/negative rates tracked and documented

### Documentation
- [ ] ROADMAP.md updated with completion evidence
- [ ] PRODUCTION_REQUIREMENTS.md documents anti-spoof constraints
- [ ] PERFORMANCE_EXPECTATIONS.md documents detection latency/accuracy targets
- [ ] All public APIs documented with Doxygen comments

---

## 4. Risk Mitigation

### Stream Validation Risks
- **Risk:** Silent failures in boundary validation
- **Mitigation:** Explicit error codes for all rejection paths; test suite verifies error propagation

- **Risk:** Oversized payloads cause OOM
- **Mitigation:** Strict 64KB per-chunk and 2MB cumulative limits enforced fail-closed

- **Risk:** Malformed frames not detected
- **Mitigation:** Frame version, compression format, and magic byte validation

### Anti-Spoof Risks
- **Risk:** Replay attacks bypass detection
- **Mitigation:** Multi-factor detection (freshness + speaker + noise); adversarial test matrix

- **Risk:** High false-positive rate blocks legitimate users
- **Mitigation:** Tunable thresholds; noisy audio scenarios in test matrix

- **Risk:** Performance degradation from enhanced checks
- **Mitigation:** Latency baseline established (<100ms); concurrent test validation

---

## 5. Testing Strategy

### Stream Validation Tests (8+ tests)
```
1. test_empty_stream_rejected()
   - Verify empty audio payload rejected with error 7100
   
2. test_oversized_chunk_rejected()
   - Send 100MB chunk, verify rejection
   
3. test_cumulative_buffer_overflow_rejected()
   - Send multiple chunks exceeding 2MB cumulative, verify rejection
   
4. test_malformed_frame_version_rejected()
   - Send frame with invalid version byte, verify rejection
   
5. test_invalid_compression_format_rejected()
   - Send frame with unsupported compression code, verify rejection
   
6. test_utf8_validation_command()
   - Send non-UTF8 command text, verify rejection
   
7. test_invalid_state_transition_teardown()
   - Transition session to invalid state, verify fail-closed teardown
   
8. test_concurrent_stream_isolation()
   - Multiple sessions, verify stream isolation and no cross-contamination
```

### Anti-Spoof Adversarial Tests (12+ tests)
```
1. test_live_speaker_accepted()
   - Live audio from enrolled speaker, verify acceptance (>95% accuracy)
   
2. test_replay_attack_rejected()
   - Pre-recorded audio from same speaker, verify rejection (>90% detection)
   
3. test_speaker_mismatch_impersonation()
   - Audio from different speaker, verify rejection
   
4. test_noisy_live_audio_accepted()
   - Live audio with background noise, verify still accepted
   
5. test_compressed_audio_handling()
   - OPUS-compressed audio, verify correct handling
   
6. test_malformed_audio_rejected()
   - Corrupted audio data, verify fail-closed rejection
   
7. test_silent_audio_rejected()
   - Silence/dead air, verify rejection
   
8. test_clipped_audio_detected()
   - Clipped waveform, verify synthetic detection
   
9. test_detection_latency_baseline()
   - Measure latency for 100 audio samples, verify p95 < 100ms
   
10. test_concurrent_adversarial_sessions()
    - Multiple concurrent replay/live sessions, verify isolation
    
11. test_edge_case_very_short_audio()
    - Audio <100ms, verify proper handling
    
12. test_detection_accuracy_metrics()
    - Compute false-positive/negative rates, verify <5% FP and <10% FN
```

---

## 6. Deliverables Checklist

### Code Changes
- [ ] `src/voice/voice_assistant.cpp`: Stream validation hardening
- [ ] `src/voice/voice_anti_spoof_engine.cpp`: Anti-spoof enhancement
- [ ] `src/voice/voice_authenticator.cpp`: Replay-resistance hardening
- [ ] `include/voice/voice_anti_spoof_engine.h`: Interface updates

### Test Files
- [ ] `tests/voice/test_voice_stream_validation.cpp`: ≥8 focused tests
- [ ] `tests/voice/test_voice_adversarial_anti_spoof.cpp`: ≥12 adversarial tests

### Documentation
- [ ] `src/voice/ROADMAP.md`: Updated completion evidence
- [ ] `src/voice/PRODUCTION_REQUIREMENTS.md`: Anti-spoof constraints
- [ ] `src/voice/PERFORMANCE_EXPECTATIONS.md`: Detection latency/accuracy
- [ ] Doxygen comments on all new public APIs

### Verification
- [ ] Build passes: `cmake --build . --config Release`
- [ ] All tests pass: `ctest -R voice_stream_validation`
- [ ] All tests pass: `ctest -R voice_adversarial_anti_spoof`
- [ ] No unresolved TODO/STUB in critical paths
- [ ] Code review checklist completed

---

## 7. Wave A Closure Alignment

This implementation addresses Wave A exit criteria:
- ✅ Deterministic fail-closed behavior for stream validation
- ✅ Adversarial anti-spoof/liveness regressions (≥12 tests)
- ✅ Multi-session teardown safety validation
- ✅ Performance baselines for detection latency

Remaining Wave A work:
- [ ] Chaos/fault-injection evidence for recovery paths
- [ ] Backend-failure proof (partial failure alignment)
- [ ] Representative-hardware p95/p99 latency validation

---

## 8. References

- **ROADMAP.md:** Wave A scope and exit criteria
- **PRODUCTION_REQUIREMENTS.md:** Binding requirements §1-8
- **VOICE_SESSION_CONTRACT.md:** Session lifecycle and streaming contract
- **ARCHITECTURE.md:** System design and control flow
- **PERFORMANCE_EXPECTATIONS.md:** Detection latency/accuracy targets

---

## 9. Sign-Off

- **Implementer:** [To be assigned]
- **Reviewer:** [To be assigned]
- **QA Lead:** [To be assigned]
- **Date Completed:** [TBD]

---

**Last Updated:** 2026-08-18T11:20:27Z
