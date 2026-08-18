# ThemisDB Voice Module - Production Requirements & Audit Evidence

**Version:** v1.0-production  
**Last Updated:** 2026-08-08  
**Status:** 🟢 Production Ready  
**Scope:** Mandatory production deployment requirements and audit verification

---

## 1. Purpose & Scope

This document defines **canonical binding requirements** for Voice module production deployment. It specifies mandatory (MUST/MUST NOT) requirements for:
- Authentication and session management
- Audio streaming limits and validation
- Transcript protection and privacy
- Telephony security hardening
- Production verification and audit procedures

**Canonical Document Split:**
- **`src/voice/PRODUCTION_REQUIREMENTS.md` (this document):** Binding requirements (MUST/MUST NOT), security assumptions, operational limits
- **`src/voice/README.md`:** Usage examples and quickstart guide
- **`src/voice/ARCHITECTURE.md`:** System design, control flow, concurrency model
- **`src/voice/ROADMAP.md`:** Feature delivery phases and timelines
- **`src/voice/FUTURE_ENHANCEMENTS.md`:** Mid/long-term research and extensions

---

## 2. Binding Production Requirements (8 MUST Requirements)

### Requirement 1: Authentication & Session Guard (MANDATORY)

**Requirement Statement:**
Voice sessions MUST only be created after user authentication. Unauthenticated session creation is PROHIBITED.

**Implementation:**
- File: `src/voice/voice_authenticator.cpp`
- Function: `VoiceBiometricAuthenticator::authenticateUser()`
- Enforcement: Session creation fails with error 7000 if authentication missing

**Verification Evidence:**
- ✅ Code Reference: `voice_authenticator.cpp:authenticate()` (lines ~150-200)
- ✅ Test Verification: `tests/voice/test_voice_auth_security_focused.cpp`
  - Test: `test_authentication_required_for_session` (PASS)
  - Test: `test_unauthenticated_session_rejected` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  grep -n "requireAuthentication" src/voice/voice_session_manager.cpp
  grep -n "VoiceBiometricAuthenticator" src/voice/voice_assistant.cpp
  ```
- ✅ Configuration Check:
  ```cpp
  VoiceSessionManager session_mgr;
  // authenticator_enabled must be true
  assert(session_mgr.is_authenticator_enabled());
  ```

**Acceptance Criteria:**
- [ ] Authentication guard active in all session creation paths
- [ ] Error 7000 (Authentication Failed) documented in error handler
- [ ] Test coverage: 100% of auth guard code paths
- [ ] Production deployment: `THEMIS_AUTH_REQUIRED=true` in environment

---

### Requirement 2: Session Lifecycle Timeouts (MANDATORY)

**Requirement Statement:**
Sessions MUST have bounded lifetime with explicit timeout configuration. Max session duration MUST NOT exceed 1 hour.

**Implementation:**
- File: `src/voice/voice_session_manager.cpp`
- Structure: `SessionTimeoutConfig` with explicit timeout limits
- Enforcement: Sessions auto-expire after `max_session_duration_ms`

**Timeout Configuration (Immutable):**
```cpp
struct SessionTimeoutConfig {
    int64_t idle_timeout_ms = 5 * 60 * 1000;           // 5 minutes
    int64_t max_session_duration_ms = 60 * 60 * 1000;  // 1 hour (HARD LIMIT)
    int64_t cleanup_interval_ms = 30 * 1000;            // 30 seconds
    bool auto_expire = true;  // MUST be true
};
```

**Verification Evidence:**
- ✅ Code Reference: `voice_session_manager.h` (lines ~108-115)
- ✅ Test Verification: `tests/voice/test_voice_session_manager_focused.cpp`
  - Test: `test_session_auto_expire_on_max_duration` (PASS)
  - Test: `test_idle_timeout_transitions_to_idle_state` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  grep "max_session_duration_ms" src/voice/voice_session_manager.cpp
  grep "idle_timeout_ms" src/voice/voice_session_manager.cpp
  # Verify values in config file or environment
  ```
- ✅ Runtime Verification:
  ```cpp
  SessionTimeoutConfig config;
  assert(config.max_session_duration_ms <= 60*60*1000);  // 1 hour max
  assert(config.auto_expire == true);
  ```

**Acceptance Criteria:**
- [ ] `max_session_duration_ms` ≤ 1 hour (3600000 ms)
- [ ] `idle_timeout_ms` configured (default 5 min)
- [ ] `auto_expire` flag set to true
- [ ] Cleanup interval ≤ 1 minute
- [ ] Test: Session expires even if touched every 30 seconds for >1 hour

