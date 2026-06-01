> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/voice/ROADMAP.md -->

# Voice Module — Public Header Roadmap

**Module Path:** `include/voice/`
**Canonical implementation roadmap:** [`../../src/voice/ROADMAP.md`](../../src/voice/ROADMAP.md)

---

## Overview

Tracks public voice API contract stability, header coverage, and future public entry points. Runtime Whisper/STT pipeline integration, TTS engine routing, telephony signalling, and wake-word inference work remain in:

→ [`../../src/voice/ROADMAP.md`](../../src/voice/ROADMAP.md)

---

## Current Status

All 18 voice headers are present. Public entry points exist for the voice assistant, audio preprocessing, TTS customisation, session management, batch processing, browser streaming, emotion analysis, intent detection, voice authentication, security, telephony, meeting support, model cache, accessibility, macros, wake-word detection, and error handling.

---

## Completed ✅

- [x] `voice_assistant.h`, `voice_session_manager.h`, `voice_model_cache.h`, `wake_word_detector.h` — core assistant and session
- [x] `audio_preprocessing.h`, `voice_tts_customizer.h`, `voice_batch_processor.h` — audio processing and TTS
- [x] `voice_browser_streaming.h`, `voice_audio_storage.h` — streaming and browser integration
- [x] `voice_intent_detector.h`, `emotion_analyzer.h` — intent and emotion analysis
- [x] `voice_auth.h`, `voice_security.h` — authentication and security
- [x] `voice_telephony.h`, `voice_meeting_support.h` — telephony and meeting support
- [x] `voice_accessibility.h`, `voice_macro.h`, `voice_error_handler.h` — accessibility, macros, and errors

---

## In Progress

- [ ] Document PII-scrubbing and deepfake-detection fail-closed semantics in `voice_security.h` (Target: 2026-Q3)
- [ ] Clarify speaker-diarisation output contract and segment-labelling guarantees in `voice_meeting_support.h` (Target: 2026-Q3)

---

## Planned

- [ ] `voice_policy.h` — per-session audio processing and recording policy contract (Target: 2026-Q4)
- [ ] Add explicit conformance notes for WebRTC/SIP protocol contracts in streaming and telephony headers (Target: 2026-Q4)
- [ ] Expose benchmark latency targets for wake-word detection and intent extraction hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Voice headers maintain backward compatibility within the active major line; audio-format and speaker-model changes require migration notes and changelog updates.
