---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Voice Public Headers — Architecture and API Map

`include/voice/` contains 23 public headers. Header declarations and comments are
the API authority; this page is an index, not a promise that every optional
integration is enabled or production-ready. Runtime composition is summarized in
[`../../src/voice/ARCHITECTURE.md`](../../src/voice/ARCHITECTURE.md).

## Public interfaces

| Header | Principal declared types |
|---|---|
| `audio_preprocessing.h` | `AudioPreprocessingPipeline`, `NoiseSuppressor`, `AudioFrame` |
| `emotion_analyzer.h` | `EmotionAnalyzer`, `EmotionAnalysis` |
| `voice_accessibility.h` | `VoiceAccessibility`, caption/export records |
| `voice_anti_spoof_engine.h` | `VoiceAntiSpoofEngine`, `SpoofAnalysis` |
| `voice_assistant.h` | `VoiceAssistant`, `VoiceSession`, `Config` |
| `voice_audio_storage.h` | `VoiceAudioStorage`, audio/storage records |
| `voice_audit_logger.h` | `VoiceAuditLogger` |
| `voice_auth.h` | `VoiceBiometricAuthenticator`, enrollment and verification results |
| `voice_batch_processor.h` | `VoiceBatchProcessor`, batch job/result records |
| `voice_browser_streaming.h` | `VoiceStreamingSession`, `VoiceStreamingManager` |
| `voice_error_handler.h` | `VoiceException`, `VoiceCircuitBreaker`, `VoiceRetryHandler`, `VoiceErrorHandler` |
| `voice_intent_detector.h` | `VoiceIntentDetector`, `ConversationContext`, `IntentResult` |
| `voice_liveness_checker.h` | `VoiceLivenessChecker`, `LivenessCheckResult` |
| `voice_liveness_detector.h` | `VoiceLivenessDetector`, challenge/result records |
| `voice_macro.h` | `VoiceMacroManager`, macro/step records |
| `voice_meeting_support.h` | `VoiceMeetingSupport`, `RealtimeMeetingSession`, meeting records |
| `voice_model_cache.h` | `VoiceModelCache`, cache records |
| `voice_security.h` | `VoiceSecurityManager`, consent/audit/deletion records |
| `voice_session_manager.h` | `VoiceSessionManager`, `ISessionPersistenceBackend`, `InMemorySessionBackend` |
| `voice_stream_validator.h` | `VoiceStreamValidator`, `StreamValidationPolicy`, `ValidatedAudioChunk` |
| `voice_telephony.h` | `SipCallSession`, `WebRtcCallSession`, `IvrEngine`, `TelephonyBridge` |
| `voice_tts_customizer.h` | `VoiceTTSCustomizer`, voice/prosody records |
| `wake_word_detector.h` | `WakeWordDetector`, `WakeWordDetectionResult` |

Types are declared in `themis::voice`. Consult each header for exact methods,
preconditions, result types, and feature-specific limits; class names and
contracts in older header summaries may not match the current declarations.

## Build and registration boundaries

- `THEMIS_ENABLE_VOICE_ASSISTANT` is an opt-in CMake feature and defaults to
  `OFF` in `cmake/CMakeLists.txt`; it gates the server voice API handler. The
  voice implementation sources listed there are gated by `THEMIS_ENABLE_LLM`.
- `THEMIS_ENABLE_RNNOISE` also defaults to `OFF`; preprocessing documents the
  fallback behavior in `audio_preprocessing.h`.
- Voice tests are discovered by the module CMake in `tests/voice/CMakeLists.txt`
  when tests are enabled. That file is auto-discovered by `tests/CMakeLists.txt`.
- Voice benchmarks are globbed from `benchmarks/voice/bench_*.cpp` when
  `THEMIS_BUILD_BENCHMARKS` is enabled. Registration is not execution evidence.

## Contract cautions

- `VoiceAssistant::Config` defaults `enable_voice_auth` and `enable_wake_word`
  to `false`; integrations must not assume these checks are active.
- Browser streaming has a placeholder transcript/TTS path unless the relevant
  backend is injected. It is not evidence of speech recognition or synthesis.
- Storage, audit, biometric, and session helpers expose in-process behavior;
  their interfaces do not establish durable storage, cryptographic protection,
  caller authorization, or compliance.
- “SIP”, “WebRTC”, “persistent”, “secure”, and “model” in a type or feature name
  do not alone prove protocol conformance, durability, security validation, or
  an external model integration. Verify the implementation and deployment.

## Canonical references

- Public contract: headers in this directory.
- Implementation/dataflow: [`../../src/voice/ARCHITECTURE.md`](../../src/voice/ARCHITECTURE.md)
- Test discovery and labels: [`../../tests/voice/CMakeLists.txt`](../../tests/voice/CMakeLists.txt)
- Benchmark discovery: [`../../benchmarks/voice/CMakeLists.txt`](../../benchmarks/voice/CMakeLists.txt)
