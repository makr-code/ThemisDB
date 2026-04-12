<!-- Status: current | validated: 2026-04-06 -->

# Voice — Roadmap

## Current Status
**v1.1.0** — Core voice pipeline stable. Meeting support, telephony, TTS customization, and accessibility headers released. Anti-spoofing and browser streaming in active development.

## Completed
- [x] Wake word detection pipeline (`wake_word_detector.h`)
- [x] Emotion analysis in audio streams (`emotion_analyzer.h`)
- [x] Voice authentication API (`voice_auth.h`)
- [x] Voice security / liveness detection (`voice_security.h`)
- [x] Browser streaming support (`voice_browser_streaming.h`)
- [x] TTS customization (`voice_tts_customizer.h`)
- [x] Meeting support / diarization (`voice_meeting_support.h`)
- [x] Telephony integration (`voice_telephony.h`)
- [x] Batch processing (`voice_batch_processor.h`)
- [x] Accessibility features (`voice_accessibility.h`)
- [x] Session manager CSPRNG-backed tokens

## Planned Features

- [ ] Anti-spoofing model upgrade in `voice_security.h` (Target: Q2 2026)
  - Replace threshold-based with ML liveness model
  - Configurable false-accept rate target
- [x] SSML injection sanitization in `voice_tts_customizer.h` (Target: Q2 2026)
- [ ] Real-time emotion streaming API in `emotion_analyzer.h` (Target: Q3 2026)
- [ ] Multi-language wake word support in `wake_word_detector.h` (Target: Q3 2026)
- [ ] End-to-end encryption for `voice_audio_storage.h` (AES-256-GCM) (Target: Q2 2026)
- [x] Path traversal protection for model names in `voice_model_cache.h` (Target: Q2 2026)
- [x] WebRTC origin allowlist in `voice_browser_streaming.h` (Target: Q2 2026)
- [ ] Meeting transcript export (GDPR-compliant redaction) in `voice_meeting_support.h` (Target: Q3 2026)
- [ ] Voice macro privilege model (prevent escalation via replay) (Target: Q3 2026)
- [ ] Telephony DTMF isolation in `voice_telephony.h` (Target: Q3 2026)
- [ ] Formal security audit of voice auth biometric storage (Target: Q4 2026)

## Implementation Phases

### Phase 1 — Design / API Contract
- [x] Define pipeline stage interfaces
- [x] Define session lifecycle in `VoiceSessionManager`
- [ ] Define SSML sanitization API contract in `voice_tts_customizer.h`
- [ ] Define model allowlist API in `voice_model_cache.h`

### Phase 2 — Core Implementation
- [ ] ML-based anti-spoofing in `voice_security.h`
- [ ] SSML sanitizer implementation
- [ ] AES-256-GCM audio storage in `voice_audio_storage.h`

### Phase 3 — Error Handling & Edge Cases
- [x] Handle WebRTC origin mismatch gracefully
- [ ] Handle liveness model unavailability (fail-closed)
- [ ] Handle telephony leg drop mid-session

### Phase 4 — Tests
- [ ] Unit tests for `WakeWordDetector` sensitivity/specificity
- [ ] Integration tests for full auth pipeline (wake → auth → intent)
- [x] Security tests for SSML injection prevention
- [ ] Privacy tests: no audio retained post-VAD window

### Phase 5 — Performance / Hardening
- [x] Wake word latency target: < 200 ms end-to-end on embedded (`bench_voice_wake_word_batch.cpp`)
- [ ] Emotion analyzer: < 50 ms per 2-second audio chunk
- [x] Batch processor: linear throughput scaling to 16 threads (`bench_voice_wake_word_batch.cpp`)

### Phase 6 — Documentation & Sign-off
- [ ] Complete API documentation for all 18 headers
- [ ] Security review of voice auth biometric storage
- [ ] GDPR compliance review for meeting transcripts
- [ ] Update CHANGELOG with v1.2.0 entries

## Production Readiness Checklist
- [x] Wake word detection in production
- [x] Browser streaming in production
- [ ] Anti-spoofing ML model deployed
- [ ] SSML sanitization implemented
- [ ] Audio storage end-to-end encryption
- [ ] Meeting transcript GDPR-compliant export
- [ ] Formal biometric storage security audit