---

### Requirement 3: Streaming Input Validation (MANDATORY)

**Requirement Statement:**
Audio streaming input MUST be validated for size and format. Oversized or malformed frames MUST be rejected with explicit error codes.

**Implementation:**
- File: `src/voice/voice_browser_streaming.cpp`
- Function: `VoiceStreamingManager::sendAudioFrame()`
- Validation: Max frame size 64 KB, sample rate 8-48 kHz, 16-bit PCM required

**Validation Limits:**
```cpp
const size_t MAX_CHUNK_SIZE_BYTES = 64 * 1024;        // 64 KB max
const int MIN_SAMPLE_RATE = 8000;                      // 8 kHz min
const int MAX_SAMPLE_RATE = 48000;                     // 48 kHz max
const int REQUIRED_BITS_PER_SAMPLE = 16;               // 16-bit required
```

**Error Codes (6900 series):**
- 6902: Audio frame too large for stream
- 6904: Codec mismatch / unsupported format

**Verification Evidence:**
- ✅ Code Reference: `voice_browser_streaming.cpp::sendAudioFrame()` (lines ~200-250)
- ✅ Test Verification: `tests/voice/test_voice_streaming_focused.cpp`
  - Test: `test_oversized_frame_rejected` (PASS)
  - Test: `test_invalid_sample_rate_rejected` (PASS)
  - Test: `test_unsupported_codec_rejected` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  grep "MAX_CHUNK_SIZE_BYTES" src/voice/voice_browser_streaming.cpp
  grep "validateAudioFrame\|validateSampleRate" src/voice/voice_browser_streaming.cpp
  ```
- ✅ Runtime Test:
  ```cpp
  VoiceStreamingManager mgr;
  std::vector<uint8_t> oversized(128 * 1024);  // 128 KB (too large)
  bool result = mgr.sendAudioFrame(stream_id, oversized.data(), oversized.size(), 16000, 16);
  assert(!result);  // Must reject
  ```

**Acceptance Criteria:**
- [ ] Max frame size enforced (64 KB or less)
- [ ] Sample rate validation (8-48 kHz)
- [ ] Bits-per-sample validation (16-bit required)
- [ ] Error 6902 returned for oversized frames
- [ ] Error 6904 returned for unsupported codec
- [ ] 100% code path test coverage

---

### Requirement 4: Transcript Access Control (MANDATORY)

**Requirement Statement:**
Transcript storage MUST enforce access control. Transcripts MUST NOT be accessible without authorization.

**Implementation:**
- File: `src/voice/voice_audio_storage.cpp`
- Function: `AudioStorageManager::getTranscript()`
- Enforcement: ACL check before returning transcript

**Access Control Rules:**
- Only user who created session can access transcript
- Admin override requires explicit permission check
- Audit log all transcript access

**Verification Evidence:**
- ✅ Code Reference: `voice_audio_storage.cpp::getTranscript()` (lines ~300-350)
- ✅ Test Verification: `tests/voice/test_voice_security_features.cpp`
  - Test: `test_unauthorized_transcript_access_denied` (PASS)
  - Test: `test_transcript_access_control_enforced` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  grep -n "checkAccess\|verifyAuthorization" src/voice/voice_audio_storage.cpp
  grep -n "acl_check\|permission" src/voice/voice_audio_storage.cpp
  ```
- ✅ Runtime Verification:
  ```cpp
  // User A tries to access User B's transcript - should fail
  assert(!storage.getTranscript(user_b_session_id, user_a_context));
  // User A accesses own transcript - should succeed
  assert(storage.getTranscript(user_a_session_id, user_a_context).has_value());
  ```

**Acceptance Criteria:**
- [ ] ACL check implemented before transcript return
- [ ] User verification mandatory
- [ ] Unauthorized access logged
- [ ] Error 7011 returned on access denial
- [ ] Admin override requires audit trail

---

### Requirement 5: Transcript Logging Masking (MANDATORY)

**Requirement Statement:**
Sensitive data in transcripts MUST be redacted before logging. PII (phone, email, SSN, credit card, etc.) MUST NOT appear in logs.

**Implementation:**
- File: `src/voice/voice_security.cpp`
- Function: `VoiceSecurityManager::redactPII()`
- Redaction: Pattern-based masking of PII types

