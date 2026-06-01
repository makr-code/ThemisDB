> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/voice/ARCHITECTURE.md -->

# Voice Module — Public Header Architecture

**Module Path:** `include/voice/`
**Implementation:** `../../src/voice/`
**Canonical architecture doc:** [`../../src/voice/ARCHITECTURE.md`](../../src/voice/ARCHITECTURE.md)

---

## 1. Overview

`include/voice/` defines the **public voice interface, streaming, intent detection, security, and telephony API contract** for ThemisDB. The 18 headers cover the voice assistant, audio preprocessing, TTS customisation, session management, batch processing, browser streaming, emotion analysis, intent detection, voice authentication, security, macros, meeting support, model cache, accessibility, telephony, wake-word detection, and error handling.

For runtime composition — Whisper/STT pipeline integration, TTS engine routing, telephony signalling, and wake-word inference internals — see:
→ [`../../src/voice/ARCHITECTURE.md`](../../src/voice/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Assistant and Session

| Header | Public Type | Purpose |
|--------|------------|---------|
| `voice_assistant.h` | `VoiceAssistant` | Top-level voice assistant lifecycle and request routing |
| `voice_session_manager.h` | `VoiceSessionManager` | Session creation, tracking, and teardown |
| `voice_model_cache.h` | `VoiceModelCache` | Cached STT/TTS model management |
| `wake_word_detector.h` | `WakeWordDetector` | Wake-word detection trigger |

### 2.2 Audio Processing and TTS

| Header | Public Type | Purpose |
|--------|------------|---------|
| `audio_preprocessing.h` | `AudioPreprocessor` | Audio normalisation, denoising, and feature extraction |
| `voice_tts_customizer.h` | `VoiceTTSCustomizer` | Voice and prosody customisation for TTS output |
| `voice_batch_processor.h` | `VoiceBatchProcessor` | Batch audio transcription and synthesis |

### 2.3 Streaming and Browser Integration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `voice_browser_streaming.h` | `VoiceBrowserStreaming` | Browser WebRTC / WebSocket voice streaming |
| `voice_audio_storage.h` | `VoiceAudioStorage` | Persistent audio storage for session recordings |

### 2.4 Intent and Emotion Analysis

| Header | Public Type | Purpose |
|--------|------------|---------|
| `voice_intent_detector.h` | `VoiceIntentDetector` | NLU-based intent extraction from transcribed speech |
| `emotion_analyzer.h` | `EmotionAnalyzer` | Emotion recognition from audio features |

### 2.5 Authentication and Security

| Header | Public Type | Purpose |
|--------|------------|---------|
| `voice_auth.h` | `VoiceAuth` | Voice biometric / speaker-ID authentication |
| `voice_security.h` | `VoiceSecurity` | Anti-spoofing, deepfake detection, and PII scrubbing |

### 2.6 Telephony and Meeting Support

| Header | Public Type | Purpose |
|--------|------------|---------|
| `voice_telephony.h` | `VoiceTelephony` | SIP/PSTN telephony gateway integration |
| `voice_meeting_support.h` | `VoiceMeetingSupport` | Multi-speaker meeting transcription and diarisation |

### 2.7 Accessibility, Macros, and Errors

| Header | Public Type | Purpose |
|--------|------------|---------|
| `voice_accessibility.h` | `VoiceAccessibility` | Accessibility output and caption generation |
| `voice_macro.h` | `VoiceMacro` | Reusable voice-command macro definition |
| `voice_error_handler.h` | `VoiceErrorHandler` | Structured error handling for voice pipeline failures |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::voice` | All voice assistant, streaming, intent, security, and telephony types |

---

## 4. Public Contract Notes

- `VoiceAssistant` and `VoiceSessionManager` form the primary entry points; STT/TTS engine selection and pipeline routing remain internal.
- Authentication headers define stable voice biometric contracts; speaker-model and anti-spoofing internals are opaque.
- Security headers must fail closed for detected deepfake or spoofing signals and must scrub PII before downstream delivery.
- Telephony and browser-streaming headers define stable signalling contracts; transport-layer protocol details remain internal.
- Emotion and intent headers expose deterministic scoring contracts; model inference internals are opaque.
- Error-handler header provides structured error propagation for all voice pipeline stages.