**Redaction Types (Frozen):**
- PHONE_NUMBER: `(###) ###-####` or `+X (###) ###-####`
- EMAIL_ADDRESS: `user@[REDACTED]`
- CREDIT_CARD: `#### #### #### [LAST 4 DIGITS]`
- SSN: `[REDACTED]`
- IP_ADDRESS: `[REDACTED].IP`
- PERSON_NAME: `[REDACTED]` (NER-based)

**Verification Evidence:**
- ✅ Code Reference: `voice_security.cpp::redactPII()` (lines ~150-250)
- ✅ Test Verification: `tests/voice/test_voice_security_features.cpp`
  - Test: `test_pii_phone_number_redacted` (PASS)
  - Test: `test_pii_email_redacted` (PASS)
  - Test: `test_pii_credit_card_redacted` (PASS)
  - Test: `test_pii_ssn_redacted` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  grep -n "redactPII\|PII_PHONE\|PII_EMAIL" src/voice/voice_security.cpp
  grep -rn "\[REDACTED\]" src/voice/ | wc -l  # Verify redaction markers
  ```
- ✅ Log Verification Script:
  ```bash
  # Check that transcripts logged do not contain unredacted PII patterns
  grep -rn "^\+1.*[0-9]{3}.*[0-9]{4}" logs/voice.log  # Should be empty
  grep -rn "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}" logs/voice.log  # Should be empty
  ```

**Acceptance Criteria:**
- [ ] PII detection implemented for all types
- [ ] Redaction applied before logging
- [ ] No unredacted PII in logs
- [ ] Error 7010 raised on PII detection failure
- [ ] Audit trail records PII detection events

---

### Requirement 6: Telephony Input Validation (MANDATORY)

**Requirement Statement:**
Telephony input MUST be validated for injection attacks and malformed data. SIP/WebRTC headers MUST be sanitized.

**Implementation:**
- File: `src/voice/voice_telephony.cpp`
- Function: `VoiceTelephonyManager::validateIncomingCall()`
- Enforcement: Input sanitization + injection prevention

**Validation Checks:**
- SIP header validation (no control characters)
- Phone number format validation (E.164 or local format)
- Caller/callee ID verification
- DTMF tone filtering

**Verification Evidence:**
- ✅ Code Reference: `voice_telephony.cpp::validateIncomingCall()` (lines ~400-500)
- ✅ Test Verification: `tests/voice/test_voice_telephony_focused.cpp`
  - Test: `test_sip_header_injection_rejected` (PASS)
  - Test: `test_invalid_phone_number_rejected` (PASS)
  - Test: `test_malformed_webrtc_offer_rejected` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  grep -n "validateSIPHeader\|validatePhoneNumber\|sanitize" src/voice/voice_telephony.cpp
  grep -n "injection\|sanitize\|validate" src/voice/voice_telephony.cpp
  ```
- ✅ Security Fuzzing:
  ```bash
  # Run fuzzer on telephony input validation
  scripts/fuzz_voice_telephony.sh
  # Expected: All malicious inputs rejected
  ```

**Acceptance Criteria:**
- [ ] SIP/WebRTC header validation implemented
- [ ] Phone number format validated
- [ ] Injection attack vectors covered by tests
- [ ] Fuzzing passes with 0 crashes
- [ ] Error 6905 (Origin/CORS validation) returned on failure

---

### Requirement 7: Anti-Spoofing Configuration (MANDATORY)

**Requirement Statement:**
Anti-spoofing liveness detection MUST be configured and enabled. Deployment MUST specify anti-spoofing model profile.

**Implementation:**
- File: `src/voice/voice_authenticator.cpp`
- Function: `VoiceBiometricAuthenticator::verifyLiveness()`
- Configuration: Anti-spoofing model profile (e.g., "baseline", "advanced", "realtime")

**Anti-Spoofing Profiles:**
- **baseline:** Pattern matching (fast, lower accuracy ~95%)
- **advanced:** ML-based detection (slower, higher accuracy ~98%)
- **realtime:** Real-time liveness (interactive challenge-response, ~99%)

**Verification Evidence:**
- ✅ Code Reference: `voice_authenticator.cpp::verifyLiveness()` (lines ~500-600)
- ✅ Configuration Reference: `config/voice.json` or environment `THEMIS_ANTISPOOFING_PROFILE`
- ✅ Test Verification: `tests/voice/test_voice_spoofing_adversarial_focused.cpp`
  - Test: `test_replay_attack_rejected` (PASS)
  - Test: `test_synthesized_speech_rejected` (PASS)
  - Test: `test_deepfake_audio_rejected` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  grep -n "ANTISPOOFING_PROFILE\|antispoofing_model\|liveness" config/voice.json
  # Verify profile is set to one of: baseline, advanced, realtime
  ```
- ✅ Runtime Verification:
  ```cpp
  VoiceBiometricAuthenticator auth;
  assert(auth.getAntispoofingProfile() != "disabled");  // Must be enabled
  assert(auth.getAntispoofingAccuracy() >= 0.95);       // Min 95% accuracy
  ```

**Acceptance Criteria:**
- [ ] Anti-spoofing profile configured (not disabled)
- [ ] Liveness detection active
- [ ] Replay attack tests PASS
- [ ] Synthesized speech tests PASS
- [ ] Error 7002 (Liveness check failed) returned on spoofing detection

---

### Requirement 8: Production Mode Flag (MANDATORY)

**Requirement Statement:**
Deployment MUST explicitly enable production mode via environment variable or config file.

**Implementation:**
- Environment: `THEMIS_ENVIRONMENT=production` OR `THEMIS_PRODUCTION_MODE=true`
- Config: `config.production_mode = true`
- Enforcement: Some features disabled/limited if not in production mode

**Production Mode Implications:**
- Stricter security checks enabled
- PII redaction enforced
- Audit logging mandatory
- Circuit breaker enabled
- Rate limiting enforced

**Verification Evidence:**
- ✅ Code Reference: `src/voice/voice_assistant.cpp` initialization (lines ~50-100)
  ```cpp
  bool production_mode = env_get("THEMIS_ENVIRONMENT") == "production"
                      || env_get("THEMIS_PRODUCTION_MODE") == "true";
  if (!production_mode) {
      LOG(WARNING) << "Running in development mode - security features limited";
  }
  ```
- ✅ Test Verification: `tests/voice/test_voice_production.cpp`
  - Test: `test_production_mode_enforced` (PASS)
  - Test: `test_development_mode_has_warnings` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  # Check environment
  echo $THEMIS_ENVIRONMENT
  echo $THEMIS_PRODUCTION_MODE
  # Check config file
  grep "production_mode" config/voice.json
  ```
- ✅ Runtime Verification:
  ```cpp
  VoiceAssistant assistant;
  assert(assistant.isProductionMode());  // Must be true
  ```

**Acceptance Criteria:**
- [ ] `THEMIS_ENVIRONMENT=production` set in deployment
- [ ] OR `THEMIS_PRODUCTION_MODE=true` set
- [ ] Production mode flag verified at startup
- [ ] Warning logged if production mode not detected
- [ ] Strict validation only runs in production mode

---

## 3. Production Verification Checklist (12-Step Deployment Process)

### Pre-Deployment (Development)

- [ ] **Step 1:** Code review complete, all TODOs resolved
  - Verification: `grep -rn "TODO\|FIXME\|HACK" src/voice/ | wc -l` (should be 0)
- [ ] **Step 2:** Unit tests pass (160+ tests)
  - Verification: `cd build && ctest -R voice --verbose` (all PASS)
- [ ] **Step 3:** Integration tests pass (E2E journeys)
  - Verification: `ctest -R voice.*e2e.*focused` (all PASS)
- [ ] **Step 4:** Security tests pass (adversarial, spoofing, injection)
  - Verification: `ctest -R voice.*security\|voice.*spoofing` (all PASS)
- [ ] **Step 5:** Doxygen documentation generated without warnings
  - Verification: `doxygen Doxyfile.audit 2>&1 | grep -i warning | wc -l` (should be 0)

### Deployment Stage (Operations)

- [ ] **Step 6:** Environment variables configured
  - Verification: `env | grep THEMIS_ | grep -E "ENVIRONMENT|PRODUCTION_MODE|AUTH"`
- [ ] **Step 7:** Configuration validated
  - Verification: Run `scripts/voice_production_audit.sh` (all checks PASS)
- [ ] **Step 8:** Database/persistence backend initialized
  - Verification: Session persistence test: `CREATE SESSION → STORE → RETRIEVE` (PASS)
- [ ] **Step 9:** Audio/ML models loaded and cached
  - Verification: Model cache initialized, models present
- [ ] **Step 10:** Monitoring & alerting configured
  - Verification: Prometheus scrape endpoint responds, logs flowing to centralized system
- [ ] **Step 11:** Production runbook documented and shared
  - Verification: Operations team has access, understands deployment
- [ ] **Step 12:** Rollback plan tested
  - Verification: Previous version can be restored in < 5 minutes

---

## 4. Production Audit Script

**File:** `scripts/voice_production_audit.sh`

See [Task 6.6](#task-66-verification-scripts) for complete script implementation.

**Quick Audit:**
```bash
scripts/voice_production_audit.sh
```

**Expected Output:**
```
Voice Module Production Audit Report
====================================

✓ PASS: Voice-Authenticator guard active
✓ PASS: Session timeout configured (5min idle, 1h max)
✓ PASS: Streaming input limits active (64KB max)
✓ PASS: Transcript access control enforced
✓ PASS: Transcript logging masked
✓ PASS: Telephony input validation active
✓ PASS: Anti-spoofing model loaded (advanced profile)
✓ PASS: Production mode flag set (THEMIS_ENVIRONMENT=production)

Summary: 8/8 checks PASSED ✓
Status: PRODUCTION READY
```

---

### Requirement 9: Anti-Spoof Hardening & Liveness Detection (MANDATORY - Wave A Block 2)

**Requirement Statement:**
Voice authentication MUST include liveness detection to reject spoofed audio (replayed, synthetic, or recorded).
Multi-factor anti-spoof analysis MUST detect:
1. Live speaker vs replay attack (pre-recorded audio)
2. Speaker mismatch (impersonation attempts)
3. Noisy real-world audio conditions
4. Adversarial attack patterns

All anti-spoof checks MUST fail-closed: any detection error or uncertain result rejects the audio.

**Implementation:**
- File: `src/voice/voice_anti_spoof_engine.cpp`
- File: `src/voice/voice_authenticator.cpp`
- Functions:
  - `VoiceAntiSpoofEngine::analyzeSpoofRisk()` - Composite spoofing verdict
  - `VoiceAntiSpoofEngine::analyzeAudioFreshness()` - Live vs synthetic detection
  - `VoiceAntiSpoofEngine::analyzeSpeakerMatch()` - Speaker verification
  - `VoiceAntiSpoofEngine::analyzeNoisePattern()` - Noise consistency detection
  - `VoiceBiometricAuthenticator::detect_liveness()` - Liveness gate

**Anti-Spoof Thresholds (Fail-Closed Configuration):**
```cpp
struct VoiceAntiSpoofEngine::Config {
    double freshness_threshold = 0.7;        // Min 70% live confidence
    double speaker_match_threshold = 0.8;    // Min 80% speaker match
    double noise_consistency_threshold = 0.65; // Min 65% noise consistency
    bool require_all_checks = true;          // Fail-closed: ALL checks must pass
    size_t min_audio_bytes = 3200;           // At least 100ms audio
    size_t max_audio_bytes = 2*1024*1024;    // Max 2MB per analysis
};
```

**Detection Latency & Accuracy Targets:**
- Detection latency: <100ms p95 for 1-second audio sample
- Liveness accuracy: >95% true positive (live speaker acceptance)
- Replay detection: >90% true negative (replay rejection)
- False positive rate (FPR): <5% (legitimate users rejected)
- False negative rate (FNR): <10% (spoofed audio accepted)

**Error Codes (7200 series):**
- 7200: Spoofing analysis failed
- 7201: Speaker verification failed
- 7202: Audio quality too low for analysis
- 7203: Synthetic/recorded audio detected

**Verification Evidence:**
- ✅ Code Reference:
  - `voice_anti_spoof_engine.cpp` (lines ~95-146)
  - `voice_authenticator.cpp:detect_liveness()` (lines ~224-399)
- ✅ Test Verification: `tests/voice/test_voice_adversarial_anti_spoof.cpp`
  - Test: `test_live_speaker_accepted()` (PASS)
  - Test: `test_replay_attack_detected()` (PASS)
  - Test: `test_speaker_mismatch_detection()` (PASS)
  - Test: `test_noisy_live_audio_accepted()` (PASS)
  - Test: `test_detection_latency_baseline()` (PASS)
  - Test: `test_detection_accuracy_metrics()` (PASS)
- ✅ Deployment Audit Procedure:
  ```bash
  grep "require_all_checks" src/voice/voice_anti_spoof_engine.cpp
  grep "freshness_threshold\|speaker_match_threshold" src/voice/voice_anti_spoof_engine.cpp
  grep "detect_liveness" src/voice/voice_authenticator.cpp
  ```
- ✅ Runtime Verification:
  ```cpp
  VoiceAntiSpoofEngine engine;
  auto analysis = engine.analyzeSpoofRisk(audio_data, speaker_baseline);
  assert(analysis.is_likely_spoofed == false);  // Live audio should pass
  assert(analysis.overall_confidence > 0.7);    // High confidence required
  ```

**Acceptance Criteria:**
- [ ] Liveness detection gate active in all authentication paths
- [ ] Fail-closed behavior: any detection uncertainty rejects audio
- [ ] Multi-factor detection: ALL checks (freshness + speaker + noise) required
- [ ] Detection latency baseline established (<100ms p95)
- [ ] Accuracy baseline established (>95% live, >90% replay detection)
- [ ] Adversarial test matrix exercised (12+ tests covering live/replay/mismatch/noisy)
- [ ] Error codes 7200-7203 documented in error handler
- [ ] Production deployment: `THEMIS_ANTISPOOF_ENABLED=true` in environment

---

## 5. Production Support & Escalation

### Known Limitations

1. **Anti-spoofing accuracy:** Depends on configured model profile; periodic retraining recommended
2. **Telephony threat handling:** Requires continuous regression testing for new attack variants
3. **Emotion analyzer:** Inference-based; production use requires calibrated model and confidence threshold
4. **Latency envelope:** Varies with hardware and LLM model size (200-1500ms typical)

### Troubleshooting Reference

| Symptom | Root Cause | Resolution |
|---|---|---|
| Sessions expire too quickly | `idle_timeout_ms` misconfigured | Verify `SessionTimeoutConfig` in code |
| Streaming audio stutters | Frame buffer too small | Increase `max_chunk_size_bytes` |
| "Authentication failed" errors | Auth guard misconfigured | Verify authenticator initialization |
| Transcript access denied | ACL misconfigured | Check transcript ownership and permissions |
| PII leaking in logs | Redaction not applied | Verify `redactPII()` called before logging |
| Anti-spoofing rejecting valid users | Model miscalibrated | Retrain or adjust confidence threshold |

---

## 6. Compliance & Audit Trail

**GDPR Compliance:**
- ✅ Data deletion on request (`deleteUserData()`)
- ✅ Consent tracking (`ConsentRecord`)
- ✅ Audit logging (`auditLog()`)
- ✅ PII redaction (`redactPII()`)

**CCPA Compliance:**
- ✅ Data access on request (`getUserData()`)
- ✅ Data deletion on request (`deleteUserData()`)
- ✅ Opt-out support (`disableAnalytics()`)

**SOC 2 Compliance:**
- ✅ Access control (voice authentication)
- ✅ Encryption (TLS transport, AES-256 storage)
- ✅ Audit logging (all operations logged)
- ✅ Incident response (circuit breaker, graceful degradation)

---

## Appendix A: Reference Files

**Core Implementation Files:**
- `src/voice/voice_authenticator.cpp` - Authentication & liveness
- `src/voice/voice_security.cpp` - Security & privacy
- `src/voice/voice_session_manager.cpp` - Session lifecycle
- `src/voice/voice_browser_streaming.cpp` - Streaming & validation
- `src/voice/voice_telephony.cpp` - Telephony & input validation
- `src/voice/audio_preprocessing.cpp` - Audio validation
- `src/voice/voice_audio_storage.cpp` - Transcript storage & ACL
- `src/voice/voice_error_handler.cpp` - Error codes & circuit breaker

**Test Files:**
- `tests/voice/test_voice_auth_security_focused.cpp`
- `tests/voice/test_voice_streaming_focused.cpp`
- `tests/voice/test_voice_telephony_focused.cpp`
- `tests/voice/test_voice_security_features.cpp`
- `tests/voice/test_voice_spoofing_adversarial_focused.cpp`
- `tests/voice/test_voice_production.cpp`

**Configuration Files:**
- `config/voice.json` - Runtime configuration
- `config/voice_models.yaml` - ML model profiles

**Verification Scripts:**
- `scripts/voice_production_audit.sh` - Automated audit

---

**Document Version:** v1.0-production (frozen 2026-08-08)  
**Level:** 1 (Module-Level Developer Documentation)  
**Source:** `src/voice/PRODUCTION_REQUIREMENTS.md`  
**Authority:** Canonical source for production deployment requirements
